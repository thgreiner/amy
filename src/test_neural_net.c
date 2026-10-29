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

#include "dbase.h"
#include "inline.h"
#include "neural_net.h"

static void test_knight_move(void) {
    struct Position *p = dbase_initial_position();

    nn_init_accumulator(p);
    nn_validate_weights(p);

    int move = dbase_make_move(b1, a3, 0);
    dbase_do_move(p, move);
    nn_validate_weights(p);

    dbase_undo_move(p, move);
    nn_validate_weights(p);

    dbase_free_position(p);
}

static void test_promotion(void) {
    struct Position *p = dbase_create_position_from_epd(
        "3r1k2/p3qppP/8/3p4/3r4/5QP1/P4PB1/4R1K1 w - -");

    nn_init_accumulator(p);
    nn_validate_weights(p);

    int move = dbase_make_promotion(h7, h8, Queen, 0);
    dbase_do_move(p, move);
    nn_validate_weights(p);

    dbase_undo_move(p, move);
    nn_validate_weights(p);
}

static void test_castle(void) {
    struct Position *p = dbase_create_position_from_epd(
        "r1bqkb1r/pppp1ppp/2n2n2/1B2p3/4P3/5N2/PPPP1PPP/RNBQK2R w KQkq -");

    nn_init_accumulator(p);
    nn_validate_weights(p);

    int move = dbase_make_move(e1, g1, M_SCASTLE);
    dbase_do_move(p, move);
    nn_validate_weights(p);

    dbase_undo_move(p, move);
    nn_validate_weights(p);
}

static void test_king_move(void) {
    struct Position *p =
        dbase_create_position_from_epd("4k3/8/8/8/8/8/8/4K3 w - -");

    nn_init_accumulator(p);
    nn_validate_weights(p);

    int move1 = dbase_make_move(e1, d2, 0);
    dbase_do_move(p, move1);
    nn_validate_weights(p);

    int move2 = dbase_make_move(e8, d8, 0);
    dbase_do_move(p, move2);
    nn_validate_weights(p);

    dbase_undo_move(p, move2);
    nn_validate_weights(p);

    dbase_undo_move(p, move1);
    nn_validate_weights(p);

    dbase_free_position(p);
}

static void test_en_passant(void) {
    struct Position *p = dbase_create_position_from_epd(
        "rnbqkbnr/1pp1pppp/p7/3pP3/8/8/PPPP1PPP/RNBQKBNR w KQkq d6");

    nn_init_accumulator(p);
    nn_validate_weights(p);

    int move = dbase_make_move(e5, d6, M_ENPASSANT);
    dbase_do_move(p, move);
    nn_validate_weights(p);

    dbase_undo_move(p, move);
    nn_validate_weights(p);
}

void test_all_neural_net(void) {
    nn_randomize_weights();

    test_knight_move();
    test_promotion();
    test_castle();
    test_king_move();
    test_en_passant();
}
