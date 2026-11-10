"""Load BPD selfplay files into a training dataset.

Feature generation is delegated to amy's C feature-extraction
library (cfeatures): the engine is the reference implementation of
the index math, so the training data can never drift from it
(PLAN-attack-map-features.md, T2).  Record validity is decided by
the same C code (bpd_record_valid); invalid records are dropped
and counted in ``n_invalid``.

No mirror augmentation: with the king-half canonicalized features
a position and its horizontal mirror image produce identical
feature vectors, so augmenting with the mirror would only duplicate
rows.
"""

from __future__ import annotations

import numpy as np

from . import cfeatures
from .bpd import RECORD_SIZE


class BpdDataset:
    """All positions of one or more BPD files as packed tensors.

    Attributes:
        feat_w: (N, K) int32, white accumulator feature indices
            (own pieces, opponent pieces, opponent king plane 10),
            padded with -1.  K = the C library's
            max_piece_features.
        feat_b: (N, K) int32, black accumulator feature indices.
        pfeat_w: (N, P) int32, white pawn pair feature indices,
            padded with -1.
        pfeat_b: (N, P) int32, black pawn pair feature indices.
        afeat_w: (N, A) int32, white attack map feature indices,
            padded with -1.
        afeat_b: (N, A) int32, black attack map feature indices.
        turn:   (N,) uint8, 0 = white to move, 1 = black.
        phase:  (N, NN_PHASE_COUNT) int8 0/1 active-phase-head masks,
            as produced by nn_phase_masks() in the C library (always
            at least the engine's nn_phase_index(); positions at the
            top of a bucket, non-pawn counts 2/5/8, additionally
            activate the adjacent higher phase for co-training).
        target: (N,) float32, stored score / target_scale.
        score:  (N,) int32, stored score in millipawns (side to move).
    """

    def __init__(self, paths: list[str], target_scale: float = 10000.0):
        if not paths:
            raise ValueError("no input files given")
        blocks: list[tuple[cfeatures.Extracted, np.ndarray]] = []
        n_invalid = 0
        for path in paths:
            raw = np.fromfile(path, dtype=np.uint8)
            if len(raw) % RECORD_SIZE:
                raise ValueError(
                    f"{path}: size {len(raw)} is not a multiple of {RECORD_SIZE}"
                )
            n = len(raw) // RECORD_SIZE
            if n == 0:
                continue
            ex = cfeatures.extract(raw, n)
            keep = ex.valid.astype(bool)
            n_invalid += n - int(keep.sum())
            if keep.any():
                blocks.append((ex, keep))
        if not blocks:
            raise ValueError(f"no valid records found in {paths}")

        def gather(field: str) -> np.ndarray:
            return np.concatenate([getattr(ex, field)[mask] for ex, mask in blocks])

        self.feat_w = gather("feat_w")
        self.feat_b = gather("feat_b")
        self.pfeat_w = gather("pfeat_w")
        self.pfeat_b = gather("pfeat_b")
        self.afeat_w = gather("afeat_w")
        self.afeat_b = gather("afeat_b")
        self.turn = gather("turn")
        # Per-sample 0/1 active-head masks (0, 0, 0, 0) from C.
        phase = gather("phase")
        self.phase = phase.astype(np.int8)
        # The engine's own selector (the lower of the active phases),
        # for QuantizedNet evaluation: nn_phase_index() = first active.
        self.phase_index = np.argmax(phase, axis=1).astype(np.int32)
        self.score = gather("score")
        self.target = self.score.astype(np.float32) / float(target_scale)
        self.n = sum(int(mask.sum()) for _, mask in blocks)
        self.n_invalid = n_invalid

    def split(self, val_fraction: float = 0.1, seed: int = 0):
        """Shuffle and return (train_idx, val_idx) as int64 arrays."""
        rng = np.random.default_rng(seed)
        idx = rng.permutation(self.n)
        n_val = int(self.n * val_fraction)
        if self.n >= 2:
            n_val = min(max(n_val, 1), self.n - 1)
        return idx[n_val:], idx[:n_val]
