"""The NNUE model in floating point, implemented with MLX.

Architecture (float counterpart of amy's quantized network):

    input (shared, both sides, all phases):
        acc_side = clip(input_b + sum_{f in side} input_w[f]
                               + sum_{p in side} pawn_w[p]
                               + sum_{a in side} attack_w[a], 0, 1)
        x = [acc_stm, acc_opponent]              # 2*ACCUMULATOR_SIZE values in [0, 1]

    per phase (NN_PHASE_COUNT = 4 independent branches):
        hidden1 = clip(h1_w @ x + h1_b, 0, 1)    # HIDDEN_SIZE_1
        hidden2 = clip(h2_w @ hidden1 + h2_b, 0, 1)
        output  = hidden2 @ out_w + out_b        # 1, in units of
                                                     score_millipawns / 10000

    blended output (training side of the phase contract):
        out = sum_phase a * output_phase / sum_phase a, where a is the
        record's 0/1 active-phase mask from nn_phase_masks() (C
        library): positions at the top of a bucket (non-pawn counts
        2, 5, 8) activate the adjacent higher phase, so training
        co-trains the two heads and the engine's hard switch at
        evaluation stays smooth.

The pawn pair and attack blocks enter the accumulator exactly like
the piece features: shared N_PAWN_FEATURES x ACCUMULATOR_SIZE and
N_ATTACK_FEATURES x ACCUMULATOR_SIZE matrices (one file block each,
both quantized int8 at the input scale), with the active rows summed
into each side's accumulator before the [0, 1] clamp.  The attack
rows are indexed per side by that side's canonical frame, just like
the engine's attack maps.
"""

from __future__ import annotations

import json
from pathlib import Path

import mlx.core as mx
import mlx.nn as nn

from .features import (
    ACCUMULATOR_SIZE,
    HIDDEN_SIZE_1,
    HIDDEN_SIZE_2,
    N_ATTACK_FEATURES,
    N_FEATURES,
    N_PAWN_FEATURES,
    NN_PHASE_COUNT,
)

# The scale amy uses to convert the raw network output to millipawns
# (SCALE_OUTPUT in neural_net.c): score_mp = round(output * 10000).
DEFAULT_TARGET_SCALE = 10000.0

# Weight tensors kept within [-1, 1] during training (biases are exempt).
WEIGHT_NAMES = ("input_w", "pawn_w", "attack_w", "h1_w", "h2_w", "out_w")


class NnueNet(nn.Module):
    def __init__(
        self,
        init_input_std: float = 0.02,
        init_hidden_gains: tuple[float, float] = (1.0, 1.0),
        seed: int = 0,
    ):
        super().__init__()
        mx.random.seed(seed)
        self.input_w = mx.random.normal((N_FEATURES, ACCUMULATOR_SIZE)) * init_input_std
        self.input_b = mx.zeros((ACCUMULATOR_SIZE,))
        # Shared pawn pair block: N_PAWN_FEATURES rows (both sides read
        # the same matrix), int8 at the piece input scale in the file.
        self.pawn_w = (
            mx.random.normal((N_PAWN_FEATURES, ACCUMULATOR_SIZE)) * init_input_std
        )
        # Shared attack block: N_ATTACK_FEATURES rows (both sides read
        # the same matrix, indexed in their own canonical frame), int8
        # at the piece input scale in the file.
        self.attack_w = (
            mx.random.normal((N_ATTACK_FEATURES, ACCUMULATOR_SIZE)) * init_input_std
        )
        # Kaiming (uniform-equivalent via normal) init for the clamped
        # ReLU layers: variance 2 / fan_in, one independent branch per
        # phase (NN_PHASE_COUNT).
        self.h1_w = mx.random.normal(
            (NN_PHASE_COUNT, 2 * ACCUMULATOR_SIZE, HIDDEN_SIZE_1)
        ) * (init_hidden_gains[0] * (2.0 / ACCUMULATOR_SIZE) ** 0.5)
        self.h1_b = mx.zeros((NN_PHASE_COUNT, HIDDEN_SIZE_1))
        self.h2_w = mx.random.normal((NN_PHASE_COUNT, HIDDEN_SIZE_1, HIDDEN_SIZE_2)) * (
            init_hidden_gains[1] * (2.0 / HIDDEN_SIZE_1) ** 0.5
        )
        self.h2_b = mx.zeros((NN_PHASE_COUNT, HIDDEN_SIZE_2))
        self.out_w = mx.random.normal((NN_PHASE_COUNT, HIDDEN_SIZE_2)) * 0.05
        self.out_b = mx.zeros((NN_PHASE_COUNT,))

    def _head(self, feat: mx.array, pfeat: mx.array, afeat: mx.array) -> mx.array:
        """One side's accumulator input, shared by all phases:
        (B, K) int32 piece feature indices + (B, P) int32 pawn pair
        indices + (B, A) int32 attack indices (all padded with -1)
        -> (B, ACCUMULATOR_SIZE), clamped to [0, 1]."""
        valid = feat >= 0
        idx = mx.where(valid, feat, 0)
        gathered = mx.take(self.input_w, idx, axis=0)  # (B, K, 256)
        gathered = mx.where(valid[:, :, None], gathered, 0.0)
        pvalid = pfeat >= 0
        pidx = mx.where(pvalid, pfeat, 0)
        pgathered = mx.take(self.pawn_w, pidx, axis=0)  # (B, P, 256)
        pgathered = mx.where(pvalid[:, :, None], pgathered, 0.0)
        avalid = afeat >= 0
        aid = mx.where(avalid, afeat, 0)
        agathered = mx.take(self.attack_w, aid, axis=0)  # (B, A, 256)
        agathered = mx.where(avalid[:, :, None], agathered, 0.0)
        acc = (
            gathered.sum(axis=1)
            + pgathered.sum(axis=1)
            + agathered.sum(axis=1)
            + self.input_b
        )
        return mx.clip(acc, 0.0, 1.0)

    def heads(
        self,
        feat_w: mx.array,
        feat_b: mx.array,
        pfeat_w: mx.array,
        pfeat_b: mx.array,
        afeat_w: mx.array,
        afeat_b: mx.array,
        turn: mx.array,
    ) -> mx.array:
        """Output of all NN_PHASE_COUNT branches: (B, NN_PHASE_COUNT).

        Each branch independently runs its own {H1, H2, output} stack
        on the shared 2 * ACCUMULATOR_SIZE-dim accumulator input
        (side to move first, as in neural_net.c).
        """
        acc_w = self._head(feat_w, pfeat_w, afeat_w)
        acc_b = self._head(feat_b, pfeat_b, afeat_b)
        white_to_move = turn == 0
        x = mx.where(
            white_to_move[:, None],
            mx.concat([acc_w, acc_b], axis=1),
            mx.concat([acc_b, acc_w], axis=1),
        )  # (B, 2 * ACCUMULATOR_SIZE)

        h1 = mx.clip(
            mx.einsum("bd,pdi->bpi", x, self.h1_w) + self.h1_b,
            0.0,
            1.0,
        )  # (B, P, H1)
        h2 = mx.clip(
            mx.einsum("bpj,pjv->bpv", h1, self.h2_w) + self.h2_b,
            0.0,
            1.0,
        )  # (B, P, H2)
        return mx.einsum("bpv,pv->bp", h2, self.out_w) + self.out_b  # (B, P)

    def __call__(
        self,
        feat_w: mx.array,
        feat_b: mx.array,
        pfeat_w: mx.array,
        pfeat_b: mx.array,
        afeat_w: mx.array,
        afeat_b: mx.array,
        turn: mx.array,
        phase: mx.array,
    ) -> mx.array:
        """Blended output for the record's active phase mask
        ``phase`` (B, NN_PHASE_COUNT) 0/1: (sum a * out) / sum(a),
        as in the training contract (nn_phase_masks of the engine
        source, see PLAN-phase-branches.md)."""
        out = self.heads(feat_w, feat_b, pfeat_w, pfeat_b, afeat_w, afeat_b, turn)
        a = phase.astype(out.dtype)
        active = mx.maximum(a.sum(axis=1), 1.0)
        return (out * a).sum(axis=1) / active

    def clip_weights(self, bound: float = 1.0) -> None:
        """Clip the weight tensors (not the biases) to [-bound, bound]."""
        for name in WEIGHT_NAMES:
            setattr(self, name, mx.clip(getattr(self, name), -bound, bound))

    # ------------------------------------------------------------------
    # (de)serialization
    # ------------------------------------------------------------------

    @property
    def parameters(self) -> dict:
        return {
            "input_w": self.input_w,
            "input_b": self.input_b,
            "pawn_w": self.pawn_w,
            "attack_w": self.attack_w,
            "h1_w": self.h1_w,
            "h1_b": self.h1_b,
            "h2_w": self.h2_w,
            "h2_b": self.h2_b,
            "out_w": self.out_w,
            "out_b": self.out_b,
        }

    def save(self, directory: str | Path) -> None:
        directory = Path(directory)
        directory.mkdir(parents=True, exist_ok=True)
        import numpy as np

        # Materialize the (possibly compiled-graph) parameter arrays.
        mx.eval(self.parameters)
        np.savez(
            directory / "weights.npz",
            **{name: np.array(value) for name, value in self.parameters.items()},
        )
        meta = {
            "n_features": N_FEATURES,
            "n_pawn_features": N_PAWN_FEATURES,
            "n_attack_features": N_ATTACK_FEATURES,
            "accumulator_size": ACCUMULATOR_SIZE,
            "hidden_size_1": HIDDEN_SIZE_1,
            "hidden_size_2": HIDDEN_SIZE_2,
            "phase_count": NN_PHASE_COUNT,
            "target_scale": DEFAULT_TARGET_SCALE,
        }
        (directory / "meta.json").write_text(json.dumps(meta, indent=2))

    # Expected shape per parameter name; used to reject checkpoints from
    # an older architecture (e.g. the 45056-feature king-square layout
    # or the single-branch network) with a clear error instead of
    # silently loading mismatched tensors.
    EXPECTED_SHAPES = {
        "input_w": (N_FEATURES, ACCUMULATOR_SIZE),
        "input_b": (ACCUMULATOR_SIZE,),
        "pawn_w": (N_PAWN_FEATURES, ACCUMULATOR_SIZE),
        "attack_w": (N_ATTACK_FEATURES, ACCUMULATOR_SIZE),
        "h1_w": (NN_PHASE_COUNT, 2 * ACCUMULATOR_SIZE, HIDDEN_SIZE_1),
        "h1_b": (NN_PHASE_COUNT, HIDDEN_SIZE_1),
        "h2_w": (NN_PHASE_COUNT, HIDDEN_SIZE_1, HIDDEN_SIZE_2),
        "h2_b": (NN_PHASE_COUNT, HIDDEN_SIZE_2),
        "out_w": (NN_PHASE_COUNT, HIDDEN_SIZE_2),
        "out_b": (NN_PHASE_COUNT,),
    }

    @classmethod
    def load(cls, directory: str | Path) -> "NnueNet":
        import numpy as np

        model = cls.__new__(cls)
        nn.Module.__init__(model)
        weights = np.load(Path(directory) / "weights.npz", allow_pickle=False)
        for name, value in weights.items():
            if name in cls.EXPECTED_SHAPES and value.shape != cls.EXPECTED_SHAPES[name]:
                raise ValueError(
                    f"{directory}: {name} has shape {value.shape}, expected "
                    f"{cls.EXPECTED_SHAPES[name]} (stale checkpoint from an "
                    "older architecture?)"
                )
            setattr(model, name, mx.array(value))
        return model
