"""Tests for the MLX model."""

import numpy as np
import pytest

pytest.importorskip("mlx.core")
import mlx.core as mx  # noqa: E402

from amy_training_nnue.features import (  # noqa: E402
    FLIP,
    N_FEATURES,
    NN_PHASE_COUNT,
    PLANE_SIZE,
    feature_indices,
    pawn_feature_indices,
)
from amy_training_nnue.model import WEIGHT_NAMES, NnueNet  # noqa: E402
from tests.test_features import _board_from_epd  # noqa: E402

# Padding widths for the synthetic batches below (real dataset widths
# come from the C feature-extraction constants, see cfeatures.py).
MAX_FEATURES = 32
MAX_PAWN_FEATURES = 128
MAX_ATTACK_FEATURES = 32


def _feats(board, batch=1):
    """Oracle piece/pawn indices plus an empty attack block (the
    synthetic tests below don't exercise attack rows; the cross-check
    suite covers those against C)."""
    w, b = feature_indices(board)
    pw, pb = pawn_feature_indices(board)
    fw = np.full((batch, MAX_FEATURES), -1, dtype=np.int32)
    fb = np.full((batch, MAX_FEATURES), -1, dtype=np.int32)
    pfw = np.full((batch, MAX_PAWN_FEATURES), -1, dtype=np.int32)
    pfb = np.full((batch, MAX_PAWN_FEATURES), -1, dtype=np.int32)
    afw = np.full((batch, MAX_ATTACK_FEATURES), -1, dtype=np.int32)
    afb = np.full((batch, MAX_ATTACK_FEATURES), -1, dtype=np.int32)
    fw[:, : len(w)] = w
    fb[:, : len(b)] = b
    pfw[:, : len(pw)] = pw
    pfb[:, : len(pb)] = pb
    return fw, fb, pfw, pfb, afw, afb


def _phase(batch, active=0, adjacent=False):
    """A one-hot (or, when adjacent, dual-active) 0/1 mask of shape
    (batch, NN_PHASE_COUNT) for the model's blended call."""
    m = np.zeros((batch, NN_PHASE_COUNT), dtype=np.int8)
    m[:, active] = 1
    if adjacent and active + 1 < NN_PHASE_COUNT:
        m[:, active + 1] = 1
    return m


def _forward(model, fw, fb, pfw, pfb, afw, afb, turn, m=None):
    if m is None:
        m = _phase(len(fw))
    return model(
        mx.array(fw),
        mx.array(fb),
        mx.array(pfw),
        mx.array(pfb),
        mx.array(afw),
        mx.array(afb),
        mx.array(turn) if isinstance(turn, list) else turn,
        mx.array(m),
    )


def test_forward_shape():
    model = NnueNet(seed=1)
    board = _board_from_epd("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -")
    fw, fb, pfw, pfb, afw, afb = _feats(board)
    out = _forward(model, fw, fb, pfw, pfb, afw, afb, [0])
    assert out.shape == (1,)
    assert np.isfinite(out.item())


def test_forward_all_heads():
    """heads() returns one value per phase branch, independent of the
    mask; the blended call with a one-hot mask equals that head."""
    model = NnueNet(seed=1)
    board = _board_from_epd("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -")
    fw, fb, pfw, pfb, afw, afb = _feats(board)
    heads = np.array(
        model.heads(
            mx.array(fw),
            mx.array(fb),
            mx.array(pfw),
            mx.array(pfb),
            mx.array(afw),
            mx.array(afb),
            mx.array([0]),
        )
    )
    assert heads.shape == (1, NN_PHASE_COUNT)
    for phase in range(NN_PHASE_COUNT):
        single = float(
            _forward(model, fw, fb, pfw, pfb, afw, afb, [0], _phase(1, phase))
        )
        assert single == pytest.approx(float(heads[0, phase]), rel=1e-6, abs=1e-7)


def test_blend_averages_active_heads():
    """With two active heads the blended output is their mean (the
    training contract: sum a*out / sum a)."""
    model = NnueNet(seed=1)
    board = _board_from_epd("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -")
    fw, fb, pfw, pfb, afw, afb = _feats(board)
    h0 = float(_forward(model, fw, fb, pfw, pfb, afw, afb, [0], _phase(1, 0)))
    h1 = float(_forward(model, fw, fb, pfw, pfb, afw, afb, [0], _phase(1, 1)))
    both = float(
        _forward(model, fw, fb, pfw, pfb, afw, afb, [0], _phase(1, 0, adjacent=True))
    )
    assert both == pytest.approx((h0 + h1) / 2, rel=1e-6, abs=1e-7)


def test_deterministic():
    m1 = NnueNet(seed=7)
    m2 = NnueNet(seed=7)
    board = _board_from_epd("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -")
    fw, fb, pfw, pfb, afw, afb = _feats(board)
    o1 = np.array(_forward(m1, fw, fb, pfw, pfb, afw, afb, [0]))
    o2 = np.array(_forward(m2, fw, fb, pfw, pfb, afw, afb, [0]))
    assert o1 == o2


def test_turn_symmetry():
    """Evaluating a position with white to move must equal evaluating
    the vertically flipped position with black to move: the features
    are identical and the STM/opponent ordering is the same."""
    model = NnueNet(seed=3)
    board = _board_from_epd(
        "r1bqkbnr/pppp1ppp/2n5/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq -"
    )
    fw, fb, pfw, pfb, afw, afb = _feats(board)
    flipped = np.zeros(64, dtype=np.int8)
    for sq in range(64):
        flipped[sq] = -board[sq ^ FLIP]
    fwf, fbf, pfwf, pfbf, afwf, afbf = _feats(flipped)
    s_w = float(_forward(model, fw, fb, pfw, pfb, afw, afb, [0]))
    s_b = float(_forward(model, fwf, fbf, pfwf, pfbf, afwf, afbf, [1]))
    assert s_w == pytest.approx(s_b, rel=1e-6, abs=1e-6)


def test_batch_matches_single():
    model = NnueNet(seed=5)
    specs = [
        ("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -", 0),
        ("rnbqkbnr/pppp1ppp/8/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq -", 0),
        ("r3k2r/8/8/8/8/8/8/4K3 b - -", 1),
    ]
    fws, fbs, pfws, pfbs, afws, afbs, turns = [], [], [], [], [], [], []
    for epd, turn in specs:
        fw, fb, pfw, pfb, afw, afb = _feats(_board_from_epd(epd))
        fws.append(fw)
        fbs.append(fb)
        pfws.append(pfw)
        pfbs.append(pfb)
        afws.append(afw)
        afbs.append(afb)
        turns.append(turn)
    n = len(specs)
    batch = np.array(
        _forward(
            model,
            np.concatenate(fws),
            np.concatenate(fbs),
            np.concatenate(pfws),
            np.concatenate(pfbs),
            np.concatenate(afws),
            np.concatenate(afbs),
            turns,
        )
    )
    for i, (epd, turn) in enumerate(specs):
        f1w, f1b, f1pw, f1pb, f1aw, f1ab = _feats(_board_from_epd(epd))
        single = float(_forward(model, f1w, f1b, f1pw, f1pb, f1aw, f1ab, [turn]))
        assert batch[i] == pytest.approx(single, rel=1e-6, abs=1e-6)


def test_clip_weights_only_weights():
    model = NnueNet(seed=1)
    for name in WEIGHT_NAMES:
        setattr(model, name, mx.full(getattr(model, name).shape, 5.0))
    for name, v in (("input_b", 9.0), ("h1_b", -9.0), ("h2_b", 8.0), ("out_b", -8.0)):
        setattr(model, name, mx.full(getattr(model, name).shape, v))
    model.clip_weights(1.0)
    for name in WEIGHT_NAMES:
        w = np.array(getattr(model, name))
        assert np.all((w >= -1.0) & (w <= 1.0))
    # biases are untouched
    for name, v in (("input_b", 9.0), ("h1_b", -9.0), ("h2_b", 8.0), ("out_b", -8.0)):
        assert np.array(getattr(model, name)) == pytest.approx(v)


def test_save_load(tmp_path):
    model = NnueNet(seed=11)
    model.save(tmp_path)
    loaded = NnueNet.load(tmp_path)
    board = _board_from_epd("4k3/8/8/8/8/8/8/R3K3 w - -")
    fw, fb, pfw, pfb, afw, afb = _feats(board)
    o1 = np.array(_forward(model, fw, fb, pfw, pfb, afw, afb, [0]))
    o2 = np.array(_forward(loaded, fw, fb, pfw, pfb, afw, afb, [0]))
    assert o1 == o2


def test_load_rejects_stale_checkpoint(tmp_path):
    """A checkpoint from the pre-plane-10 layout (one plane,
    PLANE_SIZE rows, fewer input features than today's N_FEATURES)
    must be rejected with a clear error instead of silently loading
    mismatched tensors."""
    model = NnueNet(seed=1)
    model.save(tmp_path)
    # Rewrite input_w with the old (10-plane) size.
    with np.load(tmp_path / "weights.npz") as z:
        params = {k: z[k] for k in z.files}
    params["input_w"] = params["input_w"][: N_FEATURES - PLANE_SIZE]
    np.savez(tmp_path / "weights.npz", **params)
    with pytest.raises(ValueError, match="stale checkpoint"):
        NnueNet.load(tmp_path)


def test_load_rejects_single_branch_checkpoint(tmp_path):
    """A checkpoint from before the per-phase branches (h1_w a single
    (2*ACC, H1) matrix) must be rejected with a clear error."""
    from amy_training_nnue.features import ACCUMULATOR_SIZE, HIDDEN_SIZE_1

    model = NnueNet(seed=1)
    model.save(tmp_path)
    with np.load(tmp_path / "weights.npz") as z:
        params = {k: z[k] for k in z.files}
    # Old architecture: no leading phase dimension.
    params["h1_w"] = params["h1_w"][0]
    params["h1_b"] = params["h1_b"][0]
    np.savez(tmp_path / "weights.npz", **params)
    with pytest.raises(ValueError, match="stale checkpoint"):
        NnueNet.load(tmp_path)
