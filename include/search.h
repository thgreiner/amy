/*

    Amy - a chess playing program

    Copyright (c) 2002-2025, Thorsten Greiner
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

#ifndef SEARCH_H
#define SEARCH_H

#include "config.h"
#include "dbase.h"
#include <stdint.h>

#define INF 200000 /* max. score */
#define CMLIMIT                                                                \
    100000 /* scores above this (or below -CMLIMIT)                            \
            * indicate checkmate */
#define ON_EVALUATION (INF + 1)

#define MAX_TREE_SIZE 64 /* maximum depth we will search to */

typedef enum {
    PB_NO_PB_MOVE = 0,
    PB_NO_PB_HIT,
    PB_HIT,
    PB_ALT_COMMAND

} pb_result_t;

extern int ExtendInCheck;
extern int ExtendDoubleCheck;
extern int ExtendDiscoveredCheck;
extern int ExtendSingularReply;
extern int ExtendPassedPawn;
extern int ExtendZugzwang;
extern int ReduceNullMove;
extern int ReduceNullMoveDeep;
extern int16_t ExtendRecapture[];

extern unsigned int FHTime;
extern bool AbortSearch;

#if MP
extern int NumberOfCPUs;
#endif

int Iterate(struct Position *);
void SearchRoot(struct Position *);
void AnalysisMode(struct Position *);
pb_result_t PermanentBrain(struct Position *);
#if MP
void StopHelpers(void);
#endif

#endif
