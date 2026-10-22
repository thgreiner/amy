/*

    Amy - a chess playing program

    Copyright (c) 2002-2026, Thorsten Greiner
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

#ifndef BPD_H
#define BPD_H

#include <stdbool.h>
#include <stdint.h>

/*
 * 4-bit values (nibbles) that represent the piece on a square.
 * Special values:
 * - PN_W_ROOK_CASTLE - a white rook that can still castle
 * - PN_B_ROOK_CASTLE - a black rook that can still castle
 * - PN_EN_PASSANT    - the en passant square
 */
typedef enum {
    PN_EMPTY = 0,
    PN_W_PAWN,
    PN_W_KNIGHT,
    PN_W_BISHOP,
    PN_W_ROOK,
    PN_W_ROOK_CASTLE,
    PN_W_QUEEN,
    PN_W_KING,
    PN_B_PAWN,
    PN_B_KNIGHT,
    PN_B_BISHOP,
    PN_B_ROOK,
    PN_B_ROOK_CASTLE,
    PN_B_QUEEN,
    PN_B_KING,
    PN_EN_PASSANT
} PieceNibble;

/*
 * Binary representation of a chess position.
 *
 * The board is stored as 64 nibbles, packed 16 per uint64_t:
 *   pos[0]: a1..h2    pos[1]: a3..h4
 *   pos[2]: a5..h6    pos[3]: a7..h8
 * Square s is in the (s & 15)-th nibble of pos[s >> 4].
 *
 * Castling rights are implicit: a rook on its home corner that can
 * still castle uses the PN_*_ROOK_CASTLE value. The en passant target
 * square (if any) uses PN_EN_PASSANT.
 *
 * The side to move cannot be derived from the board alone and does not
 * fit into the 64 nibbles, so it is stored separately.
 */
typedef struct {
    uint64_t pos[4]; /* the position as 64 nibbles */
    int32_t score;   /* the score of the position */
    uint8_t turn;    /* side to move: 0 == white, 1 == black */
} BinaryPosition;

struct Position; /* forward declaration, see dbase.h */

/*
 * Encode the position p together with score into *out.
 */
void bpd_encode(const struct Position *p, int score, BinaryPosition *out);

/*
 * Create a new position (see dbase_create_position_from_epd()) from the
 * binary representation. Returns NULL if the data is malformed (no king
 * of one of the colors). Release with dbase_free_position().
 */
struct Position *bpd_decode(const BinaryPosition *in);

/*
 * When true, evaluations are logged in all modes; when false, they are
 * logged only during selfplay. Set via the 'evallog' option in the
 * resource file (.amyrc / Amy.ini).
 */
extern bool EvalLog;

/*
 * Log the position with its score to the evaluation log.
 */
void bpd_log_evaluation(const struct Position *, int);

/*
 * Write all buffered evaluations to the log file. The log is buffered
 * and only written to the file once the buffer is full, so this is
 * registered with atexit() to make sure the last records are not lost.
 */
void bpd_flush_log(void);

/*
 * Create the cross-implementation test data for the nnue project: one
 * BinaryPosition per EPD in the internal test table (see the test plan
 * in ../nnue, nnue/TEST_PLAN_BPD.md), encoded with the 1-based table
 * index as score, written as a consecutive array of raw
 * BinaryPosition structs to the file at path.
 *
 * Returns 0 on success, -1 on error.
 */
int bpd_write_test_data(const char *path);

#endif
