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
 * init.c - initialization routines
 */

#include "dbase.h"
#include "inline.h"
#include "magic.h"
#include "utils.h"

BitBoard PassedMask[2][64];
BitBoard InterPath[64][64];
BitBoard Ray[64][64];
BitBoard BishopEPM[64], RookEPM[64], QueenEPM[64];
BitBoard SeventhRank[2];
BitBoard ThirdRank[2];

void init_pawn_masks(void) {
    int i, j;

    BitBoard rayw[64] = {0};
    BitBoard rayb[64] = {0};
    for (i = 0; i < 64; i++) {
        for (j = i + 8; j < 64; j += 8) {
            rayw[i] |= bitboard_set_mask(j);
        }
        for (j = i - 8; j >= 0; j -= 8) {
            rayb[i] |= bitboard_set_mask(j);
        }
#ifdef DEBUG
        bitboard_print(rayw[i]);
        bitboard_print(rayb[i]);
#endif
    }
    for (i = 0; i < 64; i++) {
        PassedMask[White][i] = rayw[i];
        if ((i & 7) > 0)
            PassedMask[White][i] |= rayw[i - 1];
        if ((i & 7) < 7)
            PassedMask[White][i] |= rayw[i + 1];
        PassedMask[Black][i] = rayb[i];
        if ((i & 7) > 0)
            PassedMask[Black][i] |= rayb[i - 1];
        if ((i & 7) < 7)
            PassedMask[Black][i] |= rayb[i + 1];
    }
}

void init_geometry(void) {
    int edge[100];
    int trto[100];
    int i, j, k, l;
    const int dirs[] = {1, -1, 10, -10, 9, -9, 11, -11};
    const int dirb[] = {9, -9, 11, -11};
    const int dirr[] = {1, -1, 10, -10};

    for (i = 0; i < 100; i++) {
        edge[i] = 0;
        trto[i] = 0;
    }

    for (i = 0; i < 10; i++) {
        edge[i] = edge[90 + i] = edge[10 * i] = edge[10 * i + 9] = 1;
        for (j = 0; j < 10; j++) {
            int x = i - 1;
            int y = j - 1;
            if (x >= 0 && y >= 0 && x < 8 && y < 8) {
                trto[i + 10 * j] = x + 8 * y;
            }
        }
    }

    for (i = 0; i < 64; i++) {
        for (j = 0; j < 64; j++) {
            InterPath[i][j] = 0;
            Ray[i][j] = 0;
        }
        BishopEPM[i] = RookEPM[i] = QueenEPM[i] = 0ULL;
    }

    for (j = 0; j < 100; j++) {
        int x = trto[j];
        if (edge[j])
            continue;
        for (i = 0; i < 8; i++) {
            int d = dirs[i];
            for (k = j + d; !edge[k]; k += d) {
                int y = trto[k];
                for (l = j + d; l != k; l += d)
                    InterPath[x][y] |= bitboard_set_mask(trto[l]);
                for (l = k + d; !edge[l]; l += d)
                    Ray[x][y] |= bitboard_set_mask(trto[l]);
            }
        }
        for (i = 0; i < 4; i++) {
            int d = dirb[i];
            for (k = j + d; !edge[k]; k += d) {
                BishopEPM[x] |= bitboard_set_mask(trto[k]);
                QueenEPM[x] |= bitboard_set_mask(trto[k]);
            }
            d = dirr[i];
            for (k = j + d; !edge[k]; k += d) {
                RookEPM[x] |= bitboard_set_mask(trto[k]);
                QueenEPM[x] |= bitboard_set_mask(trto[k]);
            }
        }
    }
}

void init_misc_masks(void) {
    int i;

    SeventhRank[White] = SeventhRank[Black] = 0;
    ThirdRank[White] = ThirdRank[Black] = 0;

    for (i = 0; i < 8; i++) {
        SeventhRank[White] |= bitboard_set_mask(a7 + i);
        SeventhRank[Black] |= bitboard_set_mask(a2 + i);
        ThirdRank[White] |= bitboard_set_mask(a3 + i);
        ThirdRank[Black] |= bitboard_set_mask(a6 + i);
    }
}

void init_all(void) {
    init_pawn_masks();
    init_geometry();
    init_misc_masks();
    magic_init();
}
