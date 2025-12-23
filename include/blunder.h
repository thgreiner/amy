#ifndef BLUNDER_H
#define BLUNDER_H

#include "types.h"
#include "dbase.h"

move_t get_best_move_from_comment(char *, struct Position *, char *);
void BlunderCheck(char *);

#endif
