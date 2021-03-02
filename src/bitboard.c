/*

    Amy - a chess playing program

    Copyright (c) 2014, Thorsten Greiner
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
 * bitboard.c - bitboard routines
 */

#include "amy.h"

#define USE_8BIT 1

#if !HAVE___BUILTIN_CLZLL
int FindSetBit(BitBoard b) {
#if HAVE_FFSLL
    return 64 - ffsll(b);
#else
    // return ffsl(b) - 1;
    union {
        BitBoard b;
        unsigned short sh[4];
        unsigned char ch[8];
    } d;

    d.b = b;

#ifdef WORDS_BIGENDIAN
    if (d.sh[1])
        return FirstBit16[d.sh[1]] + 16;
    if (d.sh[2])
        return FirstBit16[d.sh[2]] + 32;
    if (d.sh[0])
        return FirstBit16[d.sh[0]];
    return FirstBit16[d.sh[3]] + 48;
#else

#if USE_8BIT
    if (d.sh[1]) {
        if (d.ch[3])
            return FirstBit8[d.ch[3]] + 32;
        else
            return FirstBit8[d.ch[2]] + 40;
    }
    if (d.sh[2]) {
        if (d.ch[4])
            return FirstBit8[d.ch[4]] + 24;
        else
            return FirstBit8[d.ch[5]] + 16;
    }
    if (d.sh[0]) {
        if (d.ch[1])
            return FirstBit8[d.ch[1]] + 48;
        else
            return FirstBit8[d.ch[0]] + 56;
    }
    if (d.ch[6])
        return FirstBit8[d.ch[6]] + 8;
    else
        return FirstBit8[d.ch[7]];
#endif /* USE_8BIT */

#if USE_16BIT
    if (d.sh[1]) {
        return FirstBit16[d.sh[1]] + 32;
    }
    if (d.sh[2]) {
        return FirstBit16[d.sh[2]] + 16;
    }
    if (d.sh[0]) {
        return FirstBit16[d.sh[0]] + 48;
    }
    return FirstBit16[d.sh[3]];
#endif /* USE_16BIT */

#endif
#endif
}
#endif

#if !HAVE___BUILTIN_POPCOUNTLL
int CountBits(BitBoard i) {
    i = i - ((i >> 1) & 0x5555555555555555);
    i = (i & 0x3333333333333333) + ((i >> 2) & 0x3333333333333333);
    return (((i + (i >> 4)) & 0xF0F0F0F0F0F0F0F) * 0x101010101010101) >> 56;
}
#endif
