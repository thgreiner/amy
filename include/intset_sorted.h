#ifndef INTSET_SORTED_H
#define INTSET_SORTED_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifndef INTSET_SORTED_MAX
#define INTSET_SORTED_MAX 32
#endif

/*
 * IntSetSorted — a small, fixed-capacity set of int32_t, kept sorted.
 *
 * Same shape and API as IntSet (dense int32_t array + count, no allocation,
 * zero-copy access via intset_sorted_data), but the invariant is that
 * data[0..count) is sorted ascending.
 *
 *   - add / remove / contains: binary search + memmove.
 *   - union / intersect / diff: O(n+m) two-pointer merges.
 *
 * Trade-off vs IntSet: element order is by VALUE, not insertion time.
 */
typedef struct IntSetSorted {
    int32_t data[INTSET_SORTED_MAX];
    size_t count;
} IntSetSorted;

typedef enum {
    INTSET_SORTED_ADDED,
    INTSET_SORTED_PRESENT,
    INTSET_SORTED_FULL
} intset_sorted_result_t;

void intset_sorted_init(IntSetSorted *s);

intset_sorted_result_t intset_sorted_add(IntSetSorted *s, int32_t value);

bool intset_sorted_remove(IntSetSorted *s, int32_t value);

bool intset_sorted_contains(const IntSetSorted *s, int32_t value);

size_t intset_sorted_count(const IntSetSorted *s);

int32_t *intset_sorted_data(IntSetSorted *s); /* dense, sorted, count elems */

/* Result written to *dst (must be distinct from *a and *b).
 * union may return false if the result exceeds capacity (leaves *dst empty);
 * diff and intersect can never overflow. */
bool intset_sorted_diff(const IntSetSorted *a, const IntSetSorted *b,
                        IntSetSorted *dst);
bool intset_sorted_intersect(const IntSetSorted *a, const IntSetSorted *b,
                             IntSetSorted *dst);
bool intset_sorted_union(const IntSetSorted *a, const IntSetSorted *b,
                         IntSetSorted *dst);

#endif /* INTSET_SORTED_H */
