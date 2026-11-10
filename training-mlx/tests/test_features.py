"""Tests for the NNUE feature mapping against hand-computed indices.

Index formula (amy neural_net.c, king-half canonical frame):
    index = 2048 * plane + 64 * king_index + sq'
with plane 0..4 = own pawn..queen, plane 5..9 = opponent pawn..queen,
and plane 10 = the opponent king in the side's own canonical frame
(the own king is not a feature; it only defines the frame);
``king_index``
is the side's king in the canonical (left-half) frame, a1 = 0 ... d1 =
3, ..., a8 = 28 ... d8 = 31, and ``sq'`` is the piece square in that
frame (for plane 10, the opponent king's square): black's squares are
rank-flipped (``sq ^ 0x38``) and, whenever the own king stands on
files e..h, the whole frame is mirrored horizontally
(``sq ^ 0x07``).
"""

import numpy as np
import pytest

from amy_training_nnue.bpd import BLACK, WHITE
from amy_training_nnue.features import (
    FLIP,
    MIRROR,
    N_FEATURES,
    N_PAWN_FEATURES,
    PLANE_SIZE,
    canonical_frame,
    feature_indices,
    pawn_feature_indices,
    pawn_pair_index,
)
from amy_training_nnue.position import mirror_board

INIT = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -"
# After 1.e4 e5 2.Nf3: pawns on files a-d, f-h plus e4/e5 (same file).
E4E5 = "r1bqkbnr/pppp1ppp/2n5/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq -"


def _board_from_epd(epd: str) -> np.ndarray:
    pieces = {
        "P": (1, 1),
        "N": (1, 2),
        "B": (1, 3),
        "R": (1, 4),
        "Q": (1, 5),
        "K": (1, 6),
        "p": (-1, 1),
        "n": (-1, 2),
        "b": (-1, 3),
        "r": (-1, 4),
        "q": (-1, 5),
        "k": (-1, 6),
    }
    board = np.zeros(64, dtype=np.int8)
    rows = epd.split(" ")[0].split("/")
    for rank, row in enumerate(rows):
        real_rank = 7 - rank
        file = 0
        for ch in row:
            if ch.isdigit():
                file += int(ch)
            else:
                color, piece = pieces[ch]
                board[real_rank * 8 + file] = color * piece
                file += 1
    return board


def test_n_features():
    assert N_FEATURES == 11 * 32 * 64 == 22528
    # Max index: plane 10 (opponent king), king_index 31 (d8), square 63.
    assert PLANE_SIZE * 10 + 64 * 31 + 63 == N_FEATURES - 1


def test_flip_masks_commute():
    assert FLIP == 0x38
    assert MIRROR == 0x07
    assert (0 ^ FLIP) == 56  # a1 <-> a8
    assert (63 ^ FLIP) == 7  # h8 <-> h1
    assert (0 ^ MIRROR) == 7  # a1 <-> h1
    for sq in range(64):
        assert (sq ^ FLIP ^ MIRROR) == (sq ^ MIRROR ^ FLIP)


def test_canonical_frame_left_half_unchanged():
    # White king on files a..d: no mirror, king_index = file + 4 * rank.
    assert canonical_frame(WHITE, 0) == (0, 0)  # a1
    assert canonical_frame(WHITE, 3) == (0, 3)  # d1
    assert canonical_frame(WHITE, 59) == (0, 31)  # d8
    # Black king a8 (56): own perspective a1 (0), no mirror.
    assert canonical_frame(BLACK, 56) == (FLIP, 0)
    # Black king d8 (59): own perspective d1 (3), no mirror.
    assert canonical_frame(BLACK, 59) == (FLIP, 3)


def test_canonical_frame_right_half_mirrored():
    # White king e1 (4): mirrored to d1 (3); every square file-flipped.
    assert canonical_frame(WHITE, 4) == (MIRROR, 3)
    assert canonical_frame(WHITE, 7) == (MIRROR, 0)  # h1 -> a1
    assert canonical_frame(WHITE, 63) == (MIRROR, 28)  # h8 -> a8
    # Black king e8 (60): own perspective e1 (4) -> mirrored to d1 (3).
    assert canonical_frame(BLACK, 60) == (FLIP | MIRROR, 3)
    # Black king h8 (63): own perspective h1 (7) -> mirrored to a1 (0).
    assert canonical_frame(BLACK, 63) == (FLIP | MIRROR, 0)


def test_king_index_collapses_mirror_pairs():
    # Every one of the 64 king squares maps into 0..31, and a square and
    # its horizontal mirror always share a king_index (the folding that
    # makes N_FEATURES 32-per-plane instead of 64).
    for side in (WHITE, BLACK):
        for sq in range(64):
            _, k = canonical_frame(side, sq)
            assert 0 <= k <= 31
            assert k == canonical_frame(side, sq ^ MIRROR)[1]


def test_initial_position_white_features():
    board = _board_from_epd(INIT)
    w, b = feature_indices(board)
    # 15 own + 15 opponent non-king pieces plus the opponent king
    # (plane 10) per accumulator; the own king itself is not a feature
    # (it defines the frame).
    assert len(w) == 31
    assert len(b) == 31
    # White frame: king e1 (sq 4) -> mirrored to d1 -> king_index 3,
    # every square file-flipped (sq ^ 0x07).
    # White pawns a2..h2 (sq 8..15 -> 15..8): plane 0 -> 64*3 + 8..15.
    assert set(range(64 * 3 + 8, 64 * 3 + 16)) <= set(w.tolist())
    # White rooks a1/h1 (0->7, 7->0): plane 3 -> 3*2048 + 192 + {0, 7}.
    assert 3 * PLANE_SIZE + 192 + 0 in w
    assert 3 * PLANE_SIZE + 192 + 7 in w
    # White queen d1 (3->4): plane 4.
    assert 4 * PLANE_SIZE + 192 + 4 in w
    # Black pawns a7..h7 (sq 48..55 -> 55..48) are opponents (plane 5):
    assert 5 * PLANE_SIZE + 192 + 48 in w
    assert 5 * PLANE_SIZE + 192 + 55 in w
    # Black king e8 is *not* a piece feature (kings only define the
    # frame) ... except on plane 10, where it is the only feature.
    assert 9 * PLANE_SIZE + 192 + (60 ^ MIRROR) not in w
    assert PLANE_SIZE * 10 + 64 * 3 + (60 ^ MIRROR) in w
    # ... and the own (white) king's square must not appear on plane 10
    # either.
    assert PLANE_SIZE * 10 + 64 * 3 + (4 ^ MIRROR) not in w
    # Exactly one plane-10 entry per side.
    assert sum(1 for i in w if i >= PLANE_SIZE * 10) == 1
    assert sum(1 for i in b if i >= PLANE_SIZE * 10) == 1
    # Black rooks a8/h8 (56->63, 63->56) as opponents: plane 8.
    assert 8 * PLANE_SIZE + 192 + 56 in w
    assert 8 * PLANE_SIZE + 192 + 63 in w
    # Every index in range.
    assert min(w) >= 0 and max(w) < N_FEATURES
    assert min(b) >= 0 and max(b) < N_FEATURES


def test_initial_position_black_features():
    board = _board_from_epd(INIT)
    w, b = feature_indices(board)
    # Black frame: own king e8 (60) -> own perspective e1 (4) ->
    # mirrored to d1 (3), so flip = 0x38 | 0x07, king_index 3.
    # Black's own pawns a7..h7 (48..55 -> 15..8): plane 0.
    assert set(range(64 * 3 + 8, 64 * 3 + 16)) <= set(b.tolist())
    # Black's own queen d8 (59 -> 4): plane 4.
    assert 4 * PLANE_SIZE + 64 * 3 + 4 in b
    # Black's own rooks a8/h8 (56->7, 63->0): plane 3.
    assert 3 * PLANE_SIZE + 64 * 3 + 0 in b
    assert 3 * PLANE_SIZE + 64 * 3 + 7 in b
    # White pawns a2..h2 (8..15 -> 55..48) as opponents (plane 5):
    assert 5 * PLANE_SIZE + 64 * 3 + 48 in b
    assert 5 * PLANE_SIZE + 64 * 3 + 55 in b
    # White queen d1 (3 -> 60) as opponent (plane 9):
    assert 9 * PLANE_SIZE + 64 * 3 + 60 in b
    # Opponent (white) king e1 (4 -> 4 ^ 0x3f = 63) on plane 10, in
    # the black frame.
    assert PLANE_SIZE * 10 + 64 * 3 + (4 ^ (FLIP | MIRROR)) in b


def test_opponent_king_plane():
    """Plane 10 = the opponent king in the side's own canonical frame:
    idx = PLANE_SIZE * 10 + 64 * king_index + (opp_king_sq ^ flip),
    with the frame (flip, king_index) of the side being evaluated.
    """
    # Starting position: white king e1 (sq 4) -> mirrored frame
    # (flip MIRROR, king_index 3); black king e8 (sq 60).
    board = _board_from_epd(INIT)
    w, b = feature_indices(board)
    w_flip, w_king = canonical_frame(WHITE, 4)
    b_flip, b_king = canonical_frame(BLACK, 60)
    assert (w_flip, w_king) == (MIRROR, 3)
    assert (b_flip, b_king) == (FLIP | MIRROR, 3)
    assert PLANE_SIZE * 10 + 64 * w_king + (60 ^ w_flip) in w
    assert PLANE_SIZE * 10 + 64 * b_king + (4 ^ b_flip) in b
    # Left-half kings: no rank flip, no mirror.
    board = _board_from_epd("3k4/8/8/8/8/8/8/3K4 w - -")
    w, b = feature_indices(board)
    # White king d1 -> king_index 3; black king d8 (59) unflipped.
    assert w.tolist() == [PLANE_SIZE * 10 + 64 * 3 + 59]
    # Black frame: king d8 (59 -> 3) -> king_index 3; white king d1
    # (3 -> 3 ^ 0x38 = 59).
    assert b.tolist() == [PLANE_SIZE * 10 + 64 * 3 + 59]


def test_bare_kings_only_opponent_king():
    """With only kings on the board, the sole active piece feature of
    each side is the opponent king (plane 10); the own king is not a
    feature and no pawn pair features are active.

    Kings e1/e8 (both on file e -> both frames mirrored) give
    king_index 3 for both sides."""
    board = _board_from_epd("4k3/8/8/8/8/8/8/4K3 w - -")
    w, b = feature_indices(board)
    # White frame: flip MIRROR, king_index 3; black king e8 (60).
    assert w.tolist() == [PLANE_SIZE * 10 + 64 * 3 + (60 ^ MIRROR)]
    # Black frame: flip FLIP | MIRROR, king_index 3; white king e1 (4).
    assert b.tolist() == [PLANE_SIZE * 10 + 64 * 3 + (4 ^ (FLIP | MIRROR))]


def test_left_and_right_half_kings_share_features():
    """Kings e1/e8 and kings d1/d8 are horizontal mirrors of each
    other; with the canonical frame they must map to the *same*
    feature vector (CHANGES worked example 2, the data folding that
    replaces mirror augmentation)."""
    e1_e8 = _board_from_epd("4k3/8/8/8/8/8/8/4K3 w - -")
    d1_d8 = _board_from_epd("3k4/8/8/8/8/8/8/3K4 w - -")
    w1, b1 = feature_indices(e1_e8)
    w2, b2 = feature_indices(d1_d8)
    np.testing.assert_array_equal(w1, w2)
    np.testing.assert_array_equal(b1, b2)


def test_mirrored_board_has_identical_features():
    """For any position, the horizontal mirror image produces identical
    feature vectors for both sides."""
    for epd in (
        INIT,
        "r1bqkb1r/pppp1ppp/2n2n2/1B2p3/4P3/5N2/PPPP1PPP/RNBQK2R w KQkq -",
        "3r1k2/p3qppp/8/3p4/3r4/5QP1/P4PB1/4R1K1 w - -",
    ):
        board = _board_from_epd(epd)
        w1, b1 = feature_indices(board)
        w2, b2 = feature_indices(mirror_board(board))
        assert sorted(w1.tolist()) == sorted(w2.tolist()), epd
        assert sorted(b1.tolist()) == sorted(b2.tolist()), epd


def test_max_index_at_corner():
    # White king d8 (canonical, no mirror), black queen h8: plane 9
    # (opponent queen), king_index 31, square 63.  The plane-10
    # (black king a1, square 0) index is the second feature; the
    # overall maximum index (plane 10, king_index 31, square 63) is
    # N_FEATURES - 1.
    board = _board_from_epd("3K3q/8/8/8/8/8/8/k7 w - -")
    w, b = feature_indices(board)
    assert set(w.tolist()) == {
        PLANE_SIZE * 9 + 64 * 31 + 63,  # black queen h8
        PLANE_SIZE * 10 + 64 * 31 + 0,  # black king a1
    }
    # Black frame: king a1 (0) -> perspective a8 (king_index 28); own
    # queen h8 (63) -> 63 ^ 0x38 = 7 (h1), own queen plane 4.  The
    # white king (d8 -> 3 ^ 0x38 = 3, d1) is plane 10 in the black
    # frame.
    assert set(b.tolist()) == {
        PLANE_SIZE * 4 + 64 * 28 + 7,  # own queen, h1 in black frame
        PLANE_SIZE * 10 + 64 * 28 + 3,  # white king d8 -> d1
    }


def test_black_perspective_matches_flipped_white():
    """The vertically flipped position with swapped colors must give:
    flipped_white_features == original_black_features and
    flipped_black_features == original_white_features.
    This holds by construction of the index formula for any position."""
    for epd in (
        INIT,
        # asymmetric: after 1.e4 e5 2.Nf3 Nc6
        "r1bqkbnr/pppp1ppp/2n5/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq -",
    ):
        board = _board_from_epd(epd)
        w, b = feature_indices(board)
        flipped = np.zeros(64, dtype=np.int8)
        for sq in range(64):
            flipped[sq] = -board[sq ^ FLIP]
        wf, bf = feature_indices(flipped)
        assert sorted(wf.tolist()) == sorted(b.tolist())
        assert sorted(bf.tolist()) == sorted(w.tolist())


def test_missing_king_rejected():
    board = np.zeros(64, dtype=np.int8)
    board[0] = 1
    with pytest.raises(ValueError):
        feature_indices(board)


def test_en_passant_square_is_empty_for_features():
    # black pawn just played d7-d5, ep target d6 (sq 43); the target
    # square must carry no feature for either accumulator
    board = _board_from_epd("rnbqkbnr/ppp1pppp/8/3pP3/8/8/PPPP1PPP/RNBQKBNR w KQkq d6")
    w, b = feature_indices(board)
    ep = 43  # d6
    w_flip, w_king = canonical_frame(WHITE, 4)  # e1
    b_flip, b_king = canonical_frame(BLACK, 60)  # e8
    assert PLANE_SIZE * 0 + 64 * w_king + (ep ^ w_flip) not in w  # own pawn
    assert PLANE_SIZE * 5 + 64 * w_king + (ep ^ w_flip) not in w  # opponent pawn
    assert PLANE_SIZE * 0 + 64 * b_king + (ep ^ b_flip) not in b
    assert PLANE_SIZE * 5 + 64 * b_king + (ep ^ b_flip) not in b


# ---------------------------------------------------------------------------
# Pawn pair features
# ---------------------------------------------------------------------------


def test_n_pawn_features():
    # 96 color-square ids (2 colors x 48 pawn squares, ranks 2..7), one
    # feature per unordered pair.
    assert N_PAWN_FEATURES == 96 * 95 // 2 == 4560


def test_pawn_pair_index():
    # Worked example from CHANGES-pawn-features.md: starting position,
    # (b2, a2) in the white frame -> ids 6 and 7 -> index 27; in the
    # black frame -> ids 94 and 95 -> index 4559 (the maximum).
    assert pawn_pair_index(6, 7) == 27
    assert pawn_pair_index(94, 95) == 4559
    assert pawn_pair_index(0, 1) == 0
    for a in range(0, 96, 7):
        for b in range(a + 1, 96, 5):
            assert pawn_pair_index(a, b) == pawn_pair_index(b, a)


def test_pawn_pair_index_bijective_and_active_rows():
    """The combadic index is bijective over all 4560 unordered pairs of
    the 96 color-square ids, and exactly 1488 of those pairs are active:
    the two squares within one file of distance, excluding the 48
    opposite-color same-square pairs (impossible on a real board)."""
    seen = {}
    active = 0
    for a in range(96):
        fa = (a % 48 + 8) & 7
        for b in range(a + 1, 96):
            fb = (b % 48 + 8) & 7
            idx = pawn_pair_index(a, b)
            assert idx not in seen
            seen[idx] = (a, b)
            same_square_opp_color = a % 48 == b % 48
            if abs(fa - fb) <= 1 and not same_square_opp_color:
                active += 1
    assert len(seen) == N_PAWN_FEATURES
    assert max(seen) == N_PAWN_FEATURES - 1
    assert active == 1488


def test_initial_position_pawn_pairs():
    board = _board_from_epd(INIT)
    w, b = pawn_feature_indices(board)
    # 16 pawns, 2 per file: 8 same-file pairs + 7 adjacent-file pairs
    # x 4 color combinations = 36 pairs, for both sides.
    assert len(w) == 36
    assert len(b) == 36
    # Worked example from CHANGES-pawn-features.md (king e1 -> w frame,
    # king e8 -> b frame): the (b2, a2) pair is index 27 in the white
    # frame (own pawns, ids 6/7) and, with the colors swapped to 48+
    # 54-8 = 94 and 48 + 55-8 = 95, index 4559 (the maximum) in the
    # black frame.  Symmetrically, the (a7, b7) pair is 27 in the black
    # frame and 4559 in the white frame, so both indices occur on both
    # sides.
    assert 27 in w.tolist()  # white a2/b2, white frame
    assert 4559 in w.tolist()  # black a7/b7, white frame
    assert 27 in b.tolist()  # black a7/b7, black frame
    assert 4559 in b.tolist()  # white a2/b2, black frame
    # All indices in range, each pair counted once.
    assert min(w) >= 0 and max(w) < N_PAWN_FEATURES
    assert min(b) >= 0 and max(b) < N_PAWN_FEATURES
    assert len(set(w.tolist())) == len(w)
    assert len(set(b.tolist())) == len(b)


def test_pawn_pairs_after_e4e5():
    """Hand-computed e4-e5 index for 1.e4 e5 2.Nf3.

    e4 is square 28, e5 square 36.  White king e1 -> w_flip = 7: e4
    (28^7 = 27 -> id 19, own pawn) and e5 (36^7 = 35 -> id 48+27 = 75,
    opponent pawn) -> pair index 75*74/2 + 19 = 2794.  Black king e8 ->
    b_flip = 0x3f: e4 (28^0x3f = 35 -> id 48+27 = 75, opponent in the
    black frame) and e5 (36^0x3f = 27 -> id 19, own pawn) -> the same
    pair (19, 75) -> 2794.  Both accumulators see the e4-e5 pair under
    the same index.
    """
    board = _board_from_epd(E4E5)
    w, b = pawn_feature_indices(board)
    assert len(w) == 36
    assert len(b) == 36
    assert 2794 in w.tolist()
    assert 2794 in b.tolist()


def test_pawn_pairs_file_distance_filter():
    # Adjacent files -> one pair; one file apart -> none; same file ->
    # one pair.
    adjacent = _board_from_epd("4k3/8/8/8/8/8/PP6/K7 w - -")
    w, b = pawn_feature_indices(adjacent)
    assert len(w) == 1
    assert len(b) == 1
    two_files_apart = _board_from_epd("4k3/8/8/8/8/8/P1P5/K7 w - -")
    w, b = pawn_feature_indices(two_files_apart)
    assert w.tolist() == []
    assert b.tolist() == []
    same_file = _board_from_epd("4k3/8/8/8/8/2P5/2P41/K7 w - -")
    w, b = pawn_feature_indices(same_file)
    assert len(w) == 1
    assert len(b) == 1


def test_bare_kings_no_pawn_pairs():
    board = _board_from_epd("4k3/8/8/8/8/8/8/4K3 w - -")
    w, b = pawn_feature_indices(board)
    assert w.tolist() == []
    assert b.tolist() == []


def test_mirrored_board_has_identical_pawn_pairs():
    """Like the piece features: the horizontal mirror image of a
    position produces identical pawn pair index sets for both sides."""
    for epd in (INIT, E4E5, "3r1k2/p3qppp/8/3p4/3r4/5QP1/P4PB1/4R1K1 w - -"):
        board = _board_from_epd(epd)
        w1, b1 = pawn_feature_indices(board)
        w2, b2 = pawn_feature_indices(mirror_board(board))
        assert set(w1.tolist()) == set(w2.tolist()), epd
        assert set(b1.tolist()) == set(b2.tolist()), epd


def test_pawn_pairs_flip_symmetry():
    """Like the piece features: the vertically flipped position with
    swapped colors must give flipped_white_pairs == original_black_pairs
    and flipped_black_pairs == original_white_pairs."""
    for epd in (INIT, E4E5):
        board = _board_from_epd(epd)
        w, b = pawn_feature_indices(board)
        flipped = np.zeros(64, dtype=np.int8)
        for sq in range(64):
            flipped[sq] = -board[sq ^ FLIP]
        wf, bf = pawn_feature_indices(flipped)
        assert set(wf.tolist()) == set(b.tolist())
        assert set(bf.tolist()) == set(w.tolist())


def test_pawn_pairs_missing_king_rejected():
    board = np.zeros(64, dtype=np.int8)
    board[0] = 1
    with pytest.raises(ValueError):
        pawn_feature_indices(board)
