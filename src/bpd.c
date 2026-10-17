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

/*
 * bpd.c - Binary position (de)serialization
 *
 * A position is encoded as 64 nibbles (32 bytes) in four uint64_t's:
 * pos[i] holds squares 16*i .. 16*i+15, i.e. pos[0] covers a1..h2,
 * pos[1] a3..h4, pos[2] a5..h6 and pos[3] a7..h8. Square s is stored
 * in the (s & 15)-th nibble of pos[s >> 4].
 *
 * Special encodings:
 *
 * - Castling rights are not stored explicitly. A white rook on a1/h1
 *   (or a black rook on a8/h8) that can still castle is stored as
 *   PN_W_ROOK_CASTLE / PN_B_ROOK_CASTLE.
 * - The en passant target square (if any) is stored as PN_EN_PASSANT.
 *   In a legal position that square (on rank 3 or 6) is always empty,
 *   so no piece information is lost.
 *
 * The side to move does not fit into the 64 nibbles and is stored in
 * BinaryPosition.turn.
 */

#include "bpd.h"
#include "dbase.h"
#include "safe_malloc.h"

/*
 * Initial size of the game history, mirroring dbase.c.
 */
#define BPD_INITIAL_GAME_LOG_SIZE 40

/*
 * Mapping Piece value (Pawn..King) -> PieceNibble for white pieces.
 */
static const uint8_t whitePieceNibble[7] = {
    PN_EMPTY, PN_W_PAWN, PN_W_KNIGHT, PN_W_BISHOP, PN_W_ROOK, PN_W_QUEEN,
    PN_W_KING
};

/*
 * Mapping Piece value (Pawn..King) -> PieceNibble for black pieces.
 */
static const uint8_t blackPieceNibble[7] = {
    PN_EMPTY, PN_B_PAWN, PN_B_KNIGHT, PN_B_BISHOP, PN_B_ROOK, PN_B_QUEEN,
    PN_B_KING
};

/*
 * Mapping PieceNibble -> Piece value (white positive, black negative,
 * Neutral for empty squares and the en passant square).
 */
static const int8_t nibblePiece[16] = {
    Neutral,                                 /* PN_EMPTY        */
    Pawn, Knight, Bishop, Rook, Rook, Queen, King,    /* white */
    -Pawn, -Knight, -Bishop, -Rook, -Rook, -Queen, -King, /* black */
    Neutral                                 /* PN_EN_PASSANT   */
};

void bpd_encode(const struct Position *p, int score, BinaryPosition *out) {
    int i;

    out->score = score;
    out->turn = (uint8_t)p->turn;
    for (i = 0; i < 4; i++)
        out->pos[i] = 0;

    for (i = 0; i < 64; i++) {
        int8_t pc = p->piece[i];
        uint8_t nibble;

        if (pc > 0) {
            nibble = whitePieceNibble[pc];
            if (pc == Rook &&
                ((i == a1 && (p->castle & CastleMask[White][1])) ||
                 (i == h1 && (p->castle & CastleMask[White][0])))) {
                nibble = PN_W_ROOK_CASTLE;
            }
        } else if (pc < 0) {
            nibble = blackPieceNibble[-pc];
            if (pc == -Rook &&
                ((i == a8 && (p->castle & CastleMask[Black][1])) ||
                 (i == h8 && (p->castle & CastleMask[Black][0])))) {
                nibble = PN_B_ROOK_CASTLE;
            }
        } else {
            nibble = PN_EMPTY;
        }

        /*
         * Mark the en passant target square (if any). Note that
         * p->enPassant == 0 means "no en passant" and does not refer
         * to the a1 square.
         */
        if (p->enPassant != 0 && i == p->enPassant)
            nibble = PN_EN_PASSANT;

        out->pos[i >> 4] |= (uint64_t)nibble << (4 * (i & 15));
    }
}

/*
 * Fill the "raw" position fields from the binary representation.
 * The derived fields (piece masks beyond mask[c][0], attacks, material,
 * hash keys, king squares) are not touched; the caller has to call
 * dbase_recalc_attacks() afterwards.
 */
static void bpd_decode_into(const BinaryPosition *in, struct Position *p) {
    int i;

    p->turn = in->turn ? Black : White;
    p->castle = 0;
    p->enPassant = 0;
    p->ply = 0;
    p->mask[White][0] = 0;
    p->mask[Black][0] = 0;

    for (i = 0; i < 64; i++) {
        uint8_t nibble = (uint8_t)((in->pos[i >> 4] >> (4 * (i & 15))) & 15);

        p->piece[i] = nibblePiece[nibble];
        if (p->piece[i] > 0) {
            bitboard_set_bit(p->mask[White][0], i);
        } else if (p->piece[i] < 0) {
            bitboard_set_bit(p->mask[Black][0], i);
        }

        switch (nibble) {
        case PN_W_ROOK_CASTLE:
            /*
             * A malformed encoding may place a castling rook on any
             * square; in that case it is treated as a plain rook.
             */
            if (i == a1) {
                p->castle |= CastleMask[White][1];
            } else if (i == h1) {
                p->castle |= CastleMask[White][0];
            }
            break;
        case PN_B_ROOK_CASTLE:
            if (i == a8) {
                p->castle |= CastleMask[Black][1];
            } else if (i == h8) {
                p->castle |= CastleMask[Black][0];
            }
            break;
        case PN_EN_PASSANT:
            p->enPassant = (int8_t)i;
            break;
        default:
            break;
        }
    }
}

struct Position *bpd_decode(const BinaryPosition *in) {
    struct Position *p;
    int i;
    int whiteKing = 0;
    int blackKing = 0;

    /* Allocate like dbase_create_position_from_epd() does */
    p = safe_calloc(1, sizeof(struct Position));
    p->gameLogSize = BPD_INITIAL_GAME_LOG_SIZE;
    p->gameLog = safe_calloc(p->gameLogSize, sizeof(struct GameLog));
    p->actLog = p->gameLog;
    p->actLog->gl_IrrevCount = 0;

    bpd_decode_into(in, p);

    for (i = 0; i < 64; i++) {
        if (p->piece[i] == King) {
            whiteKing++;
        } else if (p->piece[i] == -King) {
            blackKing++;
        }
    }

    /*
     * A chess position must have a king for both colors;
     * dbase_recalc_attacks() would hit bitboard_find_set_bit(0)
     * otherwise.
     */
    if (!whiteKing || !blackKing) {
        dbase_free_position(p);
        return NULL;
    }

    /*
     * Recalculate all derived position data (piece masks, attacks,
     * material, hash keys, king squares).
     */
    dbase_recalc_attacks(p);

    return p;
}
