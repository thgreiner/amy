/* Cross-check: load network.quant, build positions from EPDs, and dump
 * amy's own accumulators and NNUE score so the Python implementation
 * (amy-training-nnue/quantize.py QuantizedNet) can be verified bit-exact.
 *
 * Also dumps the intermediate int32 layer values ("I" lines) of a
 * copy of the integer pipeline to debug rounding/overflow issues.
 */
#include <stdio.h>
#include <string.h>

#include "dbase.h"
#include "hashtable.h"
#include "init.h"
#include "movedata.h"
#include "neural_net.h"
#include "utils.h"

extern int8_t input_layer_weights_q[];
extern int8_t hidden_layer_1_weights_q[HIDDEN_1_SIZE][2 * ACCUMULATOR_SIZE];
extern int16_t hidden_layer_1_bias_q[HIDDEN_1_SIZE];
extern int16_t hidden_layer_2_weights_q[HIDDEN_2_SIZE][HIDDEN_1_SIZE];
extern int32_t hidden_layer_2_bias_q[HIDDEN_2_SIZE];
extern int32_t output_layer_weights_q[HIDDEN_2_SIZE];
extern int32_t output_layer_bias_q[1];

#define SCALE 8192
#define SCALE_HALF 4095
#define SCALE_INPUT 64
#define SCALE_INPUT_HALF (SCALE_INPUT / 2 - 1)
#define SCALE_OUTPUT 10000

/* A copy of the integer pipeline of nn_evaluate_position() that
 * prints the intermediate values. */
static int instrumented_eval(struct Position *p) {
    int32_t h1q[HIDDEN_1_SIZE];
    for (int i = 0; i < HIDDEN_1_SIZE; i++) {
        h1q[i] = hidden_layer_1_bias_q[i];
    }
    int16_t acc[2 * ACCUMULATOR_SIZE];
    if (p->turn == White) {
        memcpy(acc, p->accumulator_w_q, ACCUMULATOR_SIZE * sizeof(int16_t));
        memcpy(acc + ACCUMULATOR_SIZE, p->accumulator_b_q,
               ACCUMULATOR_SIZE * sizeof(int16_t));
    } else {
        memcpy(acc, p->accumulator_b_q, ACCUMULATOR_SIZE * sizeof(int16_t));
        memcpy(acc + ACCUMULATOR_SIZE, p->accumulator_w_q,
               ACCUMULATOR_SIZE * sizeof(int16_t));
    }
    for (int i = 0; i < 2 * ACCUMULATOR_SIZE; i++) {
        if (acc[i] < 0)
            acc[i] = 0;
        if (acc[i] > SCALE_INPUT)
            acc[i] = SCALE_INPUT;
    }
    for (int i = 0; i < HIDDEN_1_SIZE; i++) {
        for (int j = 0; j < 2 * ACCUMULATOR_SIZE; j++) {
            h1q[i] += acc[j] * hidden_layer_1_weights_q[i][j];
        }
        printf("I H1Q %d %d\n", i, (int)h1q[i]);
    }
    int16_t h1[HIDDEN_1_SIZE];
    for (int i = 0; i < HIDDEN_1_SIZE; i++) {
        if (h1q[i] <= 0)
            h1[i] = 0;
        else if (h1q[i] > SCALE_INPUT * SCALE_INPUT)
            h1[i] = SCALE_INPUT;
        else
            h1[i] = (h1q[i] + SCALE_INPUT_HALF) / SCALE_INPUT;
        printf("I H1 %d %d\n", i, (int)h1[i]);
    }
    int32_t h2q[HIDDEN_2_SIZE];
    memcpy(h2q, hidden_layer_2_bias_q, HIDDEN_2_SIZE * sizeof(int32_t));
    for (int i = 0; i < HIDDEN_2_SIZE; i++) {
        for (int j = 0; j < HIDDEN_1_SIZE; j++) {
            h2q[i] += h1[j] * hidden_layer_2_weights_q[i][j];
        }
        printf("I H2Q %d %d\n", i, (int)h2q[i]);
    }
    int16_t h2[HIDDEN_2_SIZE];
    for (int i = 0; i < HIDDEN_2_SIZE; i++) {
        if (h2q[i] <= 0)
            h2[i] = 0;
        else if (h2q[i] >= SCALE_INPUT * SCALE)
            h2[i] = SCALE;
        else
            h2[i] = (h2q[i] + SCALE_INPUT_HALF) / SCALE_INPUT;
        printf("I H2 %d %d\n", i, (int)h2[i]);
    }
    int32_t out_q = output_layer_bias_q[0];
    for (int j = 0; j < HIDDEN_2_SIZE; j++) {
        out_q += h2[j] * output_layer_weights_q[j];
    }
    printf("I OUTQ %d\n", (int)out_q);
    int score = ((out_q + SCALE_HALF) / SCALE) * SCALE_OUTPUT;
    score = (score + SCALE_HALF) / SCALE;
    return score;
}

int main(void) {
    utils_open_log_file("Amy.log");
    movedata_init();
    init_all();
    ht_init();
    ht_allocate();
    nn_read_weights();

    const char *epds[] = {
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -",
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR b KQkq -",
        "r1bqkb1r/pppp1ppp/2n2n2/1B2p3/4P3/5N2/PPPP1PPP/RNBQK2R w KQkq -",
        "3r1k2/p3qppP/8/3p4/3r4/5QP1/P4PB1/4R1K1 w - -",
        "rnbqkbnr/1pp1pppp/p7/3pP3/8/8/PPPP1PPP/RNBQKBNR w KQkq d6",
        /* Both kings on the left half (non-mirrored frame): the
         * horizontal mirror of the previous EPD. */
        "2k1r3/Pppq3p/8/4p3/4r3/1PQ5/1BP4P/1K1R4 w - -",
    };

    for (int i = 0; i < 6; i++) {
        struct Position *p = dbase_create_position_from_epd((char *)epds[i]);
        nn_init_accumulator(p);
        nn_validate_weights(p);
        printf("POS %d turn %d\n", i, p->turn);
        for (int k = 0; k < ACCUMULATOR_SIZE; k++) {
            printf("%d", p->accumulator_w_q[k]);
            if (k < ACCUMULATOR_SIZE - 1)
                fputc(' ', stdout);
        }
        printf("\n");
        for (int k = 0; k < ACCUMULATOR_SIZE; k++) {
            printf("%d", p->accumulator_b_q[k]);
            if (k < ACCUMULATOR_SIZE - 1)
                fputc(' ', stdout);
        }
        printf("\n");
        printf("SCORE %d\n", nn_evaluate_position(p));
        printf("SCOREI %d\n", instrumented_eval(p));
        dbase_free_position(p);
    }
    return 0;
}
