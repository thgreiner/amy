"""A decoded chess position plus EPD output.

The neural net features are built from the piece placement only
(see features.py); castling rights and the en passant square are kept
for EPD output and debugging.
"""

from __future__ import annotations

import numpy as np

from .bpd import BLACK, WHITE, square_name

_PIECE_LETTERS = {
    1: "P",
    2: "N",
    3: "B",
    4: "R",
    5: "Q",
    6: "K",
}

# Amy's castling bit assignments (see dbase.h CastleMask).
CASTLE_WQ = 1
CASTLE_WK = 2
CASTLE_BQ = 4
CASTLE_BK = 8

# Horizontal (file) flip: a1<->h1, a2<->h2, ...  Unlike FLIP (the black
# perspective rank flip, 0x38) it keeps the rank and flips the file, so
# the two masks commute.
MIRROR_FLIP = 0x07


def mirror_board(board: np.ndarray) -> np.ndarray:
    """Mirror a 64-element board horizontally (flip files a<->h).

    The piece on square ``sq`` of ``board`` ends up on square
    ``sq ^ MIRROR_FLIP``.  Colors and piece types are preserved.  The
    mirror of a position without castling rights is a valid chess
    position with the same value (its game tree is isomorphic to the
    original); the en passant square is not part of the board array
    and is irrelevant for the network features.
    """
    return board[np.arange(64) ^ MIRROR_FLIP]


class Position:
    """A chess position decoded from a BPD record."""

    __slots__ = ("board", "turn", "castle", "ep")

    def __init__(self, board: np.ndarray, turn: int, castle: int = 0, ep: int = -1):
        self.board = board
        self.turn = turn
        self.castle = castle
        self.ep = ep

    def king_sq(self, side: int) -> int:
        king = 6 if side == WHITE else -6
        sq = int(np.argmax(self.board == king))
        if self.board[sq] != king:
            raise ValueError(f"no king found for side {side}")
        return sq

    def epd(self) -> str:
        board = self.board
        rows = []
        for rank in range(7, -1, -1):
            row = ""
            empty = 0
            for file in range(8):
                value = board[rank * 8 + file]
                if value == 0:
                    empty += 1
                    continue
                if empty:
                    row += str(empty)
                    empty = 0
                letter = _PIECE_LETTERS[abs(value)]
                row += letter if value > 0 else letter.lower()
            if empty:
                row += str(empty)
            rows.append(row)
        placement = "/".join(rows)
        stm = "w" if self.turn == WHITE else "b"
        castle = (
            ("K" if self.castle & CASTLE_WK else "")
            + ("Q" if self.castle & CASTLE_WQ else "")
            + ("k" if self.castle & CASTLE_BK else "")
            + ("q" if self.castle & CASTLE_BQ else "")
        ) or "-"
        ep = square_name(self.ep) if self.ep >= 0 else "-"
        return f"{placement} {stm} {castle} {ep}"


def _nibble_at(pos: tuple[int, int, int, int], sq: int) -> int:
    return (pos[sq >> 4] >> (4 * (sq & 15))) & 0x0F


def position_from_record(record) -> Position:
    """Build a Position from a BpdRecord, recovering the castling and
    en passant metadata from the special nibbles."""
    board = record.board()
    castle = 0
    ep = -1
    for sq in range(64):
        if _nibble_at(record.pos, sq) == 15:
            ep = sq
    if board[0] == 4 and _nibble_at(record.pos, 0) == 5:
        castle |= CASTLE_WQ
    if board[7] == 4 and _nibble_at(record.pos, 7) == 5:
        castle |= CASTLE_WK
    if board[56] == -4 and _nibble_at(record.pos, 56) == 12:
        castle |= CASTLE_BQ
    if board[63] == -4 and _nibble_at(record.pos, 63) == 12:
        castle |= CASTLE_BK
    return Position(board, record.turn, castle, ep)
