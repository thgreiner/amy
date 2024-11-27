/*

    Amy - a chess playing program

    Copyright (c) 2002-2024, Thorsten Greiner
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
 * mates.c - mate threat detection routines
 */

#include "amy.h"
#include <stdio.h>

#define MT_BITS 14
#define MT_SIZE (1 << MT_BITS)
#define MT_MASK (MT_SIZE - 1)

int MateThreat(struct Position *p, int side) {
    int oside = !side;
    int ekp = p->kingSq[oside];
    BitBoard pcs;
    BitBoard ksafe;
    int fr;

    ksafe = p->atkTo[ekp] & ~p->mask[oside][0];

    /*
     * Queen checks
     */

    pcs = p->mask[side][Queen];
    while (pcs) {
        int to;
        BitBoard mvs;
        fr = FindSetBit(pcs);
        pcs &= pcs - 1;
        mvs = (p->atkTo[fr] & QueenEPM[ekp]) & ~p->mask[side][0];
        while (mvs) {
            BitBoard tmp;
            to = FindSetBit(mvs);
            mvs &= mvs - 1;
            /* check whether path is obstructed */
            tmp = InterPath[ekp][to];
            if ((p->mask[White][0] & tmp) || (p->mask[Black][0] & tmp))
                continue;
            /* check wether all flight squares are covered */
            tmp = ksafe & ~QueenEPM[to];
            if (tmp) {
                int flight;
                int free = 0;
                do {
                    BitBoard att;
                    flight = FindSetBit(tmp);
                    tmp &= tmp - 1;
                    att = p->atkFr[flight] & p->mask[side][0];
                    ClrBit(att, fr);
                    if (!att)
                        free++;
                    if (free)
                        break;
                } while (tmp);
                if (free)
                    continue;
            }
            if (TstBit(p->atkTo[ekp], to)) {
                /* contact check */
                BitBoard ray;
                tmp = p->atkFr[to];
                ClrBit(tmp, fr);
                ClrBit(tmp, ekp);
                /* square is defended by opponent */
                if (p->mask[oside][0] & tmp)
                    continue;
                /* check if we have defenders 'from behind' */
                ray = Ray[to][fr] & p->atkFr[fr];
                if ((p->mask[oside][Queen] & ray) ||
                    (p->mask[oside][Rook] & ray) ||
                    (p->mask[oside][Bishop] & ray))
                    continue;
                /* If supported by a friendly piece, its mate! */
                if (p->mask[side][0] & tmp) {
                    return TRUE;
                }
                /* check for supporters 'from behind' */
                if ((p->mask[side][Bishop] & ray) ||
                    (p->mask[side][Rook] & ray) ||
                    (p->mask[side][Queen] & ray)) {
                    return TRUE;
                }
            } else {
                /* distant check */
                int inter;
                int def = 0;
                tmp = p->atkFr[to];
                ClrBit(tmp, fr);
                /* check if defended by opponent */
                if (p->mask[oside][0] & tmp)
                    continue;
                tmp = InterPath[to][ekp];
                while (tmp) {
                    BitBoard tmp2;
                    inter = FindSetBit(tmp);
                    tmp &= tmp - 1;
                    tmp2 = p->atkFr[inter] & p->mask[oside][0];
                    if (CountBits(tmp2) < 2)
                        continue;
                    def++;
                    break;
                }
                if (!def) {
                    return TRUE;
                }
            }
        }
    }

    /*
     * Rook checks
     */

    pcs = p->mask[side][Rook];
    while (pcs) {
        int to;
        BitBoard mvs;
        fr = FindSetBit(pcs);
        pcs &= pcs - 1;
        mvs = (p->atkTo[fr] & RookEPM[ekp]) & ~p->mask[side][0];
        while (mvs) {
            BitBoard tmp;
            to = FindSetBit(mvs);
            mvs &= mvs - 1;
            /* check whether path is obstructed */
            tmp = InterPath[ekp][to];
            if ((p->mask[White][0] & tmp) || (p->mask[Black][0] & tmp))
                continue;
            /* check wether all flight squares are covered */
            tmp = ksafe & ~RookEPM[to];
            if (tmp) {
                int flight;
                int free = 0;
                do {
                    BitBoard att;
                    flight = FindSetBit(tmp);
                    tmp &= tmp - 1;
                    att = p->atkFr[flight] & p->mask[side][0];
                    ClrBit(att, fr);
                    if (!att)
                        free++;
                    if (free)
                        break;
                } while (tmp);
                if (free)
                    continue;
            }
            if (TstBit(p->atkTo[ekp], to)) {
                /* contact check */
                BitBoard ray;
                tmp = p->atkFr[to];
                ClrBit(tmp, fr);
                ClrBit(tmp, ekp);
                /* square is defended by opponent */
                if (p->mask[oside][0] & tmp)
                    continue;
                /* check if we have defenders 'from behind' */
                ray = Ray[to][fr] & p->atkFr[fr];
                if ((p->mask[oside][Queen] & ray) ||
                    (p->mask[oside][Rook] & ray) ||
                    (p->mask[oside][Bishop] & ray))
                    continue;
                /* If supported by a friendly piece, its mate! */
                if (p->mask[side][0] & tmp) {
                    return TRUE;
                }
                /* check for supporters 'from behind' */
                if ((p->mask[side][Bishop] & ray) ||
                    (p->mask[side][Rook] & ray) ||
                    (p->mask[side][Queen] & ray)) {
                    return TRUE;
                }
            } else {
                /* distant check */
                int inter;
                int def = 0;
                tmp = p->atkFr[to];
                ClrBit(tmp, fr);
                /* check if defended by opponent */
                if (p->mask[oside][0] & tmp)
                    continue;
                tmp = InterPath[to][ekp];
                while (tmp) {
                    BitBoard tmp2;
                    inter = FindSetBit(tmp);
                    tmp &= tmp - 1;
                    tmp2 = p->atkFr[inter] & p->mask[oside][0];
                    if (CountBits(tmp2) < 2)
                        continue;
                    def++;
                    break;
                }
                if (!def) {
                    return TRUE;
                }
            }
        }
    }

    /*
     * Bishop checks
     */

    pcs = p->mask[side][Bishop];
    while (pcs) {
        int to;
        BitBoard mvs;
        fr = FindSetBit(pcs);
        pcs &= pcs - 1;
        mvs = (p->atkTo[fr] & BishopEPM[ekp]) & ~p->mask[side][0];
        while (mvs) {
            BitBoard tmp;
            to = FindSetBit(mvs);
            mvs &= mvs - 1;
            /* check whether path is obstructed */
            tmp = InterPath[ekp][to];
            if ((p->mask[White][0] & tmp) || (p->mask[Black][0] & tmp))
                continue;
            /* check wether all flight squares are covered */
            tmp = ksafe & ~BishopEPM[to];
            if (tmp) {
                int flight;
                int free = 0;
                do {
                    BitBoard att;
                    flight = FindSetBit(tmp);
                    tmp &= tmp - 1;
                    att = p->atkFr[flight] & p->mask[side][0];
                    ClrBit(att, fr);
                    if (!att)
                        free++;
                    if (free)
                        break;
                } while (tmp);
                if (free)
                    continue;
            }
            if (TstBit(p->atkTo[ekp], to)) {
                /* contact check */
                BitBoard ray;
                tmp = p->atkFr[to];
                ClrBit(tmp, fr);
                ClrBit(tmp, ekp);
                /* square is defended by opponent */
                if (p->mask[oside][0] & tmp)
                    continue;
                /* check if we have defenders 'from behind' */
                ray = Ray[to][fr] & p->atkFr[fr];
                if ((p->mask[oside][Queen] & ray) ||
                    (p->mask[oside][Rook] & ray) ||
                    (p->mask[oside][Bishop] & ray))
                    continue;
                /* If supported by a friendly piece, its mate! */
                if (p->mask[side][0] & tmp) {
                    return TRUE;
                }
                /* check for supporters 'from behind' */
                if ((p->mask[side][Bishop] & ray) ||
                    (p->mask[side][Rook] & ray) ||
                    (p->mask[side][Queen] & ray)) {
                    return TRUE;
                }
            } else {
                /* distant check */
                int inter;
                int def = 0;
                tmp = p->atkFr[to];
                ClrBit(tmp, fr);
                /* check if defended by opponent */
                if (p->mask[oside][0] & tmp)
                    continue;
                tmp = InterPath[to][ekp];
                while (tmp) {
                    BitBoard tmp2;
                    inter = FindSetBit(tmp);
                    tmp &= tmp - 1;
                    tmp2 = p->atkFr[inter] & p->mask[oside][0];
                    if (CountBits(tmp2) < 2)
                        continue;
                    def++;
                    break;
                }
                if (!def) {
                    return TRUE;
                }
            }
        }
    }

    /*
     * Knight checks
     */

    pcs = p->mask[side][Knight];
    while (pcs) {
        int to;
        BitBoard mvs;
        fr = FindSetBit(pcs);
        pcs &= pcs - 1;
        mvs = (p->atkTo[fr] & KnightEPM[ekp]) & ~p->mask[side][0];
        while (mvs) {
            BitBoard def;
            to = FindSetBit(mvs);
            mvs &= mvs - 1;
            /*
             * check whether the square is defended. If so, the defender
             * must not be pinned.
             */
            def = p->atkFr[to] & p->mask[oside][0];
            if (CountBits(def) == 1) {
                int de = FindSetBit(def);
                BitBoard tmp;
                if (RookEPM[ekp] & def) {
                    tmp = p->atkFr[de] & Ray[ekp][de];
                    if (!(p->mask[side][Queen] & tmp) &&
                        !(p->mask[side][Rook] & tmp))
                        continue;
                } else if (BishopEPM[ekp] & def) {
                    tmp = p->atkFr[de] & Ray[ekp][de];
                    if (!(p->mask[side][Queen] & tmp) &&
                        !(p->mask[side][Bishop] & tmp))
                        continue;
                } else
                    continue;
            } else if (def)
                continue;
            def = ksafe & ~KnightEPM[to];
            if (def) {
                int flight;
                int free = 0;
                do {
                    BitBoard att;
                    flight = FindSetBit(def);
                    def &= def - 1;
                    att = p->atkFr[flight] & p->mask[side][0];
                    ClrBit(att, fr);
                    if (!att)
                        free++;
                    if (free)
                        break;
                } while (def);
                if (free)
                    continue;
            }
            return TRUE;
        }
    }

    return FALSE;
}
