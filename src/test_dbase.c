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

#include "inline.h"
#include <assert.h>

static void test_parse_san_promotions(void) {
    Position *p = dbase_create_position_from_epd("4K1k1/P7/8/8/8/8/8/8 w - -");

    move_t move = dbase_parse_san(p, "a8=Q");
    assert(move == dbase_make_promotion(a7, a8, Queen, 0));

    move = dbase_parse_san(p, "a8=R");
    assert(move == dbase_make_promotion(a7, a8, Rook, 0));

    move = dbase_parse_san(p, "a8=B");
    assert(move == dbase_make_promotion(a7, a8, Bishop, 0));

    move = dbase_parse_san(p, "a8=N");
    assert(move == dbase_make_promotion(a7, a8, Knight, 0));

    dbase_free_position(p);
}

/*
 * dbase_gen_checks() inspects the square one rank behind each checking
 * pawn square, which is off the board when the opponent's king stands
 * on the 2nd rank (white to move) or the 7th rank (black to move).  In
 * the first position the rook on a4 (blocked by the knights so that it
 * cannot give check itself) puts 0x01 (== Pawn) into the
 * little-endian byte directly behind p->piece[64], so an out-of-bounds
 * regression would emit a bogus check move here.
 */
static void test_gen_checks_opponent_king_near_edge(void) {
    char *epds[] = {
        "8/8/8/8/RN4/N7/4k3/7K w - -",
        "8/4K3/8/8/8/8/8/k7 b - -",
    };
    for (size_t i = 0; i < 2; i++) {
        Position *p = dbase_create_position_from_epd(epds[i]);
        heap_t heap = heap_allocate();
        heap_push_section(heap);
        dbase_gen_checks(p, heap);
        assert(heap->current_section->end == heap->current_section->start);
        heap_pop_section(heap);
        heap_free(heap);
        dbase_free_position(p);
    }
}

/*
 * Positive control: the pawn on d2 checks on d3 (the opponent's king is
 * on e4), and that is the only check move in the position.
 */
static void test_gen_checks_pawn(void) {
    Position *p = dbase_create_position_from_epd("8/8/8/8/4k3/8/3P4/K7 w - -");
    heap_t heap = heap_allocate();
    heap_push_section(heap);
    dbase_gen_checks(p, heap);
    assert(heap->current_section->end - heap->current_section->start == 1);
    assert(heap->data[heap->current_section->start] ==
           dbase_make_move(d2, d3, 0));
    heap_pop_section(heap);
    heap_free(heap);
    dbase_free_position(p);
}

void test_all_dbase(void) {
    test_parse_san_promotions();
    test_gen_checks_opponent_king_near_edge();
    test_gen_checks_pawn();
}
