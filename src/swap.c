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
 * swap.c - static exchange evaluation routines
 */

#include "dbase.h"
#include "init.h"
#include "inline.h"

static int SwapValue[] = {
    0,    100, /* Pawn */
    300,       /* Knight */
    300,       /* Bishop */
    500,       /* Rook */
    900,       /* Queen */
    10000      /* King, whose value is basically infinity */
};

static void swap_re_ray(struct Position *p, int side, BitBoard atks[2], int from,
                      int to, BitBoard *exclude) {
    BitBoard tmp;
    int i;
    int pc = TYPE(p->piece[from]);

    bitboard_clr_bit(atks[side], from);
    bitboard_clr_bit(*exclude, from);

    if (pc == Pawn || pc == Bishop || pc == Queen) {
        tmp = p->atkFr[from] & *exclude & Ray[to][from];
        if (tmp) {
            i = bitboard_find_set_bit(tmp);
            if (TYPE(p->piece[i]) == Bishop || TYPE(p->piece[i]) == Queen) {
                if (p->piece[i] > 0) {
                    bitboard_set_bit(atks[White], i);
                } else {
                    bitboard_set_bit(atks[Black], i);
                }
            }
        }
    }

    if (pc == Rook || pc == Queen) {
        tmp = p->atkFr[from] & *exclude & Ray[to][from];
        if (tmp) {
            i = bitboard_find_set_bit(tmp);
            if (TYPE(p->piece[i]) == Rook || TYPE(p->piece[i]) == Queen) {
                if (p->piece[i] > 0) {
                    bitboard_set_bit(atks[White], i);
                } else {
                    bitboard_set_bit(atks[Black], i);
                }
            }
        }
    }
}

int swap_off(struct Position *p, int move) {
    int to = M_TO(move);
    int fr = M_FROM(move);
    int side = COLOR(p->piece[fr]);
    int oside = !side;
    int swaplist[32];
    int swapcnt = 0;
    int swapval, swapside;
    int swapsign = -1;

    BitBoard atks[2];
    BitBoard exclude;

    if (move & M_PROMOTION_MASK) {
        swapval = SwapValue[dbase_promo_type(move)];
        swaplist[0] = SwapValue[TYPE(p->piece[to])] - SwapValue[Pawn] + swapval;
    } else {
        swapval = SwapValue[TYPE(p->piece[fr])];
        swaplist[0] = SwapValue[TYPE(p->piece[to])];
    }

    swapside = oside;

    atks[White] = p->mask[White][0] & p->atkFr[to];
    atks[Black] = p->mask[Black][0] & p->atkFr[to];

    exclude = p->mask[White][0] | p->mask[Black][0];

    swap_re_ray(p, side, atks, fr, to, &exclude);

    while (atks[swapside]) {
        int at;
        BitBoard tmp;

        /* find last valuable attacker */
        tmp = p->mask[swapside][Pawn] & atks[swapside];
        if (tmp)
            at = bitboard_find_set_bit(tmp);
        else {
            tmp = (p->mask[swapside][Knight] | p->mask[swapside][Bishop]) &
                  atks[swapside];
            if (tmp)
                at = bitboard_find_set_bit(tmp);
            else {
                tmp = p->mask[swapside][Rook] & atks[swapside];
                if (tmp)
                    at = bitboard_find_set_bit(tmp);
                else {
                    tmp = p->mask[swapside][Queen] & atks[swapside];
                    if (tmp)
                        at = bitboard_find_set_bit(tmp);
                    else
                        at = bitboard_find_set_bit(p->mask[swapside][King]);
                }
            }
        }

        swapcnt++;
        swaplist[swapcnt] = swaplist[swapcnt - 1] + swapsign * swapval;
        swapval = SwapValue[TYPE(p->piece[at])];
        swapsign = -swapsign;

        swap_re_ray(p, swapside, atks, at, to, &exclude);

        swapside = !swapside;
    }

    if (swapcnt & 1)
        swapsign = -1;
    else
        swapsign = 1;
    while (swapcnt) {
        if (swapsign < 0) {
            if (swaplist[swapcnt] <= swaplist[swapcnt - 1])
                swaplist[swapcnt - 1] = swaplist[swapcnt];
        } else {
            if (swaplist[swapcnt] >= swaplist[swapcnt - 1])
                swaplist[swapcnt - 1] = swaplist[swapcnt];
        }
        swapcnt--;
        swapsign = -swapsign;
    }

    return (swaplist[0]);
}
