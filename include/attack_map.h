#ifndef ATTACK_MAP_H
#define ATTACK_MAP_H

#include "dbase.h"
#include "intset_sorted.h"

#define N_ATTACK_FEATURES (24 * 4096)

void am_compute_attack_maps(struct Position *, IntSetSorted *, IntSetSorted *);

/*
 * Attack feature indices of one side into out (count, or -1 on
 * overflow of cap).  Single definition, shared with nn_extract.c.
 */
int am_compute_attack_indices(const struct Position *p, int side, int *out,
                              int cap);

#endif
