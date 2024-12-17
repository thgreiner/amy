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
int TwoTimeControls = false;

int Increment = 0;

struct SingleTimeControl {
    int moves;
    int total_time;
    int increment;
};

struct TimeControl {
    struct SingleTimeControl first;
    struct SingleTimeControl second;
    bool hasSecondTimeControl;
};

/** Stores the single global time control. */
static struct TimeControl globalTimeControl = {.first.moves = 60,
                                               .first.total_time = 5 * 60,
                                               .hasSecondTimeControl = false};

void DoTC(struct Position *p, int mtime) {
    Time[p->turn] += -mtime + Increment;

    if (Moves[p->turn] > 0) {
        Moves[p->turn] -= 1;
        if (Moves[p->turn] <= 0) {
            if (TwoTimeControls) {
                Print(0, "Switching to second time control.\n");
                TMoves = TMoves2;
                TTime = TTime2;
                TwoTimeControls = false;
            }
            Moves[p->turn] = TMoves;
            Time[p->turn] += TTime;
        }
    }
}

void CalcTime(struct Position *p, float *soft, float *hard) {
    char time_as_text[16];
    if (TMoves >= 0) {
        if (Moves[p->turn] > 0) {
            /*  int limit = (13*TTime/TMoves)/8 + (3*Increment)/4;  */
            float limit = (1.625 * TTime / TMoves) + (0.85 * Increment);

            Print(1, "TC: %d moves in %s\n", Moves[p->turn],
                  FormatTime((unsigned int)(Time[p->turn]) * ONE_SECOND,
                             time_as_text, sizeof(time_as_text)));
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
                  FormatTime((unsigned int)(Time[p->turn]) * ONE_SECOND,
                             time_as_text, sizeof(time_as_text)));
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

static struct TimeControl parse_timecontrol_xboard(char *args[]) {
    int ttmoves, ttime, tminutes, tseconds, inc = 0;

    sscanf(args[0], "%d", &ttmoves);
    char *colon = strchr(args[1], ':'); /* check for time in xx:yy format */
    if (colon) {
        sscanf(args[1], "%d:%d", &tminutes, &tseconds);
        ttime = (tminutes * 60) + tseconds;
    } else {
        sscanf(args[1], "%d", &tminutes);
        ttime = tminutes * 60;
    }
    sscanf(args[2], "%d", &inc);

    struct TimeControl result = {.first.moves = ttmoves,
                                 .first.total_time = ttime,
                                 .first.increment = inc,
                                 .hasSecondTimeControl = false};

    return result;
}

/** Literal for 'sudden death' time control. */
static const char *const sudden_death = "sd";

/** Literal for 'fixed' time control. */
static const char *const fixed = "fixed";

static struct TimeControl parse_timecontrol(char *args[]) {
    int ttmoves, ttime, inc = 0;
    int ttmoves2, ttime2;

    struct TimeControl result = globalTimeControl;

    char *x = strtok(args[0], "/+ \t\n\r");
    if (x) {
        if (!strcmp(x, sudden_death))
            ttmoves = 0;
        else if (!strcmp(x, fixed))
            ttmoves = -1;
        else
            sscanf(x, "%d", &ttmoves);
        x = strtok(NULL, "/ \t\n\r");
        if (x) {
            sscanf(x, "%d", &ttime);
            for (x++; *x; x++) {
                if (*x == '+') {
                    sscanf(x + 1, "%d", &inc);
                    break;
                }
            }
            if (args[1] != NULL) {
                x = strtok(args[1], " /\n\t\r");
                ttmoves2 = -1;
                if (!strcmp(x, sudden_death))
                    ttmoves2 = 0;
                else
                    sscanf(x, "%d", &ttmoves2);
                x = strtok(NULL, " /\n\t\r");
                if (x) {
                    ttime2 = -1;
                    sscanf(x, "%d", &ttime2);
                    if (ttmoves2 >= 0 && ttime2 > 0)
                        TwoTimeControls = true;
                }
            }
            if (ttmoves >= 0) {
                result.first.moves = ttmoves;
                result.first.total_time = ttime * 60;
                result.first.increment = inc;
                result.hasSecondTimeControl = false;

            } else {
                result.first.moves = -1;
                result.first.total_time = ttime;
                result.first.increment = 0;
                result.hasSecondTimeControl = false;
            }
            if (TwoTimeControls) {
                result.second.moves = ttmoves2;
                result.second.total_time = ttime2 * 60;
                result.second.increment = 0;
                result.hasSecondTimeControl = true;
            }
        }
    }
    return result;
}

/**
 * Set the global time control using an array of strings.
 *
 * Args:
 *     args: an array of strings for the time controls
 *     xboard_flag: indicates whether the format is expected to be
 *         for xboard or not
 */
void SetTimeControl(char *args[], bool xboard_flag) {
    if (xboard_flag) {
        globalTimeControl = parse_timecontrol_xboard(args);
    } else {
        globalTimeControl = parse_timecontrol(args);
    }
    ResetTimeControl(!xboard_flag);
}

/**
 * Reset the global time control and clocks.
 *
 * Args:
 *     verbose: if true, the settings are printed
 */
void ResetTimeControl(bool verbose) {
    TMoves = globalTimeControl.first.moves;
    TTime = globalTimeControl.first.total_time;
    Increment = globalTimeControl.first.increment;

    TwoTimeControls = globalTimeControl.hasSecondTimeControl;

    TMoves2 = globalTimeControl.second.moves;
    TTime2 = globalTimeControl.second.total_time;

    Moves[White] = Moves[Black] = TMoves;
    Time[White] = Time[Black] = TTime;

    if (verbose) {
        Print(0, "Timecontrol is ");
        if (globalTimeControl.first.moves >= 0) {
            if (globalTimeControl.first.moves == 0)
                Print(0, "all ");
            else
                Print(0, "%d ", globalTimeControl.first.moves);

            if (globalTimeControl.first.increment) {
                Print(0, "moves in %d mins + %d secs Increment\n",
                      globalTimeControl.first.total_time / 60,
                      globalTimeControl.first.increment);
            } else {
                Print(0, "moves in %d mins\n",
                      globalTimeControl.first.total_time / 60);
            }

        } else {
            Print(0, "%d seconds/move fixed time\n",
                  globalTimeControl.first.total_time);
        }

        if (globalTimeControl.hasSecondTimeControl) {
            Print(0, "Second Timecontrol is ");
            if (globalTimeControl.second.moves == 0)
                Print(0, "all ");
            else
                Print(0, "%d ", globalTimeControl.second.moves);
            Print(0, "moves in %d mins\n",
                  globalTimeControl.second.total_time / 60);
        }
    }
}
