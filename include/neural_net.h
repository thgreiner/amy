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

#ifndef NEURAL_NET_H
#define NEURAL_NET_H

#include "dbase.h"
#include "intset_sorted.h"
#include <stdbool.h>

/* If true, random noise is added to the evaluation score. */
extern bool fuzz_evaluation;

/* Maximum offset between material balance (eval_material_balance) and
 * the evaluation. Forms a cheap, optimistic score estimate
 * (material + MaxPos) to skip the expensive full evaluation.
 * Reset to nn_eval_init's initial value before every search. */
extern int MaxPos;
int eval_material_balance(const struct Position *);
void nn_eval_init(void);

/* Architecture of the quantized network (network.quant layout). */
#define HIDDEN_1_SIZE 32
#define HIDDEN_2_SIZE 32

/* Number of game phase branches: each phase has its own hidden-1,
 * hidden-2 and output layers; the input side is shared. The phase is
 * selected by the total non-pawn piece count (both sides) via
 * nn_phase_index(). */
#define NN_PHASE_COUNT 4

/* Piece feature index range: 11 planes (0..4 own P..Q, 5..9 opponent
 * P..Q, 10 opponent king) x 32 king positions x 64 squares. */
#define N_FEATURES (32 * 64 * 11)

/* Shared pawn pair feature index range (96 color-square ids, unordered
 * pairs). */
#define N_PAWN_FEATURES 4560

/* Maximum number of active input features per accumulator per side:
 * 15 own + 15 opponent non-king pieces plus the opponent king (31 in
 * any legal position); at most 96 pawn pairs; attack maps hold
 * INTSET_SORTED_MAX entries.  These caps are part of the nn_extract
 * ABI (see nn_extract.h).  The piece cap has headroom beyond 31 for
 * arbitrary (non-selfplay) test positions. */
#define NN_MAX_PIECE_FEATURES 36
#define NN_MAX_PAWN_FEATURES 128
#define NN_MAX_ATTACK_FEATURES INTSET_SORTED_MAX

/*
 * Feature index generation, shared by the engine (accumulator
 * rebuild/update) and the feature extraction library (nn_extract.c):
 * single definition of the index computation.  Each function writes at
 * most cap indices into out and returns the count, or -1 if the count
 * would exceed cap.
 */
int nn_compute_piece_indices(const struct Position *p, int side, int *out,
                             int cap);
int nn_compute_pawn_indices(const struct Position *p, int side, int *out,
                            int cap);

void nn_read_weights(void);

/* Game phase of the position (0..NN_PHASE_COUNT-1), determined by the
 * total number of non-pawn pieces on both sides. This is a shared
 * contract with the training pipeline (amy-training-nnue): samples must
 * be labeled with the same value. */
int nn_phase_index(const struct Position *p);

/*
 * Training-side of the phase contract (amy-training-nnue): the active
 * phase heads of the position as a NN_PHASE_COUNT-element 0/1 mask.
 * nn_phase_index(p) is always active; positions holding the top count
 * of a bucket (total non-pawn pieces 2, 5, 8) additionally activate
 * the higher phase, so training can co-train the two adjacent heads.
 */
void nn_phase_masks(const struct Position *p, int32_t masks[NN_PHASE_COUNT]);

/*
 * Raw network score: attack-map refresh, phase-branch selection and
 * the quantized pipeline. Pure function of the position and the
 * weights (no score hash table, no fuzz, no MaxPos). Exposed so
 * tests exercise the pipeline without the caching wrapper.
 */
int nn_compute_score(struct Position *p);

/*
 * Evaluate the position with score caching, fuzz and MaxPos
 * adjustment. Returns the evaluation in 1/1000th of a pawn.
 */
int nn_evaluate_position(struct Position *);
void nn_init_accumulator(struct Position *);
void nn_update_weights_for_piece(struct Position *p, int tp, int sq, bool turn,
                                 bool add);
void nn_update_weights_for_king(struct Position *p, int old_king_sq);
void nn_update_attack_maps(struct Position *);
void nn_validate_weights(struct Position *);
void nn_randomize_weights(void);

#endif
