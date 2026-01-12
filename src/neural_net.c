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
#include "safe_malloc.h"
#include "utils.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// #define STATISTICS 1
// #define STATISTICS_Q 1
// #define DEBUG 1

#define N_FEATURES (64 * 64 * 10)

#define SCALE 16384
#define SCALE_HALF (SCALE / 2 - 1)
#define SCALE_OUTPUT 3200

int16_t input_layer_weights_q[N_FEATURES][ACCUMULATOR_SIZE];
int16_t input_layer_bias_q[ACCUMULATOR_SIZE];
int16_t hidden_layer_1_weights_q[32][2 * ACCUMULATOR_SIZE];
int32_t hidden_layer_1_bias_q[32];
int16_t hidden_layer_2_weights_q[32][32];
int32_t hidden_layer_2_bias_q[32];
int32_t output_layer_weights_q[32];
int32_t output_layer_bias_q[1];

static bool verbose = true;

void ReadWeightsQuantized(void) {
    FILE *fin = fopen("network.quant", "rb");
    size_t items_read;

    if (fin == NULL) {
        Print(0, "No weights found.\n");
        return;
    }

    items_read = fread(input_layer_weights_q, sizeof(int16_t),
                       ACCUMULATOR_SIZE * N_FEATURES, fin);
    assert(items_read == ACCUMULATOR_SIZE * N_FEATURES);

    items_read =
        fread(input_layer_bias_q, sizeof(int16_t), ACCUMULATOR_SIZE, fin);
    assert(items_read == ACCUMULATOR_SIZE);

    items_read = fread(hidden_layer_1_weights_q, sizeof(int16_t),
                       32 * 2 * ACCUMULATOR_SIZE, fin);
    assert(items_read == 32 * 2 * ACCUMULATOR_SIZE);

    items_read = fread(hidden_layer_1_bias_q, sizeof(int32_t), 32, fin);
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

    Print(0, "Read weights.\n");
}

void ReadWeightsCompressed(void) {
    FILE *fin = fopen("network.quant.z", "rb");
    if (fin == NULL) {
        Print(0, "No weights found.\n");
        return;
    }

    fseek(fin, 0L, SEEK_END);
    long file_size = ftell(fin);
    fseek(fin, 0, SEEK_SET);

    char *buffer = safe_malloc(file_size);

    size_t items_read = fread(buffer, file_size, 1, fin);
    assert(items_read == 1);
    fclose(fin);

    size_t total_network_size =
        sizeof(input_layer_weights_q) + sizeof(input_layer_bias_q) +
        sizeof(hidden_layer_1_weights_q) + sizeof(hidden_layer_1_bias_q) +
        sizeof(hidden_layer_2_weights_q) + sizeof(hidden_layer_2_bias_q) +
        sizeof(output_layer_weights_q) + sizeof(output_layer_bias_q) + 8;

    int16_t *decompressed = safe_malloc(total_network_size);

    uint8_t *src = (uint8_t *)buffer;
    uint8_t *src_end = src + file_size;

    int16_t *dest = decompressed;

    while (src < src_end) {
        uint8_t descriptor = *(src++);
        for (int i = 0; i < 8; i++) {
            // printf("descriptor: %x\n", descriptor);

            if (descriptor & 0x80) {
                // 16 bit value
                int16_t tmp = (int16_t)*src;
                tmp |= ((int16_t)*(++src)) << 8;
                *dest = tmp;
            } else {
                // 8 bit value
                int8_t *x = (int8_t *)src;
                *dest = *x;
            }
            // printf("Read %d\n", *dest);
            src++;
            dest++;
            descriptor <<= 1;
        }
        // break;
    }

    int8_t *ptr = (int8_t *)decompressed;

    assert(0 ==
           memcmp(input_layer_weights_q, ptr, sizeof(input_layer_weights_q)));
    memcpy(input_layer_weights_q, ptr, sizeof(input_layer_weights_q));
    ptr += sizeof(input_layer_weights_q);

    assert(0 == memcmp(input_layer_bias_q, ptr, sizeof(input_layer_bias_q)));
    memcpy(input_layer_bias_q, ptr, sizeof(input_layer_bias_q));
    ptr += sizeof(input_layer_bias_q);

    assert(0 == memcmp(hidden_layer_1_weights_q, ptr,
                       sizeof(hidden_layer_1_weights_q)));
    memcpy(hidden_layer_1_weights_q, ptr, sizeof(hidden_layer_1_weights_q));
    ptr += sizeof(hidden_layer_1_weights_q);

    assert(0 ==
           memcmp(hidden_layer_1_bias_q, ptr, sizeof(hidden_layer_1_bias_q)));
    memcpy(hidden_layer_1_bias_q, ptr, sizeof(hidden_layer_1_bias_q));
    ptr += sizeof(hidden_layer_1_bias_q);

    assert(0 == memcmp(hidden_layer_2_weights_q, ptr,
                       sizeof(hidden_layer_2_weights_q)));
    memcpy(hidden_layer_2_weights_q, ptr, sizeof(hidden_layer_2_weights_q));
    ptr += sizeof(hidden_layer_2_weights_q);

    assert(0 ==
           memcmp(hidden_layer_2_bias_q, ptr, sizeof(hidden_layer_2_bias_q)));
    memcpy(hidden_layer_2_bias_q, ptr, sizeof(hidden_layer_2_bias_q));
    ptr += sizeof(hidden_layer_2_bias_q);

    assert(0 ==
           memcmp(output_layer_weights_q, ptr, sizeof(output_layer_weights_q)));
    memcpy(output_layer_weights_q, ptr, sizeof(output_layer_weights_q));
    ptr += sizeof(output_layer_weights_q);

    assert(0 == memcmp(output_layer_bias_q, ptr, sizeof(output_layer_bias_q)));
    memcpy(output_layer_bias_q, ptr, sizeof(output_layer_bias_q));
    ptr += sizeof(output_layer_bias_q);

    free(decompressed);
    free(buffer);
}

#ifdef STATISTICS_Q
static int32_t max_activation_hidden_1_q = 0;
#endif

static void update_accumulator_add(int16_t *accumulator_q, int offset) {
#ifdef DEBUG_UPDATE
    if (verbose) {
        printf("update_accumulator_add(%lx, %d)\n",
               (unsigned long)accumulator_q, offset);
        printf("accumulator_q[0]=%d\n", accumulator_q[0]);
    }
#endif
    for (int i = 0; i < ACCUMULATOR_SIZE; i++) {
        accumulator_q[i] += input_layer_weights_q[offset][i];
#ifdef STATISTICS
        if (fabsf(accumulator_w[i]) > max_activation_input) {
            max_activation_input = fabsf(accumulator_w[i]);
            print_stats = true;
        }
#endif
    }
#ifdef DEBUG_UPDATE
    if (verbose) {
        printf("accumulator_q[0]=%d\n", accumulator_q[0]);
    }
#endif
}

static void update_accumulator_sub(int16_t *accumulator_q, int offset) {
#ifdef DEBUG_UPDATE
    if (verbose) {
        printf("update_accumulator_add(%lx, %d)\n",
               (unsigned long)accumulator_q, offset);
        printf("accumulator_q[0]=%d\n", accumulator_q[0]);
    }
#endif
    for (int i = 0; i < ACCUMULATOR_SIZE; i++) {
        accumulator_q[i] -= input_layer_weights_q[offset][i];
#ifdef STATISTICS
        if (fabsf(accumulator_w[i]) > max_activation_input) {
            max_activation_input = fabsf(accumulator_w[i]);
            print_stats = true;
        }
#endif
    }
#ifdef DEBUG_UPDATE
    if (verbose) {
        printf("accumulator[0]=%f\n", accumulator[0]);
    }
#endif
}

void UpdateWeightsForPiece(struct Position *p, int tp, int sq, bool turn,
                           bool add) {
#ifdef DEBUG_UPDATE
    printf("UpdateWeightsForPiece(tp=%d, sq=%d, turn=%d, add=%d)\n", tp, sq,
           turn, add);
#endif

    const int flip_b = 0x38;
    int king_sq_w = p->kingSq[White];
    int king_sq_b = p->kingSq[Black];

    tp = ABS(tp);

    void (*update_fn)(int16_t *, int) =
        add ? update_accumulator_add : update_accumulator_sub;

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

void UpdateWeightsForKing(struct Position *p) {
#ifdef DEBUG_UPDATE
    printf("UpdateWeightsForKing()\n");
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
            int sq = FindSetBit(mask);
            mask &= mask - 1;

            int index = 4096 * (tp - 1) + 64 * king_sq + (sq ^ flip);
            update_accumulator_add(accumulator_q, index);
        }

        mask = p->mask[OPP(p->turn)][tp];

        while (mask) {
            int sq = FindSetBit(mask);
            mask &= mask - 1;

            int index = 4096 * (tp + 4) + 64 * king_sq + (sq ^ flip);
            update_accumulator_add(accumulator_q, index);
        }
    }
}

int EvaluatePositionNeuralNetwork(struct Position *p) {
    int score;
#ifdef STATISTICS
    bool print_stats = false;
#endif
#ifdef STATISTICS_Q
    bool print_stats_q = false;
#endif

#ifndef DEBUG
    STry++;
    if (ProbeST(p->hkey, &score) == Useful) {
        SHit++;
        return score;
    }
#endif

#ifdef DEBUG
    for (int i = 0; i < 16; i++) {
        printf("%d, ", p->accumulator_w_q[i]);
    }
    printf("\n");
    for (int i = 0; i < 16; i++) {
        printf("%d, ", p->accumulator_b_q[i]);
    }
    printf("\n");
#endif

    int32_t hidden_layer_1_q[32];
    memcpy(hidden_layer_1_q, hidden_layer_1_bias_q, 32 * sizeof(int32_t));

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
        if (hidden_layer_1_q[i] < 0) {
            hidden_layer_1_q16[i] = 0;
        } else {
            hidden_layer_1_q16[i] =
                (int16_t)((hidden_layer_1_q[i] + SCALE_HALF) / SCALE);
        }
    }

#ifdef DEBUG
    printf("hidden_layer_1_q:\n");
    for (int i = 0; i < 8; i++) {
        printf("%d, ", hidden_layer_1_q16[i]);
    }
    printf("\n");
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
        if (hidden_layer_2_q[i] < 0) {
            hidden_layer_2_q16[i] = 0;
        } else {
            hidden_layer_2_q16[i] =
                (int16_t)((hidden_layer_2_q[i] + SCALE_HALF) / SCALE);
        }
    }

#ifdef DEBUG
    printf("hidden_layer_2_q:\n");
    for (int i = 0; i < 8; i++) {
        printf("%d, ", hidden_layer_2_q16[i]);
    }
    printf("\n");
#endif

    int32_t output_layer_q = output_layer_bias_q[0];

    for (int j = 0; j < 32; j++) {
        output_layer_q += hidden_layer_2_q16[j] * output_layer_weights_q[j];
#ifdef STATISTICS
        if (fabsf(output_layer) > max_activation_output) {
            max_activation_output = fabsf(output_layer);
            print_stats = true;
        }
#endif
#ifdef DEBUG
        printf("output_layer_q: %d\n", output_layer_q);
#endif
    }

#ifdef STATISTICS
    if (print_stats) {
        FILE *stats_file = fopen("activation.txt", "w");
        fprintf(stats_file, "max_activation_input: %f\n", max_activation_input);
        fprintf(stats_file, "max_activation_hidden_1: %f\n",
                max_activation_hidden_1);
        fprintf(stats_file, "max_activation_hidden_2: %f\n",
                max_activation_hidden_2);
        fprintf(stats_file, "max_activation_output: %f\n",
                max_activation_output);
        fclose(stats_file);
    }
#endif

#ifdef STATISTICS_Q
    if (print_stats_q) {
        FILE *stats_file = fopen("activation_q.txt", "w");
        fprintf(stats_file, "max_activation_hidden_1: %d (%f)\n",
                max_activation_hidden_1_q,
                (double)max_activation_hidden_1_q / ((double)SCALE * SCALE));
        fclose(stats_file);
    }
#endif

    score = ((output_layer_q + SCALE_HALF) / SCALE) * SCALE_OUTPUT;
    score = (score + SCALE_HALF) / SCALE;

    // Print(0, "score: %d, score_q: %d\n", score, score_q);

    int material_balance =
        (p->turn == 0) ? MaterialBalance(p) : -MaterialBalance(p);
    int pos_score = abs(material_balance - score);

    if (pos_score > MaxPos) {
        MaxPos = pos_score;

#ifdef STATISTICS
        printf("MaxPos: %d (Material Balance: %d, score: %d)\n", MaxPos,
               material_balance, score);
#endif
    }

    StoreST(p->hkey, score);

    return score;
}

void InitAccumulator(struct Position *p) {
    memcpy(p->accumulator_w_q, input_layer_bias_q,
           sizeof(int16_t) * ACCUMULATOR_SIZE);
    memcpy(p->accumulator_b_q, input_layer_bias_q,
           sizeof(int16_t) * ACCUMULATOR_SIZE);

#ifdef DEBUG
    printf("bias:\n");
    for (int i = 0; i < 8; i++) {
        printf("%d, ", p->accumulator_w_q[i]);
    }
    printf("\n");
#endif

    for (int tp = Pawn; tp <= Queen; tp++) {
        BitBoard mask = p->mask[White][tp];

        while (mask) {
            int sq = FindSetBit(mask);
            mask &= mask - 1;

            UpdateWeightsForPiece(p, tp, sq, 0, true);
        }

        mask = p->mask[Black][tp];

        while (mask) {
            int sq = FindSetBit(mask);
            mask &= mask - 1;

            UpdateWeightsForPiece(p, tp, sq, 1, true);
        }
    }
}

void ValidateWeights(struct Position *p) {
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
            int sq = FindSetBit(mask);
            mask &= mask - 1;

            int index = 4096 * (tp - 1) + 64 * king_sq_w + sq;
            update_accumulator_add(accumulator_w_q, index);

            index = 4096 * (tp + 4) + 64 * king_sq_b + (sq ^ flip_b);
            update_accumulator_add(accumulator_b_q, index);
        }

        mask = p->mask[Black][tp];

        while (mask) {
            int sq = FindSetBit(mask);
            mask &= mask - 1;

            int index = 4096 * (tp + 4) + 64 * king_sq_w + sq;
            update_accumulator_add(accumulator_w_q, index);

            index = 4096 * (tp - 1) + 64 * king_sq_b + (sq ^ flip_b);
            update_accumulator_add(accumulator_b_q, index);
        }
    }

    verbose = true;

    for (int i = 0; i < ACCUMULATOR_SIZE; i++) {
        if (p->accumulator_w_q[i] != accumulator_w_q[i]) {
            fprintf(stderr,
                    "i=%d accumulator_w_q[i]=%d p->accumulator_w_q[i]=%d\n", i,
                    accumulator_w_q[i], p->accumulator_w_q[i]);
            ShowPosition(p);
            abort();
        }

        if (p->accumulator_b_q[i] != accumulator_b_q[i]) {
            fprintf(stderr,
                    "i=%d accumulator_b_q[i]=%d p->accumulator_b_q[i]=%d\n", i,
                    accumulator_b_q[i], p->accumulator_b_q[i]);
            ShowPosition(p);
            abort();
        }
    }
}

static void ShowWeights(void) {
#ifdef DEBUG
    printf("input weights:\n");
    for (int i = 0; i < 8; i++) {
        printf("%d, ", input_layer_weights_q[0][i]);
    }
    printf("\n");

    printf("input bias:\n");
    for (int i = 0; i < 8; i++) {
        printf("%d, ", input_layer_bias_q[i]);
    }
    printf("\n");

    printf("hidden_1 weights:\n");
    for (int i = 0; i < 8; i++) {
        printf("%d, ", hidden_layer_1_weights_q[0][i]);
    }
    printf("\n");

    printf("hidden_1 bias:\n");
    for (int i = 0; i < 8; i++) {
        printf("%d, ", hidden_layer_1_bias_q[i]);
    }
    printf("\n");

    printf("hidden_2 weights:\n");
    for (int i = 0; i < 8; i++) {
        printf("%d, ", hidden_layer_2_weights_q[0][i]);
    }
    printf("\n");

    printf("hidden_2 bias:\n");
    for (int i = 0; i < 8; i++) {
        printf("%d, ", hidden_layer_2_bias_q[i]);
    }
    printf("\n");

    printf("output weights:\n");
    for (int i = 0; i < 8; i++) {
        printf("%d, ", output_layer_weights_q[i]);
    }
    printf("\n");

    printf("output bias: %d\n", *output_layer_bias_q);

#endif
}

void ReadWeights(void) {
    ReadWeightsQuantized();
    ShowWeights();
    ReadWeightsCompressed();
    ShowWeights();
}

void RandomizeWeights(void) {
    for (int i = 0; i < N_FEATURES; i++) {
        for (int j = 0; j < ACCUMULATOR_SIZE; j++) {
            input_layer_weights_q[i][j] = (int16_t)(Random() * (SCALE / 16));
        }
    }

    for (int i = 0; i < ACCUMULATOR_SIZE; i++) {
        input_layer_bias_q[i] = (int16_t)(Random() * (SCALE / 16));
    }
}
