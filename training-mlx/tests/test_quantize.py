"""Tests for quantization to amy's amy.net format.

Shapes and scales come from the architecture constants (features.py,
quantize.py), not from hardcoded numbers, so the tests keep working
when the network structure changes for experiments.
"""

import struct

import numpy as np
import pytest

from amy_training_nnue import cfeatures
from amy_training_nnue.bpd import encode_board
from amy_training_nnue.features import (
    ACCUMULATOR_SIZE,
    HIDDEN_SIZE_1,
    HIDDEN_SIZE_2,
    N_ATTACK_FEATURES,
    N_FEATURES,
    N_PAWN_FEATURES,
    NN_PHASE_COUNT,
    feature_indices,
    pawn_feature_indices,
)
from amy_training_nnue.quantize import (
    FIELD_ORDER,
    HEADER_SIZE,
    NETWORK_QUANT_SIZE,
    SCALE,
    SCALE_INPUT,
    SCALE_OUTPUT,
    WEIGHTS_HEADER_MAGIC,
    QuantizedNet,
    quantize_parameters,
    read_network_quant,
    weights_header,
    write_network_quant,
)
from tests.test_features import _board_from_epd

# Padding widths for the synthetic batches below (real dataset widths
# come from the C feature-extraction constants, see cfeatures.py).
MAX_FEATURES = 32
MAX_PAWN_FEATURES = 128
MAX_ATTACK_FEATURES = 32


def _feats(board):
    """Oracle piece/pawn indices plus an empty attack block (the hand-computed
    synthetic tests below don't exercise attack rows; the cross-check and
    float-vs-quantized parity suites cover those)."""
    w, b = feature_indices(board)
    pw, pb = pawn_feature_indices(board)
    fw = np.full((1, MAX_FEATURES), -1, dtype=np.int32)
    fb = np.full((1, MAX_FEATURES), -1, dtype=np.int32)
    pfw = np.full((1, MAX_PAWN_FEATURES), -1, dtype=np.int32)
    pfb = np.full((1, MAX_PAWN_FEATURES), -1, dtype=np.int32)
    afw = np.full((1, MAX_ATTACK_FEATURES), -1, dtype=np.int32)
    afb = np.full((1, MAX_ATTACK_FEATURES), -1, dtype=np.int32)
    fw[0, : len(w)] = w
    fb[0, : len(b)] = b
    pfw[0, : len(pw)] = pw
    pfb[0, : len(pb)] = pb
    return fw, fb, pfw, pfb, afw, afb


def test_header_matches_c_struct():
    """The header must be byte-identical to C's WeightsHeader
    (neural_net.c): u32 magic, 3 x u16 architecture sizes (accumulator,
    piece features, pawn features), 2 bytes of alignment padding, the
    u32 attack feature count (98304 doesn't fit in a u16), 2 x u16
    hidden layer sizes, then one u8 quantization code per piece field
    (1 = int8, 2 = int16, 3 = int32) in FIELD_ORDER order (the pawn
    and attack blocks have no code): 1 2 2 3 2 3 3 3, then char[32]
    name and char[32] date.  96 bytes total
    (sizeof(WeightsHeader) is the arbiter: the 94 field bytes +
    2 bytes of trailing alignment padding)."""
    header = weights_header("Test Net", "2026-01-01")
    assert len(header) == HEADER_SIZE == 96
    assert header == struct.pack(
        "<IHHH2xIHH8BH32s32s2x",
        WEIGHTS_HEADER_MAGIC,
        ACCUMULATOR_SIZE,
        N_FEATURES,
        N_PAWN_FEATURES,
        N_ATTACK_FEATURES,
        HIDDEN_SIZE_1,
        HIDDEN_SIZE_2,
        1,
        2,
        2,
        3,
        2,
        3,
        3,
        3,
        NN_PHASE_COUNT,
        b"Test Net",
        b"2026-01-01",
    )


def _phase_offset():
    """Byte offset of wh_phase_count within the packed header."""
    return struct.calcsize("<IHHH2xIHH8B")


def test_header_phase_count_mismatch_rejected(tmp_path):
    """A file whose header phase count differs (one written with a
    different NN_PHASE_COUNT) must be rejected."""
    q = _zero_net()
    path = tmp_path / "badphases.quant"
    write_network_quant(path, q)
    data = bytearray(path.read_bytes())
    data[_phase_offset()] ^= 0xFF
    path.write_bytes(bytes(data))
    with pytest.raises(ValueError, match="phase"):
        read_network_quant(path)


def _zero_net():
    """Zeroed quantized field arrays, shaped per FIELD_ORDER."""
    return {name: np.zeros(shape, dtype=dtype) for name, dtype, shape in FIELD_ORDER}


def test_hand_computed_evaluation():
    """All-zero weights with a known activation chain: every position
    must evaluate to exactly SCALE_OUTPUT mp.

    Chain (phase 0): acc = 0 (clamped from 0); h1 pre = bias = 1.0
    (i.e. SCALE_H1 in 64*8192 units) -> act (SCALE_H1 + 4095) / 8192
    = SCALE_INPUT; h2 pre = SCALE_INPUT * SCALE (w=1.0) -> act SCALE
    (the >= clamp boundary); out = SCALE * SCALE (w=1.0) -> score =
    (SCALE * SCALE_OUTPUT + 4095) / SCALE = SCALE_OUTPUT.
    """
    q = _zero_net()
    q["hidden_layer_1_bias_q_0"][0] = SCALE_INPUT * SCALE  # 1.0
    q["hidden_layer_2_weights_q_0"][0, 0] = SCALE  # 1.0
    q["output_layer_weights_q_0"][0] = SCALE  # 1.0
    net = QuantizedNet.from_arrays(q)
    board = _board_from_epd("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -")
    fw, fb, pfw, pfb, afw, afb = _feats(board)
    phase = np.array([0], dtype=np.int32)
    score = net.evaluate(fw, fb, pfw, pfb, afw, afb, np.array([0]), phase)
    assert score[0] == SCALE_OUTPUT
    # independent of the board
    board2 = _board_from_epd("4k3/8/8/8/8/8/8/R3K3 w - -")
    fw2, fb2, pfw2, pfb2, afw2, afb2 = _feats(board2)
    assert (
        net.evaluate(fw2, fb2, pfw2, pfb2, afw2, afb2, np.array([0]), phase)[0]
        == SCALE_OUTPUT
    )


def test_phase_selection_changes_score():
    """Each phase branch is independent: perturbing one phase's output
    bias changes that phase's score but leaves the other phases
    bit-identical (mirrors the C engine's hard switch)."""
    q = _zero_net()
    for phase in range(NN_PHASE_COUNT):
        q[f"hidden_layer_1_bias_q_{phase}"][0] = SCALE_INPUT * SCALE
        q[f"hidden_layer_2_weights_q_{phase}"][0, 0] = SCALE
        q[f"output_layer_weights_q_{phase}"][0] = SCALE
    net = QuantizedNet.from_arrays(q)
    board = _board_from_epd("4k3/8/8/8/8/8/8/R3K3 w - -")
    fw, fb, pfw, pfb, afw, afb = _feats(board)
    turn = np.array([0])

    def score_of(phase):
        return int(net.evaluate(fw, fb, pfw, pfb, afw, afb, turn, np.array([phase]))[0])

    base = [score_of(p) for p in range(NN_PHASE_COUNT)]
    # All zero branches agree at SCALE_OUTPUT.
    assert base == [SCALE_OUTPUT] * NN_PHASE_COUNT
    # Perturb phase 2 only.
    saved = q["output_layer_bias_q_2"][0]
    q["output_layer_bias_q_2"][0] = 1000000
    net2 = QuantizedNet.from_arrays(q)
    s2 = [
        int(net2.evaluate(fw, fb, pfw, pfb, afw, afb, turn, np.array([p]))[0])
        for p in range(NN_PHASE_COUNT)
    ]
    assert s2[2] != base[2]
    for p in range(NN_PHASE_COUNT):
        if p != 2:
            assert s2[p] == base[p]
    q["output_layer_bias_q_2"][0] = saved


def test_hand_computed_accumulator():
    """Single active piece feature with int8 weight 100 per neuron,
    single active pawn pair row with weight 10, single active attack
    row with weight 7, bias 5 (all scale 64): accumulator = 5 + 100 +
    10 + 7 = 122.  122 exceeds the [0, 64] evaluation clamp, but
    clamping only happens in evaluate(), so the raw accumulator must
    report 122."""
    q = _zero_net()
    q["input_layer_weights_q"][7] = 100  # some piece feature
    q["input_pawn_weights_q"][27] = 10  # some pawn pair row
    q["input_attack_weights_q"][5] = 7  # some attack row
    q["input_layer_bias_q"][:] = 5
    net = QuantizedNet.from_arrays(q)
    fw = np.full((1, MAX_FEATURES), -1, dtype=np.int32)
    fb = np.full((1, MAX_FEATURES), -1, dtype=np.int32)
    pfw = np.full((1, MAX_PAWN_FEATURES), -1, dtype=np.int32)
    pfb = np.full((1, MAX_PAWN_FEATURES), -1, dtype=np.int32)
    afw = np.full((1, MAX_ATTACK_FEATURES), -1, dtype=np.int32)
    afb = np.full((1, MAX_ATTACK_FEATURES), -1, dtype=np.int32)
    fw[0, 0] = 7  # feature active on the white accumulator only
    pfw[0, 0] = 27  # pawn pair active on the white accumulator only
    afw[0, 0] = 5  # attack row active on the white accumulator only
    acc = net.accumulator(fw, pfw, afw)
    assert (acc[0] == 5 + 100 + 10 + 7).all()
    acc_b = net.accumulator(fb, pfb, afb)
    assert (acc_b[0] == 5).all()


def test_write_read_roundtrip(tmp_path):
    q = _zero_net()
    q["input_layer_bias_q"] = np.arange(ACCUMULATOR_SIZE, dtype=np.int16)
    q["output_layer_bias_q_0"] = np.array([12345], dtype=np.int32)
    q["output_layer_bias_q_3"] = np.array([98765], dtype=np.int32)
    path = tmp_path / "amy.net"
    n = write_network_quant(path, q)
    assert n == NETWORK_QUANT_SIZE
    back = read_network_quant(path)
    for key in q:
        assert (back[key] == q[key]).all(), key


def test_read_rejects_bad_size(tmp_path):
    path = tmp_path / "bad.quant"
    path.write_bytes(b"\x00" * 100)
    with pytest.raises(ValueError, match="size"):
        read_network_quant(path)


def test_read_rejects_bad_header(tmp_path):
    q = _zero_net()
    path = tmp_path / "badheader.quant"
    write_network_quant(path, q)
    for offset, (what, match) in {
        0: ("magic", "magic"),
        4: ("accumulator size", "sizes"),
        8: ("pawn features size", "sizes"),
        12: ("attack features size", "sizes"),
        20: ("quant codes", "quantization"),  # codes start after the sizes
    }.items():
        data = bytearray(path.read_bytes())
        data[offset] ^= 0xFF
        path.write_bytes(bytes(data))
        with pytest.raises(ValueError, match=match):
            read_network_quant(path)
        # restore
        data[offset] ^= 0xFF
        path.write_bytes(bytes(data))
    read_network_quant(path)  # uncorrupted file still reads fine


def test_quantize_matches_int_arrays():
    """Quantizing float values v gives round(v * scale) ints, one block
    per phase."""
    rng = np.random.default_rng(0)
    float_params = {
        "input_w": rng.normal(0, 0.02, (N_FEATURES, ACCUMULATOR_SIZE)),
        "input_b": rng.normal(0, 0.02, ACCUMULATOR_SIZE),
        "pawn_w": rng.normal(0, 0.02, (N_PAWN_FEATURES, ACCUMULATOR_SIZE)),
        "attack_w": rng.normal(0, 0.02, (N_ATTACK_FEATURES, ACCUMULATOR_SIZE)),
        "h1_w": rng.normal(
            0, 0.06, (NN_PHASE_COUNT, 2 * ACCUMULATOR_SIZE, HIDDEN_SIZE_1)
        ),
        "h1_b": rng.normal(0, 0.1, (NN_PHASE_COUNT, HIDDEN_SIZE_1)),
        "h2_w": rng.normal(0, 0.25, (NN_PHASE_COUNT, HIDDEN_SIZE_1, HIDDEN_SIZE_2)),
        "h2_b": rng.normal(0, 0.1, (NN_PHASE_COUNT, HIDDEN_SIZE_2)),
        "out_w": rng.normal(0, 0.05, (NN_PHASE_COUNT, HIDDEN_SIZE_2)),
        "out_b": rng.normal(0, 0.05, (NN_PHASE_COUNT,)),
    }
    q = quantize_parameters(float_params)
    assert (
        q["input_layer_weights_q"]
        == np.round(float_params["input_w"] * SCALE_INPUT).astype(np.int8)
    ).all()
    assert (
        q["input_pawn_weights_q"]
        == np.round(float_params["pawn_w"] * SCALE_INPUT).astype(np.int8)
    ).all()
    assert (
        q["input_attack_weights_q"]
        == np.round(float_params["attack_w"] * SCALE_INPUT).astype(np.int8)
    ).all()
    assert (
        q["input_layer_bias_q"]
        == np.round(float_params["input_b"] * SCALE_INPUT).astype(np.int16)
    ).all()
    for phase in range(NN_PHASE_COUNT):
        assert (
            q[f"hidden_layer_1_weights_q_{phase}"]
            == np.round(float_params["h1_w"][phase].T * SCALE).astype(np.int16)
        ).all(), phase
        assert (
            q[f"hidden_layer_1_bias_q_{phase}"]
            == np.round(float_params["h1_b"][phase] * SCALE_INPUT * SCALE).astype(
                np.int32
            )
        ).all(), phase
        assert (
            q[f"hidden_layer_2_weights_q_{phase}"]
            == np.round(float_params["h2_w"][phase].T * SCALE).astype(np.int16)
        ).all(), phase
        assert (
            q[f"hidden_layer_2_bias_q_{phase}"]
            == np.round(float_params["h2_b"][phase] * SCALE * SCALE_INPUT).astype(
                np.int32
            )
        ).all(), phase
        assert (
            q[f"output_layer_weights_q_{phase}"]
            == np.round(float_params["out_w"][phase] * SCALE).astype(np.int32)
        ).all(), phase
        assert (
            q[f"output_layer_bias_q_{phase}"]
            == np.round(float_params["out_b"][phase] * SCALE * SCALE).astype(np.int32)
        ).all(), phase


# ---------------------------------------------------------------------------
# Float <-> quantized structural parity (crafted weights)
#
# tests/test_crosscheck.py pins QuantizedNet bit-exactly to amy's C.
# These tests cover the other half: the quantized pipeline must implement
# the *same function* as the float NnueNet it was quantized from.  The
# weights are crafted as exact integers on each field's fixed-point grid
# (integers / scale), so quantize_parameters() is lossless and any
# float-vs-int difference is one of the documented rounding/clamp steps.
#
# The medium configuration is the structural discriminator: it is sized
# so that legitimate rounding stays around 400 mp (tolerance 1500 mp)
# while any structural error (einsum subscript mixup, transposed h2_w,
# scale error, side-order swap, phase mixup) moves the int pipeline away
# from the float model by O(1e4) mp.  Validated against the historical
# `mx.einsum("bpi,pjv->bpv")` mutation — a h2 einsum that contracted no
# cross axis at all and computed an outer product of marginal sums
# instead of the matmul: it moves the pipeline by ~1e4 mp here, while
# the old test (random weights, 500 mp tolerance, now replaced) missed
# it.
# ---------------------------------------------------------------------------

PARITY_SEED = 99
PARITY_TOL_MEDIUM_MP = 1500  # ~4x the measured clean deviation (390 mp)
PARITY_TOL_TINY_MP = 8  # tiny weights: rounding stays well below 1 mp
PARITY_TOL_SAT_MP = 8  # saturating config: lands exactly on the clamps

# Positions from tests/data/crosscheck_reference.json: piece and pawn
# features (fixtures 1-2), attack rows on both sides (fixtures 3, 5),
# a dual-active boundary position (fixture 4, masks (1,1,0,0)), and
# black to move (fixture 2) to exercise the other side order.
PARITY_FIXTURES = (
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -",
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR b KQkq -",
    "r1bqkb1r/pppp1ppp/2n2n2/1B2p3/4P3/5N2/PPPP1PPP/RNBQK2R w KQkq -",
    "3r1r2/8/8/8/8/8/4K2k/8 w - -",
    "2R1r1r1/8/8/8/8/8/4K2k/8 w - -",
)

_FIELD_SHAPES = (
    ("input_w", (N_FEATURES, ACCUMULATOR_SIZE)),
    ("input_b", (ACCUMULATOR_SIZE,)),
    ("pawn_w", (N_PAWN_FEATURES, ACCUMULATOR_SIZE)),
    ("attack_w", (N_ATTACK_FEATURES, ACCUMULATOR_SIZE)),
    ("h1_w", (2 * ACCUMULATOR_SIZE, HIDDEN_SIZE_1)),
    ("h1_b", (HIDDEN_SIZE_1,)),
    ("h2_w", (HIDDEN_SIZE_1, HIDDEN_SIZE_2)),
    ("h2_b", (HIDDEN_SIZE_2,)),
    ("out_w", (HIDDEN_SIZE_2,)),
    ("out_b", (NN_PHASE_COUNT,)),  # 1-D, like NnueNet.out_b
)

_PHASED_FIELDS = ("h1_w", "h1_b", "h2_w", "h2_b", "out_w", "out_b")

_FIELD_SCALES = {
    "input_w": SCALE_INPUT,
    "input_b": SCALE_INPUT,
    "pawn_w": SCALE_INPUT,
    "attack_w": SCALE_INPUT,
    "h1_w": SCALE,
    "h1_b": SCALE_INPUT * SCALE,
    "h2_w": SCALE,
    "h2_b": SCALE_INPUT * SCALE,
    "out_w": SCALE,
    "out_b": SCALE * SCALE,
}

# Integer ranges per field.  The medium profile is sized so that the
# per-phase signal is high and the documented rounding stays low; if it
# (or the seed) changes, re-measure: the clean deviation must stay well
# below PARITY_TOL_MEDIUM_MP and the signal-floor guard in
# test_float_quantized_parity must pass.
MEDIUM_PROFILE = {
    "input_w": (0, 8),
    "input_b": (0, 4),
    "pawn_w": (0, 8),
    "attack_w": (0, 8),
    "h1_w": (300, 600),
    "h1_b": (0, 2),
    "h2_w": (1500, 4000),
    "h2_b": (0, 500),
    "out_w": (3000, 8000),
    "out_b": (0, 100),
}
TINY_PROFILE = {field: (1, 4) for field in MEDIUM_PROFILE}


def _crafted(seed, profile):
    """Crafted float parameters plus the drawn integer values.

    Every field is seeded integers in profile[field] (drawn once per
    phase for the per-phase fields) on the field's fixed-point grid, so
    quantize_parameters() is lossless for them.
    """
    rng = np.random.default_rng(seed)

    def sints(shape, lo, hi):
        return rng.integers(lo, hi + 1, size=shape) * rng.choice([-1, 1], size=shape)

    fparams = {}
    ints = {}
    for field, shape in _FIELD_SHAPES:
        lo, hi = profile[field]
        if field == "out_b":
            # The model's out_b is 1-D (NN_PHASE_COUNT,), not (P, 1).
            drawn = sints(shape, lo, hi)
        elif field in _PHASED_FIELDS:
            drawn = np.stack([sints(shape, lo, hi) for _ in range(NN_PHASE_COUNT)])
        else:
            drawn = sints(shape, lo, hi)
        ints[field] = drawn
        fparams[field] = drawn.astype(np.float64) / _FIELD_SCALES[field]
    return fparams, ints


def _saturating_weights(sign):
    """Deterministic all-same-sign config: positive saturates every h1/h2
    neuron at 1.0 (upper clamp path), negative collapses everything to 0
    (q <= 0 branch).  The pattern varies per element, so it is not a
    degenerate constant (and the h2 patterns are asymmetric)."""
    A, H1, H2 = ACCUMULATOR_SIZE, HIDDEN_SIZE_1, HIDDEN_SIZE_2
    da = np.arange(A)[None, :]
    di, dj = np.arange(2 * A)[:, None], np.arange(H1)[None, :]
    dj2, dv = np.arange(H1)[:, None], np.arange(H2)[None, :]
    return {
        "input_w": sign * ((np.arange(N_FEATURES)[:, None] + da) % 3 + 1) / SCALE_INPUT,
        "input_b": sign * np.full(A, 8, dtype=np.float64) / SCALE_INPUT,
        "pawn_w": sign
        * ((np.arange(N_PAWN_FEATURES)[:, None] + da) % 3 + 1)
        / SCALE_INPUT,
        "attack_w": sign
        * ((np.arange(N_ATTACK_FEATURES)[:, None] + da) % 3 + 1)
        / SCALE_INPUT,
        "h1_w": np.stack(
            [
                sign * (300 + (3 * di + 5 * dj + 7 * p) % 300)
                for p in range(NN_PHASE_COUNT)
            ]
        )
        / SCALE,
        "h1_b": sign
        * np.full((NN_PHASE_COUNT, H1), 2, dtype=np.float64)
        / (SCALE_INPUT * SCALE),
        "h2_w": np.stack(
            [
                sign * (300 + (3 * dj2 + 5 * dv + 7 * p) % 300)
                for p in range(NN_PHASE_COUNT)
            ]
        )
        / SCALE,
        "h2_b": sign
        * np.full((NN_PHASE_COUNT, H2), 500, dtype=np.float64)
        / (SCALE_INPUT * SCALE),
        "out_w": np.stack(
            [
                sign * (3000 + (3 * dv.reshape(-1) + 5 * p) % 5000)
                for p in range(NN_PHASE_COUNT)
            ]
        )
        / SCALE,
        "out_b": sign
        * np.full(NN_PHASE_COUNT, 100, dtype=np.float64)
        / (SCALE * SCALE),
    }


def _parity_features(epd):
    """(turn, feature blocks, C phase masks) for an EPD via the C
    extraction library — the same indices feed both networks."""
    turn = 0 if epd.split()[1] == "w" else 1
    board = _board_from_epd(epd)
    rec = encode_board(board, turn, 0)
    assert rec.validate() == []
    ex = cfeatures.extract(rec.encode(), 1)
    assert ex.valid[0] == 1
    feats = (ex.feat_w, ex.feat_b, ex.pfeat_w, ex.pfeat_b, ex.afeat_w, ex.afeat_b)
    return turn, feats, ex.phase[0]


def _set_params(model, fparams):
    import mlx.core as mx

    for name, v in fparams.items():
        setattr(model, name, mx.array(v.astype(np.float32)))


def _build_pair(fparams):
    """(float model, quantized net) from the same crafted parameters."""
    pytest.importorskip("mlx.core")
    from amy_training_nnue.model import NnueNet

    model = NnueNet(seed=0)
    _set_params(model, fparams)
    f32 = {k: v.astype(np.float32) for k, v in fparams.items()}
    return model, QuantizedNet.from_arrays(quantize_parameters(f32))


def _assert_parity(model, net, tol_mp):
    """Compare NnueNet.heads() with QuantizedNet.evaluate() per phase on
    all fixtures; any deviation beyond tol_mp means the pipelines
    disagree beyond the documented rounding.  Returns the max |float
    head| per phase (mp) over the fixtures, for the signal-floor
    guard."""
    pytest.importorskip("mlx.core")
    import mlx.core as mx

    per_phase = [0.0] * NN_PHASE_COUNT
    for epd in PARITY_FIXTURES:
        turn, feats, _ = _parity_features(epd)
        heads = np.array(model.heads(*[mx.array(t) for t in (*feats, [turn])]))[0]
        rep = [np.repeat(x, NN_PHASE_COUNT, axis=0) for x in feats]
        ints = net.evaluate(
            *rep,
            np.full(NN_PHASE_COUNT, turn, np.uint8),
            np.arange(NN_PHASE_COUNT, dtype=np.int32),
        )
        for p in range(NN_PHASE_COUNT):
            float_mp = float(heads[p]) * 10000
            per_phase[p] = max(per_phase[p], abs(float_mp))
            dev = abs(float_mp - float(ints[p]))
            assert dev <= tol_mp, (
                f"{epd} phase {p}: float {float_mp:.1f} vs int {ints[p]} mp "
                f"(dev {dev:.1f} > {tol_mp})"
            )
    return per_phase


def _hidden_activations(net, feats, turn, h1w, h1b, h2w, h2b):
    """float64 numpy recompute of the per-phase h1/h2 activations from
    the quantized net's clamped accumulator and crafted (scaled) integer
    weights.  Fixture guards only: prove the hidden layers exercise the
    range they are supposed to."""
    fw, fb, pfw, pfb, afw, afb = feats
    acc_w = np.clip(net.accumulator(fw, pfw, afw), 0, SCALE_INPUT)
    acc_b = np.clip(net.accumulator(fb, pfb, afb), 0, SCALE_INPUT)
    x = (
        np.concatenate([acc_w, acc_b] if turn == 0 else [acc_b, acc_w], axis=1)
        / SCALE_INPUT
    )
    h1s, h2s = [], []
    for p in range(NN_PHASE_COUNT):
        h1 = np.clip(x @ h1w[p] + h1b[p], 0.0, 1.0)
        h2 = np.clip(h1 @ h2w[p] + h2b[p], 0.0, 1.0)
        h1s.append(h1)
        h2s.append(h2)
    return h1s, h2s


def test_float_quantized_parity():
    """Medium-magnitude crafted weights: the float model and its
    quantization must agree on every phase of every fixture within the
    documented rounding, and the blended output at the dual-active
    boundary position must equal the average of the two active int
    branches.  This configuration is the structural discriminator (see
    the section docstring): an einsum/transpose/scale/side-order/phase
    error moves the int pipeline away from the float model by O(1e4)
    mp, far above the tolerance."""
    pytest.importorskip("mlx.core")
    import mlx.core as mx

    fparams, ints = _crafted(PARITY_SEED, MEDIUM_PROFILE)
    model, net = _build_pair(fparams)

    # --- fixture guards: the configuration must actually discriminate ---
    # The raw accumulator stays in the int16 no-wrap range (C wraps
    # int16; the float-vs-int comparison is only meaningful without
    # wrap) and the [0, SCALE_INPUT] evaluation clamp is exercised.
    raw = 0
    turns, attack_w, attack_b = set(), False, False
    for epd in PARITY_FIXTURES:
        turn, feats, _ = _parity_features(epd)
        fw, fb, pfw, pfb, afw, afb = feats
        turns.add(turn)
        attack_w |= bool((afw[0] >= 0).any())
        attack_b |= bool((afb[0] >= 0).any())
        raw = max(
            raw,
            int(np.abs(net.accumulator(fw, pfw, afw)).max()),
            int(np.abs(net.accumulator(fb, pfb, afb)).max()),
        )
    assert raw < 2**15
    assert raw > SCALE_INPUT
    assert turns == {0, 1}  # both side orders
    assert attack_w and attack_b  # attack rows on both sides

    # The h2 patterns must be asymmetric: a transposed h2_w would
    # otherwise be invisible.
    for p in range(NN_PHASE_COUNT):
        assert (ints["h2_w"][p] != ints["h2_w"][p].T).any(), p

    # The hidden layers must not saturate (a saturated layer would not
    # exercise the matmul): the majority of neurons sit strictly
    # inside (0, 1).
    turn0, feats0, _ = _parity_features(PARITY_FIXTURES[0])
    h1s, h2s = _hidden_activations(
        net,
        feats0,
        turn0,
        [i / SCALE for i in ints["h1_w"]],
        [i / (SCALE_INPUT * SCALE) for i in ints["h1_b"]],
        [i / SCALE for i in ints["h2_w"]],
        [i / (SCALE_INPUT * SCALE) for i in ints["h2_b"]],
    )
    for p in range(NN_PHASE_COUNT):
        assert ((h1s[p] > 0.05) & (h1s[p] < 0.95)).mean() >= 0.2, p
        assert ((h2s[p] > 0.05) & (h2s[p] < 0.95)).mean() >= 0.2, p

    # --- parity, per phase ---
    per_phase = _assert_parity(model, net, PARITY_TOL_MEDIUM_MP)
    for p in range(NN_PHASE_COUNT):
        assert per_phase[p] >= 2 * PARITY_TOL_MEDIUM_MP, (
            f"phase {p} signal {per_phase[p]:.0f} mp is too weak to "
            f"discriminate against the {PARITY_TOL_MEDIUM_MP} mp tolerance"
        )

    # --- blend at the dual-active boundary position ---
    turn, feats, masks = _parity_features(PARITY_FIXTURES[3])
    active = [p for p in range(NN_PHASE_COUNT) if masks[p]]
    assert len(active) == 2  # C phase masks: this is a boundary position
    blend = (
        float(
            model(
                *[
                    mx.array(t)
                    for t in (*feats, [turn], masks.astype(np.int8).reshape(1, -1))
                ]
            ).item()
        )
        * 10000
    )
    fw, fb, pfw, pfb, afw, afb = feats
    branch = [
        float(
            net.evaluate(
                fw, fb, pfw, pfb, afw, afb, np.array([turn]), np.array([p], np.int32)
            )[0]
        )
        for p in active
    ]
    assert abs(blend - sum(branch) / len(branch)) <= PARITY_TOL_MEDIUM_MP


def test_float_quantized_parity_tiny_exact():
    """Tiny integer weights: quantize_parameters() must recover the
    drawn integers exactly (pins every field's scale and transpose — a
    scale or layout change fails here), and float/int parity holds
    within a tight tolerance."""
    pytest.importorskip("mlx.core")
    fparams, ints = _crafted(PARITY_SEED, TINY_PROFILE)
    q = quantize_parameters({k: v.astype(np.float32) for k, v in fparams.items()})
    assert (q["input_layer_weights_q"] == ints["input_w"].astype(np.int8)).all()
    assert (q["input_layer_bias_q"] == ints["input_b"].astype(np.int16)).all()
    assert (q["input_pawn_weights_q"] == ints["pawn_w"].astype(np.int8)).all()
    assert (q["input_attack_weights_q"] == ints["attack_w"].astype(np.int8)).all()
    for p in range(NN_PHASE_COUNT):
        assert (
            q[f"hidden_layer_1_weights_q_{p}"] == ints["h1_w"][p].T.astype(np.int16)
        ).all()
        assert (
            q[f"hidden_layer_1_bias_q_{p}"] == ints["h1_b"][p].astype(np.int32)
        ).all()
        assert (
            q[f"hidden_layer_2_weights_q_{p}"] == ints["h2_w"][p].T.astype(np.int16)
        ).all()
        assert (
            q[f"hidden_layer_2_bias_q_{p}"] == ints["h2_b"][p].astype(np.int32)
        ).all()
        assert (
            q[f"output_layer_weights_q_{p}"] == ints["out_w"][p].astype(np.int32)
        ).all()
        assert (
            q[f"output_layer_bias_q_{p}"] == ints["out_b"][p].astype(np.int32)
        ).all()
    model, net = _build_pair(fparams)
    _assert_parity(model, net, PARITY_TOL_TINY_MP)


def test_float_quantized_parity_saturating():
    """Saturating configs: all-positive weights drive every h1/h2 neuron
    to the 1.0 clamp, all-negative collapse everything to 0; the
    quantized pipeline must match the float model on both clamp paths
    within rounding."""
    for sign in (1, -1):
        fparams = _saturating_weights(sign)
        model, net = _build_pair(fparams)
        turn0, feats0, _ = _parity_features(PARITY_FIXTURES[0])
        h1s, h2s = _hidden_activations(
            net,
            feats0,
            turn0,
            [w.astype(np.float64) for w in fparams["h1_w"]],
            [b.astype(np.float64) for b in fparams["h1_b"]],
            [w.astype(np.float64) for w in fparams["h2_w"]],
            [b.astype(np.float64) for b in fparams["h2_b"]],
        )
        expect = 1.0 if sign > 0 else 0.0
        for p in range(NN_PHASE_COUNT):
            assert np.all(h1s[p] == expect) and np.all(h2s[p] == expect), p
        _assert_parity(model, net, PARITY_TOL_SAT_MP)


def test_float_quantized_parity_file_roundtrip(tmp_path):
    """The exported amy.net artifact must implement the trained
    function: write the medium config, load it through
    QuantizedNet.from_file, and re-run the parity."""
    pytest.importorskip("mlx.core")
    fparams, _ = _crafted(PARITY_SEED, MEDIUM_PROFILE)
    model, net = _build_pair(fparams)
    path = tmp_path / "parity.quant"
    n = write_network_quant(
        path,
        {
            k: v
            for k, v in quantize_parameters(
                {k: v.astype(np.float32) for k, v in fparams.items()}
            ).items()
        },
        name="Parity",
        date_="2026-01-01T00:00:00",
    )
    assert n == NETWORK_QUANT_SIZE
    file_net = QuantizedNet.from_file(path)
    _assert_parity(model, file_net, PARITY_TOL_MEDIUM_MP)
