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

#include "neural_net.h"
#include "amy.h"
#include "bitboard.h"
#include "dbase.h"
#include "evaluation.h"
#include "hashtable.h"
#include "inline.h"
#include "random.h"
#include "utils.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// #define STATISTICS_Q 1
// #define DEBUG 1

#define N_FEATURES (64 * 64 * 10)

#define SCALE_INPUT 64
#define SCALE_INPUT_HALF (SCALE_INPUT / 2 - 1)
#define SCALE 8192
#define SCALE_HALF (SCALE / 2 - 1)
#define SCALE_OUTPUT 10000

int8_t input_layer_weights_q[N_FEATURES][ACCUMULATOR_SIZE];
int16_t input_layer_bias_q[ACCUMULATOR_SIZE];
int8_t hidden_layer_1_weights_q[32][2 * ACCUMULATOR_SIZE];
int16_t hidden_layer_1_bias_q[32];
int16_t hidden_layer_2_weights_q[32][32];
int32_t hidden_layer_2_bias_q[32];
int32_t output_layer_weights_q[32];
int32_t output_layer_bias_q[1];

static bool verbose = true;

void nn_read_weights_quantized(void) {
    FILE *fin = fopen("network.quant", "rb");
    size_t items_read;

    if (fin == NULL) {
        print(0, "No weights found.\n");
        return;
    }

    items_read = fread(input_layer_weights_q, sizeof(int8_t),
                       ACCUMULATOR_SIZE * N_FEATURES, fin);
    assert(items_read == ACCUMULATOR_SIZE * N_FEATURES);

    items_read =
        fread(input_layer_bias_q, sizeof(int16_t), ACCUMULATOR_SIZE, fin);
    assert(items_read == ACCUMULATOR_SIZE);

    items_read = fread(hidden_layer_1_weights_q, sizeof(int8_t),
                       32 * 2 * ACCUMULATOR_SIZE, fin);
    assert(items_read == 32 * 2 * ACCUMULATOR_SIZE);

    items_read = fread(hidden_layer_1_bias_q, sizeof(int16_t), 32, fin);
    assert(items_read == 32);

    items_read = fread(hidden_layer_2_weights_q, sizeof(int16_t), 32 * 32, fin);
    assert(items_read == 32 * 32);

    items_read = fread(hidden_layer_2_bias_q, sizeof(int32_t), 32, fin);
    assert(items_read == 32);

    items_read = fread(output_layer_weights_q, sizeof(int32_t), 32, fin);
    assert(items_read == 32);

    items_read = fread(output_layer_bias_q, sizeof(int32_t), 1, fin);
    assert(items_read == 1);

    fclose(fin);

    print(0, "Read weights.\n");
}

#ifdef STATISTICS_Q
static int32_t max_activation_hidden_1_q = 0;
#endif

static void nn_update_accumulator_add(int16_t *accumulator_q, int offset) {
#ifdef DEBUG_UPDATE
    if (verbose) {
        print(1, "nn_update_accumulator_add(%lx, %d)\n",
              (unsigned long)accumulator_q, offset);
        print(1, "accumulator_q[0]=%d\n", accumulator_q[0]);
    }
#endif
    for (int i = 0; i < ACCUMULATOR_SIZE; i++) {
        accumulator_q[i] += input_layer_weights_q[offset][i];
    }
#ifdef DEBUG_UPDATE
    if (verbose) {
        print(1, "accumulator_q[0]=%d\n", accumulator_q[0]);
    }
#endif
}

static void nn_update_accumulator_sub(int16_t *accumulator_q, int offset) {
#ifdef DEBUG_UPDATE
    if (verbose) {
        print(1, "nn_update_accumulator_sub(%lx, %d)\n",
              (unsigned long)accumulator_q, offset);
        print(1, "accumulator_q[0]=%d\n", accumulator_q[0]);
    }
#endif
    for (int i = 0; i < ACCUMULATOR_SIZE; i++) {
        accumulator_q[i] -= input_layer_weights_q[offset][i];
    }
#ifdef DEBUG_UPDATE
    if (verbose) {
        print(1, "accumulator_q[0]=%d\n", accumulator_q[0]);
    }
#endif
}

void nn_update_weights_for_piece(struct Position *p, int tp, int sq, bool turn,
                                 bool add) {
#ifdef DEBUG_UPDATE
    print(1, "nn_update_weights_for_piece(tp=%d, sq=%d, turn=%d, add=%d)\n", tp,
          sq, turn, add);
#endif

    const int flip_b = 0x38;
    int king_sq_w = p->kingSq[White];
    int king_sq_b = p->kingSq[Black];

    tp = ABS(tp);

    void (*update_fn)(int16_t *, int) =
        add ? nn_update_accumulator_add : nn_update_accumulator_sub;

    king_sq_b ^= flip_b;

    if (turn == White) {
        int index = 4096 * (tp - 1) + 64 * king_sq_w + sq;
        update_fn(p->accumulator_w_q, index);

        index = 4096 * (tp + 4) + 64 * king_sq_b + (sq ^ flip_b);
        update_fn(p->accumulator_b_q, index);
    } else {
        int index = 4096 * (tp + 4) + 64 * king_sq_w + sq;
        update_fn(p->accumulator_w_q, index);

        index = 4096 * (tp - 1) + 64 * king_sq_b + (sq ^ flip_b);
        update_fn(p->accumulator_b_q, index);
    }
}

void nn_update_weights_for_king(struct Position *p) {
#ifdef DEBUG_UPDATE
    print(1, "nn_update_weights_for_king()\n");
#endif

    int flip = (p->turn == White) ? 0 : 0x38;
    int king_sq = p->kingSq[p->turn];

    king_sq ^= flip;

    int16_t *accumulator_q =
        (p->turn == White) ? p->accumulator_w_q : p->accumulator_b_q;
    memcpy(accumulator_q, input_layer_bias_q,
           sizeof(int16_t) * ACCUMULATOR_SIZE);

    for (int tp = Pawn; tp <= Queen; tp++) {
        BitBoard mask = p->mask[p->turn][tp];

        while (mask) {
            int sq = bitboard_find_set_bit(mask);
            mask &= mask - 1;

            int index = 4096 * (tp - 1) + 64 * king_sq + (sq ^ flip);
            nn_update_accumulator_add(accumulator_q, index);
        }

        mask = p->mask[OPP(p->turn)][tp];

        while (mask) {
            int sq = bitboard_find_set_bit(mask);
            mask &= mask - 1;

            int index = 4096 * (tp + 4) + 64 * king_sq + (sq ^ flip);
            nn_update_accumulator_add(accumulator_q, index);
        }
    }
}

int nn_evaluate_position(struct Position *p) {
    int score;
#ifdef STATISTICS_Q
    bool print_stats_q = false;
#endif

#ifndef DEBUG
    STry++;
    if (ht_probe_score(p->hkey, &score) == Useful) {
        SHit++;
        return score;
    }
#endif

#ifdef DEBUG
    for (int i = 0; i < 16; i++) {
        print(1, "%d, ", p->accumulator_w_q[i]);
    }
    print(1, "\n");
    for (int i = 0; i < 16; i++) {
        print(1, "%d, ", p->accumulator_b_q[i]);
    }
    print(1, "\n");
#endif

    int16_t accumulator[2 * ACCUMULATOR_SIZE];

    if (p->turn == White) {
        memcpy(accumulator, p->accumulator_w_q,
               ACCUMULATOR_SIZE * sizeof(int16_t));
        memcpy(accumulator + ACCUMULATOR_SIZE, p->accumulator_b_q,
               ACCUMULATOR_SIZE * sizeof(int16_t));
    } else {
        memcpy(accumulator, p->accumulator_b_q,
               ACCUMULATOR_SIZE * sizeof(int16_t));
        memcpy(accumulator + ACCUMULATOR_SIZE, p->accumulator_w_q,
               ACCUMULATOR_SIZE * sizeof(int16_t));
    }

    for (int i = 0; i < 2 * ACCUMULATOR_SIZE; i++) {
        if (accumulator[i] < 0) {
            accumulator[i] = 0;
        }

        if (accumulator[i] > SCALE_INPUT) {
            accumulator[i] = SCALE_INPUT;
        }
    }

    int32_t hidden_layer_1_q[32];
    for (int i = 0; i < 32; i++) {
        hidden_layer_1_q[i] = hidden_layer_1_bias_q[i];
    }

    for (int i = 0; i < 32; i++) {
        for (int j = 0; j < 2 * ACCUMULATOR_SIZE; j++) {
            hidden_layer_1_q[i] +=
                accumulator[j] * hidden_layer_1_weights_q[i][j];
        }

#ifdef STATISTICS_Q
        if (ABS(hidden_layer_1_q[i]) > max_activation_hidden_1_q) {
            max_activation_hidden_1_q = ABS(hidden_layer_1_q[i]);
            print_stats_q = true;
        }
#endif
    }

    int16_t hidden_layer_1_q16[32];

    for (int i = 0; i < 32; i++) {
        if (hidden_layer_1_q[i] <= 0) {
            hidden_layer_1_q16[i] = 0;
        } else if (hidden_layer_1_q[i] > SCALE_INPUT * SCALE_INPUT) {
            hidden_layer_1_q16[i] = SCALE_INPUT;
        } else {
            hidden_layer_1_q16[i] =
                (int16_t)((hidden_layer_1_q[i] + SCALE_INPUT_HALF) /
                          SCALE_INPUT);
        }

        assert(hidden_layer_1_q16[i] >= 0);
    }

#ifdef DEBUG
    print(1, "hidden_layer_1_q:\n");
    for (int i = 0; i < 16; i++) {
        print(1, "%d, ", hidden_layer_1_q16[i]);
    }
    print(1, "\n");
#endif

    int32_t hidden_layer_2_q[32];
    memcpy(hidden_layer_2_q, hidden_layer_2_bias_q, 32 * sizeof(int32_t));

    for (int i = 0; i < 32; i++) {
        for (int j = 0; j < 32; j++) {
            hidden_layer_2_q[i] +=
                hidden_layer_1_q16[j] * hidden_layer_2_weights_q[i][j];
        }
    }

    int16_t hidden_layer_2_q16[32];

    for (int i = 0; i < 32; i++) {
        if (hidden_layer_2_q[i] <= 0) {
            hidden_layer_2_q16[i] = 0;
        } else if (hidden_layer_2_q[i] >= SCALE_INPUT * SCALE) {
            hidden_layer_2_q16[i] = SCALE;
        } else {
            hidden_layer_2_q16[i] =
                (int16_t)((hidden_layer_2_q[i] + SCALE_INPUT_HALF) /
                          SCALE_INPUT);
        }

        assert(hidden_layer_2_q16[i] >= 0);
    }

#ifdef DEBUG
    print(1, "hidden_layer_2_q:\n");
    for (int i = 0; i < 8; i++) {
        print(1, "%d, ", hidden_layer_2_q16[i]);
    }
    print(1, "\n");
#endif

    int32_t output_layer_q = output_layer_bias_q[0];

    for (int j = 0; j < 32; j++) {
        output_layer_q += hidden_layer_2_q16[j] * output_layer_weights_q[j];
    }

#ifdef DEBUG
    print(1, "output_layer_q: %d\n", output_layer_q);
#endif

#ifdef STATISTICS_Q
    if (print_stats_q) {
        FILE *stats_file = fopen("activation_q.txt", "w");
        fprintf(stats_file, "max_activation_hidden_1: %d (%f)\n",
                max_activation_hidden_1_q,
                (double)max_activation_hidden_1_q /
                    ((double)SCALE_INPUT * SCALE_INPUT));
        fclose(stats_file);
    }
#endif

    score = ((output_layer_q + SCALE_HALF) / SCALE) * SCALE_OUTPUT;
    score = (score + SCALE_HALF) / SCALE;

    // print(0, "score: %d, score_q: %d\n", score, score_q);

    int material_balance =
        (p->turn == 0) ? eval_material_balance(p) : -eval_material_balance(p);
    int pos_score = abs(material_balance - score);

    if (pos_score > MaxPos) {
        MaxPos = pos_score;
    }

    ht_store_score(p->hkey, score);

    return score;
}

void nn_init_accumulator(struct Position *p) {
    memcpy(p->accumulator_w_q, input_layer_bias_q,
           sizeof(int16_t) * ACCUMULATOR_SIZE);
    memcpy(p->accumulator_b_q, input_layer_bias_q,
           sizeof(int16_t) * ACCUMULATOR_SIZE);

#ifdef DEBUG
    print(1, "bias:\n");
    for (int i = 0; i < 8; i++) {
        print(1, "%d, ", p->accumulator_w_q[i]);
    }
    print(1, "\n");
#endif

    for (int tp = Pawn; tp <= Queen; tp++) {
        BitBoard mask = p->mask[White][tp];

        while (mask) {
            int sq = bitboard_find_set_bit(mask);
            mask &= mask - 1;

            nn_update_weights_for_piece(p, tp, sq, 0, true);
        }

        mask = p->mask[Black][tp];

        while (mask) {
            int sq = bitboard_find_set_bit(mask);
            mask &= mask - 1;

            nn_update_weights_for_piece(p, tp, sq, 1, true);
        }
    }
}

void nn_validate_weights(struct Position *p) {
    int16_t accumulator_w_q[ACCUMULATOR_SIZE];
    int16_t accumulator_b_q[ACCUMULATOR_SIZE];

    memcpy(accumulator_w_q, input_layer_bias_q,
           sizeof(int16_t) * ACCUMULATOR_SIZE);
    memcpy(accumulator_b_q, input_layer_bias_q,
           sizeof(int16_t) * ACCUMULATOR_SIZE);

    const int flip_b = 0x38;
    int king_sq_w = p->kingSq[White];
    int king_sq_b = p->kingSq[Black];

    king_sq_b ^= flip_b;

    verbose = false;

    for (int tp = Pawn; tp <= Queen; tp++) {
        BitBoard mask = p->mask[White][tp];

        while (mask) {
            int sq = bitboard_find_set_bit(mask);
            mask &= mask - 1;

            int index = 4096 * (tp - 1) + 64 * king_sq_w + sq;
            nn_update_accumulator_add(accumulator_w_q, index);

            index = 4096 * (tp + 4) + 64 * king_sq_b + (sq ^ flip_b);
            nn_update_accumulator_add(accumulator_b_q, index);
        }

        mask = p->mask[Black][tp];

        while (mask) {
            int sq = bitboard_find_set_bit(mask);
            mask &= mask - 1;

            int index = 4096 * (tp + 4) + 64 * king_sq_w + sq;
            nn_update_accumulator_add(accumulator_w_q, index);

            index = 4096 * (tp - 1) + 64 * king_sq_b + (sq ^ flip_b);
            nn_update_accumulator_add(accumulator_b_q, index);
        }
    }

    verbose = true;

    for (int i = 0; i < ACCUMULATOR_SIZE; i++) {
        if (p->accumulator_w_q[i] != accumulator_w_q[i]) {
            fprintf(stderr,
                    "i=%d accumulator_w_q[i]=%d p->accumulator_w_q[i]=%d\n", i,
                    accumulator_w_q[i], p->accumulator_w_q[i]);
            dbase_show_position(p);
            abort();
        }

        if (p->accumulator_b_q[i] != accumulator_b_q[i]) {
            fprintf(stderr,
                    "i=%d accumulator_b_q[i]=%d p->accumulator_b_q[i]=%d\n", i,
                    accumulator_b_q[i], p->accumulator_b_q[i]);
            dbase_show_position(p);
            abort();
        }
    }
}

static void nn_show_weights(void) {
#ifdef DEBUG
    print(1, "input weights:\n");
    for (int i = 0; i < 8; i++) {
        print(1, "%d, ", input_layer_weights_q[0][i]);
    }
    print(1, "\n");

    print(1, "input bias:\n");
    for (int i = 0; i < 8; i++) {
        print(1, "%d, ", input_layer_bias_q[i]);
    }
    print(1, "\n");

    print(1, "hidden_1 weights:\n");
    for (int i = 0; i < 8; i++) {
        print(1, "%d, ", hidden_layer_1_weights_q[0][i]);
    }
    print(1, "\n");

    print(1, "hidden_1 bias:\n");
    for (int i = 0; i < 8; i++) {
        print(1, "%d, ", hidden_layer_1_bias_q[i]);
    }
    print(1, "\n");

    print(1, "hidden_2 weights:\n");
    for (int i = 0; i < 8; i++) {
        print(1, "%d, ", hidden_layer_2_weights_q[0][i]);
    }
    print(1, "\n");

    print(1, "hidden_2 bias:\n");
    for (int i = 0; i < 8; i++) {
        print(1, "%d, ", hidden_layer_2_bias_q[i]);
    }
    print(1, "\n");

    print(1, "output weights:\n");
    for (int i = 0; i < 8; i++) {
        print(1, "%d, ", output_layer_weights_q[i]);
    }
    print(1, "\n");
#endif

    print(1, "output bias: %d\n", output_layer_bias_q[0]);
}

void nn_read_weights(void) {
    nn_read_weights_quantized();
    nn_show_weights();
}

void nn_randomize_weights(void) {
    for (int i = 0; i < N_FEATURES; i++) {
        for (int j = 0; j < ACCUMULATOR_SIZE; j++) {
            // int8 scale 64 counterpart of (SCALE / 16) at scale 8192
            input_layer_weights_q[i][j] = (int8_t)(random_double() * 4);
        }
    }

    for (int i = 0; i < ACCUMULATOR_SIZE; i++) {
        // int8 scale 64 counterpart of (SCALE / 16) at scale 8192
        input_layer_bias_q[i] = (int16_t)(random_double() * 4);
    }
}
