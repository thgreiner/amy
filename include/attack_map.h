#ifndef ATTACK_MAP_H
#define ATTACK_MAP_H

#include "dbase.h"
#include "intset_sorted.h"

#define N_ATTACK_FEATURES (24 * 4096)

void am_compute_attack_maps(struct Position *, IntSetSorted *, IntSetSorted *);

#endif
