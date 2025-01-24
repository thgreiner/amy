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

#ifndef NEXT_H
#define NEXT_H

#include "config.h"
#include "heap.h"
#include "types.h"
#include <stdbool.h>

typedef enum {
    HashMove,
    GenerateCaptures,
    GainingCapture,
    Killer1,
    Killer2,
    CounterMv,
    Killer3,
    GenerateRest,
    LoosingCapture,
    HistoryMoves,
    Done,
    GenerateQChecks,
    QChecks
} SearchPhase;

struct SearchStatus {
    SearchPhase st_phase;
    move_t st_hashmove;
    move_t st_k1, st_k2, st_kl, st_cm, st_k3;
};

struct KillerEntry {
    move_t killer1, killer2;   /* killer moves */
    uint32_t kcount1, kcount2; /* killer count */
};

struct SearchData {
    struct Position *position;

    struct SearchStatus *current;
    struct SearchStatus *statusTable;
    struct KillerEntry *killer;
    struct KillerEntry *killerTable;
#if MP
    struct HTEntry *localHashTable;
    heap_t deferred_heap;
#endif

    heap_t heap;
    int32_t *data_heap;
    unsigned int data_heap_size;

    unsigned int counterTab[2][4096]; /* counter moves per side */
    unsigned int historyTab[2][4096]; /* history moves per side */

    int pv_save[64];

    uint16_t ply;

    bool master; /* true if a master process */
    unsigned long nodes_cnt, qnodes_cnt, check_nodes_cnt;

    move_t best_move;
    uint16_t depth;

    uint16_t nrootmoves;
    uint16_t movenum;
};

struct SearchData *CreateSearchData(struct Position *);
void FreeSearchData(struct SearchData *);
void EnterNode(struct SearchData *);
void LeaveNode(struct SearchData *);
int NextMove(struct SearchData *);
int NextEvasion(struct SearchData *);
int NextMoveQ(struct SearchData *, int);
void PutKiller(struct SearchData *, move_t);
void TestNextGenerators(struct Position *);

#endif
