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
 * test_nn_extract.c - tests for the batch feature-index extraction
 * API used by the Python training pipeline (nn_extract.c).
 */

#include "test_nn_extract.h"
#include "bpd.h"
#include "dbase.h"
#include "nn_extract.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Encode an EPD position as one BPD record in a malloc'd buffer.
 */
static uint8_t *make_record(const char *epd, int score) {
    struct Position *p = dbase_create_position_from_epd((char *)epd);
    BinaryPosition bp;
    uint8_t *data;

    bpd_encode(p, score, &bp);
    data = malloc(sizeof(BinaryPosition));
    memcpy(data, &bp, sizeof(BinaryPosition));
    dbase_free_position(p);
    return data;
}

/*
 * Number of valid (non -1) entries at the front of a feature row.
 */
static int row_count(const int32_t *row, int cap) {
    int n = 0;

    while (n < cap && row[n] >= 0) {
        n++;
    }
    return n;
}

static int row_contains(const int32_t *row, int cap, int value) {
    int n;

    for (n = 0; n < cap; n++) {
        if (row[n] == value) {
            return 1;
        }
        if (row[n] < 0) {
            break;
        }
    }
    return 0;
}

/*
 * Extract one record; returns 1 if the record is valid.
 * The row arrays are static so the caller can inspect them.
 */
static int extract_one(const uint8_t *data, int *fw, int *fb, int *pfw,
                       int *pfb, int *afw, int *afb, uint8_t *turn,
                       int32_t *score, int32_t *valid) {
    int n;

    n = nn_extract_features(data, 1, fw, fb, pfw, pfb, afw, afb, turn, score,
                            valid);
    assert(n == (int)valid[0]);
    return valid[0];
}

static void test_constants(void) {
    NnExtractConstants c;

    nn_export_constants(&c);
    assert(c.n_features == N_FEATURES);
    assert(c.n_pawn_features == N_PAWN_FEATURES);
    assert(c.n_attack_features == N_ATTACK_FEATURES);
    assert(c.accumulator_size == ACCUMULATOR_SIZE);
    assert(c.max_piece_features == NN_MAX_PIECE_FEATURES);
    assert(c.max_pawn_features == NN_MAX_PAWN_FEATURES);
    assert(c.max_attack_features == NN_MAX_ATTACK_FEATURES);
}

static void test_initial_position(void) {
    static int32_t fw[NN_MAX_PIECE_FEATURES];
    static int32_t fb[NN_MAX_PIECE_FEATURES];
    static int32_t pfw[NN_MAX_PAWN_FEATURES];
    static int32_t pfb[NN_MAX_PAWN_FEATURES];
    static int32_t afw[NN_MAX_ATTACK_FEATURES];
    static int32_t afb[NN_MAX_ATTACK_FEATURES];
    uint8_t turn;
    int32_t score;
    int32_t valid;
    uint8_t *data;
    int i;
    int anchors[4] = {27, 4004, 4464, 4559};

    data = make_record("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -",
                       12345);
    assert(extract_one(data, fw, fb, pfw, pfb, afw, afb, &turn, &score,
                       &valid) == 1);
    assert(turn == 0);
    assert(score == 12345);

    /* 15 own + 15 opponent non-king pieces + opponent king. */
    assert(row_count(fw, NN_MAX_PIECE_FEATURES) == 31);
    assert(row_count(fb, NN_MAX_PIECE_FEATURES) == 31);

    /*
     * 36 pawn pairs per side: all 16 pawns (both colors, each side's
     * accumulator sees the whole set) within one file of distance: 8
     * same-file + 28 adjacent-file.  In the initial position both
     * sides' frames map the set to identical indices.
     */
    assert(row_count(pfw, NN_MAX_PAWN_FEATURES) == 36);
    assert(row_count(pfb, NN_MAX_PAWN_FEATURES) == 36);
    for (i = 0; i < 4; i++) {
        assert(row_contains(pfw, NN_MAX_PAWN_FEATURES, anchors[i]));
        assert(row_contains(pfb, NN_MAX_PAWN_FEATURES, anchors[i]));
    }
    /*
     * Anchors: 27 = (a2, b2) in the white frame (ids 6/7); 4559 =
     * (g8, h8) in the white frame, which is (a2, b2) in the black
     * frame (ids 94/95, colors swapped); 4004 = (a8, b8) in the
     * white frame.
     */

    /* No pieces attack pieces in the initial position. */
    assert(row_count(afw, NN_MAX_ATTACK_FEATURES) == 0);
    assert(row_count(afb, NN_MAX_ATTACK_FEATURES) == 0);

    /*
     * Opponent king plane (plane 10): each king canonicalizes to its
     * own d1 (king_index 3); the opponent king is e8 = 60 (white
     * side, flip 7) or e1 = 4 (black side, flip 0x3f), and 60^7 =
     * 4^0x3f = 59: 2048*10 + 64*3 + 59 on both sides.
     */
    assert(row_contains(fw, NN_MAX_PIECE_FEATURES, 20480 + 192 + 59));
    assert(row_contains(fb, NN_MAX_PIECE_FEATURES, 20480 + 192 + 59));

    free(data);
}

static void test_pawn_attacks_bishop(void) {
    static int32_t fw[NN_MAX_PIECE_FEATURES];
    static int32_t fb[NN_MAX_PIECE_FEATURES];
    static int32_t pfw[NN_MAX_PAWN_FEATURES];
    static int32_t pfb[NN_MAX_PAWN_FEATURES];
    static int32_t afw[NN_MAX_ATTACK_FEATURES];
    static int32_t afb[NN_MAX_ATTACK_FEATURES];
    uint8_t turn;
    int32_t score;
    int32_t valid;
    uint8_t *data;

    /* White P b4 attacks B c5; B c5 attacks P b4. */
    data = make_record("4k3/8/8/2b5/1P6/8/8/4K3 w - -", 7);
    assert(extract_one(data, fw, fb, pfw, pfb, afw, afb, &turn, &score,
                       &valid) == 1);

    assert(row_count(fw, NN_MAX_PIECE_FEATURES) == 3);
    assert(row_count(fb, NN_MAX_PIECE_FEATURES) == 3);
    assert(row_count(pfw, NN_MAX_PAWN_FEATURES) == 0);
    assert(row_count(pfb, NN_MAX_PAWN_FEATURES) == 0);

    /*
     * 3 piece features per side: 1 own non-king + 1 opponent
     * non-king + the opponent king (the own king is not a feature).
     *
     * White: (Pawn, Bishop) plane 1; white king e1 -> flip 7,
     * b4 25^7 = 30, c5 34^7 = 37: 4096*1 + 64*30 + 37.
     * Black: the (Bishop, Pawn) attack on b4 is excluded, because
     * the pawn on b4 attacks the bishop (b4 in PawnEPM[Black][c5]).
     */
    assert(row_count(afw, NN_MAX_ATTACK_FEATURES) == 1);
    assert(afw[0] == 4096 + 64 * 30 + 37);
    assert(row_count(afb, NN_MAX_ATTACK_FEATURES) == 0);

    free(data);
}

static void test_knight_attacks_pawn(void) {
    static int32_t fw[NN_MAX_PIECE_FEATURES];
    static int32_t fb[NN_MAX_PIECE_FEATURES];
    static int32_t pfw[NN_MAX_PAWN_FEATURES];
    static int32_t pfb[NN_MAX_PAWN_FEATURES];
    static int32_t afw[NN_MAX_ATTACK_FEATURES];
    static int32_t afb[NN_MAX_ATTACK_FEATURES];
    uint8_t turn;
    int32_t score;
    int32_t valid;
    uint8_t *data;

    /* White N f3 attacks P g5; black pawn g5 attacks nothing. */
    data = make_record("4k3/8/8/6p3/8/5N2/8/4K3 w - -", -12);
    assert(extract_one(data, fw, fb, pfw, pfb, afw, afb, &turn, &score,
                       &valid) == 1);

    assert(row_count(fw, NN_MAX_PIECE_FEATURES) == 3);
    assert(row_count(fb, NN_MAX_PIECE_FEATURES) == 3);
    assert(row_count(pfw, NN_MAX_PAWN_FEATURES) == 0);
    assert(row_count(pfb, NN_MAX_PAWN_FEATURES) == 0);

    /*
     * White: (Knight, Pawn) plane 4; white king e1 -> flip 7,
     * f3 21^7 = 18, g5 38^7 = 33: 4096*4 + 64*18 + 33.
     */
    assert(row_count(afw, NN_MAX_ATTACK_FEATURES) == 1);
    assert(afw[0] == 4096 * 4 + 64 * 18 + 33);
    assert(row_count(afb, NN_MAX_ATTACK_FEATURES) == 0);

    free(data);
}

static void test_invalid_records(void) {
    static int32_t fw[NN_MAX_PIECE_FEATURES];
    static int32_t fb[NN_MAX_PIECE_FEATURES];
    static int32_t pfw[NN_MAX_PAWN_FEATURES];
    static int32_t pfb[NN_MAX_PAWN_FEATURES];
    static int32_t afw[NN_MAX_ATTACK_FEATURES];
    static int32_t afb[NN_MAX_ATTACK_FEATURES];
    uint8_t turn;
    int32_t score;
    int32_t valid;
    uint8_t *data;

    /* turn must be 0 or 1. */
    data =
        make_record("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -", 0);
    data[36] = 2;
    assert(extract_one(data, fw, fb, pfw, pfb, afw, afb, &turn, &score,
                       &valid) == 0);
    assert(row_count(fw, NN_MAX_PIECE_FEATURES) == 0);
    free(data);

    /* Pawn on rank 8. */
    data = make_record("P3k3/8/8/8/8/8/8/4K3 w - -", 0);
    assert(extract_one(data, fw, fb, pfw, pfb, afw, afb, &turn, &score,
                       &valid) == 0);
    free(data);

    /* Pawn on rank 1. */
    data = make_record("R3K2P/8/8/8/8/8/8/4k3 w - -", 0);
    assert(extract_one(data, fw, fb, pfw, pfb, afw, afb, &turn, &score,
                       &valid) == 0);
    free(data);

    /* Missing black king. */
    data = make_record("4K3/8/8/8/8/8/8/8 w - -", 0);
    assert(extract_one(data, fw, fb, pfw, pfb, afw, afb, &turn, &score,
                       &valid) == 0);
    free(data);

    /* Two white kings. */
    data = make_record("3Kk3/8/8/8/8/8/8/4K3 w - -", 0);
    assert(extract_one(data, fw, fb, pfw, pfb, afw, afb, &turn, &score,
                       &valid) == 0);
    free(data);

    /* En passant square on the wrong rank. */
    data = make_record("4k3/8/8/8/8/4P3/8/4K3 b - e4", 0);
    assert(extract_one(data, fw, fb, pfw, pfb, afw, afb, &turn, &score,
                       &valid) == 0);
    free(data);

    /* En passant square on rank 3 is fine. */
    data = make_record("4k3/8/8/8/4P3/8/4P3/4K3 b - e3", 0);
    assert(extract_one(data, fw, fb, pfw, pfb, afw, afb, &turn, &score,
                       &valid) == 1);
    assert(row_count(fw, NN_MAX_PIECE_FEATURES) == 3);
    free(data);
}

static void test_mixed_batch(void) {
    static int32_t fw[3 * NN_MAX_PIECE_FEATURES];
    static int32_t fb[3 * NN_MAX_PIECE_FEATURES];
    static int32_t pfw[3 * NN_MAX_PAWN_FEATURES];
    static int32_t pfb[3 * NN_MAX_PAWN_FEATURES];
    static int32_t afw[3 * NN_MAX_ATTACK_FEATURES];
    static int32_t afb[3 * NN_MAX_ATTACK_FEATURES];
    static uint8_t turn[3];
    static int32_t score[3];
    static int32_t valid[3];
    uint8_t *r0, *r1, *r2;
    uint8_t batch[3 * sizeof(BinaryPosition)];
    int n;

    r0 = make_record("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -",
                     100);
    r1 = make_record("P3k3/8/8/8/8/8/8/4K3 w - -", 200);
    r2 = make_record("4k3/8/8/8/8/8/8/4K3 w - -", 300);

    memcpy(batch, r0, sizeof(BinaryPosition));
    memcpy(batch + sizeof(BinaryPosition), r1, sizeof(BinaryPosition));
    memcpy(batch + 2 * sizeof(BinaryPosition), r2, sizeof(BinaryPosition));

    n = nn_extract_features(batch, 3, fw, fb, pfw, pfb, afw, afb, turn, score,
                            valid);
    assert(n == 2);
    assert(valid[0] == 1 && valid[1] == 0 && valid[2] == 1);
    assert(turn[0] == 0 && score[0] == 100);
    assert(turn[2] == 0 && score[2] == 300);
    assert(row_count(fw, NN_MAX_PIECE_FEATURES) == 31);
    assert(row_count(fw + NN_MAX_PIECE_FEATURES, NN_MAX_PIECE_FEATURES) == 0);
    assert(row_count(fw + 2 * NN_MAX_PIECE_FEATURES, NN_MAX_PIECE_FEATURES) ==
           1);

    free(r0);
    free(r1);
    free(r2);
}

void test_all_nn_extract(void) {
    test_constants();
    test_initial_position();
    test_pawn_attacks_bishop();
    test_knight_attacks_pawn();
    test_invalid_records();
    test_mixed_batch();
}
