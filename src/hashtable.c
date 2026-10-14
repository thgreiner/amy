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

/*
 * hashtable.c - hashtable management routines
 */

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "dbase.h"
#include "hashtable.h"
#include "random.h"
#include "safe_malloc.h"
#include "search.h"
#include "types.h"
#include "utils.h"

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

static int HT_Bits = 17;
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
static inline struct HTEntry ht_get_entry(hash_t key) {
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
static inline void ht_put_entry(hash_t key, struct HTEntry entry) {
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
 * Increment the processor counter of the entry matching 'key', working on
 * a fresh copy read under the write lock so no concurrent update is
 * clobbered. If the slot was meanwhile replaced by another position we
 * leave it alone.
 */
#if MP
static void ht_increment_cpus(hash_t key) {
#if HAVE_LIBPTHREAD
    atomic_int *mutex = TranspositionMutex + ((key >> 32) & MUTEX_MASK);
    acquire_write_lock(mutex);
#endif /* HAVE_LIBPTHREAD */

    struct HTEntry *h = &TranspositionTable[(key >> 32) & HT_Mask];

    if (h->ht_Key == key)
        h->ht_Flags += HT_NCPU_INCREMENT;

#if HAVE_LIBPTHREAD
    release_write_lock(mutex);
#endif /* HAVE_LIBPTHREAD */
}
#endif /* MP */

/**
 *
 */
static inline bool ht_put_entry_best_effort(const hash_t key, struct HTEntry entry,
                                        int depth) {
    const hash_t key1 = key;
    const hash_t key2 = key + 1;

    struct HTEntry entry1 = ht_get_entry(key1);
    struct HTEntry entry2 = ht_get_entry(key2);

    /* Overwrite any matching entry. */
    if (entry1.ht_Key == key) {
        ht_put_entry(key1, entry);
        return true;
    }
    if (entry2.ht_Key == key) {
        ht_put_entry(key2, entry);
        return true;
    }

    /* Overwrite entries with lower depth. */
    if (entry1.ht_Depth <= entry2.ht_Depth) {
        if (entry1.ht_Depth <= depth) {
            ht_put_entry(key1, entry);
            return true;
        }
        if (entry2.ht_Depth <= depth) {
            ht_put_entry(key2, entry);
            return true;
        }
    } else {
        if (entry2.ht_Depth <= depth) {
            ht_put_entry(key2, entry);
            return true;
        }
        if (entry1.ht_Depth <= depth) {
            ht_put_entry(key1, entry);
            return true;
        }
    }

    /* Overwrite entries from older generation. */
    if ((entry1.ht_Flags & HT_AGE) != HTGeneration) {
        ht_put_entry(key1, entry);
        return true;
    }
    if ((entry2.ht_Flags & HT_AGE) != HTGeneration) {
        ht_put_entry(key2, entry);
        return true;
    }

    return false;
}

#if MP
LookupResult ht_probe(const hash_t key, int *score, int depth, move_t *bestm,
                     bool *threat, int ply, int exclusiveP,
                     struct HTEntry *localHT)
#else
LookupResult ht_probe(const hash_t key, int *score, int depth, move_t *bestm,
                     bool *threat, int ply)
#endif
{
    hash_t effective_key = key;
    struct HTEntry h = ht_get_entry(effective_key);
    bool found = h.ht_Key == key;

    if (!found) {
        effective_key++;
        h = ht_get_entry(effective_key);
        found = h.ht_Key == key;
    }

    int result = Useless;

#if MP
    if (localHT != NULL && !found) {
        h = localHT[(key >> 32) & L_HT_Mask];
        found = h.ht_Key == key;
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
             * Correct a mate score. See comment in 'ht_store'.
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

                ht_increment_cpus(effective_key);
            }
#endif /* MP */

        } else {
            result = Useful;
        }
    }
#if MP
    else {
        h.ht_Depth = (short)depth;
        h.ht_Flags = HT_NCPU_INCREMENT;
        h.ht_Key = key;
        ht_put_entry_best_effort(key, h, depth);
    }
#endif /* MP */

    return result;
}

LookupResult ht_probe_pawn(hash_t key, int *score, struct PawnFacts *pf) {
#if MP && HAVE_LIBPTHREAD
    acquire_read_lock(PawnMutex + ((key >> 32) & MUTEX_MASK));
#endif /* MP && HAVE_LIBPTHREAD */

    struct PTEntry h = PawnTable[(key >> 32) & PT_Mask];

#if MP && HAVE_LIBPTHREAD
    release_read_lock(PawnMutex + ((key >> 32) & MUTEX_MASK));
#endif /* MP && HAVE_LIBPTHREAD */

    if (h.pt_Key == key && h.pt_Score != PT_INVALID) {
        *score = h.pt_Score;
        *pf = h.pt_PawnFacts;
        return Useful;
    }

    return Useless;
}

LookupResult ht_probe_score(hash_t key, int *score) {
#if MP && HAVE_LIBPTHREAD
    acquire_read_lock(ScoreMutex + ((key >> 32) & MUTEX_MASK));
#endif /* MP && HAVE_LIBPTHREAD */

    struct STEntry h = ScoreTable[(key >> 32) & ST_Mask];

#if MP && HAVE_LIBPTHREAD
    release_read_lock(ScoreMutex + ((key >> 32) & MUTEX_MASK));
#endif /* MP && HAVE_LIBPTHREAD */

    if (h.st_Key == key && h.st_Score != PT_INVALID) {
        *score = h.st_Score;
        return Useful;
    }

    return Useless;
}

void ht_store(hash_t key, int best, int alpha, int beta, int bestm, int depth,
             int threat, int ply
#if MP
             ,
             struct HTEntry *localHT
#endif
) {
    hash_t effective_key = key;
    struct HTEntry entry = ht_get_entry(effective_key);
    bool found = entry.ht_Key == key;

    if (!found) {
        effective_key++;
        entry = ht_get_entry(effective_key);
        found = entry.ht_Key == key;
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
    if (entry.ht_Key == key && depth == entry.ht_Depth) {
        if ((entry.ht_Flags & HT_NCPU) > 0) {
            entry.ht_Flags = (entry.ht_Flags & HT_NCPU) - HT_NCPU_INCREMENT;
        }
    } else {
        entry.ht_Key = key;
        entry.ht_Flags = 0;
    }
#else
    entry.ht_Key = key;
#endif /* MP */

    entry.ht_Move = bestm;
    entry.ht_Depth = (short)depth;
    entry.ht_Score = reduced;
#if MP
    entry.ht_Flags |= (short)HTGeneration;
#else
    entry.ht_Flags = (short)HTGeneration;
#endif /* MP */
    if (best <= alpha)
        entry.ht_Flags |= HT_UBOUND;
    else if (best >= beta)
        entry.ht_Flags |= HT_LBOUND;
    else
        entry.ht_Flags |= HT_EXACT;

    if (threat)
        entry.ht_Flags |= HT_THREAT;

    bool success = ht_put_entry_best_effort(key, entry, depth);
    if (!success) {
        HTStoreFailed++;
#if MP
        localHT[(key >> 32) & L_HT_Mask] = entry;
#endif
    }
}

void ht_store_pawn(hash_t key, int score, struct PawnFacts *pf) {
    struct PTEntry h = {.pt_Key = key, .pt_Score = score, .pt_PawnFacts = *pf};

#if MP && HAVE_LIBPTHREAD
    acquire_write_lock(PawnMutex + ((key >> 32) & MUTEX_MASK));
#endif /* MP && HAVE_LIBPTHREAD */

    PawnTable[(key >> 32) & PT_Mask] = h;

#if MP && HAVE_LIBPTHREAD
    release_write_lock(PawnMutex + ((key >> 32) & MUTEX_MASK));
#endif /* MP && HAVE_LIBPTHREAD */
}

void ht_store_score(hash_t key, int score) {
    struct STEntry h = {.st_Key = key, .st_Score = score};

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

void ht_clear(void) {
    unsigned int i;
    struct HTEntry *h = TranspositionTable;

    for (i = 0; i < HT_Size; i++, h++) {
        h->ht_Key = 0;
        h->ht_Flags = 0;
    }
}

void ht_age(void) {
    HTGeneration++;
    HTGeneration &= HT_AGE;

    HTStoreTried = 0;
    HTStoreFailed = 0;
}

void ht_clear_pawn(void) {
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

static void ht_free(void) {
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

void ht_allocate(void) {
    static bool registered_free_ht = false;

    /*
     * Register atexit() handler to free hashtable memory automatically
     */

    if (!registered_free_ht) {
        registered_free_ht = true;
        atexit(ht_free);
    }

    HT_Size = 1 << HT_Bits;
    HT_Mask = HT_Size - 1;

    TranspositionTable = safe_calloc(HT_Size, sizeof(struct HTEntry));

    /* Thread-local hash table - only calculate sizes and bits here...*/
    L_HT_Size = 1 << L_HT_Bits;
    L_HT_Mask = L_HT_Size - 1;

    PT_Size = 1 << PT_Bits;
    PT_Mask = PT_Size - 1;

    PawnTable = safe_calloc(PT_Size, sizeof(struct PTEntry));

    ST_Size = 1 << ST_Bits;
    ST_Mask = ST_Size - 1;

    ScoreTable = safe_calloc(ST_Size, sizeof(struct STEntry));

    print(0, "Hashtable sizes: %d k, %d k, %d k (%d, %d, %d bits)\n",
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

void ht_show_statistics(void) {
    unsigned int i;
    unsigned int cnt = 0;
    struct HTEntry *h = TranspositionTable;

    for (i = 0; i < HT_Size; i++, h++) {
        if ((h->ht_Flags & HT_AGE) == HTGeneration)
            cnt++;
    }

    char buf1[16], buf2[16];

    print(1, "Hashtable 1:  entries = %s, use = %s (%d %%)\n",
          utils_format_count(i, buf1, sizeof(buf1)),
          utils_format_count(cnt, buf2, sizeof(buf2)), utils_percentage(cnt, i));
    print(1, "              store failed = %s (%d %%)\n",
          utils_format_count(HTStoreFailed, buf1, sizeof(buf1)),
          utils_percentage(HTStoreFailed, HTStoreTried));
}

void ht_guess_sizes(char *size) {
    const int64_t min_size = 64LL * 1024LL;
    const int64_t max_size = 1024LL * 1024LL * 1024LL; /* 1 TB is plenty */
    int64_t total_size = min_size;
    int64_t budget;
    bool valid = false;

    if (size != NULL && *size != '\0') {
        char *endptr = NULL;
        long long value = strtoll(size, &endptr, 10);

        if (value > 0 && endptr != NULL) {
            int64_t multiplier = 1024LL; /* default: kilobytes */

            switch (*endptr) {
            case '\0':
                break;
            case 'k':
            case 'K':
                break;
            case 'm':
            case 'M':
                multiplier = 1024LL * 1024LL;
                break;
            case 'g':
            case 'G':
                multiplier = 1024LL * 1024LL * 1024LL;
                break;
            default:
                break; /* invalid suffix */
            }

            if (*endptr == '\0' || *endptr == 'k' || *endptr == 'K' ||
                *endptr == 'm' || *endptr == 'M' || *endptr == 'g' ||
                *endptr == 'G') {
                /* Avoid overflowing int64_t when multiplying. */
                if (value > max_size / multiplier)
                    value = max_size / multiplier;
                total_size = value * multiplier;
                valid = true;
            }
        }
    }

    if (!valid)
        print(0, "Invalid hashtable size, using 64k.\n");

    if (total_size < min_size) {
        print(0, "I need at least 64k of hashtables.\n");
        total_size = min_size;
    }

    if (total_size > max_size)
        total_size = max_size;

    budget = total_size * 4 / 5;

    for (HT_Bits = 1; HT_Bits < 31; HT_Bits++) {
        if ((1ULL << (HT_Bits + 1)) * (uint64_t)sizeof(struct HTEntry) >
            (uint64_t)budget)
            break;
    }

    total_size -= (1LL << HT_Bits) * sizeof(struct HTEntry);

    budget = total_size * 3 / 4;

    for (ST_Bits = 1; ST_Bits < 31; ST_Bits++) {
        if ((1ULL << (ST_Bits + 1)) * (uint64_t)sizeof(struct STEntry) >
            (uint64_t)budget)
            break;
    }

    total_size -= (1LL << ST_Bits) * sizeof(struct STEntry);

    for (PT_Bits = 1; PT_Bits < 31; PT_Bits++) {
        if ((1ULL << (PT_Bits + 1)) * (uint64_t)sizeof(struct PTEntry) >
            (uint64_t)total_size)
            break;
    }
}

void ht_init(void) {
    random_init(0);

    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 8; j++) {
            for (int k = 0; k < 64; k++) {
                HashKeys[i][j][k] = random64();
            }
        }
    }

    for (int i = 0; i < 64; i++) {
        HashKeysEP[i] = random64();
    }

    for (int i = 0; i < 16; i++) {
        HashKeysCastle[i] = random64();
    }

    STMKey = random64();
}
