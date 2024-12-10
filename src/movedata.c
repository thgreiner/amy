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
 * movedata.c - move data precalculation stuff
 */

#include "amy.h"
#include <stdio.h>

static int conv[128];

signed char NextSQ[64][64];

static int QueenDirs[] = {16, 1, -16, -1, 15, 17, -15, -17};

const BitBoard KnightEPM[64] = {
    0x20400ULL,
    0x50800ULL,
    0xa1100ULL,
    0x142200ULL,
    0x284400ULL,
    0x508800ULL,
    0xa01000ULL,
    0x402000ULL,
    0x2040004ULL,
    0x5080008ULL,
    0xa110011ULL,
    0x14220022ULL,
    0x28440044ULL,
    0x50880088ULL,
    0xa0100010ULL,
    0x40200020ULL,
    0x204000402ULL,
    0x508000805ULL,
    0xa1100110aULL,
    0x1422002214ULL,
    0x2844004428ULL,
    0x5088008850ULL,
    0xa0100010a0ULL,
    0x4020002040ULL,
    0x20400040200ULL,
    0x50800080500ULL,
    0xa1100110a00ULL,
    0x142200221400ULL,
    0x284400442800ULL,
    0x508800885000ULL,
    0xa0100010a000ULL,
    0x402000204000ULL,
    0x2040004020000ULL,
    0x5080008050000ULL,
    0xa1100110a0000ULL,
    0x14220022140000ULL,
    0x28440044280000ULL,
    0x50880088500000ULL,
    0xa0100010a00000ULL,
    0x40200020400000ULL,
    0x204000402000000ULL,
    0x508000805000000ULL,
    0xa1100110a000000ULL,
    0x1422002214000000ULL,
    0x2844004428000000ULL,
    0x5088008850000000ULL,
    0xa0100010a0000000ULL,
    0x4020002040000000ULL,
    0x400040200000000ULL,
    0x800080500000000ULL,
    0x1100110a00000000ULL,
    0x2200221400000000ULL,
    0x4400442800000000ULL,
    0x8800885000000000ULL,
    0x100010a000000000ULL,
    0x2000204000000000ULL,
    0x4020000000000ULL,
    0x8050000000000ULL,
    0x110a0000000000ULL,
    0x22140000000000ULL,
    0x44280000000000ULL,
    0x88500000000000ULL,
    0x10a00000000000ULL,
    0x20400000000000ULL,
};

const BitBoard KingEPM[64] = {
    0x302ULL,
    0x705ULL,
    0xe0aULL,
    0x1c14ULL,
    0x3828ULL,
    0x7050ULL,
    0xe0a0ULL,
    0xc040ULL,
    0x30203ULL,
    0x70507ULL,
    0xe0a0eULL,
    0x1c141cULL,
    0x382838ULL,
    0x705070ULL,
    0xe0a0e0ULL,
    0xc040c0ULL,
    0x3020300ULL,
    0x7050700ULL,
    0xe0a0e00ULL,
    0x1c141c00ULL,
    0x38283800ULL,
    0x70507000ULL,
    0xe0a0e000ULL,
    0xc040c000ULL,
    0x302030000ULL,
    0x705070000ULL,
    0xe0a0e0000ULL,
    0x1c141c0000ULL,
    0x3828380000ULL,
    0x7050700000ULL,
    0xe0a0e00000ULL,
    0xc040c00000ULL,
    0x30203000000ULL,
    0x70507000000ULL,
    0xe0a0e000000ULL,
    0x1c141c000000ULL,
    0x382838000000ULL,
    0x705070000000ULL,
    0xe0a0e0000000ULL,
    0xc040c0000000ULL,
    0x3020300000000ULL,
    0x7050700000000ULL,
    0xe0a0e00000000ULL,
    0x1c141c00000000ULL,
    0x38283800000000ULL,
    0x70507000000000ULL,
    0xe0a0e000000000ULL,
    0xc040c000000000ULL,
    0x302030000000000ULL,
    0x705070000000000ULL,
    0xe0a0e0000000000ULL,
    0x1c141c0000000000ULL,
    0x3828380000000000ULL,
    0x7050700000000000ULL,
    0xe0a0e00000000000ULL,
    0xc040c00000000000ULL,
    0x203000000000000ULL,
    0x507000000000000ULL,
    0xa0e000000000000ULL,
    0x141c000000000000ULL,
    0x2838000000000000ULL,
    0x5070000000000000ULL,
    0xa0e0000000000000ULL,
    0x40c0000000000000ULL,
};

const BitBoard PawnEPM[2][64] = {{0x200ULL,
                                  0x500ULL,
                                  0xa00ULL,
                                  0x1400ULL,
                                  0x2800ULL,
                                  0x5000ULL,
                                  0xa000ULL,
                                  0x4000ULL,
                                  0x20000ULL,
                                  0x50000ULL,
                                  0xa0000ULL,
                                  0x140000ULL,
                                  0x280000ULL,
                                  0x500000ULL,
                                  0xa00000ULL,
                                  0x400000ULL,
                                  0x2000000ULL,
                                  0x5000000ULL,
                                  0xa000000ULL,
                                  0x14000000ULL,
                                  0x28000000ULL,
                                  0x50000000ULL,
                                  0xa0000000ULL,
                                  0x40000000ULL,
                                  0x200000000ULL,
                                  0x500000000ULL,
                                  0xa00000000ULL,
                                  0x1400000000ULL,
                                  0x2800000000ULL,
                                  0x5000000000ULL,
                                  0xa000000000ULL,
                                  0x4000000000ULL,
                                  0x20000000000ULL,
                                  0x50000000000ULL,
                                  0xa0000000000ULL,
                                  0x140000000000ULL,
                                  0x280000000000ULL,
                                  0x500000000000ULL,
                                  0xa00000000000ULL,
                                  0x400000000000ULL,
                                  0x2000000000000ULL,
                                  0x5000000000000ULL,
                                  0xa000000000000ULL,
                                  0x14000000000000ULL,
                                  0x28000000000000ULL,
                                  0x50000000000000ULL,
                                  0xa0000000000000ULL,
                                  0x40000000000000ULL,
                                  0x200000000000000ULL,
                                  0x500000000000000ULL,
                                  0xa00000000000000ULL,
                                  0x1400000000000000ULL,
                                  0x2800000000000000ULL,
                                  0x5000000000000000ULL,
                                  0xa000000000000000ULL,
                                  0x4000000000000000ULL,
                                  0x0ULL,
                                  0x0ULL,
                                  0x0ULL,
                                  0x0ULL,
                                  0x0ULL,
                                  0x0ULL,
                                  0x0ULL,
                                  0x0ULL},
                                 {0x0ULL,
                                  0x0ULL,
                                  0x0ULL,
                                  0x0ULL,
                                  0x0ULL,
                                  0x0ULL,
                                  0x0ULL,
                                  0x0ULL,
                                  0x2ULL,
                                  0x5ULL,
                                  0xaULL,
                                  0x14ULL,
                                  0x28ULL,
                                  0x50ULL,
                                  0xa0ULL,
                                  0x40ULL,
                                  0x200ULL,
                                  0x500ULL,
                                  0xa00ULL,
                                  0x1400ULL,
                                  0x2800ULL,
                                  0x5000ULL,
                                  0xa000ULL,
                                  0x4000ULL,
                                  0x20000ULL,
                                  0x50000ULL,
                                  0xa0000ULL,
                                  0x140000ULL,
                                  0x280000ULL,
                                  0x500000ULL,
                                  0xa00000ULL,
                                  0x400000ULL,
                                  0x2000000ULL,
                                  0x5000000ULL,
                                  0xa000000ULL,
                                  0x14000000ULL,
                                  0x28000000ULL,
                                  0x50000000ULL,
                                  0xa0000000ULL,
                                  0x40000000ULL,
                                  0x200000000ULL,
                                  0x500000000ULL,
                                  0xa00000000ULL,
                                  0x1400000000ULL,
                                  0x2800000000ULL,
                                  0x5000000000ULL,
                                  0xa000000000ULL,
                                  0x4000000000ULL,
                                  0x20000000000ULL,
                                  0x50000000000ULL,
                                  0xa0000000000ULL,
                                  0x140000000000ULL,
                                  0x280000000000ULL,
                                  0x500000000000ULL,
                                  0xa00000000000ULL,
                                  0x400000000000ULL,
                                  0x2000000000000ULL,
                                  0x5000000000000ULL,
                                  0xa000000000000ULL,
                                  0x14000000000000ULL,
                                  0x28000000000000ULL,
                                  0x50000000000000ULL,
                                  0xa0000000000000ULL,
                                  0x40000000000000ULL}};

void InitMoves(void) {
    int sq, sq2;

    for (sq = 0; sq < 128; sq++) {
        conv[sq] = 127;
    }
    for (sq = 0; sq < 128; sq++) {
        if (!(sq & 0x88)) {
            sq2 = (sq & 7) | (sq & 0x70) >> 1;
            conv[sq] = sq2;
        }
    }

    for (sq = 0; sq < 64; sq++) {
        for (sq2 = 0; sq2 < 64; sq2++) {
            NextSQ[sq][sq2] = -1;
        }
    }

    /*
     * Inititialize NextSQ
     */

    for (sq = 0; sq < 128; sq++) {
        int dir;

        if (sq & 0x88)
            continue;

        for (dir = 0; dir < 8; dir++) {
            int next, next2;

            next = sq + QueenDirs[dir];
            if (next & 0x88)
                continue;

            for (;;) {
                next2 = next + QueenDirs[dir];
                if (next2 & 0x88)
                    break;
                NextSQ[conv[sq]][conv[next]] = conv[next2];
                next = next2;
            }
        }
    }
}
