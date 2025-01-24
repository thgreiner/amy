/*

    Amy - a chess playing program

    Copyright (c) 2002-2025, Thorsten Greiner
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
   LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
   INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
   CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
   ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
   POSSIBILITY OF SUCH DAMAGE.

*/

#ifndef DBASE_H
#define DBASE_H

#include "bitboard.h"
#include "config.h"
#include "heap.h"
#include "types.h"
#include <stdbool.h>
#include <stdint.h>

#define SQUARE(x) 'a' + ((x) & 7), '1' + ((x) >> 3)

#define OPP(x) (1 ^ (x))

#define TYPE(x) (((x) >= 0) ? (x) : -(x))
#define COLOR(x) (((x) == 0) ? Neutral : (((x) > 0) ? White : Black))
#define SAME_COLOR(p, c)                                                       \
    (((c) == White && (p) > 0) || ((c) == Black && (p) < 0))
#define PIECEID(p, c) (((c) == White) ? (p) : -(p))

#define M_FROM(m) ((m) & 63)
#define M_TO(m) (((m) >> 6) & 63)

#define M_CAPTURE (1 << 13)
#define M_SCASTLE (1 << 14)
#define M_LCASTLE (1 << 15)
#define M_PAWND (1 << 16)
#define M_PROMOTION_OFFSET 17
#define M_PROMOTION_MASK (7 << M_PROMOTION_OFFSET)
#define M_ENPASSANT (1 << 20)
#define M_NULL (1 << 21)
#define M_HASHED (1 << 22)

#define M_CANY (M_SCASTLE | M_LCASTLE)

#define M_TACTICAL (M_CAPTURE | M_ENPASSANT | M_PROMOTION_MASK)

#define M_NONE 0

/* Maximum number of good/bad moves we attempt to parse */
#define MAX_EPD_MOVES 64

/*
 * Constants for piece types and colors.
 */
typedef enum {
    Neutral = 0,
    Pawn,
    Knight,
    Bishop,
    Rook,
    Queen,
    King,
    BPawn
} Piece;
typedef enum { White = 0, Black = 1 } Color;

/*
 * Constants for chess board squares.
 */
// clang-format off
typedef enum {
    a1 = 0, b1, c1, d1, e1, f1, g1, h1,
    a2, b2, c2, d2, e2, f2, g2, h2,
    a3, b3, c3, d3, e3, f3, g3, h3,
    a4, b4, c4, d4, e4, f4, g4, h4,
    a5, b5, c5, d5, e5, f5, g5, h5,
    a6, b6, c6, d6, e6, f6, g6, h6,
    a7, b7, c7, d7, e7, f7, g7, h7,
    a8, b8, c8, d8, e8, f8, g8, h8
} Square;
// clang-format on

#if HAVE___BUILTIN_POPCOUNTLL
#define CountBits(x) __builtin_popcountll(x)
#else
int CountBits(BitBoard);
#endif

#if HAVE___BUILTIN_CTZLL
#define FindSetBit(x) __builtin_ctzll(x)
#else
int FindSetBit(BitBoard);
#endif

struct Position {
    BitBoard atkTo[64];
    BitBoard atkFr[64];
    BitBoard mask[2][7];
    BitBoard slidingPieces;
    hash_t hkey;
    hash_t pkey;
    struct GameLog *gameLog;
    struct GameLog *actLog;
    unsigned int gameLogSize;
    int material[2], nonPawn[2];
    uint16_t outOfBookCnt[2];
    uint16_t ply;
    int8_t piece[64];
    int8_t castle;
    int8_t enPassant;
    int8_t turn; /* 0 == white, 1 == black */
    int8_t kingSq[2];
    int8_t material_signature[2];
};

struct GameLog {
    move_t gl_Move;        /* the move that has been made in the position */
    int8_t gl_Piece;       /* the piece that was captured (if any) */
    int8_t gl_Castle;      /* the castling rights */
    int8_t gl_EnPassant;   /* the enpassant target square (if any) */
    uint8_t gl_IrrevCount; /* number of moves since last irreversible move */
    hash_t gl_HashKey;     /* used to detect repetitions */
    hash_t gl_PawnKey;
};

extern int Value[];
extern int goodmove[MAX_EPD_MOVES];
extern int badmove[MAX_EPD_MOVES];
extern char PieceName[];
extern const int8_t CastleMask[2][2];

void DoMove(struct Position *, move_t move);
void UndoMove(struct Position *, move_t move);
void DoNull(struct Position *);
void UndoNull(struct Position *);
void GenTo(struct Position *, Square, heap_t);
void GenEnpas(struct Position *, heap_t);
void GenFrom(struct Position *, Square, heap_t);
void GenRest(move_t *moves);
int GenCaps(move_t *moves, int good);
void GenChecks(struct Position *, heap_t);
int GenContactChecks(move_t *moves);
bool MayCastle(struct Position *, move_t move);
bool LegalMove(struct Position *, move_t move);
bool IsCheckingMove(struct Position *, move_t move);
int LegalMoves(struct Position *, heap_t);
void PLegalMoves(struct Position *, heap_t);
int Repeated(struct Position *, int mode);
char *SAN(struct Position *, move_t, char *);
move_t ParseSAN(struct Position *, char *);
move_t ParseSANList(char *, Color, move_t *, int, int *);
char *MakeEPD(struct Position *);
void ShowPosition(struct Position *);

struct Position *CreatePositionFromEPD(char *);
struct Position *InitialPosition(void);
struct Position *ClonePosition(struct Position *src);
void FreePosition(struct Position *);

void ShowMoves(struct Position *);
move_t ParseGSAN(struct Position *, char *san);
move_t ParseGSANList(char *san, Color side, move_t *mvs, int cnt);
char *ICS_SAN(move_t move);
void RecalcAttacks(struct Position *);
const char *GameEnd(struct Position *);

bool CheckDraw(const struct Position *);
bool IsPassed(const struct Position *, int, int);

#endif
