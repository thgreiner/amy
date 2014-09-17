/*

    Amy - a chess playing program

    Copyright (c) 2014, Thorsten Greiner
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this
      list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
    FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
    DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
    OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

/**
 * An implementation of an RANROT B3 random number generator.
 * See http://www.agner.org/random
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
