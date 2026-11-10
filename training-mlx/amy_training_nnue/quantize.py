"""Quantize a trained float network to amy's net format (amy.net) and
evaluate it with the exact integer arithmetic of neural_net.c.

The net file (amy.net) is a 96-byte little-endian header (the WeightsHeader
struct of neural_net.c, see weights_header()) followed by, in order:

    input_layer_weights_q  int8   [N_FEATURES][ACCUMULATOR_SIZE]
    input_pawn_weights_q   int8   [N_PAWN_FEATURES][ACCUMULATOR_SIZE]
    input_attack_weights_q int8   [N_ATTACK_FEATURES][ACCUMULATOR_SIZE]
    input_layer_bias_q     int16  [ACCUMULATOR_SIZE]

then, for each of the NN_PHASE_COUNT = 4 game phases, its own
{hidden-1, hidden-2, output} branch in the six-block order H1W,
H1B, H2W, H2B, OW, OB:

    hidden_layer_1_weights_q_<phase> int16 [HIDDEN_SIZE_1][512]
    hidden_layer_1_bias_q_<phase>    int32  [HIDDEN_SIZE_1]
    hidden_layer_2_weights_q_<phase> int16 [HIDDEN_SIZE_2][HIDDEN_SIZE_1]
    hidden_layer_2_bias_q_<phase>    int32  [HIDDEN_SIZE_2]
    output_layer_weights_q_<phase>   int32  [HIDDEN_SIZE_2]
    output_layer_bias_q_<phase>      int32  [1]

The engine selects the phase at evaluation time via
nn_phase_index() (total non-pawn piece count, both sides, bucketed by
three); QuantizedNet.evaluate() takes that phase per sample.

The pawn and attack weights have no quantization code in the header;
the engine reads both unconditionally as int8 at the same scale as the
piece input weights (only 1488 of the 4560 pawn rows are ever active
and hold zeros otherwise; most of the 98304 attack rows are never
active too).

Quantization convention (float v -> fixed point):

    input weights:  int8(v * 64); the accumulator is kept in 1/64 units
    pawn weights:   int8(v * 64); same scale, same accumulator-add
    input bias:     int16(v * 64)
    hidden 1:       int16(v * 8192) weights, int32(v * 64 * 8192) bias;
                    sum in 64*8192 units, +4095 then /8192 -> acts in [0, 64]
    hidden 2:       int16(v * 8192) weights, int32(v * 8192 * 64) bias;
                    sum in 8192*64 units, +31 then /64 -> acts in [0, 8192]
    output:         int32(v * 8192) weights, int32(v * 8192 * 8192) bias

The int8 input weights cover float magnitude 2.0 at scale 64 (weights
are kept within [-1, 1] during training, so no clipping in practice);
the int16 hidden 1 weights at scale 8192 cover +-4.0.  Out-of-range
values are clipped (with a warning) so the file stays loadable by amy.

Per-phase branches (NN_PHASE_COUNT = 4): the input side (piece,
pawn, attack and bias arrays) is shared, but each game phase has its
own hidden-1, hidden-2 and output layer block, written one phase
after the other, six blocks each in the order H1W, H1B, H2W, H2B, OW,
OB (the read order of nn_read_weights_quantized()).  The engine
selects the phase at evaluation time via nn_phase_index() (total
non-pawn piece count, both sides, bucketed by three);
quantize.py's QuantizedNet.evaluate() takes that phase per sample.

The header (little-endian, 96 bytes, must match neural_net.c's
WeightsHeader byte-for-byte; sizeof(WeightsHeader) is the arbiter
and is cross-checked against the C library):

    u32 magic 0x57594D41 (file bytes read as "AMYW")
    u16 accumulator size, u16 number of piece input features (22528),
    u16 number of pawn input features (4560),
    2 bytes of alignment padding (the attack size is a u32, so the C
    struct pads to a 4-byte boundary before it)
    u32 number of attack input features (98304; a u16 cannot hold it)
    u16 hidden layer 1 size, u16 hidden layer 2 size
    8 x u8 quantization codes (1 = int8, 2 = int16, 3 = int32), one per
    piece field in the same order as the fields above; the pawn and
    attack weights have no code (int8 at the input scale, see above)
    u16 number of phase branches (NN_PHASE_COUNT = 4)
    char[32] name (the network codename, NUL-terminated; written via
    codename() in code_name.py)
    char[32] date (training date/time, ISO8601, NUL-terminated)
    2 bytes of trailing padding (the struct's alignment is 4; the 94
    field bytes are padded to sizeof(WeightsHeader) = 96)
"""

from __future__ import annotations

import json
import struct
import warnings
from dataclasses import dataclass
from datetime import datetime
from pathlib import Path

import numpy as np

from .code_name import codename
from .features import (
    ACCUMULATOR_SIZE,
    HIDDEN_SIZE_1,
    HIDDEN_SIZE_2,
    N_ATTACK_FEATURES,
    N_FEATURES,
    N_PAWN_FEATURES,
    NN_PHASE_COUNT,
)

SCALE = 8192
SCALE_INPUT = 64  # input/hidden-1 scale; accumulator in 1/64 units
SCALE_INPUT_HALF = SCALE_INPUT // 2 - 1  # 31, as in neural_net.c
SCALE_HALF = SCALE // 2 - 1  # 4095, as in neural_net.c
SCALE_OUTPUT = 10000  # score_mp = round(output * SCALE_OUTPUT)
# Hidden-1 fixed-point scale (SCALE_H1 in neural_net.c): acc @ 64 x
# int16 weights @ 8192, and the int32 hidden-1 bias.
SCALE_H1 = SCALE_INPUT * SCALE  # 524288


# shared input side (one matrix for all phases) + NN_PHASE_COUNT
# independent {H1, H2, output} blocks, per phase in the read order of
# nn_read_weights_quantized(): H1W, H1B, H2W, H2B, OW, OB.
# Per-phase branch blocks in the read order of
# nn_read_weights_quantized(): H1W, H1B, H2W, H2B, OW, OB.
def _per_phase_fields(phase: int) -> list[tuple[str, type, tuple]]:
    return [
        (
            f"hidden_layer_1_weights_q_{phase}",
            np.int16,
            (HIDDEN_SIZE_1, 2 * ACCUMULATOR_SIZE),
        ),
        (f"hidden_layer_1_bias_q_{phase}", np.int32, (HIDDEN_SIZE_1,)),
        (f"hidden_layer_2_weights_q_{phase}", np.int16, (HIDDEN_SIZE_2, HIDDEN_SIZE_1)),
        (f"hidden_layer_2_bias_q_{phase}", np.int32, (HIDDEN_SIZE_2,)),
        (f"output_layer_weights_q_{phase}", np.int32, (HIDDEN_SIZE_2,)),
        (f"output_layer_bias_q_{phase}", np.int32, (1,)),
    ]


FIELD_ORDER = [
    # (name, numpy dtype, shape); the shared input side first
    ("input_layer_weights_q", np.int8, (N_FEATURES, ACCUMULATOR_SIZE)),
    ("input_pawn_weights_q", np.int8, (N_PAWN_FEATURES, ACCUMULATOR_SIZE)),
    ("input_attack_weights_q", np.int8, (N_ATTACK_FEATURES, ACCUMULATOR_SIZE)),
    ("input_layer_bias_q", np.int16, (ACCUMULATOR_SIZE,)),
    # then NN_PHASE_COUNT independent {H1, H2, output} blocks, one
    # phase at a time.
    *sum((_per_phase_fields(phase) for phase in range(NN_PHASE_COUNT)), []),
]

# WeightsHeader of neural_net.c (little-endian; sizeof(WeightsHeader)
# = 96: the 94 field bytes plus 2 bytes of trailing padding for the
# struct's 4-byte alignment).
WEIGHTS_HEADER_MAGIC = 0x57594D41  # file bytes read as "AMYW"
QUANT_INT8 = 1
QUANT_INT16 = 2
QUANT_INT32 = 3
# u32 magic, 3 x u16 sizes (accumulator, piece features, pawn
# features), 2 bytes of padding, u32 attack features, 2 x u16 sizes
# (h1, h2), 8 x u8 quant codes, u16 phase count, char[32] name,
# char[32] date, 2 bytes of trailing padding (sizeof 96).
HEADER_FORMAT = "<IHHH2xIHH8BH32s32s2x"
HEADER_SIZE = struct.calcsize(HEADER_FORMAT)

_QUANT_CODES = {
    "int8": QUANT_INT8,
    "int16": QUANT_INT16,
    "int32": QUANT_INT32,
}


# The eight header quantization codes (one per base field, shared by
# all NN_PHASE_COUNT phase branches; the per-phase file fields are
# named <base>_<phase> and are read with the same code).  The pawn
# and attack weights have no code in the C header: the engine reads
# both unconditionally as int8 at the input scale.
_CODED_FIELDS = [
    ("input_layer_weights_q", np.int8),
    ("input_layer_bias_q", np.int16),
    ("hidden_layer_1_weights_q", np.int16),
    ("hidden_layer_1_bias_q", np.int32),
    ("hidden_layer_2_weights_q", np.int16),
    ("hidden_layer_2_bias_q", np.int32),
    ("output_layer_weights_q", np.int32),
    ("output_layer_bias_q", np.int32),
]


def _header_quant_codes() -> list[int]:
    """The 8 header quantization codes, in the same order as the
    _CODED_FIELDS list (and the C struct's code fields)."""
    return [_QUANT_CODES[np.dtype(dtype).name] for _, dtype in _CODED_FIELDS]


def weights_header(name: str, date_: str) -> bytes:
    """The 96-byte WeightsHeader of neural_net.c for this architecture
    (94 field bytes + 2 bytes of trailing padding; sizeof is the
    arbiter).

    name is a short codename and date_ an ISO8601 timestamp (both fit
    the C struct's char[32] fields, NUL-terminated).
    """
    if len(name) > 31:
        raise ValueError(f"name must fit char[32]: {name!r} is {len(name)} chars")
    if len(date_) > 31:
        raise ValueError(f"date must fit char[32]: {date_!r} is {len(date_)} chars")
    return struct.pack(
        HEADER_FORMAT,
        WEIGHTS_HEADER_MAGIC,
        ACCUMULATOR_SIZE,
        N_FEATURES,
        N_PAWN_FEATURES,
        N_ATTACK_FEATURES,
        HIDDEN_SIZE_1,
        HIDDEN_SIZE_2,
        *_header_quant_codes(),
        NN_PHASE_COUNT,
        name.encode("ascii"),
        date_.encode("ascii"),
    )


# Byte size of the quantized field arrays alone (without the header).
PAYLOAD_SIZE = sum(
    np.dtype(dtype).itemsize * int(np.prod(shape)) for _, dtype, shape in FIELD_ORDER
)

# Total amy.net file size: header + fields.
NETWORK_QUANT_SIZE = HEADER_SIZE + PAYLOAD_SIZE


def _quantize(value: np.ndarray, scale: float, dtype: type) -> np.ndarray:
    q = np.round(value.astype(np.float64) * scale)
    info = np.iinfo(np.dtype(dtype))
    if np.any(q < info.min) or np.any(q > info.max):
        lo, hi = info.min, info.max
        n = int(np.sum((q < lo) | (q > hi)))
        warnings.warn(
            f"{n} value(s) outside int range [{lo}, {hi}], clipped "
            f"(max |q| = {np.max(np.abs(q)):.1f}); retrain with smaller weights "
            "or check the target scale",
            stacklevel=2,
        )
        q = np.clip(q, lo, hi)
    return q.astype(dtype)


def quantize_parameters(params: dict[str, np.ndarray]) -> dict[str, np.ndarray]:
    """Map float model parameters to the quantized field arrays.

    The model stores h1_w in matmul layout (NN_PHASE_COUNT,
    2*ACCUMULATOR_SIZE, HIDDEN_SIZE_1) and h2_w as (NN_PHASE_COUNT,
    HIDDEN_SIZE_1, HIDDEN_SIZE_2) with h2_w[phase, j, i] = weight from
    j to i; the file layout is the C layout [new][old], i.e. the
    transposes, and one block per phase.
    """
    out = {
        "input_layer_weights_q": _quantize(params["input_w"], SCALE_INPUT, np.int8),
        "input_pawn_weights_q": _quantize(params["pawn_w"], SCALE_INPUT, np.int8),
        "input_attack_weights_q": _quantize(params["attack_w"], SCALE_INPUT, np.int8),
        "input_layer_bias_q": _quantize(params["input_b"], SCALE_INPUT, np.int16),
    }
    for phase in range(NN_PHASE_COUNT):
        out[f"hidden_layer_1_weights_q_{phase}"] = _quantize(
            params["h1_w"][phase].T, SCALE, np.int16
        )
        out[f"hidden_layer_1_bias_q_{phase}"] = _quantize(
            params["h1_b"][phase], SCALE_INPUT * SCALE, np.int32
        )
        out[f"hidden_layer_2_weights_q_{phase}"] = _quantize(
            params["h2_w"][phase].T, SCALE, np.int16
        )
        out[f"hidden_layer_2_bias_q_{phase}"] = _quantize(
            params["h2_b"][phase], SCALE_INPUT * SCALE, np.int32
        )
        out[f"output_layer_weights_q_{phase}"] = _quantize(
            params["out_w"][phase], SCALE, np.int32
        )
        out[f"output_layer_bias_q_{phase}"] = _quantize(
            params["out_b"][phase], SCALE * SCALE, np.int32
        )
    return out


def write_network_quant(
    path: str | Path,
    quantized: dict[str, np.ndarray],
    name: str | None = None,
    date_: str | None = None,
) -> int:
    """Write the quantized network: WeightsHeader followed by amy's raw
    field layout.  name defaults to a codename and date_ to the
    current ISO8601 timestamp (full precision, e.g.
    2021-07-27T16:02:08.070557).  Returns the total byte count."""
    path = Path(path)
    header = weights_header(
        name if name is not None else codename(),
        date_ if date_ is not None else datetime.now().isoformat(),
    )
    total = HEADER_SIZE
    with open(path, "wb") as f:
        f.write(header)
        for name, dtype, shape in FIELD_ORDER:
            array = quantized[name].astype(dtype).reshape(shape)
            f.write(array.tobytes())
            total += array.nbytes

    return total


def read_network_quant(path: str | Path) -> dict[str, np.ndarray]:
    """Read an amy.net file back into a dict of named numpy arrays."""
    path = Path(path)
    data = path.read_bytes()
    if len(data) != NETWORK_QUANT_SIZE:
        raise ValueError(
            f"{path}: size {len(data)} != expected {NETWORK_QUANT_SIZE} bytes"
        )
    (
        magic,
        acc_size,
        num_features,
        pawn_features,
        attack_features,
        h1_size,
        h2_size,
        *rest,
    ) = struct.unpack_from(HEADER_FORMAT, data)
    codes = list(rest[:8])
    phase_count = rest[8]
    # name and date: char[32] fields, NUL-terminated in the C struct
    _name = rest[9].rstrip(b"\x00").decode("ascii")
    _date = rest[10].rstrip(b"\x00").decode("ascii")
    if magic != WEIGHTS_HEADER_MAGIC:
        raise ValueError(
            f"{path}: bad magic {magic:#010x}, expected {WEIGHTS_HEADER_MAGIC:#010x}"
        )
    if (
        acc_size,
        num_features,
        pawn_features,
        attack_features,
        h1_size,
        h2_size,
    ) != (
        ACCUMULATOR_SIZE,
        N_FEATURES,
        N_PAWN_FEATURES,
        N_ATTACK_FEATURES,
        HIDDEN_SIZE_1,
        HIDDEN_SIZE_2,
    ):
        raise ValueError(
            f"{path}: header sizes {(acc_size, num_features, pawn_features, attack_features, h1_size, h2_size)} != "
            f"({ACCUMULATOR_SIZE}, {N_FEATURES}, {N_PAWN_FEATURES}, {N_ATTACK_FEATURES}, {HIDDEN_SIZE_1}, {HIDDEN_SIZE_2}) "
            "(stale net file from an older architecture?)"
        )
    if phase_count != NN_PHASE_COUNT:
        raise ValueError(
            f"{path}: header phase count {phase_count} != {NN_PHASE_COUNT} "
            "(stale net file from a single-branch architecture?)"
        )
    expected_codes = _header_quant_codes()
    if codes != expected_codes:
        raise ValueError(
            f"{path}: header quantization codes {codes} != {expected_codes}"
        )
    out = {}
    offset = HEADER_SIZE
    for name, dtype, shape in FIELD_ORDER:
        count = int(np.prod(shape))
        out[name] = np.frombuffer(
            data, dtype=dtype, count=count, offset=offset
        ).reshape(shape)
        offset += np.dtype(dtype).itemsize * count
    return out


def _trunc_div(n: np.ndarray, d: int) -> np.ndarray:
    """Integer division truncating toward zero, like C for all signs."""
    return np.sign(n) * (np.abs(n) // d)


@dataclass
class Phase:
    """One game-phase branch (hidden-1, hidden-2 and output head),
    as loaded from / written to amy.net."""

    h1_w: np.ndarray  # int16 (HIDDEN_SIZE_1, 2*ACCUMULATOR_SIZE), scale 8192
    h1_b: np.ndarray  # int32 (HIDDEN_SIZE_1,), scale 64*8192
    h2_w: np.ndarray  # int16 (HIDDEN_SIZE_2, HIDDEN_SIZE_1), scale 8192
    h2_b: np.ndarray  # int32 (HIDDEN_SIZE_2,), scale 8192*64
    out_w: np.ndarray  # int32 (HIDDEN_SIZE_2,)
    out_b: np.ndarray  # int32 (1,)


@dataclass
class QuantizedNet:
    """Integer simulation of amy's quantized network (neural_net.c):
    shared input side + NN_PHASE_COUNT {H1, H2, output} branches, one
    per phase (the engine selects the phase with nn_phase_index())."""

    input_w: np.ndarray  # int8 (N_FEATURES, ACCUMULATOR_SIZE), scale 64
    pawn_w: np.ndarray  # int8 (N_PAWN_FEATURES, ACCUMULATOR_SIZE), scale 64
    attack_w: np.ndarray  # int8 (N_ATTACK_FEATURES, ACCUMULATOR_SIZE), scale 64
    input_b: np.ndarray  # int16 (ACCUMULATOR_SIZE,), scale 64
    phases: tuple[Phase, ...][Phase, ...]

    @staticmethod
    def from_arrays(quantized: dict[str, np.ndarray]) -> "QuantizedNet":
        shapes = {name: shape for name, _, shape in FIELD_ORDER}

        def shaped(name: str) -> np.ndarray:
            return np.asarray(quantized[name]).reshape(shapes[name])

        phases = tuple(
            Phase(
                h1_w=shaped(f"hidden_layer_1_weights_q_{phase}"),
                h1_b=shaped(f"hidden_layer_1_bias_q_{phase}"),
                h2_w=shaped(f"hidden_layer_2_weights_q_{phase}"),
                h2_b=shaped(f"hidden_layer_2_bias_q_{phase}"),
                out_w=shaped(f"output_layer_weights_q_{phase}"),
                out_b=shaped(f"output_layer_bias_q_{phase}"),
            )
            for phase in range(NN_PHASE_COUNT)
        )
        return QuantizedNet(
            input_w=shaped("input_layer_weights_q"),
            pawn_w=shaped("input_pawn_weights_q"),
            attack_w=shaped("input_attack_weights_q"),
            input_b=shaped("input_layer_bias_q"),
            phases=phases,
        )

    @staticmethod
    def from_file(path: str | Path) -> "QuantizedNet":
        return QuantizedNet.from_arrays(read_network_quant(path))

    # -- integer pipeline, matching neural_net.c step by step ----------

    @staticmethod
    def _add_rows(acc: np.ndarray, rows: np.ndarray, feat: np.ndarray) -> None:
        """Add the active rows of `rows` (int8 @ 64) into the raw
        accumulator with int16 wrap-around, one feature slot at a time,
        as in neural_net.c (the `int16 += int8` hot loop)."""
        for k in range(feat.shape[1]):
            idx = feat[:, k]
            valid = idx >= 0
            if not np.any(valid):
                continue
            acc[valid] = (
                (acc[valid] + rows[idx[valid]].astype(np.int32))
                .astype(np.int16)
                .astype(np.int32)
            )

    def accumulator(
        self,
        feat: np.ndarray,
        pfeat: np.ndarray | None = None,
        afeat: np.ndarray | None = None,
    ) -> np.ndarray:
        """(B, K) int32 indices (padded -1) -> raw int16 accumulator (B, ACCUMULATOR_SIZE).

        Like amy: the accumulator is maintained in int16 with wrap-around
        in units of 1/SCALE_INPUT, starting from the int16 input bias
        (same scale), plus the piece feature rows and, if given, the
        pawn pair rows and the attack rows (padded -1 the same way).
        The clamping to [0, SCALE_INPUT] happens only at evaluation
        time (see evaluate()).  Shared by all phases.
        """
        acc = np.zeros((feat.shape[0], ACCUMULATOR_SIZE), dtype=np.int32)
        acc += self.input_b.astype(np.int32)
        self._add_rows(acc, self.input_w, feat)
        if pfeat is not None:
            self._add_rows(acc, self.pawn_w, pfeat)
        if afeat is not None:
            self._add_rows(acc, self.attack_w, afeat)
        return acc

    def evaluate(
        self,
        feat_w: np.ndarray,
        feat_b: np.ndarray,
        pfeat_w: np.ndarray | None,
        pfeat_b: np.ndarray | None,
        afeat_w: np.ndarray | None,
        afeat_b: np.ndarray | None,
        turn: np.ndarray,
        phase: np.ndarray,
    ) -> np.ndarray:
        """Scores in millipawns, side to move. turn: (B,) 0/1; phase:
        (B,) int in 0..NN_PHASE_COUNT-1 (nn_phase_index() of the
        position, first active of the training masks; the engine hard-
        switches to exactly one branch).

        The afeat blocks are the per-side attack feature indices
        (padded -1); they feed the same shared int8 attack matrix as
        the engine, and may be None only when the attack matrix is
        known to be zero.  C computes
        hidden_layer_i_q[i] = bias_i + sum_j acc[j] * W[i][j]
        with W in [new][old] file layout; in matrix form that is
        (W @ z.T).T.
        """
        acc_w = self.accumulator(feat_w, pfeat_w, afeat_w)
        acc_b = self.accumulator(feat_b, pfeat_b, afeat_b)
        n = feat_w.shape[0]
        acc = np.empty((n, 2 * ACCUMULATOR_SIZE), dtype=np.int32)
        wm = turn == 0
        acc[wm] = np.concatenate([acc_w[wm], acc_b[wm]], axis=1)
        acc[~wm] = np.concatenate([acc_b[~wm], acc_w[~wm]], axis=1)
        # neural_net.c clamps the local 2*ACCUMULATOR_SIZE-dim accumulator copy
        # stored accumulators stay raw int16), in 1/64 units.
        acc = np.clip(acc, 0, SCALE_INPUT)

        h1 = np.zeros((n, HIDDEN_SIZE_1), dtype=np.int32)
        h2 = np.zeros((n, HIDDEN_SIZE_2), dtype=np.int32)
        out_q = np.zeros(n, dtype=np.int64)
        for ph_idx, ph in enumerate(self.phases):
            sel = phase == ph_idx
            if not np.any(sel):
                continue
            # hidden layer 1 of this phase for its samples
            # (n_sel, H1); the bias adds along axis 1.
            h1q = acc[sel] @ ph.h1_w.T + ph.h1_b[None, :]  # 64*8192 units
            h1q = np.where(h1q <= 0, 0, h1q)
            h1q = np.where(h1q > SCALE_H1, SCALE_INPUT, (h1q + SCALE_HALF) // SCALE)
            h1q = h1q.astype(np.int32)
            h1[sel] = h1q

            # hidden layer 2 of this phase
            h2q = h1q @ ph.h2_w.T + ph.h2_b[None, :]  # 8192*64 units
            h2q = np.where(h2q <= 0, 0, h2q)
            # neural_net.c clamps with >= here (vs > for hidden layer 1).
            h2q = np.where(
                h2q >= SCALE_INPUT * SCALE,
                SCALE,
                (h2q + SCALE_INPUT_HALF) // SCALE_INPUT,
            )
            h2q = h2q.astype(np.int32)
            h2[sel] = h2q

            # this phase's output head (int64: int32 @ int32)
            out_q[sel] = h2q @ ph.out_w + ph.out_b[0]

        # C integer division truncates toward zero (not floor).
        r = _trunc_div(out_q + SCALE_HALF, SCALE)
        score = _trunc_div(r * SCALE_OUTPUT + SCALE_HALF, SCALE)
        return score.astype(np.int32)


def checkpoint_meta(checkpoint_dir: str | Path) -> dict:
    meta_file = Path(checkpoint_dir) / "meta.json"
    if meta_file.exists():
        return json.loads(meta_file.read_text())
    return {}
