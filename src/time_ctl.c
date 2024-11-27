/*

    Amy - a chess playing program

    Copyright (c) 2002-2024, Thorsten Greiner
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

/*
 * time_ctl.c - time management routines
 */

#include "amy.h"
#include <stdio.h>
#include <string.h>

int TMoves = 60, TTime = 5 * 60;
int Moves[3] = {60, 60}, Time[3] = {5 * 60, 5 * 60};
int TMoves2, TTime2;
int TwoTimeControls = FALSE;

int Increment = 0;

void DoTC(struct Position *p, int mtime) {
    Time[p->turn] += -mtime + Increment;

    if (Moves[p->turn] > 0) {
        Moves[p->turn] -= 1;
        if (Moves[p->turn] <= 0) {
            if (TwoTimeControls) {
                Print(0, "Switching to second time control.\n");
                TMoves = TMoves2;
                TTime = TTime2;
                TwoTimeControls = FALSE;
            }
            Moves[p->turn] = TMoves;
            Time[p->turn] += TTime;
        }
    }
}

void CalcTime(struct Position *p, float *soft, float *hard) {
    if (TMoves >= 0) {
        if (Moves[p->turn] > 0) {
            /*  int limit = (13*TTime/TMoves)/8 + (3*Increment)/4;  */
            float limit = (1.625 * TTime / TMoves) + (0.85 * Increment);

            Print(1, "TC: %d moves in %s\n", Moves[p->turn],
                  TimeToText((unsigned int)(Time[p->turn]) * ONE_SECOND));

            /*  *soft = (7*Time[p->turn]/Moves[p->turn])/8 + (3*Increment)/4; */
            *soft =
                (0.875 * Time[p->turn] / Moves[p->turn]) + (0.75 * Increment);
            if (*soft > limit)
                *soft = limit;

            if (TwoTimeControls && Moves[p->turn] <= 5) {
                int moves = TMoves2;
                int soft2;
                if (moves == 0)
                    moves = 60;
                soft2 = TTime2 / moves;
                /*    *soft = ((*soft)+(float)soft2)/2;    */
                *soft = 0.5 * ((*soft) + (float)soft2);
                Print(1, "Adjusted timing to %.4f secs\n", *soft);
            }
            *hard = 4.0 * (*soft);
        } else {
            /*  expect additional game length of 60 moves beyond current move */
            /*  use equations from section above with fixed Moves[p->turn] of 60
             */
            /*  rearrange equation to eliminate floating point division  */
            /*  1.625 / 60 = 0.0271  */
            float limit = 0.0271 * ((float)Time[p->turn] + 60 * Increment);

            Print(1, "TC: all moves in %s\n",
                  TimeToText((unsigned int)(Time[p->turn]) * ONE_SECOND));

            /*  0.875 / 60.0 = 0.0146  */
            *soft = 0.0146 * (float)Time[p->turn] + (0.85 * Increment);
            if (*soft > limit)
                *soft = limit;
            *hard = 4.0 * (*soft);
        }
        if (*hard > Time[p->turn])
            *hard = 0.5 * (float)Time[p->turn];
        if (*soft > *hard)
            *soft = 0.67 * (*hard);

        Print(1, "TL: %.2f/%.2f\n", *soft, *hard);
    } else {
        *soft = *hard = (float)TTime;
    }
}
