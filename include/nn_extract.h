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

#ifndef NN_EXTRACT_H
#define NN_EXTRACT_H

#include <stddef.h>
#include <stdint.h>

#include "attack_map.h" /* N_ATTACK_FEATURES */
#include "neural_net.h" /* N_FEATURES, N_PAWN_FEATURES, NN_MAX_* */

/*
 * Network architecture constants and per-record output capacities,
 * exported for the Python feature-extraction wrapper so that both
 * sides can never drift apart (see PLAN-attack-map-features.md, T2).
 */
typedef struct {
    int32_t n_features;          /* N_FEATURES */
    int32_t n_pawn_features;     /* N_PAWN_FEATURES */
    int32_t n_attack_features;   /* N_ATTACK_FEATURES */
    int32_t accumulator_size;    /* ACCUMULATOR_SIZE */
    int32_t max_piece_features;  /* NN_MAX_PIECE_FEATURES */
    int32_t max_pawn_features;   /* NN_MAX_PAWN_FEATURES */
    int32_t max_attack_features; /* NN_MAX_ATTACK_FEATURES */
    int32_t phase_count;         /* NN_PHASE_COUNT */
} NnExtractConstants;

void nn_export_constants(NnExtractConstants *out);

/*
 * Extract NNUE input feature indices from packed BPD records.
 *
 * data is a buffer of n_records consecutive BinaryPosition records
 * (40 bytes each, see bpd.h; typically a whole .bin file read with
 * np.fromfile on the Python side).
 *
 * For every record, all feature arrays are first filled with -1; for
 * valid records the indices are then written at the front of each
 * array (no -1 among valid entries) and valid[i] is set to 1,
 * turn[i] and score[i] are copied from the record.  A record whose
 * nibbles fail bpd_record_valid() (or whose feature counts exceed an
 * output capacity) stays all -1 with valid[i] = 0.
 *
 * Index computation is shared with the engine itself
 * (nn_compute_piece_indices / nn_compute_pawn_indices /
 * am_compute_attack_indices), so the training data can never drift
 * from what the engine feeds its network.
 *
 * phase (n_records x NN_PHASE_COUNT, int32) carries the per-record
 * active phase-head masks from nn_phase_masks(): always at least the
 * phase of nn_phase_index(); positions at the top of a bucket
 * (counts 2, 5, 8) additionally activate the higher phase.  Invalid
 * records carry an all-zero phase row (like their all -1 feature
 * rows).
 *
 * Returns the number of valid records (>= 0).
 */
int nn_extract_features(const uint8_t *data, size_t n_records, int32_t *feat_w,
                        int32_t *feat_b, int32_t *pfeat_w, int32_t *pfeat_b,
                        int32_t *afeat_w, int32_t *afeat_b, uint8_t *turn,
                        int32_t *score, int32_t *valid, int32_t *phase);

#endif
