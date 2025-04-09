#include "neural_net.h"
#include "amy.h"
#include "bitboard.h"
#include "dbase.h"
#include "evaluation.h"
#include "hashtable.h"
#include "inline.h"
#include "random.h"
#include "utils.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// #define STATISTICS 1
// #define STATISTICS_Q 1
// #define DEBUG 1

#define SCALE 16384
#define SCALE_HALF (SCALE / 2 - 1)
#define SCALE_OUTPUT 3200

float input_layer_weights[20480][ACCUMULATOR_SIZE];
float input_layer_bias[ACCUMULATOR_SIZE];
float hidden_layer_1_weights[32][2 * ACCUMULATOR_SIZE];
float hidden_layer_1_bias[32];
float hidden_layer_2_weights[32][32];
float hidden_layer_2_bias[32];
float output_layer_weights[32];
float output_layer_bias[1];

int16_t input_layer_weights_q[20480][ACCUMULATOR_SIZE];
int16_t input_layer_bias_q[ACCUMULATOR_SIZE];
int16_t hidden_layer_1_weights_q[32][2 * ACCUMULATOR_SIZE];
int32_t hidden_layer_1_bias_q[32];
int16_t hidden_layer_2_weights_q[32][32];
int32_t hidden_layer_2_bias_q[32];
int32_t output_layer_weights_q[32];
int32_t output_layer_bias_q[1];

static bool verbose = true;

static void quantize_weights(void);

void ReadWeightsQuantized(void) {
    FILE *fin = fopen("network.quant", "r");
    size_t items_read;

    if (fin == NULL) {
        Print(0, "No weights found.\n");
        return;
    }

    items_read = fread(input_layer_weights_q, sizeof(int16_t),
                       ACCUMULATOR_SIZE * 20480, fin);
    assert(items_read == ACCUMULATOR_SIZE * 20480);

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

#ifdef STATISTICS
static float max_activation_input = 0.0;
static float max_activation_hidden_1 = 0.0;
static float max_activation_hidden_2 = 0.0;
static float max_activation_output = 0.0;
#endif

#ifdef STATISTICS_Q
static int32_t max_activation_hidden_1_q = 0;
#endif

static void update_accumulator_add(float *accumulator, int16_t *accumulator_q,
                                   int offset) {
#ifdef DEBUG_UPDATE
    if (verbose) {
        printf("update_accumulator_add(%lx, %lx, %d)\n",
               (unsigned long)accumulator, (unsigned long)accumulator_q,
               offset);
        printf("accumulator_q[0]=%d\n", accumulator_q[0]);
    }
#endif
    for (int i = 0; i < ACCUMULATOR_SIZE; i++) {
#ifdef USE_FLOAT_NN
        accumulator[i] += input_layer_weights[offset][i];
#endif
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

static void update_accumulator_sub(float *accumulator, int16_t *accumulator_q,
                                   int offset) {
#ifdef DEBUG_UPDATE
    if (verbose) {
        printf("update_accumulator_add(%lx, %lx, %d)\n",
               (unsigned long)accumulator, (unsigned long)accumulator_q,
               offset);
        printf("accumulator_q[0]=%d\n", accumulator_q[0]);
    }
#endif
    for (int i = 0; i < ACCUMULATOR_SIZE; i++) {
#ifdef USE_FLOAT_NN
        accumulator[i] -= input_layer_weights[offset][i];
#endif
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

    int flip_w = 0;
    int flip_b = 0x38;
    int king_sq_w = p->kingSq[White];
    int king_sq_b = p->kingSq[Black];

    tp = ABS(tp);

    void (*update_fn)(float *, int16_t *, int) =
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
        update_fn(p->accumulator_w, p->accumulator_w_q, index);

        index = 2048 * (tp + 4) + 64 * king_sq_b + (sq ^ flip_b);
        update_fn(p->accumulator_b, p->accumulator_b_q, index);
    } else {
        int index = 2048 * (tp + 4) + 64 * king_sq_w + (sq ^ flip_w);
        update_fn(p->accumulator_w, p->accumulator_w_q, index);

        index = 2048 * (tp - 1) + 64 * king_sq_b + (sq ^ flip_b);
        update_fn(p->accumulator_b, p->accumulator_b_q, index);
    }
}

void UpdateWeightsForKing(struct Position *p) {
#ifdef DEBUG_UPDATE
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
#ifdef USE_FLOAT_NN
    memcpy(accumulator, input_layer_bias, sizeof(float) * ACCUMULATOR_SIZE);
#endif

    int16_t *accumulator_q =
        (p->turn == 0) ? p->accumulator_w_q : p->accumulator_b_q;
    memcpy(accumulator_q, input_layer_bias_q,
           sizeof(int16_t) * ACCUMULATOR_SIZE);

    for (int tp = Pawn; tp <= Queen; tp++) {
        BitBoard mask = p->mask[p->turn][tp];

        while (mask) {
            int sq = FindSetBit(mask);
            mask &= mask - 1;

            int index = 2048 * (tp - 1) + 64 * king_sq + (sq ^ flip);
            update_accumulator_add(accumulator, accumulator_q, index);
        }

        mask = p->mask[OPP(p->turn)][tp];

        while (mask) {
            int sq = FindSetBit(mask);
            mask &= mask - 1;

            int index = 2048 * (tp + 4) + 64 * king_sq + (sq ^ flip);
            update_accumulator_add(accumulator, accumulator_q, index);
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

#ifdef USE_FLOAT_NN
    float hidden_layer_1[32];
    memcpy(hidden_layer_1, hidden_layer_1_bias, 32 * sizeof(float));
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
            hidden_layer_1_q16[i] = (hidden_layer_1_q[i] + SCALE_HALF) / SCALE;
        }
    }

#ifdef DEBUG
    printf("hidden_layer_1_q:\n");
    for (int i = 0; i < 8; i++) {
        printf("%d, ", hidden_layer_1_q16[i]);
    }
    printf("\n");
#endif

#ifdef USE_FLOAT_NN
    float hidden_layer_2[32];
    memcpy(hidden_layer_2, hidden_layer_2_bias, 32 * sizeof(float));
#endif
    int32_t hidden_layer_2_q[32];
    memcpy(hidden_layer_2_q, hidden_layer_2_bias_q, 32 * sizeof(int32_t));

    for (int i = 0; i < 32; i++) {
        for (int j = 0; j < 32; j++) {
            hidden_layer_2_q[i] +=
                hidden_layer_1_q16[j] * hidden_layer_2_weights_q[i][j];
#ifdef USE_FLOAT_NN
            hidden_layer_2[i] +=
                hidden_layer_1[j] * hidden_layer_2_weights[i][j];
#ifdef STATISTICS
            if (fabsf(hidden_layer_2[i]) > max_activation_hidden_2) {
                max_activation_hidden_2 = fabsf(hidden_layer_2[i]);
                print_stats = true;
            }
#endif
#endif
        }
    }

    int16_t hidden_layer_2_q16[32];

    for (int i = 0; i < 32; i++) {
        if (hidden_layer_2_q[i] < 0) {
            hidden_layer_2_q16[i] = 0;
        } else {
            hidden_layer_2_q16[i] = (hidden_layer_2_q[i] + SCALE_HALF) / SCALE;
        }
#ifdef USE_FLOAT_NN
        if (hidden_layer_2[i] <= 0) {
            hidden_layer_2[i] = 0.0;
        }
#endif
    }

#ifdef DEBUG
    printf("hidden_layer_2_q:\n");
    for (int i = 0; i < 8; i++) {
        printf("%d, ", hidden_layer_2_q16[i]);
    }
    printf("\n");
#endif

#ifdef USE_FLOAT_NN
    float output_layer = output_layer_bias[0];
#endif
    int32_t output_layer_q = output_layer_bias_q[0];

    for (int j = 0; j < 32; j++) {
#ifdef USE_FLOAT_NN
        output_layer += hidden_layer_2[j] * output_layer_weights[j];
#endif
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

#ifdef USE_FLOAT_NN
    score = output_layer * SCALE_OUTPUT;
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
    memcpy(p->accumulator_w, input_layer_bias,
           sizeof(float) * ACCUMULATOR_SIZE);
    memcpy(p->accumulator_b, input_layer_bias,
           sizeof(float) * ACCUMULATOR_SIZE);

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

static int max_delta = 0;

void ValidateWeights(struct Position *p) {
    float accumulator_w[ACCUMULATOR_SIZE];
    float accumulator_b[ACCUMULATOR_SIZE];

#ifdef USE_FLOAT_NN
    memcpy(accumulator_w, input_layer_bias, sizeof(float) * ACCUMULATOR_SIZE);
    memcpy(accumulator_b, input_layer_bias, sizeof(float) * ACCUMULATOR_SIZE);
#endif
    int16_t accumulator_w_q[ACCUMULATOR_SIZE];
    int16_t accumulator_b_q[ACCUMULATOR_SIZE];

    memcpy(accumulator_w_q, input_layer_bias_q,
           sizeof(int16_t) * ACCUMULATOR_SIZE);
    memcpy(accumulator_b_q, input_layer_bias_q,
           sizeof(int16_t) * ACCUMULATOR_SIZE);

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
            update_accumulator_add(accumulator_w, accumulator_w_q, index);

            index = 2048 * (tp + 4) + 64 * king_sq_b + (sq ^ flip_b);
            update_accumulator_add(accumulator_b, accumulator_b_q, index);
        }

        mask = p->mask[Black][tp];

        while (mask) {
            int sq = FindSetBit(mask);
            mask &= mask - 1;

            int index = 2048 * (tp + 4) + 64 * king_sq_w + (sq ^ flip_w);
            update_accumulator_add(accumulator_w, accumulator_w_q, index);

            index = 2048 * (tp - 1) + 64 * king_sq_b + (sq ^ flip_b);
            update_accumulator_add(accumulator_b, accumulator_b_q, index);
        }
    }

    verbose = true;

    for (int i = 0; i < ACCUMULATOR_SIZE; i++) {
#ifdef USE_FLOAT_NN
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
#endif
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

#ifdef USE_FLOAT_NN
        int delta =
            ((int)(p->accumulator_w[i] * SCALE)) - p->accumulator_w_q[i];
        if (ABS(delta) > max_delta) {
            fprintf(stderr,
                    "i=%d delta=%d accumulator_w_q[i]=%d accumulator_w[i]=%d\n",
                    i, delta, p->accumulator_w_q[i],
                    (int)(p->accumulator_w[i] * SCALE));
            max_delta = ABS(delta);
            // ShowPosition(p);
            // abort();
        }

        delta = ((int)(p->accumulator_b[i] * SCALE)) - p->accumulator_b_q[i];
        if (ABS(delta) > max_delta) {
            fprintf(stderr,
                    "i=%d delta=%d accumulator_b_q[i]=%d accumulator_b[i]=%d\n",
                    i, delta, p->accumulator_b_q[i],
                    (int)(p->accumulator_b[i] * SCALE));
            max_delta = ABS(delta);
            // ShowPosition(p);
            // abort();
        }
#endif
    }
}

float input_layer_weights[20480][ACCUMULATOR_SIZE];

void quantize_weights(void) {
    const int q1 = SCALE;

    int max_w = 0, min_w = 0;
    for (int i = 0; i < 20480; i++) {
        for (int j = 0; j < ACCUMULATOR_SIZE; j++) {
            float value = round(input_layer_weights[i][j] * q1);
            assert(fabsf(value) < SCALE);

            input_layer_weights_q[i][j] = (int16_t)value;

            if (input_layer_weights_q[i][j] > max_w) {
                max_w = input_layer_weights_q[i][j];
            }

            if (input_layer_weights_q[i][j] < min_w) {
                min_w = input_layer_weights_q[i][j];
            }
        }
    }

    Print(0, "input_layer_weights_q: %d < x < %d\n", min_w, max_w);

    max_w = 0;
    min_w = 0;

    for (int i = 0; i < ACCUMULATOR_SIZE; i++) {
        input_layer_bias_q[i] = (int16_t)round(input_layer_bias[i] * q1);

        if (input_layer_bias_q[i] > max_w) {
            max_w = input_layer_bias_q[i];
        }

        if (input_layer_bias_q[i] < min_w) {
            min_w = input_layer_bias_q[i];
        }
    }

    Print(0, "input_layer_bias_q: %d < x < %d\n", min_w, max_w);

    max_w = 0;
    min_w = 0;

    for (int i = 0; i < 32; i++) {
        for (int j = 0; j < 2 * ACCUMULATOR_SIZE; j++) {
            hidden_layer_1_weights_q[i][j] =
                (int16_t)round(hidden_layer_1_weights[i][j] * q1);

            if (hidden_layer_1_weights_q[i][j] > max_w) {
                max_w = hidden_layer_1_weights_q[i][j];
            }

            if (hidden_layer_1_weights_q[i][j] < min_w) {
                min_w = hidden_layer_1_weights_q[i][j];
            }
        }
    }

    Print(0, "hidden_layer_1_weights_q: %d < x < %d\n", min_w, max_w);

    max_w = 0;
    min_w = 0;

    for (int i = 0; i < 32; i++) {
        hidden_layer_1_bias_q[i] =
            (int32_t)round(hidden_layer_1_bias[i] * q1 * q1);

        if (hidden_layer_1_bias_q[i] > max_w) {
            max_w = hidden_layer_1_bias_q[i];
        }

        if (hidden_layer_1_bias_q[i] < min_w) {
            min_w = hidden_layer_1_bias_q[i];
        }
    }

    Print(0, "hidden_layer_1_bias_q: %d < x < %d\n", min_w, max_w);

    max_w = 0;
    min_w = 0;

    for (int i = 0; i < 32; i++) {
        for (int j = 0; j < 32; j++) {
            hidden_layer_2_weights_q[i][j] =
                (int16_t)round(hidden_layer_2_weights[i][j] * q1);

            if (hidden_layer_2_weights_q[i][j] > max_w) {
                max_w = hidden_layer_2_weights_q[i][j];
            }

            if (hidden_layer_2_weights_q[i][j] < min_w) {
                min_w = hidden_layer_2_weights_q[i][j];
            }
        }
    }

    Print(0, "hidden_layer_2_weights_q: %d < x < %d\n", min_w, max_w);

    max_w = 0;
    min_w = 0;

    for (int i = 0; i < 32; i++) {
        hidden_layer_2_bias_q[i] =
            (int32_t)round(hidden_layer_2_bias[i] * q1 * q1);

        if (hidden_layer_2_bias_q[i] > max_w) {
            max_w = hidden_layer_2_bias_q[i];
        }

        if (hidden_layer_2_bias_q[i] < min_w) {
            min_w = hidden_layer_2_bias_q[i];
        }
    }

    Print(0, "hidden_layer_2_bias_q: %d < x < %d\n", min_w, max_w);

    max_w = 0;
    min_w = 0;

    for (int i = 0; i < 32; i++) {
        output_layer_weights_q[i] =
            (int32_t)round(output_layer_weights[i] * q1);

        if (output_layer_weights_q[i] > max_w) {
            max_w = output_layer_weights_q[i];
        }

        if (output_layer_weights_q[i] < min_w) {
            min_w = output_layer_weights_q[i];
        }
    }

    Print(0, "output_layer_weights_q: %d < x < %d\n", min_w, max_w);

    output_layer_bias_q[0] = (int32_t)round(output_layer_bias[0] * q1 * q1);

    Print(0, "output_layer_bias_q: %d\n", output_layer_bias_q[0]);
}

void ReadWeightsFloat(void) {
    FILE *fin = fopen("network.raw", "r");
    size_t items_read;

    if (fin == NULL) {
        Print(0, "No weights found.\n");
        return;
    }

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

    quantize_weights();
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
    // ReadWeightsFloat();
    // ShowWeights();
}

void RandomizeWeights(void) {
    for (int i = 0; i < 20480; i++) {
        for (int j = 0; j < ACCUMULATOR_SIZE; j++) {
            input_layer_weights_q[i][j] = Random() * (SCALE / 16);
        }
    }

    for (int i = 0; i < ACCUMULATOR_SIZE; i++) {
        input_layer_bias_q[i] = Random() * (SCALE / 16);
    }
}
