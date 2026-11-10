"""NNUE input features, bit-exact with amy's neural_net.c.

Amy (branch feature/king-half-mirrored) uses king-relative features in
a canonical frame per side.  Each side's own king defines the frame:

1. Own perspective: for black every square is rank-flipped
   (``sq ^ 0x38``, a1<->a8 style vertical flip).
2. Left-half canonicalization: if the side's own king (in the step-1
   frame) stands on files e..h, the *whole* frame is mirrored
   horizontally (``sq ^ 0x07``, file -> 7 - file), so the king is
   always in the left half (files a..d).  The two flips commute
   (independent bit masks), so a position and its horizontal mirror
   image produce the *same* feature vectors for both sides.

There are 10 piece planes (0..4 = own pawn..queen, 5..9 = opponent
pawn..queen) plus plane 10, which encodes the *opponent* king in the
side's own canonical frame; the own king is *not* a feature, it only
defines the frame.  Together with 32 canonical own king positions and
64 squares:

    N_FEATURES = 11 * 32 * 64 = 22528
    index = 2048 * plane + 64 * king_index + sq'

with ``sq'`` the piece square (for plane 10: the opponent king's
square) in the side's canonical frame (0..63; pieces are not
restricted to the left half, only the king) and
``king_index = file + 4 * rank`` of the canonical own king
(a1 = 0 ... d1 = 3, a2 = 4 ... d2 = 7, ..., a8 = 28 ... d8 = 31), as
in ``index_half_mirrored()`` of neural_net.c.

On top of the piece features there are pawn *pair* features (see
nn_init_pawns() of neural_net.c): every unordered pair of pawns on the
board within one file of distance (same file, any rank, or adjacent
files) is a feature.  Color combinations matter (own/own, opponent/
opponent, own/opponent); a pair is counted once.  Within one canonical
frame each pawn maps to a color-square id in 0..95
(``color * 48 + (sq_frame - 8)``, ranks 2..7 only; in the white frame
white pawns have color 0, in the black frame the colors are swapped,
so in either frame own pawns are 0..47 and opponent pawns 48..95), and
the unordered pair (i1, i2) maps to the combadic index
``hi * (hi - 1) / 2 + lo`` with hi = max, lo = min, in 0..4559:

    N_PAWN_FEATURES = 96 * 95 / 2 = 4560

Only the 1488 pairs whose two squares are within one file are ever
active; the remaining rows of the shared N_PAWN_FEATURES x
ACCUMULATOR_SIZE int8 pawn weight matrix are dead (zero in the file).
Each side's accumulator sums the rows of all active pairs, computed in
that side's canonical frame; both sides read the same matrix.

Each side has its own ACCUMULATOR_SIZE-dim accumulator (int16 in amy)
initialized with the input bias plus the sum of the weights of the
active features; the side-to-move accumulator is the first half and the
opponent's the second half of the 2 * ACCUMULATOR_SIZE-dim network
input.
"""

from __future__ import annotations

import numpy as np

from .bpd import BLACK, WHITE

N_PIECE_TYPES = 10
# One plane per piece type (0..4 own pawn..queen, 5..9 opponent
# pawn..queen), plus plane 10 = the opponent king in the side's own
# canonical frame.  The own king is not a feature; it defines the
# frame.
N_FEATURES = 11 * 32 * 64  # 22528
# Pawn pair features: 2 colors x 48 pawn squares (ranks 2..7) = 96
# color-square ids, one feature per unordered pair (combadic index).
N_PAWN_SQUARES = 48  # ranks 2..7 of one frame
N_PAWN_IDS = 2 * N_PAWN_SQUARES  # 96
N_PAWN_FEATURES = N_PAWN_IDS * (N_PAWN_IDS - 1) // 2  # 4560
# Attack features: 24 planes (attacker P..Q x victim P..Q, minus
# pawn->pawn and all king attacks), each indexed by (attacker square,
# victim square) in the side's canonical frame: 64 x 64, i.e. not
# king-relative (see attack_map.c in the engine; test_nn_extract.c
# pins the plane numbering).
ATTACK_PLANE_SIZE = 64 * 64  # 4096
N_ATTACK_FEATURES = 24 * ATTACK_PLANE_SIZE  # 98304
ACCUMULATOR_SIZE = 256
HIDDEN_SIZE_1 = 32
HIDDEN_SIZE_2 = 32
# Number of game-phase branches (NN_PHASE_COUNT in amy's neural_net.h):
# each phase has its own hidden-1, hidden-2 and output layer block in
# the file; the input side is shared.  C's nn_export_constants() is
# the arbiter (cross-checked in test_cfeatures.test_constants_sync).
NN_PHASE_COUNT = 4
# Vertical (rank) flip used for the black perspective.
FLIP = 0x38
# Horizontal (file) flip used to mirror the frame to the left half.
MIRROR = 0x07
# Plane stride: 32 king positions x 64 squares.
PLANE_SIZE = 32 * 64  # 2048

# Amy's Piece enum: Pawn=1..Queen=5, King=6.
PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING = 1, 2, 3, 4, 5, 6


def canonical_frame(side: int, king_sq: int) -> tuple[int, int]:
    """Return (flip_mask, king_index) of the side's canonical frame.

    ``flip_mask`` is the square transform applied to *every* square in
    the frame (0, FLIP, MIRROR, or FLIP | MIRROR); ``king_index`` is
    the own king's canonical position, 0..31, as in
    ``index_half_mirrored()`` of neural_net.c.
    """
    flip = FLIP if side == BLACK else 0
    k = king_sq ^ flip
    # Mirror the frame if the king stands on files e..h.
    if (k & 7) > 3:
        k ^= MIRROR
        flip |= MIRROR
    return flip, (k & 3) | ((k & 0x38) >> 1)


def pawn_pair_index(i1: int, i2: int) -> int:
    """Combadic index of an unordered pair of color-square ids 0..95,
    matching nn_pp_index() of neural_net.c: hi * (hi - 1) / 2 + lo with
    hi = max(i1, i2), lo = min(i1, i2).  Result in 0..N_PAWN_FEATURES-1.
    """
    hi, lo = max(i1, i2), min(i1, i2)
    return hi * (hi - 1) // 2 + lo


def pawn_feature_indices(board: np.ndarray) -> tuple[np.ndarray, np.ndarray]:
    """Return (white_pawn_pair_indices, black_pawn_pair_indices).

    Bit-exact twin of nn_init_pawns() of neural_net.c: every unordered
    pair of pawns (any color combination) within one file of distance
    contributes one combadic index (see pawn_pair_index) computed in
    the side's canonical frame (the same frame as the piece features).
    Both sides get the same pairs, mapped through their own frame.

    ``board`` is a 64-element int8 array as in BpdRecord.board();
    pawns must be on ranks 2..7 (ranks 1/8 would index out of bounds,
    as in the engine).
    """
    if (board == 6).sum() != 1 or (board == -6).sum() != 1:
        raise ValueError("board must contain exactly one king per side")
    white_king = int(np.argmax(board == 6))
    black_king = int(np.argmax(board == -6))
    w_flip, _ = canonical_frame(WHITE, white_king)
    b_flip, _ = canonical_frame(BLACK, black_king)

    # Pawn squares ascending, with c = 0 for white pawns and 1 for
    # black pawns (white-frame colors; the black frame swaps them).
    sqs = np.flatnonzero(np.abs(board) == 1)
    colors = (board[sqs] < 0).astype(np.int64)
    n = len(sqs)
    if n < 2:
        empty = np.empty(0, dtype=np.int32)
        return empty, empty

    i1, i2 = np.triu_indices(n, 1)
    sel = np.abs((sqs[i1] & 7) - (sqs[i2] & 7)) <= 1
    i1, i2 = i1[sel], i2[sel]

    def pairs(flip: int, swapped: bool) -> np.ndarray:
        # swapped=True (black frame): own pawns are black pawns, so the
        # color id is negated (1 - c).
        c1 = (1 - colors[i1]) if swapped else colors[i1]
        c2 = (1 - colors[i2]) if swapped else colors[i2]
        a = c1 * N_PAWN_SQUARES + (sqs[i1] ^ flip) - 8
        b = c2 * N_PAWN_SQUARES + (sqs[i2] ^ flip) - 8
        hi = np.maximum(a, b)
        lo = np.minimum(a, b)
        return (hi * (hi - 1) // 2 + lo).astype(np.int32)

    white = pairs(w_flip, False)
    black = pairs(b_flip, True)
    return white, black


def feature_indices(board: np.ndarray) -> tuple[np.ndarray, np.ndarray]:
    """Return (white_accumulator_indices, black_accumulator_indices).

    ``board`` is a 64-element int8 array as in BpdRecord.board().
    Each returned array holds the index of every active feature of
    that side's accumulator (up to 31 entries: 15 own and 15 opponent
    pieces, plus the opponent king on plane 10; the own king is not a
    feature, it only defines the frame).
    """
    if (board == 6).sum() != 1 or (board == -6).sum() != 1:
        raise ValueError("board must contain exactly one king per side")
    white_king = int(np.argmax(board == 6))
    black_king = int(np.argmax(board == -6))
    w_flip, w_king = canonical_frame(WHITE, white_king)
    b_flip, b_king = canonical_frame(BLACK, black_king)

    white = []
    black = []
    for sq, value in enumerate(board):
        if value == 0:
            continue
        v = int(value)
        if v > 0:
            t = v - 1
            if t < QUEEN:  # own pawn..queen (the king is not a feature)
                white.append(PLANE_SIZE * t + 64 * w_king + (sq ^ w_flip))
                black.append(PLANE_SIZE * (t + 5) + 64 * b_king + (sq ^ b_flip))
        else:
            t = -v - 1
            if t < QUEEN:
                black.append(PLANE_SIZE * t + 64 * b_king + (sq ^ b_flip))
                white.append(PLANE_SIZE * (t + 5) + 64 * w_king + (sq ^ w_flip))
    # Plane 10: the opponent king, in the side's own canonical frame
    # (nn_build_accumulator() of neural_net.c).  The square being
    # transformed is the opponent's; the frame (flip, king_index)
    # belongs to the side being evaluated.
    white.append(PLANE_SIZE * 10 + 64 * w_king + (black_king ^ w_flip))
    black.append(PLANE_SIZE * 10 + 64 * b_king + (white_king ^ b_flip))
    return (np.asarray(white, dtype=np.int32), np.asarray(black, dtype=np.int32))
