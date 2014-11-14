/*

    Amy - a chess playing program

    Copyright (c) 2014, Thorsten Greiner
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this
      list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
    FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
    DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
    OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

/*
 * hashtable.c - hashtable management routines
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "amy.h"

#define HT_AGE            (0x3f)
#define HT_NCPU           ((0x3f) << 6)
#define HT_NCPU_INCREMENT (1 << 6)
#define HT_THREAT         (1 << 12)
#define HT_EXACT          (1 << 13)
#define HT_LBOUND         (1 << 14)
#define HT_UBOUND         (1 << 15)

#define PT_INVALID 0xffff

hash_t HashKeys[2][8][64];
hash_t HashKeysEP[64];
hash_t HashKeysCastle[16];
hash_t STMKey;

int HT_Bits  = 17;
static int PT_Bits  = 15;
static int ST_Bits  = 15;

static int HT_Size, 
           HT_Mask;

static int PT_Size, 
           PT_Mask;

static int ST_Size, 
           ST_Mask;

static struct HTEntry     *TranspositionTable = NULL;
static struct PTEntry     *PawnTable          = NULL;
static struct STEntry     *ScoreTable         = NULL;
static int    HTGeneration                    = 0;

#if MP && HAVE_LIBPTHREAD
static pthread_mutex_t TranspositionMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t PawnMutex          = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t ScoreMutex         = PTHREAD_MUTEX_INITIALIZER;
#endif

static struct HTEntry *SelectHTEntry(hash_t key, int depth)
{
    struct HTEntry *h1, *h2;

    h1 = TranspositionTable + ((key >> 32)& HT_Mask);
    if(h1->ht_Signature == (int)key) return h1;
    h2 = TranspositionTable + (((key >> 32) + 1) & HT_Mask);
    if(h2->ht_Signature == (int)key) return h2;

    if(h1->ht_Depth <= h2->ht_Depth) {
    	if(h1->ht_Depth <= depth) return h1;
       	if(h2->ht_Depth <= depth) return h2;
    }
    else { 
       	if(h2->ht_Depth <= depth) return h2;
    	if(h1->ht_Depth <= depth) return h1;
    }

    if((h1->ht_Flags & HT_AGE) != HTGeneration) return h1;
    if((h2->ht_Flags & HT_AGE) != HTGeneration) return h2;

    return NULL;
}

#if MP
int ProbeHT(hash_t key, int *score, int depth, int *bestm, int *threat, int ply,
            int exclusiveP)
#else
int ProbeHT(hash_t key, int *score, int depth, int *bestm, int *threat, int ply)
#endif
{
    struct HTEntry *h1 = TranspositionTable + ((key >> 32)& HT_Mask);
    struct HTEntry *h = NULL;
    int result = Useless;

#if MP && HAVE_LIBPTHREAD
    pthread_mutex_lock(&TranspositionMutex);
#endif /* MP && HAVE_LIBPTHREAD */

    if(h1->ht_Signature == (int)key) h = h1;
    else {
        h1 = TranspositionTable + (((key >> 32) + 1) & HT_Mask);
        if(h1->ht_Signature == (int)key) h = h1;
    }

    if(h != NULL) {
        *bestm = h->ht_Move;
        *threat = (h->ht_Flags & HT_THREAT);

#if MP
	if((int)h->ht_Depth == depth && exclusiveP && 
	   (h->ht_Flags & HT_NCPU) > 0) {

	    result = OnEvaluation;

	} else 
#endif
	
	if((int)h->ht_Depth >= depth) {
            *score = h->ht_Score;

            /*
             * Correct a mate score. See comment in 'StoreHT'.
             */

            if(*score > CMLIMIT)       { *score -= ply; } 
            else if(*score < -CMLIMIT) { *score += ply; }

            if(h->ht_Flags & HT_EXACT) {
                result = ExactScore;
            }
            else if(h->ht_Flags & HT_LBOUND) {
		result = LowerBound;
            }
            else if(h->ht_Flags & HT_UBOUND) {
		result = UpperBound;
            }

#if MP
	    if((int)h->ht_Depth == depth) {

		/* 
		 * increment processor count 
		 */

		h->ht_Flags += HT_NCPU_INCREMENT;
	    }
#endif /* MP */

        } else {
	    result = Useful;
	}
    }
#if MP
    else {
	h = SelectHTEntry(key, depth);
	if(h) {
	    h->ht_Depth = depth;
	    h->ht_Flags = HT_NCPU_INCREMENT;
	    h->ht_Signature = (int)key;
	}
    }
#endif /* MP */

#if MP && HAVE_LIBPTHREAD
    pthread_mutex_unlock(&TranspositionMutex);
#endif /* MP && HAVE_LIBPTHREAD */

    return result;
}

int ProbePT(hash_t key, int *score, struct PawnFacts *pf)
{
    struct PTEntry *h = PawnTable + ((key >> 32) & PT_Mask);
    int result = Useless;

#if MP && HAVE_LIBPTHREAD
    pthread_mutex_lock(&PawnMutex);
#endif /* MP && HAVE_LIBPTHREAD */

    if(h->pt_Signature == (int)key && h->pt_Score != PT_INVALID) {
	*score = h->pt_Score;
        *pf    = h->pt_PawnFacts;
	result = Useful;
    }

#if MP && HAVE_LIBPTHREAD
    pthread_mutex_unlock(&PawnMutex);
#endif /* MP && HAVE_LIBPTHREAD */

    return result;
}

int ProbeST(hash_t key, int *score)
{
    struct STEntry *h = ScoreTable + ((key >> 32) & ST_Mask);
    int result = Useless;

#if MP && HAVE_LIBPTHREAD
    pthread_mutex_lock(&ScoreMutex);
#endif /* MP && HAVE_LIBPTHREAD */
    if(h->st_Signature == (int)key && h->st_Score != PT_INVALID) {
	*score = h->st_Score;
	result = Useful;
    }
#if MP && HAVE_LIBPTHREAD
    pthread_mutex_unlock(&ScoreMutex);
#endif /* MP && HAVE_LIBPTHREAD */

    return result;
}

void StoreHT(hash_t key, 
             int best, int alpha, int beta, 
             int bestm, int depth, 
             int threat, int ply)
{
    struct HTEntry *h;
    

#if MP && HAVE_LIBPTHREAD
    pthread_mutex_lock(&TranspositionMutex);
#endif /* MP && HAVE_LIBPTHREAD */

    if((h = SelectHTEntry(key, depth)) != NULL) {
	int reduced = best;

	/*
	 * Handling of mate scores is a bit tricky.
	 * Upon storing we correct it to mean 'mate in n from this position'
	 * instead of 'mate in n from root position'. Upon retrieval this has
	 * to be corrected.
	 */

	if(best > CMLIMIT) {
	    reduced += ply;
	} else if(best < -CMLIMIT) {
	    reduced -= ply;
	}

#if MP
        if(h->ht_Signature == (int)key && depth == h->ht_Depth) {
            if((h->ht_Flags & HT_NCPU) > 0) {
                h->ht_Flags = (h->ht_Flags & HT_NCPU) - HT_NCPU_INCREMENT;
            }
        } else {
            h->ht_Signature = (int)key;
            h->ht_Flags = 0;
        }
#else
	h->ht_Signature = (int)key;
#endif /* MP */

	h->ht_Move = bestm;
	h->ht_Depth = depth;
	h->ht_Score = reduced;
#if MP
        h->ht_Flags |= HTGeneration;
	if(best <= alpha)     h->ht_Flags |= HT_UBOUND;
	else if(best >= beta) h->ht_Flags |= HT_LBOUND;
	else                  h->ht_Flags |= HT_EXACT;
#else
        h->ht_Flags = HTGeneration;
	if(best <= alpha)     h->ht_Flags |= HT_UBOUND;
	else if(best >= beta) h->ht_Flags |= HT_LBOUND;
	else                  h->ht_Flags |= HT_EXACT;
#endif /* MP */
	if(threat)            h->ht_Flags |= HT_THREAT;
    }

#if MP && HAVE_LIBPTHREAD
    pthread_mutex_unlock(&TranspositionMutex);
#endif /* MP && HAVE_LIBPTHREAD */

}

void StorePT(hash_t key, int score, struct PawnFacts *pf)
{
    struct PTEntry *h = PawnTable + ((key >> 32) & PT_Mask);
#if MP && HAVE_LIBPTHREAD
    pthread_mutex_lock(&PawnMutex);
#endif /* MP && HAVE_LIBPTHREAD */
    h->pt_Signature = (int)key;
    h->pt_Score     = score;
    h->pt_PawnFacts = *pf;
#if MP && HAVE_LIBPTHREAD
    pthread_mutex_unlock(&PawnMutex);
#endif /* MP && HAVE_LIBPTHREAD */
}

void StoreST(hash_t key, int score)
{
    struct STEntry *h = ScoreTable + ((key >> 32) & ST_Mask);
#if MP && HAVE_LIBPTHREAD
    pthread_mutex_lock(&ScoreMutex);
#endif /* MP && HAVE_LIBPTHREAD */
    h->st_Signature = (int)key;
    h->st_Score     = score;
#if MP && HAVE_LIBPTHREAD
    pthread_mutex_unlock(&ScoreMutex);
#endif /* MP && HAVE_LIBPTHREAD */
}

/* Moved this to a seperate routine to make the PB-Move
 * selection work better..
 */

void ClearHashTable(void)
{
    int i;
    struct HTEntry *h = TranspositionTable;
    
    for(i=0; i<HT_Size; i++, h++) {
        h->ht_Signature = 0;
	h->ht_Flags = h->ht_Flags = 0;
    }
}

void AgeHashTable(void)
{
    HTGeneration++;
    HTGeneration &= HT_AGE;
}

void ClearPawnHashTable(void)
{
    int i;
    struct PTEntry *ph;
    struct STEntry *sh;

    ph = PawnTable;
    for(i=0; i<PT_Size; i++,ph++) {
	ph->pt_Score = PT_INVALID;
    }

    sh = ScoreTable;
    for(i=0; i<ST_Size; i++,sh++) {
	sh->st_Score = PT_INVALID;
    }
}

static void FreeHT(void)
{
    if(TranspositionTable) {
	free(TranspositionTable);
	TranspositionTable = NULL;
    }

    if(PawnTable) {
	free(PawnTable);
	PawnTable = NULL;
    }

    if(ScoreTable) {
	free(ScoreTable);
	ScoreTable = NULL;
    }
}

void AllocateHT(void)
{
    static int registered_free_ht = FALSE;

    /*
     * Register atexit() handler to free hashtable memory automatically
     */

    if(!registered_free_ht) {
	registered_free_ht = TRUE;
	atexit(FreeHT);
    }

    HT_Size   = 1 << HT_Bits;
    HT_Mask   = HT_Size - 1;
    
    TranspositionTable = calloc(HT_Size, sizeof(struct HTEntry));

    PT_Size = 1 << PT_Bits;
    PT_Mask = PT_Size - 1;

    PawnTable = calloc(PT_Size, sizeof(struct PTEntry));

    ST_Size = 1 << ST_Bits;
    ST_Mask = ST_Size - 1;

    ScoreTable = calloc(ST_Size, sizeof(struct STEntry));

    Print(0, "Hashtable sizes: %d k, %d k, %d k\n",
          ((1 << HT_Bits) * sizeof(struct HTEntry)) / 1024,
          ((1 << PT_Bits) * sizeof(struct PTEntry)) / 1024,
          ((1 << ST_Bits) * sizeof(struct STEntry)) / 1024 );
}

void ShowHashStatistics(void)
{
    int i;
    int cnt = 0;
    struct HTEntry *h = TranspositionTable;

    for(i=0; i<HT_Size; i++, h++) {
        if((h->ht_Flags & HT_AGE) == HTGeneration) cnt++;
    }

    Print(1, "Hashtable 1:  entries = %d, use = %d\n",
           i, cnt);
}

void GuessHTSizes(char *size)
{
    int last = strlen(size)-1;
    long total_size;
    long tmp;

    if(size[last] == 'k') {
        total_size = atoi(size) * 1024;
    }
    else if(size[last] == 'm') {
        total_size = atoi(size) * 1024 * 1024;
    }
    else {
        total_size = atoi(size) * 1024;
    }

    if(total_size < 64*1024) {
        Print(0, "I need at least 64k of hashtables.\n");
        total_size = 64*1024;
    }

    tmp = total_size * 4 / 5;

    for(HT_Bits = 1; HT_Bits < 32; HT_Bits++) {
        long tmp2 = (1 << (HT_Bits+1)) * sizeof(struct HTEntry);
        if(tmp2 > tmp) break;
    }

    total_size -= (1 << HT_Bits) * sizeof(struct HTEntry);

    tmp = total_size / 2;

    for(PT_Bits = 1; PT_Bits < 32; PT_Bits++) {
        long tmp2 = (1 << (PT_Bits+1)) * sizeof(struct PTEntry);
        if(tmp2 > tmp) break;
    }

    total_size -= (1 << PT_Bits) * sizeof(struct PTEntry);

    for(ST_Bits = 1; ST_Bits < 32; ST_Bits++) {
        long tmp2 = (1 << (ST_Bits+1)) * sizeof(struct STEntry);
        if(tmp2 > total_size) break;
    }
}

void HashInit(void)
{
    int i,j,k;

    InitRandom(0);

    for(i=0; i<2; i++) {
        for(j=0; j<8; j++) {
            for(k=0; k<64; k++) {
                HashKeys[i][j][k] = Random64();
            }
        }
    }

    for(i=0; i<64; i++) {
        HashKeysEP[i] = Random64();
    }

    for(i=0; i<16; i++) {
        HashKeysCastle[i] = Random64();
    }


    STMKey = Random64();
}
