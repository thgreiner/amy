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
 * search_io.c - output search results
 */

#include "amy.h"
#include <stdio.h>

#define PV_BUFFER_SIZE 512

static void PrintPV(char *pv) {
    static char PVBuffer[PV_BUFFER_SIZE];
    char *x;
    int len = 21;

    strncpy(PVBuffer, pv, PV_BUFFER_SIZE);
    PVBuffer[PV_BUFFER_SIZE - 1] = '\0';

    for (x = PVBuffer; *x;) {
        char *y = x;

        while (*y != ' ' && *y != '\0')
            y++;
        if (*y == '\0')
            *(y + 1) = '\0';
        *y = '\0';

        len += strlen(x);

        if (len >= 79) {
            Print(1, "\n                    ");
            len = 21 + strlen(x);
        }
        Print(1, "%s ", x);
        len += 1;
        x = y + 1;
    }
    Print(1, "\n");
}

void SearchHeader(void) {
    Print(1, "It    Time   Score  principal Variation\n");
}

void SearchOutput(int depth, int time, int score, char *line, int nodes) {
    Print(1, "%2d  %s %7s  ", depth, TimeToText(time), ScoreToText(score));
    PrintPV(line);
    if (PostMode) {
        char *short_line = strdup(line);
        int s = score / 10;

        if (s >= 9999) {
            s = 9999;
        } else if (s <= -9999) {
            s = -9999;
        }

        if (short_line) {
            if (strlen(short_line) > 80) {
                int idx;
                for (idx = 80; idx > 1; idx--) {
                    if (short_line[idx] == ' ' && short_line[idx - 1] != '.') {
                        break;
                    }
                }
                short_line[idx] = '\0';
            }
            PrintNoLog(0, "%d %d %d %d %s\n", depth, s, time, nodes,
                       short_line);
            free(short_line);
        }
    }
}

void SearchOutputFailHighLow(int depth, int time, int isfailhigh, char *move,
                             int nodes) {
    if (isfailhigh) {
        Print(1, "%2d  %s     +++  %s\n", depth, TimeToText(time), move);
        if (PostMode) {
            PrintNoLog(0, "%d 0 %d %d %s!\n", depth, time, nodes, move);
        }
    } else {
        Print(1, "%2d  %s     ---  %s\n", depth, TimeToText(time), move);
        if (PostMode) {
            PrintNoLog(0, "%d 0 %d %d %s?\n", depth, time, nodes, move);
        }
    }
}
