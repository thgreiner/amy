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

#include "intset_sorted.h"

#include <assert.h>
#include <stdbool.h>
#include <string.h>

/*
 * Returns 1 if data[0..count) is strictly ascending, which also catches
 * duplicates.
 */
static int is_sorted_ascending(const IntSetSorted *s) {
    size_t i;

    for (i = 1; i < s->count; i++) {
        if (s->data[i - 1] >= s->data[i])
            return 0;
    }
    return 1;
}

/*
 * Add values in unsorted order and check the sorted invariant holds,
 * then exercise contains / remove.
 */
static void test_intset_sorted_basics(void) {
    IntSetSorted s;
    int32_t junk[] = {50, 10, 30, 20, 40, 0, -1, 60};
    const int32_t *d;
    int32_t want[] = {-1, 0, 10, 20, 30, 40, 50, 60};
    size_t i;

    intset_sorted_init(&s);
    assert(intset_sorted_count(&s) == 0);

    for (i = 0; i < sizeof(junk) / sizeof(junk[0]); i++) {
        assert(intset_sorted_add(&s, junk[i]) == INTSET_SORTED_ADDED);
        assert(is_sorted_ascending(&s));
    }
    assert(intset_sorted_add(&s, 30) == INTSET_SORTED_PRESENT);
    assert(intset_sorted_count(&s) == 8);

    d = intset_sorted_data(&s);
    assert(memcmp(d, want, sizeof(want)) == 0);

    assert(intset_sorted_contains(&s, 30));
    assert(!intset_sorted_contains(&s, 25));

    /* remove a middle element -> still sorted */
    assert(intset_sorted_remove(&s, 30));
    assert(is_sorted_ascending(&s));
    assert(intset_sorted_count(&s) == 7);
    assert(!intset_sorted_contains(&s, 30));
    assert(intset_sorted_remove(&s, 999) == false);
}

/*
 * Fill the set to capacity in descending order; a further add must
 * report the set is full.
 */
static void test_intset_sorted_full(void) {
    IntSetSorted s;
    int i;

    intset_sorted_init(&s);
    for (i = 0; i < INTSET_SORTED_MAX; i++) {
        assert(intset_sorted_add(&s, INTSET_SORTED_MAX - i) ==
               INTSET_SORTED_ADDED);
    }
    assert(intset_sorted_add(&s, 1000000) == INTSET_SORTED_FULL);
    assert(is_sorted_ascending(&s));
}

/* Set operations on sorted inputs, including union overflow. */
static void test_intset_sorted_set_ops(void) {
    IntSetSorted a, b, r;
    IntSetSorted big1, big2, small;
    const int32_t av[] = {1, 2, 3, 4, 5};
    const int32_t bv[] = {2, 4, 6};
    int32_t dr[] = {1, 3, 5};
    int32_t ir[] = {2, 4};
    int32_t ur[] = {1, 2, 3, 4, 5, 6};
    size_t i;
    int j;

    intset_sorted_init(&a);
    intset_sorted_init(&b);
    intset_sorted_init(&r);
    for (i = 0; i < 5; i++)
        intset_sorted_add(&a, av[i]);
    for (i = 0; i < 3; i++)
        intset_sorted_add(&b, bv[i]);

    /* diff = {1,3,5} */
    assert(intset_sorted_diff(&a, &b, &r));
    assert(is_sorted_ascending(&r));
    assert(intset_sorted_count(&r) == 3);
    assert(memcmp(intset_sorted_data(&r), dr, sizeof(dr)) == 0);

    /* intersect = {2,4} */
    assert(intset_sorted_intersect(&a, &b, &r));
    assert(intset_sorted_count(&r) == 2);
    assert(memcmp(intset_sorted_data(&r), ir, sizeof(ir)) == 0);

    /* union = {1,2,3,4,5,6} */
    assert(intset_sorted_union(&a, &b, &r));
    assert(is_sorted_ascending(&r));
    assert(intset_sorted_count(&r) == 6);
    assert(memcmp(intset_sorted_data(&r), ur, sizeof(ur)) == 0);

    /* union overflow */
    intset_sorted_init(&big1);
    intset_sorted_init(&big2);
    intset_sorted_init(&small);
    for (j = 0; j < INTSET_SORTED_MAX; j++)
        intset_sorted_add(&big1, j);
    for (j = INTSET_SORTED_MAX; j < 2 * INTSET_SORTED_MAX; j++) {
        intset_sorted_add(&big2, j);
    }
    assert(intset_sorted_union(&big1, &big2, &small) == false);
    assert(intset_sorted_count(&small) == 0);
}

void test_all_intset_sorted(void) {
    test_intset_sorted_basics();
    test_intset_sorted_full();
    test_intset_sorted_set_ops();
}
