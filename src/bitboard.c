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
 * bitboard.c - bitboard routines
 *
 * $Id: bitboard.c 27 2003-02-11 22:39:17Z thorsten $
 *
 */

#include "amy.h"

#define USE_8BIT 1

int FindSetBit(BitBoard b)
{
#if 1
    return 64 - ffsll(b);
#else
    // return ffsl(b) - 1;
    union {
        BitBoard b;
        unsigned short sh[4];
        unsigned char  ch[8];
    } d;

    d.b = b;

#ifdef WORDS_BIGENDIAN
    if(d.sh[1]) return FirstBit16[d.sh[1]] + 16;
    if(d.sh[2]) return FirstBit16[d.sh[2]] + 32;
    if(d.sh[0]) return FirstBit16[d.sh[0]];
    return FirstBit16[d.sh[3]] + 48;
#else

#if USE_8BIT
    if(d.sh[1]) {
        if(d.ch[3]) return FirstBit8[d.ch[3]] + 32;
        else        return FirstBit8[d.ch[2]] + 40;
    }
    if(d.sh[2]) {
        if(d.ch[4]) return FirstBit8[d.ch[4]] + 24;
        else        return FirstBit8[d.ch[5]] + 16;
    }
    if(d.sh[0]) {
        if(d.ch[1]) return FirstBit8[d.ch[1]] + 48;
        else        return FirstBit8[d.ch[0]] + 56;
    }
    if(d.ch[6]) return FirstBit8[d.ch[6]] + 8;
    else        return FirstBit8[d.ch[7]];
#endif /* USE_8BIT */

#if USE_16BIT
    if(d.sh[1]) {
        return FirstBit16[d.sh[1]] + 32;
    }
    if(d.sh[2]) {
        return FirstBit16[d.sh[2]] + 16;
    }
    if(d.sh[0]) {
        return FirstBit16[d.sh[0]] + 48;
    }
    return FirstBit16[d.sh[3]];
#endif /* USE_16BIT */

#endif
#endif
}

int CountBits(BitBoard d)
{
    register int c=0;

    while(d) {
      c++;
      d &= d - 1;
    }
    return(c);

}
