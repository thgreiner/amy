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

/**
 * An implementation of an RANROT B3 random number generator.
 * See http://www.agner.org/random
 *
 * $Id: random.c 27 2003-02-11 22:39:17Z thorsten $
 */

#include "amy.h"

#define ROT(x, r) (((x) << (r)) | ((x) >> (sizeof(ran_t)*8 - (r))))

#define LEN  27
#define DIF1 10
#define DIF2 21
#define R1    5
#define R2    3
#define R3   53

static ran_t buffer[LEN];
static int idx1, idx2, idx3;
static double scale;

ran_t Random64()
{
    ran_t result = ROT(buffer[idx1], R1) + ROT(buffer[idx2], R2) +
                   ROT(buffer[idx3], R3);

    buffer[idx1] = result;

    if(--idx1 < 0) idx1 = LEN-1;
    if(--idx2 < 0) idx2 = LEN-1;
    if(--idx3 < 0) idx3 = LEN-1;

    return result;
}

double Random()
{
    double result = (double) Random64();
    return result * scale;
}

void InitRandom(ran_t seed)
{
    int i;

    for(i=0; i<LEN; i++) {
	buffer[i] = seed;
	seed = ROT(seed, 5) + 97;
    }

    idx1 = 0;
    idx2 = DIF1;
    idx3 = DIF2;

    scale = ldexp(1.0, -sizeof(ran_t)*8);

    for(i=0; i < 50000; i++) Random();
}
