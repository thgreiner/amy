"""Tests for the C feature-extraction bindings (T2).

The C library is the reference implementation of feature generation;
these tests pin its ABI and output:

- test_constants: the exported architecture constants.
- test_constants_sync: C-exported constants vs the Python constants
  (the machine-checked C-is-source-of-truth invariant).
- test_golden_bpd_test_bin: the 5-record BPD fixture through the full
  extraction, compared against a committed golden file (generated
  from the C library; the hand-computed cases below and the C unit
  tests in src/test_nn_extract.c are the independent
  anchors).
- test_attack_features_hand_computed: attack plane indices from
  hand computation, including the pawn-attacks-bishop exclusion.
- test_invalid_records_cross: C validation (the arbiter) and
  BpdRecord.validate() agree on a battery of invalid records.
- test_parity_with_python_features: C piece/pawn indices are
  bit-identical to the (soon retired) Python oracle on the whole
  frozen selfplay sample.
"""

import json
from pathlib import Path

import numpy as np

from amy_training_nnue import cfeatures
from amy_training_nnue.bpd import (
    RECORD_SIZE,
    BpdRecord,
    encode_board,
)
from amy_training_nnue.dataset import BpdDataset
from amy_training_nnue.features import (
    ACCUMULATOR_SIZE,
    ATTACK_PLANE_SIZE,
    N_ATTACK_FEATURES,
    N_FEATURES,
    N_PAWN_FEATURES,
    NN_PHASE_COUNT,
    feature_indices,
    pawn_feature_indices,
)
from tests.test_features import _board_from_epd

DATA = Path(__file__).parent / "data"


def _active(row):
    """The front-packed active indices of one feature row."""
    return row[row >= 0].tolist()


def test_constants():
    k = cfeatures.constants()
    assert k.n_features == 11 * 32 * 64
    assert k.n_pawn_features == 4560
    assert k.n_attack_features == 24 * 64 * 64
    assert k.accumulator_size == 256
    assert k.max_piece_features == 36
    assert k.max_pawn_features == 128
    assert k.max_attack_features == 32
    assert k.phase_count == 4


def test_constants_sync():
    """The C-exported architecture constants must match the Python
    constants (a machine-checked invariant, replacing the doc-level
    "must match neural_net.c").  If the C architecture changes, this
    fails until the Python side is updated."""
    k = cfeatures.constants()
    assert k.n_features == N_FEATURES
    assert k.n_pawn_features == N_PAWN_FEATURES
    assert k.n_attack_features == N_ATTACK_FEATURES
    assert k.accumulator_size == ACCUMULATOR_SIZE
    assert k.phase_count == NN_PHASE_COUNT
    # Row-width caps: the Python ABI literals in cfeatures.py.
    assert k.max_piece_features == cfeatures.MAX_PIECE_FEATURES
    assert k.max_pawn_features == cfeatures.MAX_PAWN_FEATURES
    assert k.max_attack_features == cfeatures.MAX_ATTACK_FEATURES


def _phase_masks(board: np.ndarray) -> list[int]:
    """Python oracle for nn_phase_masks(): total non-pawn pieces
    (both sides), buckets of three, plus the adjacent higher phase at
    the top of a bucket (counts 2, 5, 8)."""
    count = int(np.sum((np.abs(board) >= 2) & (np.abs(board) <= 5)))
    phase = min(count // 3, NN_PHASE_COUNT - 1)
    masks = [0] * NN_PHASE_COUNT
    masks[phase] = 1
    if phase < NN_PHASE_COUNT - 1 and count % 3 == 2:
        masks[phase + 1] = 1
    return masks


def test_phase_masks_cross():
    """The C library's per-record active-phase masks must match the
    Python oracle on a battery of positions covering every phase and
    both boundary doubles (EPDs verified against the C pipeline)."""
    specs = [
        # (epd, expected count, expected masks)
        ("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -", 14, [0, 0, 0, 1]),
        ("r7/8/8/8/8/8/4K2k/8 w - -", 1, [1, 0, 0, 0]),
        ("3r1r2/8/8/8/8/8/4K2k/8 w - -", 2, [1, 1, 0, 0]),
        ("2R1r1r1/8/8/8/8/8/4K2k/8 w - -", 3, [0, 1, 0, 0]),
        ("3rr3/8/8/8/8/8/2RRR3/K6k w - -", 5, [0, 1, 1, 0]),
    ]
    for epd, count, expected in specs:
        board = _board_from_epd(epd)
        assert int(np.sum((np.abs(board) >= 2) & (np.abs(board) <= 5))) == count, epd
        rec = encode_board(board, 0, 0)
        ex = cfeatures.extract(rec.encode(), 1)
        assert ex.valid[0] == 1, epd
        assert ex.phase[0].tolist() == expected, (epd, ex.phase[0].tolist())
        assert ex.phase[0].tolist() == _phase_masks(board), epd


def test_phase_masks_invalid_zero():
    """Invalid records carry an all-zero phase row (like their all -1
    feature rows)."""
    board = _board_from_epd("4K3/8/8/8/8/8/8/8 w - -")  # no black king
    rec = encode_board(board, 0, 0)
    ex = cfeatures.extract(rec.encode(), 1)
    assert ex.valid[0] == 0
    assert (ex.phase[0] == 0).all()


def test_golden_bpd_test_bin():
    data = (DATA / "bpd_test.bin").open("rb").read()
    ex = cfeatures.extract(data, len(data) // RECORD_SIZE)
    golden = json.load((DATA / "nn_extract_golden.json").open())
    assert len(golden["records"]) == len(ex.valid)
    for i, rec in enumerate(golden["records"]):
        assert ex.valid[i] == rec["valid"], i
        if not rec["valid"]:
            continue
        assert ex.turn[i] == rec["turn"]
        assert ex.score[i] == rec["score"]
        for field in (
            "feat_w",
            "feat_b",
            "pfeat_w",
            "pfeat_b",
            "afeat_w",
            "afeat_b",
        ):
            assert _active(getattr(ex, field)[i]) == rec[field], (i, field)


def test_attack_features_hand_computed():
    def extract_epd(epd, turn, score):
        board = _board_from_epd(epd)
        rec = encode_board(board, turn, score)
        return cfeatures.extract(rec.encode(), 1)

    # White P b4 attacks B c5; the (Bishop, Pawn) direction is
    # excluded because the pawn attacks the bishop.
    ex = extract_epd("4k3/8/8/2b5/1P6/8/8/4K3 w - -", 0, 7)
    assert ex.valid[0] == 1
    assert (ex.feat_w[0] >= 0).sum() == 3
    assert (ex.feat_b[0] >= 0).sum() == 3
    assert (ex.pfeat_w[0] >= 0).sum() == 0
    # (Pawn, Bishop) = plane 1; white king e1 -> flip 7:
    # ATTACK_PLANE_SIZE*1 + 64*(25^7) + (34^7)
    assert _active(ex.afeat_w[0]) == [ATTACK_PLANE_SIZE * 1 + 64 * 30 + 37]
    assert (ex.afeat_b[0] == -1).all()

    # White N f3 attacks P g5; the black pawn attacks nothing.
    ex = extract_epd("4k3/8/8/6p3/8/5N2/8/4K3 w - -", 0, -12)
    assert ex.valid[0] == 1
    # (Knight, Pawn) = plane 4; ATTACK_PLANE_SIZE*4 + 64*(21^7) + (38^7)
    assert _active(ex.afeat_w[0]) == [ATTACK_PLANE_SIZE * 4 + 64 * 18 + 33]
    assert (ex.afeat_b[0] == -1).all()

    # White B c1 attacks N f4 unblocked.  This is a sliding (magic
    # bitboard) attack, the path that needs the engine's precomputed
    # tables (init_all) — it is empty if the library forgot to init.
    ex = extract_epd("4k3/8/8/8/5n2/8/8/2B1K3 w - -", 0, 3)
    assert ex.valid[0] == 1
    # (Bishop, Knight) = plane 10; white king e1 -> mirror 7:
    # ATTACK_PLANE_SIZE*10 + 64*(2^7) + (29^7)
    assert _active(ex.afeat_w[0]) == [ATTACK_PLANE_SIZE * 10 + 64 * 5 + 26]
    # Black N f4 attacks nothing (no white piece on its reach squares).
    assert (ex.afeat_b[0] == -1).all()


def _records():
    """(name, epd, turn, score, ep_sq_or_-1) of the validation battery."""
    return [
        ("startpos", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -", 0, 0, -1),
        ("pawn rank 8", "P3k3/8/8/8/8/8/8/4K3 w - -", 0, 0, -1),
        ("pawn rank 1", "R3K2P/8/8/8/8/8/8/4k3 w - -", 0, 0, -1),
        ("no black king", "4K3/8/8/8/8/8/8/8 w - -", 0, 0, -1),
        ("two white kings", "3Kk3/8/8/8/8/8/8/4K3 w - -", 0, 0, -1),
        ("EP rank 4", "4k3/8/8/8/8/4P3/8/4K3 b - -", 1, 0, 28),
        ("EP rank 3", "4k3/8/8/8/4P3/8/4P3/4K3 b - -", 1, 0, 20),
    ]


def test_invalid_records_cross():
    expected_invalid = {
        "pawn rank 8",
        "pawn rank 1",
        "no black king",
        "two white kings",
        "EP rank 4",
    }
    for name, epd, turn, score, ep in _records():
        board = _board_from_epd(epd)
        rec = encode_board(board, turn, score, ep=ep)
        ex = cfeatures.extract(rec.encode(), 1)
        problems = rec.validate()
        assert ex.valid[0] == (0 if name in expected_invalid else 1), name
        assert bool(problems) == (name in expected_invalid), (
            name,
            problems,
        )
        if name in expected_invalid:
            # Invalid rows are all -1 on every side.
            for field in (
                "feat_w",
                "feat_b",
                "pfeat_w",
                "pfeat_b",
                "afeat_w",
                "afeat_b",
            ):
                assert (getattr(ex, field)[0] == -1).all(), (name, field)


def test_parity_with_python_features():
    """C and the Python oracle must agree bit-for-bit on piece and
    pawn indices (order-insensitive: accumulator sums are
    commutative)."""
    raw = (DATA / "selfplay_sample.bin").read_bytes()
    n = len(raw) // RECORD_SIZE
    ex = cfeatures.extract(raw, n)
    assert ex.valid.all()
    for i in range(n):
        rec = BpdRecord.decode(raw, i * RECORD_SIZE)
        board = rec.board()
        w, b = feature_indices(board)
        pw, pb = pawn_feature_indices(board)
        assert sorted(_active(ex.feat_w[i])) == sorted(w.tolist()), i
        assert sorted(_active(ex.feat_b[i])) == sorted(b.tolist()), i
        assert sorted(_active(ex.pfeat_w[i])) == sorted(pw.tolist()), i
        assert sorted(_active(ex.pfeat_b[i])) == sorted(pb.tolist()), i


def test_dataset_smoke():
    ds = BpdDataset([str(DATA / "selfplay_sample.bin")])
    k = cfeatures.constants()
    assert ds.n == 59980
    assert ds.n_invalid == 0
    assert ds.feat_w.shape == (ds.n, k.max_piece_features)
    assert ds.pfeat_w.shape == (ds.n, k.max_pawn_features)
    assert ds.afeat_w.shape == (ds.n, k.max_attack_features)
    assert (ds.target == ds.score.astype(np.float32) / 10000.0).all()
    tr, va = ds.split()
    assert len(tr) + len(va) == ds.n
