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

#ifndef HASHTABLE_H
#define HASHTABLE_H

#include "config.h"
#include "dbase.h"
#include "evaluation.h"
#include "types.h"
#include <stdbool.h>
#include <stdint.h>

#if HAVE_STDATOMIC_H && MP
#include <stdatomic.h>
#define OPTIONAL_ATOMIC _Atomic
#else
#define OPTIONAL_ATOMIC
#endif

typedef enum {
    ExactScore,
    LowerBound,
    UpperBound,
    Useful,
    Useless,
    OnEvaluation
} LookupResult;

struct HTEntry {
    hash_t ht_Key;
    move_t ht_Move;
    int ht_Score;
    short ht_Flags;
    short ht_Depth;
};

struct PTEntry {
    hash_t pt_Key;
    int pt_Score;
    struct PawnFacts pt_PawnFacts;
};

struct STEntry {
    hash_t st_Key;
    int st_Score;
};

extern hash_t HashKeys[2][8][64];
extern hash_t HashKeysEP[64];
extern hash_t HashKeysCastle[16];
extern hash_t STMKey;

extern OPTIONAL_ATOMIC unsigned long PHit, PTry, SHit, STry, HHit, HTry;
extern int L_HT_Bits, L_HT_Size, L_HT_Mask;

void ht_clear(void);
void ht_age(void);
void ht_clear_pawn(void);
void ht_allocate(void);
#if MP
LookupResult ht_probe(hash_t, int *, int, move_t *, bool *, int, int,
                      struct HTEntry *);
void ht_store(hash_t, int, int, int, int, int, int, int, struct HTEntry *);
#else
LookupResult ht_probe(hash_t, int *, int, move_t *, bool *, int);
void ht_store(hash_t, int, int, int, int, int, int, int);
#endif
LookupResult ht_probe_pawn(hash_t, int *, struct PawnFacts *);
void ht_store_pawn(hash_t, int, struct PawnFacts *);
LookupResult ht_probe_score(hash_t, int *);
void ht_store_score(hash_t, int);
void ht_show_statistics(void);
void ht_guess_sizes(char *);
void ht_init(void);

#endif
