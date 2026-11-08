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
 * nn_extract.c - batch feature-index extraction for the training
 * pipeline.
 *
 * Decodes packed BPD records into full positions and computes the
 * NNUE input feature indices (piece, shared pawn pair, attack).  The
 * index computation is shared with the engine itself (see
 * neural_net.c and attack_map.c), so the training data and the
 * engine can never drift apart.
 *
 * bpd_record_valid() is the arbiter for BPD record validation; the
 * Python side mirrors it in a cross-test (PLAN-attack-map-features.md,
 * T2).
 */

#include "nn_extract.h"
#include "bpd.h"
#include "dbase.h"
#include "init.h"

#include <string.h>

/*
 * The extraction entry points need the engine's precomputed tables
 * (most importantly the magic bitboard tables behind the bishop,
 * rook and queen attack masks); the Amy binary calls init_all() in
 * main(), but the standalone library does not, so run it here exactly
 * once.  Inside the Amy binary this re-runs harmlessly: the tables
 * are rebuilt idempotently.
 */
static int nn_extract_initialized = 0;

static void nn_extract_ensure_init(void) {
    if (!nn_extract_initialized) {
        init_all();
        nn_extract_initialized = 1;
    }
}

/*
 * Validate one BPD record.
 *
 * Rules (mirrored by BpdRecord.validate() in the training repo; this
 * C version is the arbiter):
 * - turn is 0 (white) or 1 (black)
 * - exactly one king per side
 * - pawns only on ranks 2..7 (rank 1 or 8 would run the pawn pair
 *   index sq' - 8 out of bounds)
 * - the en passant square, if present, is on rank 3 or 6 (it is
 *   stored in place of a piece, so the square is empty by
 *   construction)
 */
static bool bpd_record_valid(const BinaryPosition *bp) {
    int white_kings = 0;
    int black_kings = 0;
    int i;
    int k;

    if (bp->turn != 0 && bp->turn != 1) {
        return false;
    }

    for (i = 0; i < 4; i++) {
        uint64_t word = bp->pos[i];

        for (k = 0; k < 16; k++) {
            uint8_t nibble = (uint8_t)((word >> (4 * k)) & 0xF);
            int sq = i * 16 + k;

            switch (nibble) {
            case PN_W_KING:
                white_kings++;
                break;
            case PN_B_KING:
                black_kings++;
                break;
            case PN_W_PAWN:
            case PN_B_PAWN:
                if ((sq >> 3) < 1 || (sq >> 3) > 6) {
                    return false;
                }
                break;
            case PN_EN_PASSANT:
                if ((sq >> 3) != 2 && (sq >> 3) != 5) {
                    return false;
                }
                break;
            default:
                break;
            }
        }
    }

    return white_kings == 1 && black_kings == 1;
}

void nn_export_constants(NnExtractConstants *out) {
    out->n_features = N_FEATURES;
    out->n_pawn_features = N_PAWN_FEATURES;
    out->n_attack_features = N_ATTACK_FEATURES;
    out->accumulator_size = ACCUMULATOR_SIZE;
    out->max_piece_features = NN_MAX_PIECE_FEATURES;
    out->max_pawn_features = NN_MAX_PAWN_FEATURES;
    out->max_attack_features = NN_MAX_ATTACK_FEATURES;
    out->phase_count = NN_PHASE_COUNT;
}

int nn_extract_features(const uint8_t *data, size_t n_records, int32_t *feat_w,
                        int32_t *feat_b, int32_t *pfeat_w, int32_t *pfeat_b,
                        int32_t *afeat_w, int32_t *afeat_b, uint8_t *turn,
                        int32_t *score, int32_t *valid, int32_t *phase) {
    size_t i;
    int n_valid = 0;

    nn_extract_ensure_init();

    for (i = 0; i < n_records; i++) {
        int32_t *fw = feat_w + i * NN_MAX_PIECE_FEATURES;
        int32_t *fb = feat_b + i * NN_MAX_PIECE_FEATURES;
        int32_t *pfw = pfeat_w + i * NN_MAX_PAWN_FEATURES;
        int32_t *pfb = pfeat_b + i * NN_MAX_PAWN_FEATURES;
        int32_t *afw = afeat_w + i * NN_MAX_ATTACK_FEATURES;
        int32_t *afb = afeat_b + i * NN_MAX_ATTACK_FEATURES;
        int32_t *pm = phase + i * NN_PHASE_COUNT;
        BinaryPosition bp;
        struct Position *p;

        memset(fw, -1, NN_MAX_PIECE_FEATURES * sizeof(int32_t));
        memset(fb, -1, NN_MAX_PIECE_FEATURES * sizeof(int32_t));
        memset(pfw, -1, NN_MAX_PAWN_FEATURES * sizeof(int32_t));
        memset(pfb, -1, NN_MAX_PAWN_FEATURES * sizeof(int32_t));
        memset(afw, -1, NN_MAX_ATTACK_FEATURES * sizeof(int32_t));
        memset(afb, -1, NN_MAX_ATTACK_FEATURES * sizeof(int32_t));
        memset(pm, 0, NN_PHASE_COUNT * sizeof(int32_t));
        valid[i] = 0;

        memcpy(&bp, data + i * sizeof(BinaryPosition), sizeof(BinaryPosition));

        if (!bpd_record_valid(&bp)) {
            continue;
        }

        p = bpd_decode(&bp);
        if (p == NULL) {
            continue;
        }

        if (nn_compute_piece_indices(p, White, fw, NN_MAX_PIECE_FEATURES) < 0 ||
            nn_compute_piece_indices(p, Black, fb, NN_MAX_PIECE_FEATURES) < 0 ||
            nn_compute_pawn_indices(p, White, pfw, NN_MAX_PAWN_FEATURES) < 0 ||
            nn_compute_pawn_indices(p, Black, pfb, NN_MAX_PAWN_FEATURES) < 0 ||
            am_compute_attack_indices(p, White, afw, NN_MAX_ATTACK_FEATURES) <
                0 ||
            am_compute_attack_indices(p, Black, afb, NN_MAX_ATTACK_FEATURES) <
                0) {
            /*
             * A valid position exceeded an output capacity.  This
             * should not happen (the caps cover every legal position
             * and common test positions); mark the record invalid
             * rather than trusting a partially filled row.
             */
            dbase_free_position(p);
            continue;
        }

        turn[i] = bp.turn;
        score[i] = bp.score;
        nn_phase_masks(p, pm);
        valid[i] = 1;
        n_valid++;

        dbase_free_position(p);
    }

    return n_valid;
}
