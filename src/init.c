/*

    Amy - a chess playing program

    Copyright (c) 2002-2025, Thorsten Greiner
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

#include "amy.h"
#include "dbase.h"
#include "inline.h"
#include "magic.h"
#include "utils.h"

BitBoard ShiftUpMask, ShiftDownMask;
BitBoard ShiftLeftMask, ShiftRightMask;

BitBoard FileMask[8], IsoMask[8];
BitBoard RankMask[8];
BitBoard ForwardRayW[64], ForwardRayB[64];
BitBoard PassedMaskW[64], PassedMaskB[64];
BitBoard OutpostMaskW[64], OutpostMaskB[64];
BitBoard InterPath[64][64];
BitBoard Ray[64][64];
BitBoard WPawnEPM[64], BPawnEPM[64];
BitBoard BishopEPM[64], RookEPM[64], QueenEPM[64];
BitBoard SeventhRank[2], EighthRank[2];
BitBoard ThirdRank[2];
BitBoard LeftOf[8], RightOf[8], FarLeftOf[8], FarRightOf[8];
BitBoard EdgeMask;
BitBoard BlackSquaresMask, WhiteSquaresMask;
BitBoard KingSquareW[64], KingSquareB[64];
BitBoard NotAFileMask, NotHFileMask;
BitBoard CornerMaskA1, CornerMaskA8, CornerMaskH1, CornerMaskH8;
BitBoard WPawnBackwardMask[64], BPawnBackwardMask[64];
BitBoard KingSideMask, QueenSideMask;
BitBoard ConnectedMask[64];

void InitMasks(void) {
    int i;

    ShiftUpMask = ShiftDownMask = ShiftLeftMask = ShiftRightMask = -1;
    for (i = 0; i < 8; i++) {
        ShiftUpMask &= ClrMask(i);
        ShiftDownMask &= ClrMask(56 + i);
        ShiftRightMask &= ClrMask(8 * i + 7);
        ShiftLeftMask &= ClrMask(8 * i);
    }
}

void PrintBitBoard(BitBoard x) {
    int i, j;
    for (i = 7; i >= 0; i--) {
        for (j = 0; j < 8; j++) {
            int k = i * 8 + j;
            if (TstBit(x, k))
                Print(0, "*");
            else
                Print(0, ".");
        }
        Print(0, "\n");
    }
}

void InitPawnMasks(void) {
    int i, j;

    for (i = 0; i < 8; i++) {
        FileMask[i] = 0;
        IsoMask[i] = 0;
        for (j = 0; j < 8; j++) {
            FileMask[i] |= SetMask(8 * j + i);
            if (i > 0)
                IsoMask[i] |= SetMask(8 * j + i - 1);
            if (i < 7)
                IsoMask[i] |= SetMask(8 * j + i + 1);
        }
#ifdef DEBUG
        PrintBitBoard(IsoMask[i]);
#endif
    }
    for (i = 0; i < 64; i++) {
        ForwardRayW[i] = ForwardRayB[i] = 0;
        for (j = i + 8; j < 64; j += 8) {
            ForwardRayW[i] |= SetMask(j);
        }
        for (j = i - 8; j >= 0; j -= 8) {
            ForwardRayB[i] |= SetMask(j);
        }
#ifdef DEBUG
        PrintBitBoard(ForwardRayW[i]);
        PrintBitBoard(ForwardRayB[i]);
#endif
    }
    for (i = 0; i < 64; i++) {
        PassedMaskW[i] = ForwardRayW[i];
        if ((i & 7) > 0)
            PassedMaskW[i] |= ForwardRayW[i - 1];
        if ((i & 7) < 7)
            PassedMaskW[i] |= ForwardRayW[i + 1];
        PassedMaskB[i] = ForwardRayB[i];
        if ((i & 7) > 0)
            PassedMaskB[i] |= ForwardRayB[i - 1];
        if ((i & 7) < 7)
            PassedMaskB[i] |= ForwardRayB[i + 1];
        /* PrintBitBoard(PassedMaskW[i]); */
        /* PrintBitBoard(PassedMaskB[i]); */
        OutpostMaskW[i] = OutpostMaskB[i] = 0;
        if ((i & 7) > 0) {
            OutpostMaskW[i] |= ForwardRayW[i - 1];
            OutpostMaskB[i] |= ForwardRayB[i - 1];
        }
        if ((i & 7) < 7) {
            OutpostMaskW[i] |= ForwardRayW[i + 1];
            OutpostMaskB[i] |= ForwardRayB[i + 1];
        }
        /*
        printf("\n%c%c:\n", SQUARE(i));
        Print2BitBoards(ArtIsoMaskW[i], ArtIsoMaskB[i]);
        */
    }

    for (i = 0; i < 64; i++) {
        int sq;

        WPawnBackwardMask[i] = BPawnBackwardMask[i] = 0;
        for (sq = i; sq > 0; sq -= 8) {
            if ((sq & 7) > 0) {
                WPawnBackwardMask[i] |= SetMask(sq - 1);
            }
            if ((sq & 7) < 7) {
                WPawnBackwardMask[i] |= SetMask(sq + 1);
            }
        }
        for (sq = i; sq < 64; sq += 8) {
            if ((sq & 7) > 0) {
                BPawnBackwardMask[i] |= SetMask(sq - 1);
            }
            if ((sq & 7) < 7) {
                BPawnBackwardMask[i] |= SetMask(sq + 1);
            }
        }
    }

    for (i = 0; i < 64; i++) {
        ConnectedMask[i] = 0;

        if ((i & 7) < 7) {
            SetBit(ConnectedMask[i], i + 1);
            if ((i >> 3) > 1) {
                SetBit(ConnectedMask[i], i - 7);
            }
            if ((i >> 3) < 6) {
                SetBit(ConnectedMask[i], i + 9);
            }
        }
        if ((i & 7) > 0) {
            SetBit(ConnectedMask[i], i - 1);
            if ((i >> 3) > 1) {
                SetBit(ConnectedMask[i], i + 7);
            }
            if ((i >> 3) < 6) {
                SetBit(ConnectedMask[i], i - 9);
            }
        }
    }
}

void InitGeometry(void) {
    int edge[100];
    int trto[100];
    int i, j, k, l;
    int dirs[] = {1, -1, 10, -10, 9, -9, 11, -11};
    int dirb[] = {9, -9, 11, -11};
    int dirr[] = {1, -1, 10, -10};

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
        WPawnEPM[i] = BPawnEPM[i] = BishopEPM[i] = RookEPM[i] = QueenEPM[i] =
            0ULL;
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
                    InterPath[x][y] |= SetMask(trto[l]);
                for (l = k + d; !edge[l]; l += d)
                    Ray[x][y] |= SetMask(trto[l]);
            }
        }
        for (i = 0; i < 4; i++) {
            int d = dirb[i];
            for (k = j + d; !edge[k]; k += d) {
                BishopEPM[x] |= SetMask(trto[k]);
                QueenEPM[x] |= SetMask(trto[k]);
            }
            d = dirr[i];
            for (k = j + d; !edge[k]; k += d) {
                RookEPM[x] |= SetMask(trto[k]);
                QueenEPM[x] |= SetMask(trto[k]);
            }
        }
        if (!edge[j + 9])
            WPawnEPM[x] |= SetMask(x + 7);
        if (!edge[j + 11])
            WPawnEPM[x] |= SetMask(x + 9);
        if (!edge[j - 9])
            BPawnEPM[x] |= SetMask(x - 7);
        if (!edge[j - 11])
            BPawnEPM[x] |= SetMask(x - 9);
    }
}

void InitMiscMasks(void) {
    int i, j;

    SeventhRank[White] = SeventhRank[Black] = 0;
    EighthRank[White] = EighthRank[Black] = 0;
    ThirdRank[White] = ThirdRank[Black] = 0;

    for (i = 0; i < 8; i++) {
        RankMask[i] = 0;
        SeventhRank[White] |= SetMask(a7 + i);
        SeventhRank[Black] |= SetMask(a2 + i);
        EighthRank[White] |= SetMask(a8 + i);
        EighthRank[Black] |= SetMask(a1 + i);
        ThirdRank[White] |= SetMask(a3 + i);
        ThirdRank[Black] |= SetMask(a6 + i);

        for (j = 0; j < 8; j++) {
            RankMask[i] |= SetMask(8 * i + j);
        }
    }

    for (i = 0; i < 8; i++) {
        LeftOf[i] = RightOf[i] = FarLeftOf[i] = FarRightOf[i] = 0;
        for (j = i - 1; j >= 0; j--)
            LeftOf[i] |= FileMask[j];
        for (j = i - 2; j >= 0; j--)
            FarLeftOf[i] |= FileMask[j];
        for (j = i + 1; j < 8; j++)
            RightOf[i] |= FileMask[j];
        for (j = i + 2; j < 8; j++)
            FarRightOf[i] |= FileMask[j];
    }

    EdgeMask = 0;

    for (i = 0; i < 8; i++) {
        SetBit(EdgeMask, a1 + i);
        SetBit(EdgeMask, a8 + i);
        SetBit(EdgeMask, a1 + 8 * i);
        SetBit(EdgeMask, h1 + 8 * i);
    }

    WhiteSquaresMask = BlackSquaresMask = 0;
    for (i = 0; i < 8; i++) {
        for (j = 0; j < 8; j++) {
            if (((i + j) & 1) == 0) {
                SetBit(BlackSquaresMask, (i * 8 + j));
            } else {
                SetBit(WhiteSquaresMask, (i * 8 + j));
            }
        }
    }

    for (i = 0; i < 64; i++) {
        int bdist = (i >> 3);
        int wdist = 7 - bdist;
        int wtarget = (i & 7) + a8;
        int btarget = (i & 7) + a1;

        KingSquareW[i] = KingSquareB[i] = 0;
        for (j = 0; j < 64; j++) {
            if (KingDist(wtarget, j) <= wdist) {
                SetBit(KingSquareW[i], j);
            }
            if (KingDist(btarget, j) <= bdist) {
                SetBit(KingSquareB[i], j);
            }
        }
    }

    NotAFileMask = NotHFileMask = 0;
    for (i = 0; i < 7; i++) {
        NotAFileMask |= FileMask[i + 1];
        NotHFileMask |= FileMask[i];
    }

    CornerMaskA1 = SetMask(a1) | SetMask(a2) | SetMask(b1) | SetMask(b2);
    CornerMaskA8 = SetMask(a8) | SetMask(a7) | SetMask(b8) | SetMask(b7);
    CornerMaskH1 = SetMask(h1) | SetMask(h2) | SetMask(g1) | SetMask(g2);
    CornerMaskH8 = SetMask(h8) | SetMask(h7) | SetMask(g8) | SetMask(g7);

    KingSideMask = FileMask[7] | FileMask[6] | FileMask[5] | FileMask[4];
    QueenSideMask = FileMask[0] | FileMask[1] | FileMask[2] | FileMask[3];
}

void InitAll(void) {
    InitMasks();
    InitPawnMasks();
    InitGeometry();
    InitMiscMasks();
    InitMagic();
}
