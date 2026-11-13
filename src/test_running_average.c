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

#include "running_average.h"

#include <assert.h>

/*
 * After initialization the buffer must be completely filled with the
 * initial value, with the index pointing at the oldest element.
 */
static void test_ra_init(void) {
    RunningAverageBuffer buf;

    ra_init(&buf, 7);

    assert(buf.ra_index == 0);
    assert(buf.ra_sum == 7 * WINDOW_SIZE);
    for (int i = 0; i < WINDOW_SIZE; i++)
        assert(buf.ra_buffer[i] == 7);
}

/*
 * Feeding a constant stream must return the constant at all times,
 * including across the wrap-around of the circular index.
 */
static void test_ra_update_constant(void) {
    RunningAverageBuffer buf;

    ra_init(&buf, 7);

    for (int i = 0; i < 2 * WINDOW_SIZE + 1; i++) {
        assert(ra_update(&buf, 7) == 7);
        assert(buf.ra_sum == 7 * WINDOW_SIZE);
    }
}

/*
 * A single spike must be evicted exactly after WINDOW_SIZE further
 * updates, at which point the average is back at the baseline.
 */
static void test_ra_window_eviction(void) {
    RunningAverageBuffer buf;

    ra_init(&buf, 0);

    /* One spike; the average is the spike diluted by the empty window. */
    assert(ra_update(&buf, 1000000) == 1000000 / WINDOW_SIZE);

    /* The spike stays in the window for WINDOW_SIZE - 1 more updates. */
    for (int i = 0; i < WINDOW_SIZE - 1; i++)
        assert(ra_update(&buf, 0) == 1000000 / WINDOW_SIZE);

    /* Now the spike's own slot is replaced; back at baseline, so a
     * second spike behaves identically. */
    assert(ra_update(&buf, 0) == 0);
    assert(buf.ra_sum == 0);
    assert(ra_update(&buf, 1000000) == 1000000 / WINDOW_SIZE);
}

/*
 * The circular buffer must evict exactly the oldest element: after a
 * full window of ones, replacing the oldest with 16384 must give
 * (WINDOW_SIZE - 1 + 16384) / WINDOW_SIZE.
 */
static void test_ra_oldest_evicted(void) {
    RunningAverageBuffer buf;

    ra_init(&buf, 0);

    /* Fill the window with ones: until it is full the average
     * truncates to 0, then it is exactly 1. */
    for (int i = 0; i < WINDOW_SIZE - 1; i++)
        assert(ra_update(&buf, 1) == 0);
    assert(ra_update(&buf, 1) == 1);
    assert(buf.ra_sum == WINDOW_SIZE);

    /* The oldest element (a 1) is evicted. */
    assert(ra_update(&buf, 16384) == (WINDOW_SIZE - 1 + 16384) / WINDOW_SIZE);
}

/*
 * Signed values: alternating +/-5 samples cancel out exactly, a single
 * non-zero slot truncates to zero (C division, toward zero), and
 * negative initialization / constant streams must work as well.
 */
static void test_ra_signed_values(void) {
    RunningAverageBuffer buf;
    int sign = -1;

    ra_init(&buf, -3);
    for (int i = 0; i < WINDOW_SIZE; i++)
        assert(buf.ra_buffer[i] == -3);
    assert(buf.ra_sum == -3 * WINDOW_SIZE);
    for (int i = 0; i < WINDOW_SIZE; i++)
        assert(ra_update(&buf, -3) == -3);

    /* Fill exactly one window: until the window is full, a single
     * non-zero slot leaves the sum at +/-5, which truncates to 0. */
    ra_init(&buf, 0);
    for (int i = 0; i < WINDOW_SIZE; i += 2) {
        assert(ra_update(&buf, 5 * sign) == 0);
        assert(buf.ra_sum == 5 * sign);
        assert(ra_update(&buf, -5 * sign) == 0);
        assert(buf.ra_sum == 0);
        sign = -sign;
    }
}

/*
 * Deterministic pseudo-random stream checked against an independent
 * reference: the returned average must equal the exact window sum
 * divided by WINDOW_SIZE (C integer division), for values large enough
 * to overflow a 32-bit window sum and thus exercise the long ra_sum.
 */
static void test_ra_matches_reference(void) {
    RunningAverageBuffer buf;
    long ref_sum = 0;
    int ref_window[WINDOW_SIZE];
    unsigned long state = 0x12345678;

    ra_init(&buf, 0);
    for (int i = 0; i < WINDOW_SIZE; i++)
        ref_window[i] = 0;

    for (int i = 0; i < 3 * WINDOW_SIZE; i++) {
        /* LCG: values in [-1000000, 1000000]. */
        state = state * 1103515245 + 12345;
        int value = (int)(state % 2000001) - 1000000;

        /* ra_update() replaces the slot at the current index and then
         * advances it, so mirror that here without touching the index. */
        int idx = buf.ra_index;
        ref_sum += value - ref_window[idx];
        ref_window[idx] = value;

        assert(ra_update(&buf, value) == (int)(ref_sum / WINDOW_SIZE));
    }
}

void test_all_running_average(void) {
    test_ra_init();
    test_ra_update_constant();
    test_ra_window_eviction();
    test_ra_oldest_evicted();
    test_ra_signed_values();
    test_ra_matches_reference();
}
