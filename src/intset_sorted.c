#include "intset_sorted.h"

#include <string.h>

void intset_sorted_init(IntSetSorted *s) { s->count = 0; }

/* Index of the first slot whose value is >= v (lower bound). */
static size_t lower_bound(const IntSetSorted *s, int32_t v) {
    size_t lo = 0, hi = s->count;
    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;
        if (s->data[mid] < v)
            lo = mid + 1;
        else
            hi = mid;
    }
    return lo;
}

intset_sorted_result_t intset_sorted_add(IntSetSorted *s, int32_t value) {
    size_t pos = lower_bound(s, value);
    if (pos < s->count && s->data[pos] == value)
        return INTSET_SORTED_PRESENT;
    if (s->count >= INTSET_SORTED_MAX)
        return INTSET_SORTED_FULL;
    /* Open a gap by shifting the tail right (0 bytes when pos == count). */
    memmove(&s->data[pos + 1], &s->data[pos],
            (s->count - pos) * sizeof(int32_t));
    s->data[pos] = value;
    s->count++;
    return INTSET_SORTED_ADDED;
}

bool intset_sorted_contains(const IntSetSorted *s, int32_t value) {
    size_t pos = lower_bound(s, value);
    return pos < s->count && s->data[pos] == value;
}

bool intset_sorted_remove(IntSetSorted *s, int32_t value) {
    size_t pos = lower_bound(s, value);
    if (pos >= s->count || s->data[pos] != value)
        return false;
    size_t tail = s->count - pos - 1;
    if (tail)
        memmove(&s->data[pos], &s->data[pos + 1], tail * sizeof(int32_t));
    s->count--;
    return true;
}

size_t intset_sorted_count(const IntSetSorted *s) { return s->count; }

int32_t *intset_sorted_data(IntSetSorted *s) { return s->data; }

/* a - b, two-pointer merge. Result is sorted and <= a->count, so it can't
 * overflow. */
bool intset_sorted_diff(const IntSetSorted *a, const IntSetSorted *b,
                        IntSetSorted *dst) {
    IntSetSorted tmp;
    tmp.count = 0;
    size_t i = 0, j = 0;
    while (i < a->count && j < b->count) {
        if (a->data[i] < b->data[j])
            tmp.data[tmp.count++] = a->data[i++];
        else if (a->data[i] > b->data[j])
            j++;
        else {
            i++;
            j++;
        } /* equal: excluded from diff */
    }
    while (i < a->count)
        tmp.data[tmp.count++] = a->data[i++];
    *dst = tmp;
    return true;
}

/* a ∩ b, two-pointer merge. */
bool intset_sorted_intersect(const IntSetSorted *a, const IntSetSorted *b,
                             IntSetSorted *dst) {
    IntSetSorted tmp;
    tmp.count = 0;
    size_t i = 0, j = 0;
    while (i < a->count && j < b->count) {
        if (a->data[i] < b->data[j])
            i++;
        else if (a->data[i] > b->data[j])
            j++;
        else {
            tmp.data[tmp.count++] = a->data[i];
            i++;
            j++;
        }
    }
    *dst = tmp;
    return true;
}

/* a ∪ b, two-pointer merge (dedupes on equal). Can overflow -> may fail. */
bool intset_sorted_union(const IntSetSorted *a, const IntSetSorted *b,
                         IntSetSorted *dst) {
    IntSetSorted tmp;
    tmp.count = 0;
    size_t i = 0, j = 0;
    while (i < a->count && j < b->count) {
        if (tmp.count >= INTSET_SORTED_MAX) {
            dst->count = 0;
            return false;
        }
        if (a->data[i] < b->data[j])
            tmp.data[tmp.count++] = a->data[i++];
        else if (a->data[i] > b->data[j])
            tmp.data[tmp.count++] = b->data[j++];
        else {
            tmp.data[tmp.count++] = a->data[i];
            i++;
            j++;
        }
    }
    while (i < a->count) {
        if (tmp.count >= INTSET_SORTED_MAX) {
            dst->count = 0;
            return false;
        }
        tmp.data[tmp.count++] = a->data[i++];
    }
    while (j < b->count) {
        if (tmp.count >= INTSET_SORTED_MAX) {
            dst->count = 0;
            return false;
        }
        tmp.data[tmp.count++] = b->data[j++];
    }
    *dst = tmp;
    return true;
}
