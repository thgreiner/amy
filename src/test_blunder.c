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

#include "blunder.h"
#include "dbase.h"
#include "inline.h"
#include "string.h"

static void test_get_best_move_from_comment(void) {
    struct Position *p = InitialPosition();
    char eval_buf[16];

    char comment[] = "q=0.5003; p=[a3:18, b3:22, c3:7, d3:8, e3:7, f3:12, "
                     "g3:12, h3:8, Na3:11, Nc3:19, Nf3:35, Nh3:7, a4:7, b4:31, "
                     "c4:24, d4:62, e4:64, f4:9, g4:10, h4:9]";

    int move = get_best_move_from_comment(comment, p, eval_buf);

    assert(move == make_move(e2, e4, M_PAWND));
    assert(strcmp(eval_buf, "0.5003") == 0);

    FreePosition(p);
}

void test_all_blunder(void) { test_get_best_move_from_comment(); }
