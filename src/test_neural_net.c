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
    struct Position *p = InitialPosition();

    InitAccumulator(p);
    ValidateWeights(p);

    int move = make_move(b1, a3, 0);
    DoMove(p, move);
    ValidateWeights(p);

    UndoMove(p, move);
    ValidateWeights(p);

    FreePosition(p);
}

static void test_promotion(void) {
    struct Position *p =
        CreatePositionFromEPD("3r1k2/p3qppP/8/3p4/3r4/5QP1/P4PB1/4R1K1 w - -");

    InitAccumulator(p);
    ValidateWeights(p);

    int move = make_promotion(h7, h8, Queen, 0);
    DoMove(p, move);
    ValidateWeights(p);

    UndoMove(p, move);
    ValidateWeights(p);
}

static void test_castle(void) {
    struct Position *p = CreatePositionFromEPD(
        "r1bqkb1r/pppp1ppp/2n2n2/1B2p3/4P3/5N2/PPPP1PPP/RNBQK2R w KQkq -");

    InitAccumulator(p);
    ValidateWeights(p);

    int move = make_move(e1, g1, M_SCASTLE);
    DoMove(p, move);
    ValidateWeights(p);

    UndoMove(p, move);
    ValidateWeights(p);
}

static void test_en_passant(void) {
    struct Position *p = CreatePositionFromEPD(
        "rnbqkbnr/1pp1pppp/p7/3pP3/8/8/PPPP1PPP/RNBQKBNR w KQkq d6");

    InitAccumulator(p);
    ValidateWeights(p);

    int move = make_move(e5, d6, M_ENPASSANT);
    DoMove(p, move);
    ValidateWeights(p);

    UndoMove(p, move);
    ValidateWeights(p);
}

void test_all_neural_net(void) {
    RandomizeWeights();

    test_knight_move();
    test_promotion();
    test_castle();
    test_en_passant();
}
