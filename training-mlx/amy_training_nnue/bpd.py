"""Binary Position (BPD) format used by amy's evaluation log.

A record is the raw ``BinaryPosition`` struct from amy (see
``include/bpd.h`` in the amy repository):

    struct {
        uint64_t pos[4];  /* 64 nibbles, little endian */
        int32_t  score;   /* search score in millipawns, side to move */
        uint8_t  turn;    /* 0 = white to move, 1 = black to move */
        uint8_t  pad[3];  /* padding, must be zero */
    };

The board is stored as 64 nibbles, packed 16 per uint64:
``pos[0]`` covers a1..h2, ``pos[1]`` a3..h4, ``pos[2]`` a5..h6 and
``pos[3]`` a7..h8.  Square ``s`` is the ``(s & 15)``-th nibble of
``pos[s >> 4]``.  Squares are numbered 0..63 starting at a1, i.e.
``sq = file + 8 * rank`` (0-based file/rank).

Nibble values:

    0  empty
    1  white pawn        8  black pawn
    2  white knight      9  black knight
    3  white bishop     10  black bishop
    4  white rook       11  black rook
    5  white rook that can still castle   12 black rook that can castle
    6  white queen      13  black queen
    7  white king       14  black king
    15 en passant target square (the square is empty)

The castling-rook nibbles and the en passant nibble are pure
position metadata; for neural net features a castling rook is just a
rook and an en passant square is empty.
"""

from __future__ import annotations

import struct
from dataclasses import dataclass

import numpy as np

RECORD_STRUCT = struct.Struct("<QQQQiB3x")
RECORD_SIZE = RECORD_STRUCT.size
assert RECORD_SIZE == 40

WHITE = 0
BLACK = 1

# Nibble -> (color, piece) with color in {None, 1, -1} (None=empty/EP)
# and piece in 1..6 (Pawn..King).
_NIBBLE_PIECE = [
    (0, 0),  # 0  empty
    (1, 1),  # 1  white pawn
    (1, 2),  # 2  white knight
    (1, 3),  # 3  white bishop
    (1, 4),  # 4  white rook
    (1, 4),  # 5  white rook (castling)
    (1, 5),  # 6  white queen
    (1, 6),  # 7  white king
    (-1, 1),  # 8  black pawn
    (-1, 2),  # 9  black knight
    (-1, 3),  # 10 black bishop
    (-1, 4),  # 11 black rook
    (-1, 4),  # 12 black rook (castling)
    (-1, 5),  # 13 black queen
    (-1, 6),  # 14 black king
    (0, 0),  # 15 en passant target (empty)
]

_FILES = "abcdefgh"


def square_name(sq: int) -> str:
    """a1..h8 name of a 0-based square."""
    return _FILES[sq & 7] + str((sq >> 3) + 1)


@dataclass
class BpdRecord:
    """One 40-byte BinaryPosition record."""

    pos: tuple[int, int, int, int]
    score: int
    turn: int

    def encode(self) -> bytes:
        return RECORD_STRUCT.pack(*self.pos, self.score, self.turn)

    @staticmethod
    def decode(data: bytes, offset: int = 0) -> "BpdRecord":
        p0, p1, p2, p3, score, turn = RECORD_STRUCT.unpack_from(data, offset)
        return BpdRecord((p0, p1, p2, p3), score, turn)

    def board(self) -> np.ndarray:
        """Decode the board as a 64-element int8 array.

        Values: 0 empty, +1..+6 white pawn..king, -1..-6 black
        pawn..king.
        """
        board = np.zeros(64, dtype=np.int8)
        for word_idx, word in enumerate(self.pos):
            base = word_idx * 16
            for k in range(16):
                nibble = (word >> (4 * k)) & 0x0F
                color, piece = _NIBBLE_PIECE[nibble]
                board[base + k] = color * piece
        return board

    def validate(self) -> list[str]:
        """Return a list of problems with the record (empty if OK)."""
        problems: list[str] = []
        if self.turn not in (WHITE, BLACK):
            problems.append(f"turn {self.turn} not in (0, 1)")
        board = self.board()
        if (board == 6).sum() != 1:
            problems.append("expected exactly one white king")
        if (board == -6).sum() != 1:
            problems.append("expected exactly one black king")
        # Pawns only exist on ranks 2..7 (0-based ranks 1..6); on rank
        # 1 or 8 the pawn pair feature index (sq_frame - 8) would run
        # out of bounds, as in the engine.
        pawn_sqs = np.flatnonzero(np.abs(board) == 1)
        pawn_ranks = pawn_sqs >> 3
        for sq in pawn_sqs[(pawn_ranks < 1) | (pawn_ranks > 6)]:
            problems.append(f"pawn on {square_name(sq)} off-rank")
        # The en passant square, if present, must be on rank 3 or 6
        # (ranks 2 or 5, 0-based) and must be empty.
        for word_idx, word in enumerate(self.pos):
            base = word_idx * 16
            for k in range(16):
                if (word >> (4 * k)) & 0x0F == 15:
                    sq = base + k
                    if (sq >> 3) not in (2, 5):
                        problems.append(f"en passant square {square_name(sq)} off-rank")
                    if board[sq] != 0:
                        problems.append(
                            f"en passant square {square_name(sq)} not empty"
                        )
        return problems


def read_records(path: str):
    """Yield all BpdRecord of a file, validating the file size."""
    with open(path, "rb") as f:
        data = f.read()
    if len(data) % RECORD_SIZE != 0:
        raise ValueError(f"{path}: size {len(data)} is not a multiple of {RECORD_SIZE}")
    for offset in range(0, len(data), RECORD_SIZE):
        yield BpdRecord.decode(data, offset)


def write_records(path: str, records: list[BpdRecord]) -> None:
    with open(path, "wb") as f:
        for record in records:
            f.write(record.encode())


# ---------------------------------------------------------------------------
# Encoding a board into a BpdRecord (inverse of board()).  Used for
# generating cross-implementation test data and for tests.
# ---------------------------------------------------------------------------

_PIECE_NIBBLE = [
    0,  # empty
    1,
    2,
    3,
    4,
    6,
    7,  # white pawn..king (plain rook)
]


def encode_board(
    board: np.ndarray, turn: int, score: int, castle: int = 0, ep: int = -1
) -> BpdRecord:
    """Encode a board (see BpdRecord.board) into a BpdRecord.

    ``castle`` is a bit mask with the amy bit assignments
    (1: white queenside, 2: white kingside, 4: black queenside,
    8: black kingside) used to pick the castling-rook nibbles;
    ``ep`` is the en passant target square or -1.
    """
    pos = [0, 0, 0, 0]
    rook_castle_sq = {
        "wq": 0,  # a1
        "wk": 7,  # h1
        "bq": 56,  # a8
        "bk": 63,  # h8
    }
    for sq, value in enumerate(board):
        if value == 0:
            nibble = 0
        elif value > 0:
            nibble = _PIECE_NIBBLE[value]
            if value == 4:
                if (sq == rook_castle_sq["wq"]) and (castle & 1):
                    nibble = 5
                if (sq == rook_castle_sq["wk"]) and (castle & 2):
                    nibble = 5
        else:
            piece = -value
            nibble = _PIECE_NIBBLE[piece] + 7
            if piece == 4:
                if (sq == rook_castle_sq["bq"]) and (castle & 4):
                    nibble = 12
                if (sq == rook_castle_sq["bk"]) and (castle & 8):
                    nibble = 12
        if ep >= 0 and sq == ep:
            nibble = 15
        pos[sq >> 4] |= nibble << (4 * (sq & 15))
    return BpdRecord(tuple(pos), score, turn)
