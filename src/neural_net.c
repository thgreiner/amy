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
#include "attack_map.h"
#include "bitboard.h"
#include "dbase.h"
#include "hashtable.h"
#include "inline.h"
#include "intset_sorted.h"
#include "lzav.h"
#include "random.h"
#include "safe_malloc.h"
#include "utils.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// #define STATISTICS_Q 1
// #define DEBUG 1

/*
 * Type definitions
 */

typedef struct {
    uint32_t wh_magic;                // should always be wh_magic
    uint16_t wh_accumulator_size;     // Size of the accumulator
    uint16_t wh_num_features;         // Number of input features (planes)
    uint16_t wh_pawn_features_size;   // Number of pawn input features
    uint32_t wh_attack_features_size; // Number of attack input features
                                      // (u32: N_ATTACK_FEATURES doesn't fit
                                      // in a uint16_t; the compiler inserts
                                      // 2 bytes of alignment padding before
                                      // it)
    uint16_t wh_hidden_layer_1_size;  // Size of the hidden layer 1
    uint16_t wh_hidden_layer_2_size;  // Size of the hidden layer 2
    uint8_t wh_accumulator_weights_quantization;
    uint8_t wh_accumulator_bias_quantization;
    uint8_t wh_hidden_layer_1_weights_quantization;
    uint8_t wh_hidden_layer_1_bias_quantization;
    uint8_t wh_hidden_layer_2_weights_quantization;
    uint8_t wh_hidden_layer_2_bias_quantization;
    uint8_t wh_output_layer_weights_quantization;
    uint8_t wh_output_layer_bias_quantization;
    uint16_t wh_phase_count; // Number of game phase branches
    char wh_name[32];        // Name for the weights
    char wh_date[32];        // Date of training, ISO8601 formatted
} WeightsHeader;

typedef struct {
    WeightsHeader cwh_header;
    int32_t cwh_compressed_size;
} CompressedWeightsHeader;

/*
 * Global and static variables
 */

#define NET_NAME "nets/amy.net"
#define NET_NAME_COMPRESSED "nets/amy.net.z"

/*
 * Indexing schema for features:
 *   - King position mirrored to the left half (a1..d1 up to a8..d8)
 *   - Piece square (flipped and mirrored depending on king position)
 *   - Piece type (0..4 own pawn..queen, 5..9 opponent pawn..queen,
 *     10 opponent king)
 */

#define SCALE_INPUT 64
#define SCALE_INPUT_HALF (SCALE_INPUT / 2 - 1)
#define SCALE 8192
#define SCALE_HALF (SCALE / 2 - 1)
#define SCALE_OUTPUT 10000

/* Hidden-1 fixed-point scale: acc @ SCALE_INPUT x int16 weights @ SCALE,
 * and the int32 hidden-1 bias. */
#define SCALE_H1 (SCALE_INPUT * SCALE)
#define SCALE_H1_HALF (SCALE_H1 / 2 - 1)

bool fuzz_evaluation = false;

/* MaxPos is the maximum difference between the material balance and
 * the evaluation. It is used to form a cheap optimistic score estimate
 * (material + MaxPos) which can replace the expensive full evaluation.
 * The initial value (two pawns in millipawns) is re-set before every
 * search by nn_eval_init(). */
static const int MaxPosInit = 2000;
int MaxPos = MaxPosInit;

static bool verbose = true;

static const uint32_t wh_magic = 0x57594d41;            // "AMYW"
static const uint32_t wh_magic_compressed = 0x5A594d41; // "AMYZ"

static const uint8_t wh_quantization_int_8 = 1;
static const uint8_t wh_quantization_int_16 = 2;
static const uint8_t wh_quantization_int_32 = 3;

static WeightsHeader nn_header;

int8_t input_layer_weights_q[N_FEATURES][ACCUMULATOR_SIZE];
int8_t input_pawn_weights_q[N_PAWN_FEATURES][ACCUMULATOR_SIZE];
int8_t input_attack_weights_q[N_ATTACK_FEATURES][ACCUMULATOR_SIZE];
int16_t input_layer_bias_q[ACCUMULATOR_SIZE];
int16_t hidden_layer_1_weights_q[NN_PHASE_COUNT][HIDDEN_1_SIZE]
                                [2 * ACCUMULATOR_SIZE];
int32_t hidden_layer_1_bias_q[NN_PHASE_COUNT][HIDDEN_1_SIZE];
int16_t hidden_layer_2_weights_q[NN_PHASE_COUNT][HIDDEN_2_SIZE][HIDDEN_1_SIZE];
int32_t hidden_layer_2_bias_q[NN_PHASE_COUNT][HIDDEN_2_SIZE];
int32_t output_layer_weights_q[NN_PHASE_COUNT][HIDDEN_2_SIZE];
int32_t output_layer_bias_q[NN_PHASE_COUNT][1];

static BitBoard right_of_file_mask[] = {
    0x0303030303030303ULL, 0x0707070707070707ULL, 0x0e0e0e0e0e0e0e0eULL,
    0x1c1c1c1c1c1c1c1cULL, 0x3838383838383838ULL, 0x7070707070707070ULL,
    0xe0e0e0e0e0e0e0e0ULL, 0xc0c0c0c0c0c0c0c0ULL,
};

#ifdef STATISTICS_Q
static int32_t max_activation_hidden_1_q = 0;
#endif

static int nn_uncompressed_length =
    (int)(sizeof(input_layer_weights_q) + sizeof(input_pawn_weights_q) +
          sizeof(input_attack_weights_q) + sizeof(input_layer_bias_q) +
          sizeof(hidden_layer_1_weights_q) + sizeof(hidden_layer_1_bias_q) +
          sizeof(hidden_layer_2_weights_q) + sizeof(hidden_layer_2_bias_q) +
          sizeof(output_layer_weights_q) + sizeof(output_layer_bias_q));

/*
 * Forward declarations
 */

static void nn_update_weights_for_pawn(Position *p, int sq1, bool turn,
                                       bool add);
static void nn_build_pawn_accumulator(int16_t *accumulator, const Position *p,
                                      int side);
static void nn_save_weights_compressed(const char *);
static void nn_check_header(const WeightsHeader *header);
static bool nn_load_weights_compressed(const char *);
static bool nn_load_weights_uncompressed(const char *);

/*
 * Functions
 */

/**
 * Do the pre-search initialization of evaluation.
 */
void nn_eval_init(void) { MaxPos = MaxPosInit; }

/**
 * Evaluate the material balance (from white's perspective).
 */
int eval_material_balance(const Position *p) {
    return p->material[White] - p->material[Black];
}

void nn_read_weights_quantized(void) {
    bool weights_loaded =
        nn_load_weights_compressed(NET_NAME_COMPRESSED) || nn_load_weights_uncompressed(NET_NAME);

    if (weights_loaded) {
        print(0, "Net is \"%s (%s)\".\n\n", nn_header.wh_name,
              nn_header.wh_date);
    } else {
        print(0, "No net found.\n\n");
    }
}

static bool nn_load_weights_uncompressed(const char *file_name) {
    FILE *fin = fopen(file_name, "rb");
    size_t items_read;

    if (fin == NULL) {
        return false;
    }

    WeightsHeader header;

    items_read = fread(&header, sizeof(header), 1, fin);
    assert(items_read == 1);

    nn_check_header(&header);
    nn_header = header;

    items_read = fread(input_layer_weights_q, sizeof(int8_t),
                       ACCUMULATOR_SIZE * N_FEATURES, fin);
    assert(items_read == ACCUMULATOR_SIZE * N_FEATURES);

    items_read = fread(input_pawn_weights_q, sizeof(int8_t),
                       ACCUMULATOR_SIZE * N_PAWN_FEATURES, fin);
    assert(items_read == ACCUMULATOR_SIZE * N_PAWN_FEATURES);

    items_read = fread(input_attack_weights_q, sizeof(int8_t),
                       ACCUMULATOR_SIZE * N_ATTACK_FEATURES, fin);
    assert(items_read == ACCUMULATOR_SIZE * N_ATTACK_FEATURES);

    items_read =
        fread(input_layer_bias_q, sizeof(int16_t), ACCUMULATOR_SIZE, fin);
    assert(items_read == ACCUMULATOR_SIZE);

    for (int phase = 0; phase < NN_PHASE_COUNT; phase++) {
        items_read = fread(hidden_layer_1_weights_q[phase], sizeof(int16_t),
                           HIDDEN_1_SIZE * 2 * ACCUMULATOR_SIZE, fin);
        assert(items_read == HIDDEN_1_SIZE * 2 * ACCUMULATOR_SIZE);

        items_read = fread(hidden_layer_1_bias_q[phase], sizeof(int32_t),
                           HIDDEN_1_SIZE, fin);
        assert(items_read == HIDDEN_1_SIZE);

        items_read = fread(hidden_layer_2_weights_q[phase], sizeof(int16_t),
                           HIDDEN_1_SIZE * HIDDEN_2_SIZE, fin);
        assert(items_read == HIDDEN_1_SIZE * HIDDEN_2_SIZE);

        items_read = fread(hidden_layer_2_bias_q[phase], sizeof(int32_t),
                           HIDDEN_2_SIZE, fin);
        assert(items_read == HIDDEN_2_SIZE);

        items_read = fread(output_layer_weights_q[phase], sizeof(int32_t),
                           HIDDEN_2_SIZE, fin);
        assert(items_read == HIDDEN_2_SIZE);

        items_read = fread(output_layer_bias_q[phase], sizeof(int32_t), 1, fin);
        assert(items_read == 1);
    }

    fclose(fin);

    nn_save_weights_compressed(NET_NAME_COMPRESSED);

    return true;
}

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

/*
 * Maps a square in the a1..d1 up to a8..d8 half of the board to an index
 * between 0 and 31.
 */
static int index_half_mirrored(int sq) { return (sq & 3) | ((sq & 0x38) >> 1); }

/*
 * Compute the piece feature indices of the given side: own and opponent
 * pieces (planes 0..9) plus the opponent king (plane 10), in the side's
 * own frame (own king at the origin, squares mirrored for black; the
 * king's frame is additionally mirrored horizontally when the king
 * stands on files e..h).  This is the single definition of the piece
 * feature index computation, shared with nn_extract.c.
 */
int nn_compute_piece_indices(const Position *p, int side, int *out, int cap) {
    int flip = (side == Black) ? 0x38 : 0;
    int king_sq = p->kingSq[side] ^ flip;

    // Mirror the king position if it is on file e..h
    if ((king_sq & 7) > 3) {
        king_sq ^= 7;
        flip |= 7;
    }

    int king_index = index_half_mirrored(king_sq);
    int n = 0;

    for (int tp = Pawn; tp <= Queen; tp++) {
        BitBoard mask = p->mask[side][tp];

        while (mask) {
            int sq = bitboard_find_set_bit(mask);
            mask &= mask - 1;

            if (n == cap) {
                return -1;
            }
            out[n++] = 2048 * (tp - 1) + 64 * king_index + (sq ^ flip);
        }

        mask = p->mask[OPP(side)][tp];

        while (mask) {
            int sq = bitboard_find_set_bit(mask);
            mask &= mask - 1;

            if (n == cap) {
                return -1;
            }
            out[n++] = 2048 * (tp + 4) + 64 * king_index + (sq ^ flip);
        }
    }

    /* opponent king relative to own king (plane 10) */
    if (n == cap) {
        return -1;
    }
    out[n++] = 2048 * 10 + 64 * king_index + (p->kingSq[OPP(side)] ^ flip);
    return n;
}

/*
 * Build the accumulator for the given side from scratch: input bias plus the
 * contributions of all pieces and the opponent king.
 */
static void nn_build_accumulator(int16_t *accumulator_q, const Position *p,
                                 int side) {
    int indices[NN_MAX_PIECE_FEATURES];
    int n = nn_compute_piece_indices(p, side, indices, NN_MAX_PIECE_FEATURES);

    assert(n >= 0);

    memcpy(accumulator_q, input_layer_bias_q,
           sizeof(int16_t) * ACCUMULATOR_SIZE);
    for (int i = 0; i < n; i++) {
        nn_update_accumulator_add(accumulator_q, indices[i]);
    }
}

void nn_update_weights_for_piece(Position *p, int tp, int sq, bool turn,
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
        int mirror_w = 0;

        // Mirror the white king position if it is on file e..h
        if ((king_sq_w & 7) > 3) {
            king_sq_w ^= 7;
            mirror_w = 7;
        }

        int king_index_w = index_half_mirrored(king_sq_w);

        int index = 2048 * (tp - 1) + 64 * king_index_w + (sq ^ mirror_w);
        update_fn(p->accumulator_w_q, index);

        int mirror_b = 0;

        // Mirror the black king position if it is on file e..h
        if ((king_sq_b & 7) > 3) {
            king_sq_b ^= 7;
            mirror_b = 7;
        }

        int king_index_b = index_half_mirrored(king_sq_b);

        index = 2048 * (tp + 4) + 64 * king_index_b + (sq ^ flip_b ^ mirror_b);
        update_fn(p->accumulator_b_q, index);

    } else {
        int mirror_w = 0;

        // Mirror the white king position if it is on file e..h
        if ((king_sq_w & 7) > 3) {
            king_sq_w ^= 7;
            mirror_w = 7;
        }

        int king_index_w = index_half_mirrored(king_sq_w);

        int index = 2048 * (tp + 4) + 64 * king_index_w + (sq ^ mirror_w);
        update_fn(p->accumulator_w_q, index);

        int mirror_b = 0;

        // Mirror the black king position if it is on file e..h
        if ((king_sq_b & 7) > 3) {
            king_sq_b ^= 7;
            mirror_b = 7;
        }

        int king_index_b = index_half_mirrored(king_sq_b);

        index = 2048 * (tp - 1) + 64 * king_index_b + (sq ^ flip_b ^ mirror_b);
        update_fn(p->accumulator_b_q, index);
    }

    if (tp == Pawn) {
        nn_update_weights_for_pawn(p, sq, turn, add);
    }
}

void nn_update_weights_for_king(Position *p, int old_king_sq) {
#ifdef DEBUG_UPDATE
    print(1, "nn_update_weights_for_king(old_king_sq=%d)\n", old_king_sq);
#endif
    int side = p->turn;
    int opp = OPP(side);
    int16_t *accumulator =
        (side == White) ? p->accumulator_w_q : p->accumulator_b_q;

    nn_build_accumulator(accumulator, p, side);
    nn_build_pawn_accumulator(accumulator, p, side);

    /*
     * The rebuild above does not include the attack contribution, so
     * the stored attack map of the side whose king moved must be
     * reset. The next nn_update_attack_maps() call re-adds the full
     * current attack contribution of that side from scratch. The
     * opponent's accumulator is only adjusted incrementally and keeps
     * its attack contribution, so its stored map stays valid.
     */
    intset_sorted_init((side == White) ? &p->attack_map_w : &p->attack_map_b);

    /*
     * The opponent's accumulator only changed on plane 10 (opponent king
     * relative to own king): the side to move's king moved. The opponent's
     * own king, and hence its frame, is unchanged.
     */
    int16_t *opponent_accumulator_q =
        (opp == White) ? p->accumulator_w_q : p->accumulator_b_q;

    int flip = (opp == Black) ? 0x38 : 0;
    int king_sq_opp = p->kingSq[opp] ^ flip;
    int mirror = 0;

    if ((king_sq_opp & 7) > 3) {
        king_sq_opp ^= 7;
        mirror = 7;
    }

    int king_index_opp = index_half_mirrored(king_sq_opp);
    int base = 2048 * 10 + 64 * king_index_opp;

    nn_update_accumulator_sub(opponent_accumulator_q,
                              base + (old_king_sq ^ flip ^ mirror));
    nn_update_accumulator_add(opponent_accumulator_q,
                              base + (p->kingSq[side] ^ flip ^ mirror));
}

/*
 * Calculate the pawn index for the pawn pair on sq1, sq2 with the colors
 * c1 and c2.
 */
static int nn_pp_index(int sq1, bool c1, int sq2, bool c2) {
    // print(1, "    nn_pp_index(%d, %d, %d, %d)\n", sq1, c1, sq2, c2);

    int i1 = c1 * 48 + sq1 - 8;
    int i2 = c2 * 48 + sq2 - 8;

    // print(1, "      i1 = %d  i2 = %d\n", i1, i2);

    int hi = MAX(i1, i2);
    int lo = MIN(i1, i2);

    // print(1, "      hi = %d  lo = %d\n", h1, lo);

    return hi * (hi - 1) / 2 + lo;
}

/*
 * Populates accumulator_w and accumulator_b with the pawn features of p.
 *
 * Iterates through all unique pairs of pawns that are within one file of
 * distance. For example, in the starting position the white pawn at b2 would be
 * paired up with the white pawns (b2, a2) and (b2, c2) and the black pawns (b2,
 * a7), (b2, b7) and (b2, c7). For every pair of pawns and their colors the
 * index into the features array is calculated using nn_pp_index, and the
 * accumulators are updated.
 */

/*
 * Compute the shared pawn pair feature indices of the given side: every
 * unordered pair of pawns (of either color) within one file of distance,
 * indexed in the side's own canonical frame.  This is the single
 * definition of the pawn pair index computation, shared with
 * nn_extract.c.
 */
int nn_compute_pawn_indices(const Position *p, int side, int *out, int cap) {
    BitBoard mask = p->mask[White][Pawn] | p->mask[Black][Pawn];

    int king_sq = p->kingSq[side];
    int mirror = (king_sq & 7) > 3 ? 7 : 0;
    if (side == Black) {
        mirror |= 0x38;
    }

    int n = 0;

    while (mask) {
        int sq1 = bitboard_find_set_bit(mask);
        int c1 = bitboard_tst_bit(p->mask[side][Pawn], sq1) == 0;

        mask &= mask - 1;

        BitBoard other_mask = mask & right_of_file_mask[sq1 & 7];
        while (other_mask) {
            int sq2 = bitboard_find_set_bit(other_mask);
            int c2 = bitboard_tst_bit(p->mask[side][Pawn], sq2) == 0;
            other_mask &= other_mask - 1;

            if (n == cap) {
                return -1;
            }
            out[n++] = nn_pp_index(sq1 ^ mirror, c1, sq2 ^ mirror, c2);
        }
    }
    return n;
}

/*
 * Populates accumulator_w and accumulator_b with the pawn features of p.
 */
static void nn_build_pawn_accumulator(int16_t *accumulator, const Position *p,
                                      int side) {
    int indices[NN_MAX_PAWN_FEATURES];
    int n = nn_compute_pawn_indices(p, side, indices, NN_MAX_PAWN_FEATURES);

    assert(n >= 0);

    for (int i = 0; i < n; i++) {
        for (int j = 0; j < ACCUMULATOR_SIZE; j++) {
            accumulator[j] += input_pawn_weights_q[indices[i]][j];
        }
    }
}

/*
 * Incrementally update the accumulators in p for a pawn move. The pawn on sq1
 * with color turn was either added (add == true) or removed (add == false).
 */
static void nn_update_weights_for_pawn(Position *p, int sq1, bool turn,
                                       bool add) {
#ifdef DEBUG
    print(1, "nn_update_weights_for_pawn(%c%c, %d, %d)\n", SQUARE(sq1), turn,
          add);
#endif

    BitBoard mask = (p->mask[White][Pawn] | p->mask[Black][Pawn]) &
                    right_of_file_mask[sq1 & 7];
    int king_sq_w = p->kingSq[White];
    int king_sq_b = p->kingSq[Black];

    int mirror_w = (king_sq_w & 7) > 3 ? 7 : 0;
    int mirror_b = (king_sq_b & 7) > 3 ? 0x3f : 0x38;

    while (mask) {
        int sq2 = bitboard_find_set_bit(mask);
        mask &= mask - 1;

        if (sq1 != sq2) {
            int c2 = bitboard_tst_bit(p->mask[White][Pawn], sq2) == 0;

            int index_w = nn_pp_index(sq1 ^ mirror_w, turn, sq2 ^ mirror_w, c2);
            int index_b =
                nn_pp_index(sq1 ^ mirror_b, !turn, sq2 ^ mirror_b, !c2);
            if (add) {
                for (int i = 0; i < ACCUMULATOR_SIZE; i++) {
                    p->accumulator_w_q[i] += input_pawn_weights_q[index_w][i];
                }
                for (int i = 0; i < ACCUMULATOR_SIZE; i++) {
                    p->accumulator_b_q[i] += input_pawn_weights_q[index_b][i];
                }
            } else {
                for (int i = 0; i < ACCUMULATOR_SIZE; i++) {
                    p->accumulator_w_q[i] -= input_pawn_weights_q[index_w][i];
                }
                for (int i = 0; i < ACCUMULATOR_SIZE; i++) {
                    p->accumulator_b_q[i] -= input_pawn_weights_q[index_b][i];
                }
            }
        }
    }
}

#ifdef DEBUG
static void print_attack_map(const char *prefix, IntSetSorted *p) {
    if (p->count == 0)
        return;

    print(0, "%s: ", prefix);
    for (size_t i = 0; i < p->count; i++) {
        if (i != 0) {
            print(0, ", ");
        }
        print(0, "%d", p->data[i]);
    }
    fprintf(stderr, "\n");
    fflush(stderr);
}
#endif

static void update_attack_map_add(int16_t *accumulator, IntSetSorted *indexes) {
    for (size_t j = 0; j < indexes->count; j++) {
        int index = indexes->data[j];
        for (int i = 0; i < ACCUMULATOR_SIZE; i++) {
            accumulator[i] += input_attack_weights_q[index][i];
        }
    }
}

static void update_attack_map_sub(int16_t *accumulator, IntSetSorted *indexes) {
    for (size_t j = 0; j < indexes->count; j++) {
        int index = indexes->data[j];
        for (int i = 0; i < ACCUMULATOR_SIZE; i++) {
            accumulator[i] -= input_attack_weights_q[index][i];
        }
    }
}

void nn_update_attack_maps(Position *p) {
    IntSetSorted current_attack_map_w, current_attack_map_b;
    intset_sorted_init(&current_attack_map_w);
    intset_sorted_init(&current_attack_map_b);

    am_compute_attack_maps(p, &current_attack_map_w, &current_attack_map_b);

    IntSetSorted diff_attack_map_w, diff_attack_map_b;
    intset_sorted_init(&diff_attack_map_w);
    intset_sorted_init(&diff_attack_map_b);

    intset_sorted_diff(&p->attack_map_w, &current_attack_map_w,
                       &diff_attack_map_w);
    intset_sorted_diff(&p->attack_map_b, &current_attack_map_b,
                       &diff_attack_map_b);

    update_attack_map_sub(p->accumulator_w_q, &diff_attack_map_w);
    update_attack_map_sub(p->accumulator_b_q, &diff_attack_map_b);

#ifdef DEBUG
    print_attack_map("diff_attack_map_w", &diff_attack_map_w);
    print_attack_map("diff_attack_map_b", &diff_attack_map_b);
#endif

    IntSetSorted add_attack_map_w, add_attack_map_b;
    intset_sorted_init(&add_attack_map_w);
    intset_sorted_init(&add_attack_map_b);

    intset_sorted_diff(&current_attack_map_w, &p->attack_map_w,
                       &add_attack_map_w);
    intset_sorted_diff(&current_attack_map_b, &p->attack_map_b,
                       &add_attack_map_b);

#ifdef DEBUG
    print_attack_map("add_attack_map_w", &add_attack_map_w);
    print_attack_map("add_attack_map_b", &add_attack_map_b);
#endif

    update_attack_map_add(p->accumulator_w_q, &add_attack_map_w);
    update_attack_map_add(p->accumulator_b_q, &add_attack_map_b);

    p->attack_map_w = current_attack_map_w;
    p->attack_map_b = current_attack_map_b;
}

/*
 * Compute the raw network score for the position: refresh the stored
 * attack maps, select the phase branch, and run the quantized
 * pipeline. Pure function of the position and the weights: no score
 * hash table, no fuzz, no MaxPos adjustment.
 */
int nn_compute_score(Position *p) {
    int score;
    /* Phase branches share the input side; only the selection of the
     * hidden/output block depends on the position. */
    int phase = nn_phase_index(p);
#ifdef STATISTICS_Q
    bool print_stats_q = false;
#endif

    nn_update_attack_maps(p);

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

    int32_t hidden_layer_1_q[HIDDEN_1_SIZE];
    for (int i = 0; i < HIDDEN_1_SIZE; i++) {
        hidden_layer_1_q[i] = hidden_layer_1_bias_q[phase][i];
    }

    for (int i = 0; i < HIDDEN_1_SIZE; i++) {
        for (int j = 0; j < 2 * ACCUMULATOR_SIZE; j++) {
            hidden_layer_1_q[i] +=
                accumulator[j] * hidden_layer_1_weights_q[phase][i][j];
        }

#ifdef STATISTICS_Q
        if (ABS(hidden_layer_1_q[i]) > max_activation_hidden_1_q) {
            max_activation_hidden_1_q = ABS(hidden_layer_1_q[i]);
            print_stats_q = true;
        }
#endif
    }

    int16_t hidden_layer_1_q16[HIDDEN_1_SIZE];

    for (int i = 0; i < HIDDEN_1_SIZE; i++) {
        if (hidden_layer_1_q[i] <= 0) {
            hidden_layer_1_q16[i] = 0;
        } else if (hidden_layer_1_q[i] > SCALE_H1) {
            hidden_layer_1_q16[i] = SCALE_INPUT;
        } else {
            hidden_layer_1_q16[i] =
                (int16_t)((hidden_layer_1_q[i] + SCALE_HALF) / SCALE);
        }

        // assert(hidden_layer_1_q16[i] >= 0);
    }

#ifdef DEBUG
    print(1, "hidden_layer_1_q:\n");
    for (int i = 0; i < HIDDEN_1_SIZE; i++) {
        print(1, "%d, ", hidden_layer_1_q16[i]);
    }
    print(1, "\n");
#endif

    int32_t hidden_layer_2_q[HIDDEN_2_SIZE];
    memcpy(hidden_layer_2_q, hidden_layer_2_bias_q[phase],
           HIDDEN_2_SIZE * sizeof(int32_t));

    for (int i = 0; i < HIDDEN_2_SIZE; i++) {
        for (int j = 0; j < HIDDEN_1_SIZE; j++) {
            hidden_layer_2_q[i] +=
                hidden_layer_1_q16[j] * hidden_layer_2_weights_q[phase][i][j];
        }
    }

    int16_t hidden_layer_2_q16[HIDDEN_2_SIZE];

    for (int i = 0; i < HIDDEN_2_SIZE; i++) {
        if (hidden_layer_2_q[i] <= 0) {
            hidden_layer_2_q16[i] = 0;
        } else if (hidden_layer_2_q[i] >= SCALE_INPUT * SCALE) {
            hidden_layer_2_q16[i] = SCALE;
        } else {
            hidden_layer_2_q16[i] =
                (int16_t)((hidden_layer_2_q[i] + SCALE_INPUT_HALF) /
                          SCALE_INPUT);
        }

        // assert(hidden_layer_2_q16[i] >= 0);
    }

#ifdef DEBUG
    print(1, "hidden_layer_2_q:\n");
    for (int i = 0; i < HIDDEN_2_SIZE; i++) {
        print(1, "%d, ", hidden_layer_2_q16[i]);
    }
    print(1, "\n");
#endif

    int32_t output_layer_q = output_layer_bias_q[phase][0];

    for (int j = 0; j < HIDDEN_2_SIZE; j++) {
        output_layer_q +=
            hidden_layer_2_q16[j] * output_layer_weights_q[phase][j];
    }

#ifdef DEBUG
    print(1, "output_layer_q: %d\n", output_layer_q);
#endif

#ifdef STATISTICS_Q
    if (print_stats_q) {
        FILE *stats_file = fopen("activation_q.txt", "w");
        fprintf(stats_file, "max_activation_hidden_1: %d (%f)\n",
                max_activation_hidden_1_q,
                (double)max_activation_hidden_1_q / ((double)SCALE_H1));
        fclose(stats_file);
    }
#endif

    score = ((output_layer_q + SCALE_HALF) / SCALE) * SCALE_OUTPUT;
    score = (score + SCALE_HALF) / SCALE;

    return score;
}

/*
 * Evaluate the position using neural networks, with score caching,
 * fuzz and MaxPos adjustment. Returns the evaluation in 1/1000th of
 * a pawn.
 */
int nn_evaluate_position(Position *p) {
    int score;

#ifndef DEBUG
    STry++;
    if (ht_probe_score(p->hkey, &score) == Useful) {
        SHit++;
        return score;
    }
#endif

    score = nn_compute_score(p);

    /** Add a bit of fuzz. */
    if (fuzz_evaluation) {
        score += random_fuzz();
    }

    /*
     * Adjust MaxPos if necessary. If the current difference between the
     * material balance and the evaluation is greater than MaxPos, adopt
     * it as the new maximum.
     */
    int material_balance =
        (p->turn == 0) ? eval_material_balance(p) : -eval_material_balance(p);
    int pos_score = abs(material_balance - score);

    if (pos_score > MaxPos) {
        MaxPos = pos_score;
    }

    ht_store_score(p->hkey, score);

    return score;
}

void nn_init_accumulator(Position *p) {
    nn_build_accumulator(p->accumulator_w_q, p, White);
    nn_build_pawn_accumulator(p->accumulator_w_q, p, White);
    nn_build_accumulator(p->accumulator_b_q, p, Black);
    nn_build_pawn_accumulator(p->accumulator_b_q, p, Black);

    /*
     * Both accumulators are rebuilt without attack contributions, so
     * the stored attack maps must be empty again. The next
     * nn_update_attack_maps() call re-adds the full current attack
     * contribution from scratch.
     */
    intset_sorted_init(&p->attack_map_w);
    intset_sorted_init(&p->attack_map_b);

#ifdef DEBUG
    print(1, "accumulator:\n");
    for (int i = 0; i < 8; i++) {
        print(1, "%d, ", p->accumulator_w_q[i]);
    }
    print(1, "\n");
#endif
}

void nn_validate_weights(Position *p) {
    int16_t accumulator_w_q[ACCUMULATOR_SIZE];
    int16_t accumulator_b_q[ACCUMULATOR_SIZE];

    verbose = false;

    nn_build_accumulator(accumulator_w_q, p, White);
    nn_build_pawn_accumulator(accumulator_w_q, p, White);
    nn_build_accumulator(accumulator_b_q, p, Black);
    nn_build_pawn_accumulator(accumulator_b_q, p, Black);

    /* The accumulators carry the attack features of the stored attack
     * maps, which may lag behind the current position until the next
     * nn_update_attack_maps() call, so the reference must include
     * exactly those. */
    for (size_t i = 0; i < p->attack_map_w.count; i++) {
        int index = p->attack_map_w.data[i];
        for (int j = 0; j < ACCUMULATOR_SIZE; j++) {
            accumulator_w_q[j] += input_attack_weights_q[index][j];
        }
    }

    for (size_t i = 0; i < p->attack_map_b.count; i++) {
        int index = p->attack_map_b.data[i];
        for (int j = 0; j < ACCUMULATOR_SIZE; j++) {
            accumulator_b_q[j] += input_attack_weights_q[index][j];
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

#ifdef DEBUG
static void nn_show_weights(void) {
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

    print(1, "hidden_1 weights (phase 0):\n");
    for (int i = 0; i < 8; i++) {
        print(1, "%d, ", hidden_layer_1_weights_q[0][0][i]);
    }
    print(1, "\n");

    print(1, "hidden_1 bias (phase 0):\n");
    for (int i = 0; i < 8; i++) {
        print(1, "%d, ", hidden_layer_1_bias_q[0][i]);
    }
    print(1, "\n");

    print(1, "hidden_2 weights (phase 0):\n");
    for (int i = 0; i < 8; i++) {
        print(1, "%d, ", hidden_layer_2_weights_q[0][0][i]);
    }
    print(1, "\n");

    print(1, "hidden_2 bias (phase 0):\n");
    for (int i = 0; i < 8; i++) {
        print(1, "%d, ", hidden_layer_2_bias_q[0][i]);
    }
    print(1, "\n");

    print(1, "output weights (phase 0):\n");
    for (int i = 0; i < 8; i++) {
        print(1, "%d, ", output_layer_weights_q[0][i]);
    }
    print(1, "\n");

    print(1, "output bias (phase 0): %d\n", output_layer_bias_q[0][0]);
}
#endif

void nn_read_weights(void) {
    nn_read_weights_quantized();
#ifdef DEBUG
    nn_show_weights();
#endif
}

/*
 * Game phase of the position, determined by the total number of
 * non-pawn pieces on both sides (0..12 for legal positions), grouped
 * into buckets of three (MIN(count / 3, 3)):
 *   phase 0:  0..2   pieces (deep endgame)
 *   phase 1:  3..5   pieces (simplified midgame)
 *   phase 2:  6..8   pieces (typical midgame)
 *   phase 3:  9..12  pieces (start of the game)
 *
 * This is a shared contract with the training pipeline:
 * amy-training-nnue must label samples with the same function.
 */
static int nn_non_pawn_piece_count(const Position *p) {
    return bitboard_count_bits(p->mask[White][Knight] | p->mask[Black][Knight] |
                               p->mask[White][Bishop] | p->mask[Black][Bishop] |
                               p->mask[White][Rook] | p->mask[Black][Rook] |
                               p->mask[White][Queen] | p->mask[Black][Queen]);
}

int nn_phase_index(const Position *p) {
    return MIN(nn_non_pawn_piece_count(p) / 3, NN_PHASE_COUNT - 1);
}

void nn_phase_masks(const Position *p, int32_t masks[NN_PHASE_COUNT]) {
    int count = nn_non_pawn_piece_count(p);
    int phase = MIN(count / 3, NN_PHASE_COUNT - 1);

    for (int i = 0; i < NN_PHASE_COUNT; i++) {
        masks[i] = 0;
    }

    masks[phase] = 1;
    /*
     * Positions at the top of a bucket (counts 2, 5, 8): one capture
     * away from the next phase.  Training (amy-training-nnue) co-trains
     * the adjacent head so the hard switch at evaluation time is as
     * smooth as possible; this rule is the shared contract.
     */
    if (phase < NN_PHASE_COUNT - 1 && count % 3 == 2) {
        masks[phase + 1] = 1;
    }
}

void nn_randomize_weights(void) {

    for (int i = 0; i < N_FEATURES; i++) {
        for (int j = 0; j < ACCUMULATOR_SIZE; j++) {
            input_layer_weights_q[i][j] = (int8_t)(random_double() * 64);
        }
    }

    for (int i = 0; i < N_PAWN_FEATURES; i++) {
        for (int j = 0; j < ACCUMULATOR_SIZE; j++) {
            input_pawn_weights_q[i][j] = (int8_t)(random_double() * 64);
        }
    }

    for (int i = 0; i < N_ATTACK_FEATURES; i++) {
        for (int j = 0; j < ACCUMULATOR_SIZE; j++) {
            input_attack_weights_q[i][j] = (int8_t)(random_double() * 64);
        }
    }

    for (int i = 0; i < ACCUMULATOR_SIZE; i++) {
        input_layer_bias_q[i] = (int16_t)(random_double() * 64);
    }

    /* Randomize all phase branches independently, then replicate
     * phase 0 onto the other phases: the existing test suite expects
     * a phase-independent evaluation, and the phase-selection tests
     * introduce the per-phase divergence themselves. */
    for (int phase = 0; phase < NN_PHASE_COUNT; phase++) {
        for (int i = 0; i < HIDDEN_1_SIZE; i++) {
            for (int j = 0; j < 2 * ACCUMULATOR_SIZE; j++) {
                hidden_layer_1_weights_q[phase][i][j] =
                    (int16_t)(random_double() * 64);
            }

            hidden_layer_1_bias_q[phase][i] = (int32_t)(random_double() * 64);
        }

        for (int i = 0; i < HIDDEN_2_SIZE; i++) {
            for (int j = 0; j < HIDDEN_1_SIZE; j++) {
                hidden_layer_2_weights_q[phase][i][j] =
                    (int16_t)(random_double() * 64);
            }

            hidden_layer_2_bias_q[phase][i] = (int32_t)(random_double() * 64);
        }

        for (int i = 0; i < HIDDEN_2_SIZE; i++) {
            output_layer_weights_q[phase][i] = (int32_t)(random_double() * 64);
        }

        output_layer_bias_q[phase][0] = (int32_t)(random_double() * 64);
    }

    for (int phase = 1; phase < NN_PHASE_COUNT; phase++) {
        memcpy(hidden_layer_1_weights_q[phase], hidden_layer_1_weights_q[0],
               sizeof(hidden_layer_1_weights_q[0]));
        memcpy(hidden_layer_1_bias_q[phase], hidden_layer_1_bias_q[0],
               sizeof(hidden_layer_1_bias_q[0]));
        memcpy(hidden_layer_2_weights_q[phase], hidden_layer_2_weights_q[0],
               sizeof(hidden_layer_2_weights_q[0]));
        memcpy(hidden_layer_2_bias_q[phase], hidden_layer_2_bias_q[0],
               sizeof(hidden_layer_2_bias_q[0]));
        memcpy(output_layer_weights_q[phase], output_layer_weights_q[0],
               sizeof(output_layer_weights_q[0]));
        memcpy(output_layer_bias_q[phase], output_layer_bias_q[0],
               sizeof(output_layer_bias_q[0]));
    }
}

static void nn_save_weights_compressed(const char *file_name) {
    if (nn_header.wh_magic != wh_magic)
        return;

    int8_t *data = safe_malloc(nn_uncompressed_length);

    int8_t *ptr = data;

    memcpy(ptr, input_layer_weights_q, sizeof(input_layer_weights_q));
    ptr += sizeof(input_layer_weights_q);

    memcpy(ptr, input_pawn_weights_q, sizeof(input_pawn_weights_q));
    ptr += sizeof(input_pawn_weights_q);

    memcpy(ptr, input_attack_weights_q, sizeof(input_attack_weights_q));
    ptr += sizeof(input_attack_weights_q);

    memcpy(ptr, input_layer_bias_q, sizeof(input_layer_bias_q));
    ptr += sizeof(input_layer_bias_q);

    memcpy(ptr, hidden_layer_1_weights_q, sizeof(hidden_layer_1_weights_q));
    ptr += sizeof(hidden_layer_1_weights_q);

    memcpy(ptr, hidden_layer_1_bias_q, sizeof(hidden_layer_1_bias_q));
    ptr += sizeof(hidden_layer_1_bias_q);

    memcpy(ptr, hidden_layer_2_weights_q, sizeof(hidden_layer_2_weights_q));
    ptr += sizeof(hidden_layer_2_weights_q);

    memcpy(ptr, hidden_layer_2_bias_q, sizeof(hidden_layer_2_bias_q));
    ptr += sizeof(hidden_layer_2_bias_q);

    memcpy(ptr, output_layer_weights_q, sizeof(output_layer_weights_q));
    ptr += sizeof(output_layer_weights_q);

    memcpy(ptr, output_layer_bias_q, sizeof(output_layer_bias_q));
    ptr += sizeof(output_layer_bias_q);

    int max_len = lzav_compress_bound_hi(
        nn_uncompressed_length); // Note the different bound function
    void *comp_buf = safe_malloc(max_len);
    int comp_len =
        lzav_compress_hi(data, comp_buf, nn_uncompressed_length, max_len);

    CompressedWeightsHeader new_header;
    new_header.cwh_header = nn_header;
    new_header.cwh_header.wh_magic = wh_magic_compressed;
    new_header.cwh_compressed_size = comp_len;

    FILE *fout = fopen(file_name, "w");
    if (fout) {
        fwrite(&new_header, sizeof(new_header), 1, fout);
        fwrite(comp_buf, comp_len, 1, fout);
        fclose(fout);
    }

    free(comp_buf);
    free(data);
}

static bool nn_load_weights_compressed(const char *file_name) {
    FILE *fin = fopen(file_name, "rb");

    if (fin == NULL) {
        return false;
    }

    CompressedWeightsHeader header;
    size_t items_read = fread(&header, sizeof(header), 1, fin);
    assert(items_read == 1);

    nn_check_header(&header.cwh_header);
    nn_header = header.cwh_header;

    int8_t *buffer = safe_malloc(header.cwh_compressed_size);
    items_read = fread(buffer, header.cwh_compressed_size, 1, fin);
    assert(items_read == 1);

    int8_t *data = safe_malloc(nn_uncompressed_length);

    int l = lzav_decompress(buffer, data, header.cwh_compressed_size,
                            nn_uncompressed_length);
    assert(l >= 0);

    fclose(fin);

    int8_t *ptr = data;

    memcpy(input_layer_weights_q, ptr, sizeof(input_layer_weights_q));
    ptr += sizeof(input_layer_weights_q);

    memcpy(input_pawn_weights_q, ptr, sizeof(input_pawn_weights_q));
    ptr += sizeof(input_pawn_weights_q);

    memcpy(input_attack_weights_q, ptr, sizeof(input_attack_weights_q));
    ptr += sizeof(input_attack_weights_q);

    memcpy(input_layer_bias_q, ptr, sizeof(input_layer_bias_q));
    ptr += sizeof(input_layer_bias_q);

    memcpy(hidden_layer_1_weights_q, ptr, sizeof(hidden_layer_1_weights_q));
    ptr += sizeof(hidden_layer_1_weights_q);

    memcpy(hidden_layer_1_bias_q, ptr, sizeof(hidden_layer_1_bias_q));
    ptr += sizeof(hidden_layer_1_bias_q);

    memcpy(hidden_layer_2_weights_q, ptr, sizeof(hidden_layer_2_weights_q));
    ptr += sizeof(hidden_layer_2_weights_q);

    memcpy(hidden_layer_2_bias_q, ptr, sizeof(hidden_layer_2_bias_q));
    ptr += sizeof(hidden_layer_2_bias_q);

    memcpy(output_layer_weights_q, ptr, sizeof(output_layer_weights_q));
    ptr += sizeof(output_layer_weights_q);

    memcpy(output_layer_bias_q, ptr, sizeof(output_layer_bias_q));
    ptr += sizeof(output_layer_bias_q);

    free(buffer);
    free(data);

    return true;
}

static void nn_check_header(const WeightsHeader *header) {
    assert(header->wh_accumulator_size == ACCUMULATOR_SIZE);
    assert(header->wh_num_features == N_FEATURES);
    assert(header->wh_pawn_features_size == N_PAWN_FEATURES);
    assert(header->wh_attack_features_size == N_ATTACK_FEATURES);
    assert(header->wh_hidden_layer_1_size == HIDDEN_1_SIZE);
    assert(header->wh_hidden_layer_2_size == HIDDEN_2_SIZE);
    assert(header->wh_accumulator_weights_quantization ==
           wh_quantization_int_8);
    assert(header->wh_accumulator_bias_quantization == wh_quantization_int_16);
    assert(header->wh_hidden_layer_1_weights_quantization ==
           wh_quantization_int_16);
    assert(header->wh_hidden_layer_1_bias_quantization ==
           wh_quantization_int_32);
    assert(header->wh_hidden_layer_2_weights_quantization ==
           wh_quantization_int_16);
    assert(header->wh_hidden_layer_2_bias_quantization ==
           wh_quantization_int_32);
    assert(header->wh_output_layer_weights_quantization ==
           wh_quantization_int_32);
    assert(header->wh_output_layer_bias_quantization == wh_quantization_int_32);
    assert(header->wh_phase_count == NN_PHASE_COUNT);
}
