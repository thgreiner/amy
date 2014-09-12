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
 * mates.c - mate threat detection routines
 *
 * $Id: mates.c 27 2003-02-11 22:39:17Z thorsten $
 *
 */

#include <stdio.h>
#include "amy.h"

#define MT_BITS 14
#define MT_SIZE (1 << MT_BITS)
#define MT_MASK (MT_SIZE-1)

int MateThreat(struct Position *p, int side)
{
    int oside = !side;
    int ekp = p->kingSq[oside];
    BitBoard pcs;
    BitBoard ksafe;
    int fr;

    ksafe = p->atkTo[ekp] & ~ p->mask[oside][0];

    /*
     * Queen checks 
     */

    pcs =  p->mask[side][Queen];
    while(pcs) {
	int to;
	BitBoard mvs;
        fr = FindSetBit(pcs); ClrBit(pcs, fr);
        mvs = (p->atkTo[fr] & QueenEPM[ekp]) & ~ p->mask[side][0];
        while(mvs) {
	    	BitBoard tmp;
            to=FindSetBit(mvs); ClrBit(mvs, to);
	    	/* check wether path is obstructed */
	    	tmp = InterPath[ekp][to];
	    	if((p->mask[White][0] & tmp) || (p->mask[Black][0] & tmp)) continue;
	    	/* check wether all flight squares are covered */
            tmp = ksafe & ~QueenEPM[to];
	    	if(tmp) {
				int flight;
				int free = 0;
                do {
		    		BitBoard att;
                    flight = FindSetBit(tmp); ClrBit(tmp, flight);
                    att = p->atkFr[flight] & p->mask[side][0];
		    		ClrBit(att, fr);
		    		if(!att) free++;
		    		if(free) break;
				} while(tmp);
				if(free) continue;
	    	}
	    	if(TstBit(p->atkTo[ekp], to)) {
				/* contact check */
				BitBoard ray;
                tmp = p->atkFr[to];
				ClrBit(tmp, fr);
				ClrBit(tmp, ekp);
                /* square is defended by opponent */
				if(p->mask[oside][0] & tmp) continue;
                /* check if we have defenders 'from behind' */
                ray = Ray[to][fr] & p->atkFr[fr];
				if((p->mask[oside][Queen] & ray) ||
		   			(p->mask[oside][Rook] & ray) ||
		   			(p->mask[oside][Bishop] & ray)) continue;
                /* If supported by a friendly piece, its mate! */
				if(p->mask[side][0] & tmp) {
                    return TRUE;
                }
                /* check for supporters 'from behind' */
				if((p->mask[side][Bishop] & ray) ||
		   			(p->mask[side][Rook] & ray) ||
                   (p->mask[side][Queen] & ray)) {
                    return TRUE;
                }
	    	}
	    	else {
				/* distant check */
				int inter;
				int def = 0;
				tmp = p->atkFr[to];
				ClrBit(tmp, fr);
                /* check if defended by opponent */
				if(p->mask[oside][0] & tmp) continue;
				tmp = InterPath[to][ekp];
                while(tmp) { 
		    		BitBoard tmp2;
                    inter=FindSetBit(tmp); ClrBit(tmp, inter);
		    		tmp2 = p->atkFr[inter] & p->mask[oside][0];
		    		if(CountBits(tmp2) < 2) continue;
		    		def++; break;
				}
				if(!def) {
                    return TRUE;
                }
	    	}
		}
    }

    /*
     * Rook checks 
     */

    pcs = p->mask[side][Rook];
    while(pcs) {
	int to;
	BitBoard mvs;
        fr = FindSetBit(pcs); ClrBit(pcs, fr);
        mvs = (p->atkTo[fr] & RookEPM[ekp]) & ~p->mask[side][0];
        while(mvs) {
	    BitBoard tmp;
            to=FindSetBit(mvs); ClrBit(mvs, to);
	    /* check wether path is obstructed */
	    tmp = InterPath[ekp][to];
	    if((p->mask[White][0] & tmp) || (p->mask[Black][0] & tmp)) continue;
	    /* check wether all flight squares are covered */
            tmp = ksafe & ~RookEPM[to];
	    if(tmp) {
		int flight;
		int free = 0;
                do {
		    BitBoard att;
                    flight = FindSetBit(tmp); ClrBit(tmp, flight);
                    att = p->atkFr[flight] & p->mask[side][0];
		    ClrBit(att, fr);
		    if(!att) free++;
		    if(free) break;
		} while(tmp);
		if(free) continue;
	    }
	    if(TstBit(p->atkTo[ekp], to)) {
		/* contact check */
		BitBoard ray;
                tmp = p->atkFr[to];
		ClrBit(tmp, fr);
		ClrBit(tmp, ekp);
                /* square is defended by opponent */
		if(p->mask[oside][0] & tmp) continue;
                /* check if we have defenders 'from behind' */
                ray = Ray[to][fr] & p->atkFr[fr];
		if((p->mask[oside][Queen] & ray) ||
		   (p->mask[oside][Rook] & ray) ||
		   (p->mask[oside][Bishop] & ray)) continue;
                /* If supported by a friendly piece, its mate! */
		if(p->mask[side][0] & tmp) {
                    return TRUE;
                }
                /* check for supporters 'from behind' */
		if((p->mask[side][Bishop] & ray) ||
		   (p->mask[side][Rook] & ray) ||
		   (p->mask[side][Queen] & ray)) {
                   return TRUE;
                }
	    }
	    else {
		/* distant check */
		int inter;
		int def = 0;
		tmp = p->atkFr[to];
		ClrBit(tmp, fr);
                /* check if defended by opponent */
		if(p->mask[oside][0] & tmp) continue;
		tmp = InterPath[to][ekp];
                while(tmp) { 
		    BitBoard tmp2;
                    inter=FindSetBit(tmp); ClrBit(tmp, inter);
		    tmp2 = p->atkFr[inter] & p->mask[oside][0];
		    if(CountBits(tmp2) < 2) continue;
		    def++; break;
		}
		if(!def) {
                    return TRUE;
                }
	    }
	}
    }

    /* 
     * Bishop checks
     */

    pcs = p->mask[side][Bishop];
    while(pcs) {
	int to;
	BitBoard mvs;
        fr = FindSetBit(pcs); ClrBit(pcs, fr);
        mvs = (p->atkTo[fr] & BishopEPM[ekp]) & ~p->mask[side][0];
        while(mvs) {
	    BitBoard tmp;
            to=FindSetBit(mvs); ClrBit(mvs, to);
	    /* check wether path is obstructed */
	    tmp = InterPath[ekp][to];
	    if((p->mask[White][0] & tmp) || (p->mask[Black][0] & tmp)) continue;
	    /* check wether all flight squares are covered */
            tmp = ksafe & ~BishopEPM[to];
	    if(tmp) {
		int flight;
		int free = 0;
                do {
		    BitBoard att;
                    flight = FindSetBit(tmp); ClrBit(tmp, flight);
                    att = p->atkFr[flight] & p->mask[side][0];
		    ClrBit(att, fr);
		    if(!att) free++;
		    if(free) break;
		} while(tmp);
		if(free) continue;
	    }
	    if(TstBit(p->atkTo[ekp], to)) {
		/* contact check */
		BitBoard ray;
                tmp = p->atkFr[to];
		ClrBit(tmp, fr);
		ClrBit(tmp, ekp);
                /* square is defended by opponent */
		if(p->mask[oside][0] & tmp) continue;
                /* check if we have defenders 'from behind' */
                ray = Ray[to][fr] & p->atkFr[fr];
		if((p->mask[oside][Queen] & ray) ||
		   (p->mask[oside][Rook] & ray) ||
		   (p->mask[oside][Bishop] & ray)) continue;
                /* If supported by a friendly piece, its mate! */
		if(p->mask[side][0] & tmp) {
                    return TRUE;
                }
                /* check for supporters 'from behind' */
		if((p->mask[side][Bishop] & ray) ||
		   (p->mask[side][Rook] & ray) ||
		   (p->mask[side][Queen] & ray)) {
                    return TRUE;
                }
	    }
	    else {
		/* distant check */
		int inter;
		int def = 0;
		tmp = p->atkFr[to];
		ClrBit(tmp, fr);
                /* check if defended by opponent */
		if(p->mask[oside][0] & tmp) continue;
		tmp = InterPath[to][ekp];
                while(tmp) { 
		    BitBoard tmp2;
                    inter=FindSetBit(tmp); ClrBit(tmp, inter);
		    tmp2 = p->atkFr[inter] & p->mask[oside][0];
		    if(CountBits(tmp2) < 2) continue;
		    def++; break;
		}
		if(!def) {
                    return TRUE;
                }
	    }
	}
    }

    /*
     * Knight checks
     */

    pcs = p->mask[side][Knight];
    while(pcs) {
	int to;
	BitBoard mvs;
        fr=FindSetBit(pcs); ClrBit(pcs, fr);
        mvs = (p->atkTo[fr] & KnightEPM[ekp]) &~ p->mask[side][0];
	while(mvs) {
	    BitBoard def;
            to=FindSetBit(mvs); ClrBit(mvs, to);
	    /* 
	     * check wether the square is defended. If so, the defender
	     * must not be pinned.
	     */
            def = p->atkFr[to] & p->mask[oside][0];
	    if(CountBits(def) == 1) {
		int de = FindSetBit(def);
		BitBoard tmp;
		if(RookEPM[ekp] & def) {
                    tmp = p->atkFr[de] & Ray[ekp][de];
		    if(!(p->mask[side][Queen] & tmp) &&
		       !(p->mask[side][Rook] & tmp)) continue;
		}
		else if(BishopEPM[ekp] & def) {
                    tmp = p->atkFr[de] & Ray[ekp][de];
		    if(!(p->mask[side][Queen] & tmp) &&
		       !(p->mask[side][Bishop] & tmp)) continue;
		}
		else continue;
	    }
	    else if(def) continue;
            def = ksafe & ~KnightEPM[to];
	    if(def) {
		int flight;
		int free = 0;
		do {
		    BitBoard att;
                    flight=FindSetBit(def); ClrBit(def, flight);
                    att = p->atkFr[flight] & p->mask[side][0];
		    ClrBit(att, fr);
		    if(!att) free++;
		    if(free) break;
                } while(def);
		if(free) continue;
	    }
	    return TRUE;
	}
    }

    return FALSE;
}

#if 0

int GenMates(struct Position * p, int *ptr)
{
    int oside = !p->turn;
    int ekp = p->kingSq[!p->turn];
    BitBoard pcs;
    BitBoard ksafe;
	BitBoard allpieces = p->mask[White][0] | p->mask[Black][0];
    int fr;
	int cnt = 0;

    ksafe = p->atkTo[ekp] & ~p->mask[oside][0];

    /*
     * Queen checks 
     */

    pcs = p->mask[p->turn][Queen];
    while(pcs) {
		int to;
		BitBoard mvs;
        fr = FindSetBit(pcs); ClrBit(pcs, fr);
        mvs = (p->atkTo[fr] & QueenEPM[ekp]) & ~allpieces;
        while(mvs) {
	    	BitBoard tmp;
            to=FindSetBit(mvs); ClrBit(mvs, to);
	    	/* check wether path is obstructed */
	    	tmp = InterPath[ekp][to];
			if(allpieces & tmp) continue;
	    	/* check wether all flight squares are covered */
            tmp = ksafe & ~QueenEPM[to];
	    	if(tmp) {
				int flight;
				int free = 0;
                do {
		    		BitBoard att;
                    flight = FindSetBit(tmp); ClrBit(tmp, flight);
                    att = p->atkFr[flight] & p->mask[p->turn][0];
		    		ClrBit(att, fr);
		    		if(!att) free++;
		    		if(free) break;
				} while(tmp);
				if(free) continue;
	    	}
	    	if(TstBit(p->atkTo[ekp], to)) {
				/* contact check */
				BitBoard ray;
                tmp = p->atkFr[to];
				ClrBit(tmp, fr);
				ClrBit(tmp, ekp);
                /* square is defended by opponent */
				if(p->mask[oside][0] & tmp) continue;
                /* check if we have defenders 'from behind' */
                ray = Ray[to][fr] & p->atkFr[fr];
				if((p->mask[oside][Queen] & ray) ||
		   			(p->mask[oside][Rook] & ray) ||
		   			(p->mask[oside][Bishop] & ray)) continue;
                /* If supported by a friendly piece, its mate! */
				if(p->mask[p->turn][0] & tmp) {
					*(ptr++) = fr | (to << 6);
					cnt++;
					continue;
                }
                /* check for supporters 'from behind' */
				if((p->mask[p->turn][Bishop] & ray) ||
		   			(p->mask[p->turn][Rook] & ray) ||
                   (p->mask[p->turn][Queen] & ray)) {
					*(ptr++) = fr | (to << 6);
					cnt++;
					continue;
                }
	    	}
	    	else {
				/* distant check */
				int inter;
				int def = 0;
				tmp = p->atkFr[to];
				ClrBit(tmp, fr);
                /* check if defended by opponent */
				if(p->mask[oside][0] & tmp) continue;
				tmp = InterPath[to][ekp];
                while(tmp) { 
		    		BitBoard tmp2;
                    inter=FindSetBit(tmp); ClrBit(tmp, inter);
		    		tmp2 = p->atkFr[inter] & p->mask[oside][0];
		    		if(CountBits(tmp2) < 2) continue;
		    		def++; break;
				}
				if(!def) {
					*(ptr++) = fr | (to << 6);
					cnt++;
					continue;
                }
	    	}
		}
    }

    /*
     * Rook checks 
     */

    pcs = p->mask[p->turn][Rook];
    while(pcs) {
		int to;
		BitBoard mvs;
        fr = FindSetBit(pcs); ClrBit(pcs, fr);
        mvs = (p->atkTo[fr] & RookEPM[ekp]) & ~allpieces;
        while(mvs) {
	    	BitBoard tmp;
            to=FindSetBit(mvs); ClrBit(mvs, to);
	    	/* check wether path is obstructed */
	    	tmp = InterPath[ekp][to];
	    	if(tmp & allpieces) continue;
		    /* check wether all flight squares are covered */
            tmp = ksafe & ~RookEPM[to];
	    	if(tmp) {
				int flight;
				int free = 0;
                do {
		    		BitBoard att;
                    flight = FindSetBit(tmp); ClrBit(tmp, flight);
                    att = p->atkFr[flight] & p->mask[p->turn][0];
		    		ClrBit(att, fr);
		    		if(!att) free++;
		    		if(free) break;
				} while(tmp);
				if(free) continue;
	    	}
	    	if(TstBit(p->atkTo[ekp], to)) {
				/* contact check */
				BitBoard ray;
                tmp = p->atkFr[to];
				ClrBit(tmp, fr);
				ClrBit(tmp, ekp);
                /* square is defended by opponent */
				if(p->mask[oside][0] & tmp) continue;
                /* check if we have defenders 'from behind' */
                ray = Ray[to][fr] & p->atkFr[fr];
				if((p->mask[oside][Queen] & ray) ||
		   				(p->mask[oside][Rook] & ray) ||
		   				(p->mask[oside][Bishop] & ray)) continue;
                /* If supported by a friendly piece, its mate! */
				if(p->mask[p->turn][0] & tmp) {
					*(ptr++) = fr | (to << 6);
					cnt++;
					continue;
                }
                /* check for supporters 'from behind' */
				if((p->mask[p->turn][Bishop] & ray) ||
		   			(p->mask[p->turn][Rook] & ray) ||
		   			(p->mask[p->turn][Queen] & ray)) {
					*(ptr++) = fr | (to << 6);
					cnt++;
					continue;
                }
	    	}
	    	else {
				/* distant check */
				int inter;
				int def = 0;
				tmp = p->atkFr[to];
				ClrBit(tmp, fr);
                /* check if defended by opponent */
				if(p->mask[oside][0] & tmp) continue;
				tmp = InterPath[to][ekp];
                while(tmp) { 
		    		BitBoard tmp2;
                    inter=FindSetBit(tmp); ClrBit(tmp, inter);
		    		tmp2 = p->atkFr[inter] & p->mask[oside][0];
		    		if(CountBits(tmp2) < 2) continue;
		    		def++; break;
				}
				if(!def) {
					*(ptr++) = fr | (to << 6);
					cnt++;
					continue;
                }
	    	}
		}
    }

    /* 
     * Bishop checks
     */

    pcs = p->mask[p->turn][Bishop];
    while(pcs) {
		int to;
		BitBoard mvs;
        fr = FindSetBit(pcs); ClrBit(pcs, fr);
        mvs = (p->atkTo[fr] & BishopEPM[ekp]) & ~allpieces;
        while(mvs) {
	    	BitBoard tmp;
            to=FindSetBit(mvs); ClrBit(mvs, to);
	    	/* check wether path is obstructed */
	    	tmp = InterPath[ekp][to];
	    	if(tmp & allpieces) continue;
	    	/* check wether all flight squares are covered */
            tmp = ksafe & ~BishopEPM[to];
	    	if(tmp) {
				int flight;
				int free = 0;
                do {
		    		BitBoard att;
                    flight = FindSetBit(tmp); ClrBit(tmp, flight);
                    att = p->atkFr[flight] & p->mask[p->turn][0];
		    		ClrBit(att, fr);
		    		if(!att) free++;
		    		if(free) break;
				} while(tmp);
				if(free) continue;
	    	}
	    	if(TstBit(p->atkTo[ekp], to)) {
				/* contact check */
				BitBoard ray;
                tmp = p->atkFr[to];
				ClrBit(tmp, fr);
				ClrBit(tmp, ekp);
                /* square is defended by opponent */
				if(p->mask[oside][0] & tmp) continue;
                /* check if we have defenders 'from behind' */
                ray = Ray[to][fr] & p->atkFr[fr];
				if((p->mask[oside][Queen] & ray) ||
		   			(p->mask[oside][Rook] & ray) ||
		   			(p->mask[oside][Bishop] & ray)) continue;
                	/* If supported by a friendly piece, its mate! */
				if(p->mask[p->turn][0] & tmp) {
					*(ptr++) = fr | (to << 6);
					cnt++;
					continue;
                }
                /* check for supporters 'from behind' */
				if((p->mask[p->turn][Bishop] & ray) ||
		   				(p->mask[p->turn][Rook] & ray) ||
		   				(p->mask[p->turn][Queen] & ray)) {
					*(ptr++) = fr | (to << 6);
					cnt++;
					continue;
                }
	    	}
	    	else {
				/* distant check */
				int inter;
				int def = 0;
				tmp = p->atkFr[to];
				ClrBit(tmp, fr);
                /* check if defended by opponent */
				if(p->mask[oside][0] & tmp) continue;
				tmp = InterPath[to][ekp];
                while(tmp) { 
		    		BitBoard tmp2;
                    inter=FindSetBit(tmp); ClrBit(tmp, inter);
		    		tmp2 = p->atkFr[inter] & p->mask[oside][0];
		    		if(CountBits(tmp2) < 2) continue;
		    		def++; break;
				}
				if(!def) {
					*(ptr++) = fr | (to << 6);
					cnt++;
					continue;
                }
	    	}
		}
    }

    /*
     * Knight checks
     */

    pcs = p->mask[p->turn][Knight];
    while(pcs) {
		int to;
		BitBoard mvs;
        fr=FindSetBit(pcs); ClrBit(pcs, fr);
        mvs = (p->atkTo[fr] & KnightEPM[ekp]) &~ allpieces;
		while(mvs) {
	    	BitBoard def;
            to=FindSetBit(mvs); ClrBit(mvs, to);
	    	/* 
	    	 * check wether the square is defended. If so, the defender
	    	 * must not be pinned.
	    	 */
            def = p->atkFr[to] & p->mask[oside][0];
	    	if(CountBits(def) == 1) {
				int de = FindSetBit(def);
				BitBoard tmp;
				if(RookEPM[ekp] & def) {
                    tmp = p->atkFr[de] & Ray[ekp][de];
		    		if(!(p->mask[p->turn][Queen] & tmp) &&
		       		   !(p->mask[p->turn][Rook] & tmp)) continue;
				}
				else if(BishopEPM[ekp] & def) {
                    tmp = p->atkFr[de] & Ray[ekp][de];
		    		if(!(p->mask[p->turn][Queen] & tmp) &&
		       			!(p->mask[p->turn][Bishop] & tmp)) continue;
				}
				else continue;
	    	}
	    	else if(def) continue;
            def = ksafe & ~KnightEPM[to];
	    	if(def) {
				int flight;
				int free = 0;
				do {
		    		BitBoard att;
                    flight=FindSetBit(def); ClrBit(def, flight);
                    att = p->atkFr[flight] & p->mask[p->turn][0];
		    		ClrBit(att, fr);
		    		if(!att) free++;
		    		if(free) break;
                } while(def);
				if(free) continue;
	    	}
			*(ptr++) = fr | (to << 6);
			cnt++;
		}
    }

    return cnt;
}

#endif
