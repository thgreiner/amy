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

/*
 * hashtable.c - hashtable management routines
 */

#include "amy.h"

#define HT_AGE (0x3f)
#define HT_NCPU ((0x3f) << 6)
#define HT_NCPU_INCREMENT (1 << 6)
#define HT_THREAT (1 << 12)
#define HT_EXACT (1 << 13)
#define HT_LBOUND (1 << 14)
#define HT_UBOUND (1 << 15)

#define PT_INVALID 0xffff

hash_t HashKeys[2][8][64];
hash_t HashKeysEP[64];
hash_t HashKeysCastle[16];
hash_t STMKey;

int HT_Bits = 17;
static int PT_Bits = 15;
static int ST_Bits = 15;

static unsigned int HT_Size, HT_Mask;
static unsigned int PT_Size, PT_Mask;
static unsigned int ST_Size, ST_Mask;

int L_HT_Bits = 16, L_HT_Size, L_HT_Mask;

static struct HTEntry *TranspositionTable = NULL;
static struct PTEntry *PawnTable = NULL;
static struct STEntry *ScoreTable = NULL;
static int HTGeneration = 0;

static OPTIONAL_ATOMIC unsigned int HTStoreFailed = 0, HTStoreTried = 0;

#if MP && HAVE_LIBPTHREAD

#define MUTEX_BITS 8
#define MUTEX_COUNT (1 << MUTEX_BITS)
#define MUTEX_MASK (MUTEX_COUNT - 1)
static atomic_int TranspositionMutex[MUTEX_COUNT];
static atomic_int PawnMutex[MUTEX_COUNT];
static atomic_int ScoreMutex[MUTEX_COUNT];

/**
 * Acquire a read lock for the given pointer. There can be many read locks,
 * but only a single write lock.
 */
static void acquire_read_lock(atomic_int *data) {
    for (;;) {
        int val = *data;
        if (val >= 0) {
            bool result = atomic_compare_exchange_strong(data, &val, val + 1);
            if (result)
                return;
        }
    }
}

/**
 * Release the read lock.
 */
static void release_read_lock(atomic_int *data) {
    for (;;) {
        int val = *data;
        if (val > 0) {
            bool result = atomic_compare_exchange_strong(data, &val, val - 1);
            if (result)
                return;
        }
    }
}

/**
 * Acquire a write lock for the given pointer. There can be many read locks,
 * but only a single write lock.
 */
static void acquire_write_lock(atomic_int *data) {
    for (;;) {
        int val = *data;
        if (val == 0) {
            bool result = atomic_compare_exchange_strong(data, &val, -1);
            if (result)
                return;
        }
    }
}

/**
 * Release the write lock.
 */
static void release_write_lock(atomic_int *data) {
    for (;;) {
        int val = *data;
        if (val == -1) {
            bool result = atomic_compare_exchange_strong(data, &val, 0);
            if (result)
                return;
        }
    }
}

#endif

/**
 * Gets an entry from the global transposition table.
 */
static inline struct HTEntry GetHTEntry(hash_t key) {
#if MP && HAVE_LIBPTHREAD
    atomic_int *mutex = TranspositionMutex + ((key >> 32) & MUTEX_MASK);
    acquire_read_lock(mutex);
#endif /* MP && HAVE_LIBPTHREAD */

    struct HTEntry entry = TranspositionTable[(key >> 32) & HT_Mask];

#if MP && HAVE_LIBPTHREAD
    release_read_lock(mutex);
#endif /* MP && HAVE_LIBPTHREAD */

    return entry;
}

/**
 * Puts an entry to the global transposition table.
 */
static inline void PutHTEntry(hash_t key, struct HTEntry entry) {
#if MP && HAVE_LIBPTHREAD
    atomic_int *mutex = TranspositionMutex + ((key >> 32) & MUTEX_MASK);
    acquire_write_lock(mutex);
#endif /* MP && HAVE_LIBPTHREAD */

    TranspositionTable[(key >> 32) & HT_Mask] = entry;

#if MP && HAVE_LIBPTHREAD
    release_write_lock(mutex);
#endif /* MP && HAVE_LIBPTHREAD */
}

/**
 *
 */
static inline bool PutHTEntryBestEffort(hash_t key, struct HTEntry entry,
                                        int depth) {
    const hash_t key1 = key;
    const hash_t key2 = key + 1;

    struct HTEntry entry1 = GetHTEntry(key1);
    struct HTEntry entry2 = GetHTEntry(key2);

    /* Overwrite any matching entry. */
    if (entry1.ht_Signature == (unsigned int)key) {
        PutHTEntry(key1, entry);
        return true;
    }
    if (entry2.ht_Signature == (unsigned int)key) {
        PutHTEntry(key2, entry);
        return true;
    }

    /* Overwrite entries with lower depth. */
    if (entry1.ht_Depth <= entry2.ht_Depth) {
        if (entry1.ht_Depth <= depth) {
            PutHTEntry(key1, entry);
            return true;
        }
        if (entry2.ht_Depth <= depth) {
            PutHTEntry(key2, entry);
            return true;
        }
    } else {
        if (entry2.ht_Depth <= depth) {
            PutHTEntry(key2, entry);
            return true;
        }
        if (entry1.ht_Depth <= depth) {
            PutHTEntry(key1, entry);
            return true;
        }
    }

    /* Overwrite entries from older generation. */
    if ((entry1.ht_Flags & HT_AGE) != HTGeneration) {
        PutHTEntry(key1, entry);
        return true;
    }
    if ((entry2.ht_Flags & HT_AGE) != HTGeneration) {
        PutHTEntry(key2, entry);
        return true;
    }

    return false;
}

#if MP
LookupResult ProbeHT(hash_t key, int *score, int depth, move_t *bestm,
                     bool *threat, int ply, int exclusiveP,
                     struct HTEntry *localHT)
#else
LookupResult ProbeHT(hash_t key, int *score, int depth, move_t *bestm,
                     bool *threat, int ply)
#endif
{
    hash_t effective_key = key;
    struct HTEntry h = GetHTEntry(effective_key);
    bool found = h.ht_Signature == (unsigned int)key;

    if (!found) {
        effective_key++;
        h = GetHTEntry(effective_key);
        found = h.ht_Signature == (unsigned int)key;
    }

    int result = Useless;

#if MP
    if (localHT != NULL && !found) {
        h = localHT[(key >> 32) & L_HT_Mask];
        found = h.ht_Signature == (unsigned int)key;
    }
#endif

    if (found) {
        *bestm = h.ht_Move;
        *threat = (h.ht_Flags & HT_THREAT);

#if MP
        if ((int)h.ht_Depth == depth && exclusiveP &&
            (h.ht_Flags & HT_NCPU) > 0) {

            result = OnEvaluation;

        } else
#endif

            if ((int)h.ht_Depth >= depth) {
            *score = h.ht_Score;

            /*
             * Correct a mate score. See comment in 'StoreHT'.
             */

            if (*score > CMLIMIT) {
                *score -= ply;
            } else if (*score < -CMLIMIT) {
                *score += ply;
            }

            if (h.ht_Flags & HT_EXACT) {
                result = ExactScore;
            } else if (h.ht_Flags & HT_LBOUND) {
                result = LowerBound;
            } else if (h.ht_Flags & HT_UBOUND) {
                result = UpperBound;
            }

#if MP
            if ((int)h.ht_Depth == depth) {

                /*
                 * increment processor count
                 */

                h.ht_Flags += HT_NCPU_INCREMENT;
                PutHTEntry(effective_key, h);
            }
#endif /* MP */

        } else {
            result = Useful;
        }
    }
#if MP
    else {
        h.ht_Depth = depth;
        h.ht_Flags = HT_NCPU_INCREMENT;
        h.ht_Signature = (int)key;
        PutHTEntryBestEffort(key, h, depth);
    }
#endif /* MP */

    return result;
}

LookupResult ProbePT(hash_t key, int *score, struct PawnFacts *pf) {
#if MP && HAVE_LIBPTHREAD
    acquire_read_lock(PawnMutex + ((key >> 32) & MUTEX_MASK));
#endif /* MP && HAVE_LIBPTHREAD */

    struct PTEntry h = PawnTable[(key >> 32) & PT_Mask];

#if MP && HAVE_LIBPTHREAD
    release_read_lock(PawnMutex + ((key >> 32) & MUTEX_MASK));
#endif /* MP && HAVE_LIBPTHREAD */

    if (h.pt_Signature == (unsigned int)key && h.pt_Score != PT_INVALID) {
        *score = h.pt_Score;
        *pf = h.pt_PawnFacts;
        return Useful;
    }

    return Useless;
}

LookupResult ProbeST(hash_t key, int *score) {
#if MP && HAVE_LIBPTHREAD
    acquire_read_lock(ScoreMutex + ((key >> 32) & MUTEX_MASK));
#endif /* MP && HAVE_LIBPTHREAD */

    struct STEntry h = ScoreTable[(key >> 32) & ST_Mask];

#if MP && HAVE_LIBPTHREAD
    release_read_lock(ScoreMutex + ((key >> 32) & MUTEX_MASK));
#endif /* MP && HAVE_LIBPTHREAD */

    if (h.st_Signature == (unsigned int)key && h.st_Score != PT_INVALID) {
        *score = h.st_Score;
        return Useful;
    }

    return Useless;
}

void StoreHT(hash_t key, int best, int alpha, int beta, int bestm, int depth,
             int threat, int ply
#if MP
             ,
             struct HTEntry *localHT
#endif
) {
    hash_t effective_key = key;
    struct HTEntry entry = GetHTEntry(effective_key);
    bool found = entry.ht_Signature == (unsigned int)key;

    if (!found) {
        effective_key++;
        entry = GetHTEntry(effective_key);
        found = entry.ht_Signature == (unsigned int)key;
    }

    HTStoreTried++;

#if MP
    if (!found) {
        entry = localHT[(key >> 32) & L_HT_Mask];
    }
#endif

    int reduced = best;

    /*
     * Handling of mate scores is a bit tricky.
     * Upon storing we correct it to mean 'mate in n from this position'
     * instead of 'mate in n from root position'. Upon retrieval this has
     * to be corrected.
     */

    if (best > CMLIMIT) {
        reduced += ply;
    } else if (best < -CMLIMIT) {
        reduced -= ply;
    }

#if MP
    if (entry.ht_Signature == (unsigned int)key && depth == entry.ht_Depth) {
        if ((entry.ht_Flags & HT_NCPU) > 0) {
            entry.ht_Flags = (entry.ht_Flags & HT_NCPU) - HT_NCPU_INCREMENT;
        }
    } else {
        entry.ht_Signature = (unsigned int)key;
        entry.ht_Flags = 0;
    }
#else
    entry.ht_Signature = (unsigned int)key;
#endif /* MP */

    entry.ht_Move = bestm;
    entry.ht_Depth = depth;
    entry.ht_Score = reduced;
#if MP
    entry.ht_Flags |= HTGeneration;
#else
    entry.ht_Flags = HTGeneration;
#endif /* MP */
    if (best <= alpha)
        entry.ht_Flags |= HT_UBOUND;
    else if (best >= beta)
        entry.ht_Flags |= HT_LBOUND;
    else
        entry.ht_Flags |= HT_EXACT;

    if (threat)
        entry.ht_Flags |= HT_THREAT;

    bool success = PutHTEntryBestEffort(key, entry, depth);
    if (!success) {
        HTStoreFailed++;
#if MP
        localHT[(key >> 32) & L_HT_Mask] = entry;
#endif
    }
}

void StorePT(hash_t key, int score, struct PawnFacts *pf) {
    struct PTEntry h = {.pt_Signature = (unsigned int)key,
                        .pt_Score = score,
                        .pt_PawnFacts = *pf};

#if MP && HAVE_LIBPTHREAD
    acquire_write_lock(PawnMutex + ((key >> 32) & MUTEX_MASK));
#endif /* MP && HAVE_LIBPTHREAD */

    PawnTable[(key >> 32) & PT_Mask] = h;

#if MP && HAVE_LIBPTHREAD
    release_write_lock(PawnMutex + ((key >> 32) & MUTEX_MASK));
#endif /* MP && HAVE_LIBPTHREAD */
}

void StoreST(hash_t key, int score) {
    struct STEntry h = {.st_Signature = (unsigned int)key, .st_Score = score};

#if MP && HAVE_LIBPTHREAD
    acquire_write_lock(ScoreMutex + ((key >> 32) & MUTEX_MASK));
#endif /* MP && HAVE_LIBPTHREAD */

    ScoreTable[(key >> 32) & ST_Mask] = h;

#if MP && HAVE_LIBPTHREAD
    release_write_lock(ScoreMutex + ((key >> 32) & MUTEX_MASK));
#endif /* MP && HAVE_LIBPTHREAD */
}

/* Moved this to a seperate routine to make the PB-Move
 * selection work better..
 */

void ClearHashTable(void) {
    unsigned int i;
    struct HTEntry *h = TranspositionTable;

    for (i = 0; i < HT_Size; i++, h++) {
        h->ht_Signature = 0;
        h->ht_Flags = 0;
    }
}

void AgeHashTable(void) {
    HTGeneration++;
    HTGeneration &= HT_AGE;

    HTStoreTried = 0;
    HTStoreFailed = 0;
}

void ClearPawnHashTable(void) {
    unsigned int i;
    struct PTEntry *ph;
    struct STEntry *sh;

    ph = PawnTable;
    for (i = 0; i < PT_Size; i++, ph++) {
        ph->pt_Score = PT_INVALID;
    }

    sh = ScoreTable;
    for (i = 0; i < ST_Size; i++, sh++) {
        sh->st_Score = PT_INVALID;
    }
}

static void FreeHT(void) {
    if (TranspositionTable) {
        free(TranspositionTable);
        TranspositionTable = NULL;
    }

    if (PawnTable) {
        free(PawnTable);
        PawnTable = NULL;
    }

    if (ScoreTable) {
        free(ScoreTable);
        ScoreTable = NULL;
    }
}

void AllocateHT(void) {
    static bool registered_free_ht = false;

    /*
     * Register atexit() handler to free hashtable memory automatically
     */

    if (!registered_free_ht) {
        registered_free_ht = true;
        atexit(FreeHT);
    }

    HT_Size = 1 << HT_Bits;
    HT_Mask = HT_Size - 1;

    TranspositionTable = calloc(HT_Size, sizeof(struct HTEntry));

    /* Thread-local hash table - only calculate sizes and bits here...*/
    L_HT_Size = 1 << L_HT_Bits;
    L_HT_Mask = L_HT_Size - 1;

    PT_Size = 1 << PT_Bits;
    PT_Mask = PT_Size - 1;

    PawnTable = calloc(PT_Size, sizeof(struct PTEntry));

    ST_Size = 1 << ST_Bits;
    ST_Mask = ST_Size - 1;

    ScoreTable = calloc(ST_Size, sizeof(struct STEntry));

    Print(0, "Hashtable sizes: %d k, %d k, %d k (%d, %d, %d bits)\n",
          ((1 << HT_Bits) * sizeof(struct HTEntry)) / 1024,
          ((1 << PT_Bits) * sizeof(struct PTEntry)) / 1024,
          ((1 << ST_Bits) * sizeof(struct STEntry)) / 1024, HT_Bits, PT_Bits,
          ST_Bits);

#if MP && HAVE_LIBPTHREAD
    for (int i = 0; i < MUTEX_COUNT; i++) {
        TranspositionMutex[i] = 0;
        PawnMutex[i] = 0;
        ScoreMutex[i] = 0;
    }
#endif
}

void ShowHashStatistics(void) {
    unsigned int i;
    unsigned int cnt = 0;
    struct HTEntry *h = TranspositionTable;

    for (i = 0; i < HT_Size; i++, h++) {
        if ((h->ht_Flags & HT_AGE) == HTGeneration)
            cnt++;
    }

    char buf1[16], buf2[16];

    Print(1, "Hashtable 1:  entries = %s, use = %s (%d %%)\n",
          FormatCount(i, buf1, sizeof(buf1)),
          FormatCount(cnt, buf2, sizeof(buf2)), Percentage(cnt, i));
    Print(1, "              store failed = %s (%d %%)\n",
          FormatCount(HTStoreFailed, buf1, sizeof(buf1)),
          Percentage(HTStoreFailed, HTStoreTried));
}

void GuessHTSizes(char *size) {
    int last = strlen(size) - 1;
    int64_t total_size;
    int64_t tmp;

    if (size[last] == 'k') {
        total_size = atoi(size) * 1024L;
    } else if (size[last] == 'm') {
        total_size = atoi(size) * 1024L * 1024L;
    } else {
        total_size = atoi(size) * 1024;
    }

    if (total_size < 64 * 1024) {
        Print(0, "I need at least 64k of hashtables.\n");
        total_size = 64 * 1024;
    }

    tmp = total_size * 4 / 5;

    for (HT_Bits = 1; HT_Bits < 32; HT_Bits++) {
        long tmp2 = (1 << (HT_Bits + 1)) * sizeof(struct HTEntry);
        if (tmp2 > tmp)
            break;
    }

    total_size -= (1 << HT_Bits) * sizeof(struct HTEntry);

    tmp = 3 * total_size / 4;

    for (ST_Bits = 1; ST_Bits < 32; ST_Bits++) {
        long tmp2 = (1 << (ST_Bits + 1)) * sizeof(struct STEntry);
        if (tmp2 > tmp)
            break;
    }

    total_size -= (1 << ST_Bits) * sizeof(struct STEntry);

    for (PT_Bits = 1; PT_Bits < 32; PT_Bits++) {
        long tmp2 = (1 << (PT_Bits + 1)) * sizeof(struct PTEntry);
        if (tmp2 > total_size)
            break;
    }
}

void HashInit(void) {
    int i, j, k;

    InitRandom(0);

    for (i = 0; i < 2; i++) {
        for (j = 0; j < 8; j++) {
            for (k = 0; k < 64; k++) {
                HashKeys[i][j][k] = Random64();
            }
        }
    }

    for (i = 0; i < 64; i++) {
        HashKeysEP[i] = Random64();
    }

    for (i = 0; i < 16; i++) {
        HashKeysCastle[i] = Random64();
    }

    STMKey = Random64();
}
