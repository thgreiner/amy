/*

    Amy - a chess playing program
    Copyright (C) 2002 Thorsten Greiner

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/

/*
 * swap.c - static exchange evaluation routines
 *
 * $Id: swap.c 27 2003-02-11 22:39:17Z thorsten $
 *
 */

#include <stdio.h>
#include "amy.h"

static int SwapValue[] = {
       0,
       100, /* Pawn */
       300, /* Knight */
       300, /* Bishop */
       500, /* Rook */
       900, /* Queen */
     10000  /* King, whose value is basically infinity */
};

static void SwapReRay(
    struct Position *p, int side, BitBoard atks[2], int from, int to, 
    BitBoard *exclude)
{
    BitBoard tmp;
    int i;
    int pc = TYPE(p->piece[from]);

    ClrBit(atks[side], from);
    ClrBit(*exclude, from);

    if(pc == Pawn || pc == Bishop || pc == Queen) {
        tmp = p->atkFr[from] & *exclude & Ray[to][from];
        if(tmp) {
            i=FindSetBit(tmp);
            if(TYPE(p->piece[i]) == Bishop || TYPE(p->piece[i]) == Queen) {
                if(p->piece[i] > 0) SetBit(atks[White], i);
                else                SetBit(atks[Black], i);
            }
        }
    }

    if(pc == Rook || pc == Queen) {
        tmp = p->atkFr[from] & *exclude & Ray[to][from];
        if(tmp) {
            i=FindSetBit(tmp);
            if(TYPE(p->piece[i]) == Rook || TYPE(p->piece[i]) == Queen) {
                if(p->piece[i] > 0) SetBit(atks[White], i);
                else                SetBit(atks[Black], i);
            }
        }
    }
}

int SwapOff(struct Position *p, int move) 
{
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


    if(move & M_PANY) {
        swapval     = SwapValue[PromoType(move)];
        swaplist[0] = SwapValue[TYPE(p->piece[to])] - SwapValue[Pawn] + swapval;
    }
    else {
        swapval     = SwapValue[TYPE(p->piece[fr])];
        swaplist[0] = SwapValue[TYPE(p->piece[to])];
    }

    swapside    = oside;
    
    atks[White] = p->mask[White][0] & p->atkFr[to];
    atks[Black] = p->mask[Black][0] & p->atkFr[to];

    exclude = p->mask[White][0] | p->mask[Black][0];

    SwapReRay(p, side, atks, fr, to, &exclude);

    while(atks[swapside]) {
        int at;
        BitBoard tmp;

        /* find last valuable attacker */
        tmp = p->mask[swapside][Pawn] & atks[swapside];
        if(tmp) at = FindSetBit(tmp);
        else {
            tmp = (p->mask[swapside][Knight] | p->mask[swapside][Bishop]) & 
                  atks[swapside];
            if(tmp) at = FindSetBit(tmp);
            else {
                tmp = p->mask[swapside][Rook] & atks[swapside];
                if(tmp) at = FindSetBit(tmp);
                else {
                    tmp = p->mask[swapside][Queen] & atks[swapside];
                    if(tmp) at = FindSetBit(tmp);
                    else    at = FindSetBit(p->mask[swapside][King]);
                }
            }
        }

        swapcnt++;
        swaplist[swapcnt] = swaplist[swapcnt-1] + swapsign*swapval;
        swapval = SwapValue[TYPE(p->piece[at])];
        swapsign = -swapsign;

        SwapReRay(p, swapside, atks, at, to, &exclude);

        swapside = !swapside;
    }

    if(swapcnt & 1) swapsign = -1;
    else            swapsign =  1;
    while(swapcnt) {
        if(swapsign < 0) {
            if(swaplist[swapcnt] <= swaplist[swapcnt-1]) 
                swaplist[swapcnt-1]=swaplist[swapcnt];
        }
        else {
            if(swaplist[swapcnt] >= swaplist[swapcnt-1]) 
                swaplist[swapcnt-1]=swaplist[swapcnt];
        }
        swapcnt--;
        swapsign = -swapsign;
    }

    return (swaplist[0]);
}
