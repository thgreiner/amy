#include "bitboard.h"
#include "dbase.h"
#include "evaluation.h"
#include "hashtable.h"
#include "inline.h"
#include "utils.h"
#include <math.h>
#include <stdio.h>

// #define STATISTICS 1

float input_layer_weights[20480][256];
float input_layer_bias[256];
float hidden_layer_1_weights[32][512];
float hidden_layer_1_bias[32];
float hidden_layer_2_weights[32][32];
float hidden_layer_2_bias[32];
float output_layer_weights[32];
float output_layer_bias[1];

void ReadWeights(void) {
    FILE *fin = fopen("network.raw", "r");
    fread(input_layer_weights, sizeof(float), 256 * 20480, fin);
    fread(input_layer_bias, sizeof(float), 256, fin);
    fread(hidden_layer_1_weights, sizeof(float), 32 * 512, fin);
    fread(hidden_layer_1_bias, sizeof(float), 32, fin);
    fread(hidden_layer_2_weights, sizeof(float), 32 * 32, fin);
    fread(hidden_layer_2_bias, sizeof(float), 32, fin);
    fread(output_layer_weights, sizeof(float), 32, fin);
    fread(output_layer_bias, sizeof(float), 1, fin);
    fclose(fin);

    for (int i = 0; i < 4; i++) {
        printf("%f, ", input_layer_weights[10][i]);
    }
    printf("\n");

    Print(0, "Read weights.\n");
}

#ifdef STATISTICS
static float max_activation_input = 0.0;
static float max_activation_hidden_1 = 0.0;
static float max_activation_hidden_2 = 0.0;
static float max_activation_output = 0.0;
#endif

int EvaluatePositionNeuralNetwork(const struct Position *p) {
    int flip_w = 0;
    int flip_b = 0x38;
    int king_sq_w = p->kingSq[White];
    int king_sq_b = p->kingSq[Black];
    float accumulator_w[256], accumulator_b[256];
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

    for (int i = 0; i < 256; i++) {
        accumulator_w[i] = input_layer_bias[i];
        accumulator_b[i] = input_layer_bias[i];
    }

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

    for (int tp = Pawn; tp <= Queen; tp++) {
        BitBoard mask = p->mask[White][tp];

        while (mask) {
            int pos = FindSetBit(mask);
            mask &= mask - 1;

            int index = 2048 * (tp - 1) + 64 * king_sq_w + (pos ^ flip_w);
            // printf("True: %d\n", index);
            for (int i = 0; i < 256; i++) {
                accumulator_w[i] += input_layer_weights[index][i];
#ifdef STATISTICS
                if (fabsf(accumulator_w[i]) > max_activation_input) {
                    max_activation_input = fabsf(accumulator_w[i]);
                    print_stats = true;
                }
#endif
            }

            index = 2048 * (tp + 4) + 64 * king_sq_b + (pos ^ flip_b);
            // printf("False: %d\n", index);
            for (int i = 0; i < 256; i++) {
                accumulator_b[i] += input_layer_weights[index][i];
#ifdef STATISTICS
                if (fabsf(accumulator_b[i]) > max_activation_input) {
                    max_activation_input = fabsf(accumulator_b[i]);
                    print_stats = true;
                }
#endif
            }
        }

        mask = p->mask[Black][tp];

        while (mask) {
            int pos = FindSetBit(mask);
            mask &= mask - 1;

            int index = 2048 * (tp + 4) + 64 * king_sq_w + (pos ^ flip_w);
            // printf("True: %d\n", index);
            for (int i = 0; i < 256; i++) {
                accumulator_w[i] += input_layer_weights[index][i];
#ifdef STATISTICS
                if (fabsf(accumulator_w[i]) > max_activation_input) {
                    max_activation_input = fabsf(accumulator_w[i]);
                    print_stats = true;
                }
#endif
            }

            index = 2048 * (tp - 1) + 64 * king_sq_b + (pos ^ flip_b);
            // printf("False: %d\n", index);
            for (int i = 0; i < 256; i++) {
                accumulator_b[i] += input_layer_weights[index][i];
#ifdef STATISTICS
                if (fabsf(accumulator_b[i]) > max_activation_input) {
                    max_activation_input = fabsf(accumulator_b[i]);
                    print_stats = true;
                }
#endif
            }
        }
    }

    for (int i = 0; i < 256; i++) {
        if (accumulator_w[i] < 0.0) {
            accumulator_w[i] = 0.0;
        }
        if (accumulator_b[i] < 0.0) {
            accumulator_b[i] = 0.0;
        }
    }

#ifdef DEBUG
    for (int i = 0; i < 16; i++) {
        printf("%f, ", accumulator_w[i]);
    }
    printf("\n");
    for (int i = 0; i < 16; i++) {
        printf("%f, ", accumulator_b[i]);
    }
    printf("\n");
#endif

    float hidden_layer_1[32];

    for (int i = 0; i < 32; i++) {
        hidden_layer_1[i] = hidden_layer_1_bias[i];

        float *input = (p->turn == White) ? accumulator_w : accumulator_b;

        for (int j = 0; j < 256; j++) {
            hidden_layer_1[i] += input[j] * hidden_layer_1_weights[i][j];
#ifdef STATISTICS
            if (fabsf(hidden_layer_1[i]) > max_activation_hidden_1) {
                max_activation_hidden_1 = fabsf(hidden_layer_1[i]);
                print_stats = true;
            }
#endif
        }

        input = (p->turn == White) ? accumulator_b : accumulator_w;

        for (int j = 0; j < 256; j++) {
            hidden_layer_1[i] += input[j] * hidden_layer_1_weights[i][j + 256];
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
        printf("MaxPos: %d (Material Balance: %d, score: %d)\n", MaxPos,
               material_balance, score);
    }

    StoreST(p->hkey, score);

    return score;
}
