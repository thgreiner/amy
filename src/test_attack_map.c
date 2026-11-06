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

#include "attack_map.h"
#include "dbase.h"

#include <assert.h>
#include <stddef.h>

/*
 * Verify that the set contains exactly the given values (in ascending
 * order, which is also the invariant of IntSetSorted).
 */
static void assert_intset_eq(IntSetSorted *set, const int32_t expected[],
                             size_t n) {
    size_t i;

    assert(intset_sorted_count(set) == n);
    for (i = 0; i < n; i++) {
        assert(intset_sorted_data(set)[i] == expected[i]);
    }
}

static void assert_intset_empty(IntSetSorted *set) {
    assert(intset_sorted_count(set) == 0);
}

/*
 * No piece in the initial position attacks a piece of the other side,
 * so both attack maps must be empty.
 */
static void test_attack_map_initial_position(void) {
    struct Position *p;
    IntSetSorted w;
    IntSetSorted b;

    p = dbase_create_position_from_epd(
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -");
    intset_sorted_init(&w);
    intset_sorted_init(&b);

    am_compute_attack_maps(p, &w, &b);

    assert_intset_empty(&w);
    assert_intset_empty(&b);

    dbase_free_position(p);
}

/*
 * White rook b1 and black rook g1 attack each other (the rooks block
 * each other's rank at the far end).

   white: Rook b1 -> Rook g1: plane 17, sq 1*64+6=70    -> 69702
   black: Rook g1 -> Rook b1: plane 17, sq 6*64+1=385   -> 70017
 */
static void test_attack_map_rook_both_sides(void) {
    struct Position *p;
    IntSetSorted w;
    IntSetSorted b;
    int32_t expected_w[] = {69702};
    int32_t expected_b[] = {73342};

    p = dbase_create_position_from_epd("4k3/8/8/8/8/8/K7/1R4r1 w - -");
    intset_sorted_init(&w);
    intset_sorted_init(&b);

    am_compute_attack_maps(p, &w, &b);

    assert_intset_eq(&w, expected_w, 1);
    assert_intset_eq(&b, expected_b, 1);

    dbase_free_position(p);
}

/*
 * White pawn b4 captures both the black knight on a5 and the black
 * bishop on c5, and the white knight on b2 attacks the black rook on
 * d3. The black bishop c5 attacks the pawn on b4, but that attack must
 * be excluded because the pawn on b4 already attacks the bishop.

   white: Pawn b4 -> Knight a5:  plane 0, sq 25*64+32=1632  -> 1632
          Pawn b4 -> Bishop c5:  plane 1, sq 25*64+34=1634  -> 5730
          Knight b2 -> Rook d3:  plane 7, sq 9*64+19=595    -> 29267
   black: (empty, the only attack is the excluded bishop -> pawn one)
 */
static void test_attack_map_pawn_and_knight(void) {
    struct Position *p;
    IntSetSorted w;
    IntSetSorted b;
    int32_t expected_w[] = {1632, 5730, 29267};

    p = dbase_create_position_from_epd("7k/8/8/n1b5/1P6/3r4/1N6/K7 w - -");
    intset_sorted_init(&w);
    intset_sorted_init(&b);

    am_compute_attack_maps(p, &w, &b);

    assert_intset_eq(&w, expected_w, 3);
    assert_intset_empty(&b);

    dbase_free_position(p);
}

/*
 * The white queen on d4 attacks the black knight c5 (knight move), the
 * black bishop d6 (same file, i.e. only via rook moves) and the black
 * rook e5 (same diagonal, i.e. only via bishop moves). No black piece
 * attacks a white piece here.

   white: Queen d4 -> Knight c5:  plane 20, sq 27*64+34=1762  -> 83682
          Queen d4 -> Bishop d6:  plane 21, sq 27*64+43=1771  -> 87787
          Queen d4 -> Rook e5:    plane 22, sq 27*64+36=1764  -> 91876
 */
static void test_attack_map_queen_attacks(void) {
    struct Position *p;
    IntSetSorted w;
    IntSetSorted b;
    int32_t expected_w[] = {83682, 87787, 91876};

    p = dbase_create_position_from_epd("6k1/8/3b4/2n1r3/3Q4/8/8/1K6 w - -");
    intset_sorted_init(&w);
    intset_sorted_init(&b);

    am_compute_attack_maps(p, &w, &b);

    assert_intset_eq(&w, expected_w, 3);
    assert_intset_empty(&b);

    dbase_free_position(p);
}

/*
 * The queen on d4 attacks the black rook on d6 (same file) and the
 * black bishop on e5 (same diagonal). Queen attacks on bishops are
 * only counted via rook moves and queen attacks on rooks are only
 * counted via bishop moves, so neither attack is in the white attack
 * map. Both black pieces attack the queen.

   white: (empty)
   black: Bishop e5 -> Queen d4:  plane 13, sq 36*64+27=2331  -> 55579
          Rook d6 -> Queen d4:    plane 18, sq 43*64+27=2779  -> 76507
 */
static void test_attack_map_queen_line_filter(void) {
    struct Position *p;
    IntSetSorted w;
    IntSetSorted b;
    int32_t expected_b[] = {55012, 75044};

    p = dbase_create_position_from_epd("4k3/8/3r4/4b3/K2Q4/8/8/8 w - -");
    intset_sorted_init(&w);
    intset_sorted_init(&b);

    am_compute_attack_maps(p, &w, &b);

    assert_intset_empty(&w);
    assert_intset_eq(&b, expected_b, 2);

    dbase_free_position(p);
}

void test_all_attack_map(void) {
    test_attack_map_initial_position();
    test_attack_map_rook_both_sides();
    test_attack_map_pawn_and_knight();
    test_attack_map_queen_attacks();
    test_attack_map_queen_line_filter();
}
