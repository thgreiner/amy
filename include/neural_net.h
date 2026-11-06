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

/* Architecture of the quantized network (network.quant layout). */
#define HIDDEN_1_SIZE 32
#define HIDDEN_2_SIZE 32

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
int nn_evaluate_position(struct Position *);
void nn_init_accumulator(struct Position *);
void nn_update_weights_for_piece(struct Position *p, int tp, int sq, bool turn,
                                 bool add);
void nn_update_weights_for_king(struct Position *p, int old_king_sq);
void nn_update_attack_maps(struct Position *);
void nn_validate_weights(struct Position *);
void nn_randomize_weights(void);

#endif
