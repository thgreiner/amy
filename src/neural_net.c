#include "neural_net.h"
#include "amy.h"
#include "bitboard.h"
#include "dbase.h"
#include "evaluation.h"
#include "hashtable.h"
#include "inline.h"
#include "utils.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

// #define STATISTICS 1
// #define DEBUG 1

float input_layer_weights[20480][ACCUMULATOR_SIZE];
float input_layer_bias[ACCUMULATOR_SIZE];
float hidden_layer_1_weights[32][2 * ACCUMULATOR_SIZE];
float hidden_layer_1_bias[32];
float hidden_layer_2_weights[32][32];
float hidden_layer_2_bias[32];
float output_layer_weights[32];
float output_layer_bias[1];

static bool verbose = true;

void ReadWeights(void) {
    FILE *fin = fopen("network.raw", "r");
    size_t items_read;

    items_read = fread(input_layer_weights, sizeof(float),
                       ACCUMULATOR_SIZE * 20480, fin);
    assert(items_read == ACCUMULATOR_SIZE * 20480);

    items_read = fread(input_layer_bias, sizeof(float), ACCUMULATOR_SIZE, fin);
    assert(items_read == ACCUMULATOR_SIZE);

    items_read = fread(hidden_layer_1_weights, sizeof(float),
                       32 * 2 * ACCUMULATOR_SIZE, fin);
    assert(items_read == 32 * 2 * ACCUMULATOR_SIZE);

    items_read = fread(hidden_layer_1_bias, sizeof(float), 32, fin);
    assert(items_read == 32);

    items_read = fread(hidden_layer_2_weights, sizeof(float), 32 * 32, fin);
    assert(items_read == 32 * 32);

    items_read = fread(hidden_layer_2_bias, sizeof(float), 32, fin);
    assert(items_read == 32);

    items_read = fread(output_layer_weights, sizeof(float), 32, fin);
    assert(items_read == 32);

    items_read = fread(output_layer_bias, sizeof(float), 1, fin);
    assert(items_read == 1);

    fclose(fin);

#ifdef DEBUG
    printf("weights:\n");
    for (int i = 0; i < 8; i++) {
        printf("%f, ", input_layer_weights[0][i]);
    }
    printf("\n");

    printf("bias:\n");
    for (int i = 0; i < 8; i++) {
        printf("%f, ", input_layer_bias[i]);
    }
    printf("\n");
#endif

    Print(0, "Read weights.\n");
}

#ifdef STATISTICS
static float max_activation_input = 0.0;
static float max_activation_hidden_1 = 0.0;
static float max_activation_hidden_2 = 0.0;
static float max_activation_output = 0.0;
#endif

static void update_accumulator_add(float *accumulator, int offset) {
#ifdef DEBUG
    if (verbose) {
        printf("update_accumulator_add(%lx, %d)\n", (unsigned long)accumulator,
               offset);
        printf("accumulator[0]=%f\n", accumulator[0]);
    }
#endif
    for (int i = 0; i < ACCUMULATOR_SIZE; i++) {
        accumulator[i] += input_layer_weights[offset][i];
#ifdef STATISTICS
        if (fabsf(accumulator_w[i]) > max_activation_input) {
            max_activation_input = fabsf(accumulator_w[i]);
            print_stats = true;
        }
#endif
    }
#ifdef DEBUG
    if (verbose) {
        printf("accumulator[0]=%f\n", accumulator[0]);
    }
#endif
}

static void update_accumulator_sub(float *accumulator, int offset) {
#ifdef DEBUG
    if (verbose) {
        printf("update_accumulator_sub(%lx, %d)\n", (unsigned long)accumulator,
               offset);
        printf("accumulator[0]=%f\n", accumulator[0]);
    }
#endif
    for (int i = 0; i < ACCUMULATOR_SIZE; i++) {
        accumulator[i] -= input_layer_weights[offset][i];
#ifdef STATISTICS
        if (fabsf(accumulator_w[i]) > max_activation_input) {
            max_activation_input = fabsf(accumulator_w[i]);
            print_stats = true;
        }
#endif
    }
#ifdef DEBUG
    if (verbose) {
        printf("accumulator[0]=%f\n", accumulator[0]);
    }
#endif
}

void UpdateWeightsForPiece(struct Position *p, int tp, int sq, bool turn,
                           bool add) {
#ifdef DEBUG
    printf("UpdateWeightsForPiece(tp=%d, sq=%d, turn=%d, add=%d)\n", tp, sq,
           turn, add);
#endif

    int flip_w = 0;
    int flip_b = 0x38;
    int king_sq_w = p->kingSq[White];
    int king_sq_b = p->kingSq[Black];

    tp = ABS(tp);

    void (*update_fn)(float *, int) =
        add ? update_accumulator_add : update_accumulator_sub;

    if ((king_sq_w & 7) >= 4) {
        flip_w = 7;
    }

    if ((king_sq_b & 7) >= 4) {
        flip_b |= 7;
    }

    king_sq_w ^= flip_w;
    king_sq_w = (king_sq_w & 3) + 4 * (king_sq_w >> 3);

    king_sq_b ^= flip_b;
    king_sq_b = (king_sq_b & 3) + 4 * (king_sq_b >> 3);

    if (turn == 0) {
        int index = 2048 * (tp - 1) + 64 * king_sq_w + (sq ^ flip_w);
        update_fn(p->accumulator_w, index);

        index = 2048 * (tp + 4) + 64 * king_sq_b + (sq ^ flip_b);
        update_fn(p->accumulator_b, index);
    } else {
        int index = 2048 * (tp + 4) + 64 * king_sq_w + (sq ^ flip_w);
        update_fn(p->accumulator_w, index);

        index = 2048 * (tp - 1) + 64 * king_sq_b + (sq ^ flip_b);
        update_fn(p->accumulator_b, index);
    }
}

void UpdateWeightsForKing(struct Position *p) {
#ifdef DEBUG
    printf("UpdateWeightsForKing()\n");
#endif

    int flip = (p->turn == 0) ? 0 : 0x38;
    int king_sq = p->kingSq[p->turn];

    if ((king_sq & 7) >= 4) {
        flip |= 7;
    }

    king_sq ^= flip;
    king_sq = (king_sq & 3) + 4 * (king_sq >> 3);

    float *accumulator = (p->turn == 0) ? p->accumulator_w : p->accumulator_b;
    memcpy(accumulator, input_layer_bias, sizeof(float) * ACCUMULATOR_SIZE);

    for (int tp = Pawn; tp <= Queen; tp++) {
        BitBoard mask = p->mask[p->turn][tp];

        while (mask) {
            int sq = FindSetBit(mask);
            mask &= mask - 1;

            int index = 2048 * (tp - 1) + 64 * king_sq + (sq ^ flip);
            update_accumulator_add(accumulator, index);
        }

        mask = p->mask[OPP(p->turn)][tp];

        while (mask) {
            int sq = FindSetBit(mask);
            mask &= mask - 1;

            int index = 2048 * (tp + 4) + 64 * king_sq + (sq ^ flip);
            update_accumulator_add(accumulator, index);
        }
    }
}

int EvaluatePositionNeuralNetwork(struct Position *p) {
    int score;
#ifdef STATISTICS
    bool print_stats = false;
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
        printf("%f, ", p->accumulator_w[i]);
    }
    printf("\n");
    for (int i = 0; i < 16; i++) {
        printf("%f, ", p->accumulator_b[i]);
    }
    printf("\n");
#endif

    float hidden_layer_1[32];

    for (int i = 0; i < 32; i++) {
        hidden_layer_1[i] = hidden_layer_1_bias[i];

        float *input = (p->turn == White) ? p->accumulator_w : p->accumulator_b;

        for (int j = 0; j < ACCUMULATOR_SIZE; j++) {
            if (input[j] > 0.0) {
                hidden_layer_1[i] += input[j] * hidden_layer_1_weights[i][j];
            }
#ifdef STATISTICS
            if (fabsf(hidden_layer_1[i]) > max_activation_hidden_1) {
                max_activation_hidden_1 = fabsf(hidden_layer_1[i]);
                print_stats = true;
            }
#endif
        }

        input = (p->turn == White) ? p->accumulator_b : p->accumulator_w;

        for (int j = 0; j < ACCUMULATOR_SIZE; j++) {
            if (input[j] > 0.0) {
                hidden_layer_1[i] +=
                    input[j] * hidden_layer_1_weights[i][j + ACCUMULATOR_SIZE];
            }
#ifdef STATISTICS
            if (fabsf(hidden_layer_1[i]) > max_activation_hidden_1) {
                max_activation_hidden_1 = fabsf(hidden_layer_1[i]);
                print_stats = true;
            }
#endif
        }
    }

    for (int i = 0; i < 32; i++) {
        if (hidden_layer_1[i] <= 0.0) {
            hidden_layer_1[i] = 0.0;
        }
    }

#ifdef DEBUG
    for (int i = 0; i < 8; i++) {
        printf("%f, ", hidden_layer_1[i]);
    }
    printf("\n");
#endif

    float hidden_layer_2[32];

    for (int i = 0; i < 32; i++) {
        hidden_layer_2[i] = hidden_layer_2_bias[i];
        for (int j = 0; j < 32; j++) {
            hidden_layer_2[i] +=
                hidden_layer_1[j] * hidden_layer_2_weights[i][j];
#ifdef STATISTICS
            if (fabsf(hidden_layer_2[i]) > max_activation_hidden_2) {
                max_activation_hidden_2 = fabsf(hidden_layer_2[i]);
                print_stats = true;
            }
#endif
        }
    }

    for (int i = 0; i < 32; i++) {
        if (hidden_layer_2[i] <= 0) {
            hidden_layer_2[i] = 0.0;
        }
    }

    float output_layer = output_layer_bias[0];
    for (int j = 0; j < 32; j++) {
        output_layer += hidden_layer_2[j] * output_layer_weights[j];
#ifdef STATISTICS
        if (fabsf(output_layer) > max_activation_output) {
            max_activation_output = fabsf(output_layer);
            print_stats = true;
        }
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
    score = output_layer * 3200;

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
    memcpy(p->accumulator_w, input_layer_bias,
           sizeof(float) * ACCUMULATOR_SIZE);
    memcpy(p->accumulator_b, input_layer_bias,
           sizeof(float) * ACCUMULATOR_SIZE);

#ifdef DEBUG
    printf("bias:\n");
    for (int i = 0; i < 8; i++) {
        printf("%f, ", p->accumulator_w[i]);
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
    float accumulator_w[ACCUMULATOR_SIZE];
    float accumulator_b[ACCUMULATOR_SIZE];

    memcpy(accumulator_w, input_layer_bias, sizeof(float) * ACCUMULATOR_SIZE);
    memcpy(accumulator_b, input_layer_bias, sizeof(float) * ACCUMULATOR_SIZE);

    int flip_w = 0;
    int flip_b = 0x38;
    int king_sq_w = p->kingSq[White];
    int king_sq_b = p->kingSq[Black];

    if ((king_sq_w & 7) >= 4) {
        flip_w = 7;
    }

    if ((king_sq_b & 7) >= 4) {
        flip_b |= 7;
    }

    king_sq_w ^= flip_w;
    king_sq_w = (king_sq_w & 3) + 4 * (king_sq_w >> 3);

    king_sq_b ^= flip_b;
    king_sq_b = (king_sq_b & 3) + 4 * (king_sq_b >> 3);

    verbose = false;

    for (int tp = Pawn; tp <= Queen; tp++) {
        BitBoard mask = p->mask[White][tp];

        while (mask) {
            int sq = FindSetBit(mask);
            mask &= mask - 1;

            int index = 2048 * (tp - 1) + 64 * king_sq_w + (sq ^ flip_w);
            update_accumulator_add(accumulator_w, index);

            index = 2048 * (tp + 4) + 64 * king_sq_b + (sq ^ flip_b);
            update_accumulator_add(accumulator_b, index);
        }

        mask = p->mask[Black][tp];

        while (mask) {
            int sq = FindSetBit(mask);
            mask &= mask - 1;

            int index = 2048 * (tp + 4) + 64 * king_sq_w + (sq ^ flip_w);
            update_accumulator_add(accumulator_w, index);

            index = 2048 * (tp - 1) + 64 * king_sq_b + (sq ^ flip_b);
            update_accumulator_add(accumulator_b, index);
        }
    }

    verbose = true;

    for (int i = 0; i < ACCUMULATOR_SIZE; i++) {
        if (fabsf(accumulator_w[i] - p->accumulator_w[i]) > 1e-3) {
            fprintf(stderr, "i=%d accumulator_w[i]=%f p->accumulator_w[i]=%f\n",
                    i, accumulator_w[i], p->accumulator_w[i]);
            ShowPosition(p);
            abort();
        }

        if (fabsf(accumulator_b[i] - p->accumulator_b[i]) > 1e-3) {
            fprintf(stderr, "i=%d accumulator_b[i]=%f p->accumulator_b[i]=%f\n",
                    i, accumulator_b[i], p->accumulator_b[i]);
            ShowPosition(p);
            abort();
        }
    }
}
