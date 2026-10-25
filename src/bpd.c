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
 *   For a fresh en passant flag (set by the opponent's immediately
 *   preceding double push) that square (on rank 3 or 6) is always
 *   empty, so no piece information is lost.
 *
 * The side to move does not fit into the 64 nibbles and is stored in
 * BinaryPosition.turn.
 */

#include <stdio.h>
#include <stdlib.h>

#include "bpd.h"
#include "dbase.h"
#include "safe_malloc.h"
#include "utils.h"

/*
 * The on-disk format of the evaluation log and of the test data files
 * depends on the exact size of BinaryPosition.
 */
static_assert(sizeof(BinaryPosition) == 40,
              "BinaryPosition size changed; the on-disk format is no "
              "longer 40 bytes");

/*
 * Initial size of the game history, mirroring dbase.c.
 */
#define BPD_INITIAL_GAME_LOG_SIZE 40
#define BPD_LOG_BUFFER_SIZE 256
#define BPD_LOG_FILE_NAME "evaluation_log.bin"

/*
 * Mapping Piece value (Pawn..King) -> PieceNibble for white pieces.
 */
static const uint8_t whitePieceNibble[7] = {PN_EMPTY,    PN_W_PAWN, PN_W_KNIGHT,
                                            PN_W_BISHOP, PN_W_ROOK, PN_W_QUEEN,
                                            PN_W_KING};

/*
 * Mapping Piece value (Pawn..King) -> PieceNibble for black pieces.
 */
static const uint8_t blackPieceNibble[7] = {PN_EMPTY,    PN_B_PAWN, PN_B_KNIGHT,
                                            PN_B_BISHOP, PN_B_ROOK, PN_B_QUEEN,
                                            PN_B_KING};

/*
 * Mapping PieceNibble -> Piece value (white positive, black negative,
 * Neutral for empty squares and the en passant square).
 */
static const int8_t nibblePiece[16] = {
    Neutral,                                               /* PN_EMPTY        */
    Pawn,    Knight,  Bishop,  Rook,  Rook,  Queen,  King, /* white */
    -Pawn,   -Knight, -Bishop, -Rook, -Rook, -Queen, -King, /* black */
    Neutral /* PN_EN_PASSANT   */
};

/*
 * When true, evaluations are logged in all modes; when false, they are
 * logged only during selfplay (see bpd.h).
 */
bool EvalLog = false;

/* Buffer to store logged evaluations */
static BinaryPosition *log_buffer = NULL;

static int log_index = 0;

/*
 * Once the log file could not be opened, logging is permanently
 * disabled for the rest of the run.
 */
static bool log_disabled = false;

/*
 * Write all buffered evaluations to the log file. Returns true on
 * success. If the file cannot be opened, the buffered evaluations are
 * discarded and false is returned.
 */
static bool bpd_write_log(void) {
    FILE *fout;

    if (log_index == 0)
        return true;

    fout = fopen(BPD_LOG_FILE_NAME, "a");
    if (fout == NULL) {
        print(0, "Could not open " BPD_LOG_FILE_NAME
                 "; evaluation logging is disabled.\n");
        log_index = 0;
        return false;
    }

    fwrite(log_buffer, sizeof(BinaryPosition), log_index, fout);

    fclose(fout);

    log_index = 0;
    return true;
}

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

void bpd_log_evaluation(const struct Position *p, int score) {
    if (log_disabled)
        return;

    if (!log_buffer) {
        log_buffer = safe_calloc(BPD_LOG_BUFFER_SIZE, sizeof(BinaryPosition));
        /*
         * Make sure the evaluations still in the buffer reach the log
         * file when the program exits. This also covers the exit() in
         * cmd_quit(), which does not run the cleanup code in main().
         */
        atexit(bpd_flush_log);
    }

    bpd_encode(p, score, &log_buffer[log_index]);

    if (++log_index == BPD_LOG_BUFFER_SIZE && !bpd_write_log())
        log_disabled = true;
}

void bpd_flush_log(void) {
    if (!log_disabled)
        bpd_write_log();
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
     * A chess position must have exactly one king for each color;
     * with none, dbase_recalc_attacks() would hit
     * bitboard_find_set_bit(0), and with more than one it would pick
     * an arbitrary king square.
     */
    if (whiteKing != 1 || blackKing != 1) {
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

/*
 * The EPD table for the cross-implementation test data (see the test
 * plan in ../nnue, nnue/TEST_PLAN_BPD.md, for the concrete scenarios).
 * The table is defined here in the source code only; the EPDs are not
 * written to disk.
 *
 * The table currently covers scenarios 1-4 of the plan; the remaining
 * scenarios are picked up later.
 */
static const char *bpd_test_epds[] = {
    /* 1: initial position, white to move */
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -",
    /* 2: initial position, black to move */
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR b KQkq -",
    /* 3: all castling rights still present (after 1.e4 e5 2.Nf3) */
    "rnbqkbnr/pppp1ppp/8/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq -",
    /* 4a: white rook on its home corner (a1) without castling right */
    "4k3/8/8/8/8/8/8/R3K3 w - -",
    /* 4b: black rooks on their home corners (a8, h8) without castling right */
    "r3k2r/8/8/8/8/8/8/4K3 b - -",
};

/*
 * Create the test data for the nnue project: encode every EPD from
 * bpd_test_epds with bpd_encode() (score = 1-based index into the
 * table) into a single array of BinaryPosition and write the array
 * to the file at path.
 *
 * On file, each record is the raw BinaryPosition struct
 * (sizeof == 40: 32 bytes pos + 4 bytes score + 1 byte turn + 3 bytes
 * padding), little-endian on x86/ARM.
 */
int bpd_write_test_data(const char *path) {
    FILE *f = fopen(path, "wb");
    int i;
    int n = sizeof(bpd_test_epds) / sizeof(bpd_test_epds[0]);
    BinaryPosition *testdata = safe_malloc(n * sizeof(BinaryPosition));

    if (f == NULL) {
        print(0, "bpd_write_test_data(): cannot open %s\n", path);
        free(testdata);
        return -1;
    }

    for (i = 0; i < n; i++) {
        struct Position *p =
            dbase_create_position_from_epd((char *)bpd_test_epds[i]);

        bpd_encode(p, i + 1, &testdata[i]);
        dbase_free_position(p);
    }

    if (fwrite(testdata, sizeof(BinaryPosition), n, f) != (size_t)n) {
        print(0, "bpd_write_test_data(): write error on %s\n", path);
        fclose(f);
        free(testdata);
        return -1;
    }

    fclose(f);
    free(testdata);
    print(0, "bpd_write_test_data(): wrote %d record(s) to %s\n", n, path);
    return 0;
}
