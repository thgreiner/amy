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
 * init.c - initialization routines
 */

#include "amy.h"

BitBoard SetMask[64], ClrMask[64];
BitBoard ShiftUpMask, ShiftDownMask;
BitBoard ShiftLeftMask, ShiftRightMask;

BitBoard FileMask[8], IsoMask[8];
BitBoard RankMask[8];
BitBoard ForwardRayW[64], ForwardRayB[64];
BitBoard PassedMaskW[64], PassedMaskB[64];
BitBoard ArtIsoMaskW[64], ArtIsoMaskB[64];
BitBoard OutpostMaskW[64], OutpostMaskB[64];
BitBoard InterPath[64][64];
BitBoard Ray[64][64];
BitBoard WPawnEPM[64], BPawnEPM[64];
BitBoard KnightEPM[64], BishopEPM[64], RookEPM[64], QueenEPM[64];
BitBoard KingEPM[64];
BitBoard SeventhRank[2], EighthRank[2];
BitBoard ThirdRank[2];
BitBoard LeftOf[8], RightOf[8], FarLeftOf[8], FarRightOf[8];
BitBoard CenterMask, ExtCenterMask, NoCenterMask;
BitBoard EdgeMask;
BitBoard BlackSquaresMask, WhiteSquaresMask;
BitBoard KingSquareW[64], KingSquareB[64];
BitBoard NotAFileMask, NotHFileMask;
BitBoard CornerMaskA1, CornerMaskA8, CornerMaskH1, CornerMaskH8;
BitBoard WKingTrapsRook1, WKingTrapsRook2;
BitBoard WRookTrapped1, WRookTrapped2;
BitBoard BKingTrapsRook1, BKingTrapsRook2;
BitBoard BRookTrapped1, BRookTrapped2;
BitBoard StrongSquareW[64], StrongSquareB[64];
BitBoard KingSafetyMask[64];
BitBoard WKingOpeningMask, BKingOpeningMask;
BitBoard WPawnOpeningMask, BPawnOpeningMask;
BitBoard WPawnBackwardMask[64], BPawnBackwardMask[64];
BitBoard WPawnKingAttacks[4];
BitBoard BPawnKingAttacks[4];
BitBoard PawnCenterMask;
BitBoard Rook7thKingMask[2];
BitBoard KingSideMask, QueenSideMask;
BitBoard WhitesHalf, BlacksHalf;
BitBoard FianchettoMaskWhiteKingSide, FianchettoMaskBlackKingSide;
BitBoard FianchettoMaskWhiteQueenSide, FianchettoMaskBlackQueenSide;
BitBoard ConnectedMask[64];

void InitMasks(void) {
    int i;

    for (i = 0; i < 64; i++) {
        BitBoard mask = 1ULL << i;
        SetMask[i] = mask;
        ClrMask[i] = ~mask;
    }

    ShiftUpMask = ShiftDownMask = ShiftLeftMask = ShiftRightMask = -1;
    for (i = 0; i < 8; i++) {
        ShiftUpMask &= ClrMask[i];
        ShiftDownMask &= ClrMask[56 + i];
        ShiftLeftMask &= ClrMask[8 * i + 7];
        ShiftRightMask &= ClrMask[8 * i];
    }
}

void PrintBitBoard(BitBoard x) {
    int i, j;
    for (i = 7; i >= 0; i--) {
        for (j = 0; j < 8; j++) {
            int k = i * 8 + j;
            if (x & SetMask[k])
                Print(0, "*");
            else
                Print(0, ".");
        }
        Print(0, "\n");
    }
}

void Print2BitBoards(BitBoard x1, BitBoard x2) {
    int i, j;
    for (i = 7; i >= 0; i--) {
        for (j = 0; j < 8; j++) {
            int k = i * 8 + j;
            if (x1 & SetMask[k])
                Print(0, "*");
            else
                Print(0, ".");
        }
        printf("   ");
        for (j = 0; j < 8; j++) {
            int k = i * 8 + j;
            if (x2 & SetMask[k])
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
            FileMask[i] |= SetMask[8 * j + i];
            if (i > 0)
                IsoMask[i] |= SetMask[8 * j + i - 1];
            if (i < 7)
                IsoMask[i] |= SetMask[8 * j + i + 1];
        }
#ifdef DEBUG
        PrintBitBoard(IsoMask[i]);
#endif
    }
    for (i = 0; i < 64; i++) {
        ForwardRayW[i] = ForwardRayB[i] = 0;
        for (j = i + 8; j < 64; j += 8) {
            ForwardRayW[i] |= SetMask[j];
        }
        for (j = i - 8; j >= 0; j -= 8) {
            ForwardRayB[i] |= SetMask[j];
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
        /* PrintBitBoard(OutpostMaskW[i]); */
        /* PrintBitBoard(OutpostMaskB[i]); */
        ArtIsoMaskW[i] = ArtIsoMaskB[i] = IsoMask[i & 7];
        for (j = i - 3 * 8; j > 0; j -= 8) {
            if ((i & 7) > 0)
                ClrBit(ArtIsoMaskW[i], j - 1);
            if ((i & 7) < 7)
                ClrBit(ArtIsoMaskW[i], j + 1);
        }
        for (j = i + 3 * 8; j < 64; j += 8) {
            if ((i & 7) > 0)
                ClrBit(ArtIsoMaskB[i], j - 1);
            if ((i & 7) < 7)
                ClrBit(ArtIsoMaskB[i], j + 1);
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
                WPawnBackwardMask[i] |= SetMask[sq - 1];
            }
            if ((sq & 7) < 7) {
                WPawnBackwardMask[i] |= SetMask[sq + 1];
            }
        }
        for (sq = i; sq < 64; sq += 8) {
            if ((sq & 7) > 0) {
                BPawnBackwardMask[i] |= SetMask[sq - 1];
            }
            if ((sq & 7) < 7) {
                BPawnBackwardMask[i] |= SetMask[sq + 1];
            }
        }

        /*
        printf("\n%c%c:\n", SQUARE(i));
        Print2BitBoards(WPawnBackwardMask[i], BPawnBackwardMask[i]);
        */
    }

    WPawnKingAttacks[0] = SetMask[f6] | SetMask[g6] | SetMask[h6];
    WPawnKingAttacks[1] = SetMask[f7] | SetMask[g7] | SetMask[h7];
    WPawnKingAttacks[2] = SetMask[c6] | SetMask[b6] | SetMask[a6];
    WPawnKingAttacks[3] = SetMask[c7] | SetMask[b7] | SetMask[a7];
    BPawnKingAttacks[0] = SetMask[f3] | SetMask[g3] | SetMask[h3];
    BPawnKingAttacks[1] = SetMask[f2] | SetMask[g2] | SetMask[h2];
    BPawnKingAttacks[2] = SetMask[c3] | SetMask[b3] | SetMask[a3];
    BPawnKingAttacks[3] = SetMask[c2] | SetMask[b2] | SetMask[a2];

    /*
    Print2BitBoards(WPawnKingAttacks[0], BPawnKingAttacks[0]);
    printf("\n");
    Print2BitBoards(WPawnKingAttacks[1], BPawnKingAttacks[1]);
    printf("\n");
    Print2BitBoards(WPawnKingAttacks[2], BPawnKingAttacks[2]);
    printf("\n");
    Print2BitBoards(WPawnKingAttacks[3], BPawnKingAttacks[3]);
    printf("\n");
    */

    PawnCenterMask = SetMask[d3] | SetMask[e3] | SetMask[d4] | SetMask[e4] |
                     SetMask[d5] | SetMask[e5] | SetMask[d6] | SetMask[e6];

    /* PrintBitBoard(PawnCenterMask); */

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

        /*
        printf("\n%c%c:\n", SQUARE(i));
        PrintBitBoard(ConnectedMask[i]);
        */
    }
}

void InitGeometry(void) {
    int edge[100];
    int trto[100], trfr[64];
    int i, j, k, l;
    int dirs[] = {1, -1, 10, -10, 9, -9, 11, -11};
    int dirb[] = {9, -9, 11, -11};
    int dirr[] = {1, -1, 10, -10};
    int dirn[] = {19, 21, -19, -21, 12, 8, -12, -8};
    int dirk[] = {-11, -10, -9, -1, 1, 9, 10, 11};

    for (i = 0; i < 100; i++)
        edge[i] = trto[i] = 0;
    for (i = 0; i < 64; i++)
        trfr[i] = 0;

    for (i = 0; i < 10; i++) {
        edge[i] = edge[90 + i] = edge[10 * i] = edge[10 * i + 9] = 1;
        for (j = 0; j < 10; j++) {
            int x = i - 1;
            int y = j - 1;
            if (x >= 0 && y >= 0 && x < 8 && y < 8) {
                trto[i + 10 * j] = x + 8 * y;
                trfr[x + 8 * y] = i + 10 * j;
            }
        }
    }

    for (i = 0; i < 64; i++) {
        for (j = 0; j < 64; j++) {
            InterPath[i][j] = 0;
            Ray[i][j] = 0;
        }
        WPawnEPM[i] = BPawnEPM[i] = KnightEPM[i] = BishopEPM[i] = RookEPM[i] =
            QueenEPM[i] = KingEPM[i] = 0;
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
                    InterPath[x][y] |= SetMask[trto[l]];
                for (l = k + d; !edge[l]; l += d)
                    Ray[x][y] |= SetMask[trto[l]];
                /*
                if(InterPath[x][y]) {
                    printf("%c%c <-> %c%c\n", SQUARE(x), SQUARE(y));
                    PrintBitBoard(InterPath[x][y]);
                }
                if(Ray[x][y]) {
                    printf("%c%c <-> %c%c\n", SQUARE(x), SQUARE(y));
                    PrintBitBoard(Ray[x][y]);
                }
                */
            }
        }
        for (i = 0; i < 4; i++) {
            int d = dirb[i];
            for (k = j + d; !edge[k]; k += d) {
                BishopEPM[x] |= SetMask[trto[k]];
                QueenEPM[x] |= SetMask[trto[k]];
            }
            d = dirr[i];
            for (k = j + d; !edge[k]; k += d) {
                RookEPM[x] |= SetMask[trto[k]];
                QueenEPM[x] |= SetMask[trto[k]];
            }
        }
        for (i = 0; i < 8; i++) {
            k = j + dirn[i];
            if (k >= 0 && k < 100 && !edge[k])
                KnightEPM[x] |= SetMask[trto[k]];
            k = j + dirk[i];
            if (k >= 0 && k < 100 && !edge[k])
                KingEPM[x] |= SetMask[trto[k]];
        }
        if (!edge[j + 9])
            WPawnEPM[x] |= SetMask[x + 7];
        if (!edge[j + 11])
            WPawnEPM[x] |= SetMask[x + 9];
        if (!edge[j - 9])
            BPawnEPM[x] |= SetMask[x - 7];
        if (!edge[j - 11])
            BPawnEPM[x] |= SetMask[x - 9];
        /*
        printf("%c%c\n", SQUARE(x));
        PrintBitBoard(KingEPM[x]);
        PrintBitBoard(WPawnEPM[x]); printf("\n");
        PrintBitBoard(BPawnEPM[x]); printf("\n");
        PrintBitBoard(KnightEPM[x]); printf("\n");
        PrintBitBoard(BishopEPM[x]); printf("\n");
        PrintBitBoard(RookEPM[x]); printf("\n");
        PrintBitBoard(QueenEPM[x]);
        */
    }
}

void InitMiscMasks(void) {
    int i, j;

    SeventhRank[White] = SeventhRank[Black] = 0;
    EighthRank[White] = EighthRank[Black] = 0;
    ThirdRank[White] = ThirdRank[Black] = 0;

    for (i = 0; i < 8; i++) {
        RankMask[i] = 0;
        SeventhRank[White] |= SetMask[a7 + i];
        SeventhRank[Black] |= SetMask[a2 + i];
        EighthRank[White] |= SetMask[a8 + i];
        EighthRank[Black] |= SetMask[a1 + i];
        ThirdRank[White] |= SetMask[a3 + i];
        ThirdRank[Black] |= SetMask[a6 + i];

        for (j = 0; j < 8; j++) {
            RankMask[i] |= SetMask[8 * i + j];
        }
    }
    /*
    PrintBitBoard(ThirdRank[White]); printf("\n");
    PrintBitBoard(ThirdRank[Black]); printf("\n");
    PrintBitBoard(SeventhRank[White]); printf("\n");
    PrintBitBoard(SeventhRank[Black]); printf("\n");
    */

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

        /*
        printf("%d:\n", i);
        PrintBitBoard(LeftOf[i]); printf("\n");
        PrintBitBoard(FarLeftOf[i]); printf("\n");
        PrintBitBoard(RightOf[i]); printf("\n");
        PrintBitBoard(FarRightOf[i]); printf("\n");
        */
    }

    CenterMask = ExtCenterMask = 0;
    SetBit(CenterMask, e4);
    SetBit(CenterMask, e5);
    SetBit(CenterMask, d4);
    SetBit(CenterMask, d5);

    SetBit(ExtCenterMask, c3);
    SetBit(ExtCenterMask, d3);
    SetBit(ExtCenterMask, e3);
    SetBit(ExtCenterMask, f3);
    SetBit(ExtCenterMask, c4);
    SetBit(ExtCenterMask, f4);
    SetBit(ExtCenterMask, c5);
    SetBit(ExtCenterMask, f5);
    SetBit(ExtCenterMask, c6);
    SetBit(ExtCenterMask, d6);
    SetBit(ExtCenterMask, e6);
    SetBit(ExtCenterMask, f6);

    NoCenterMask = ~(CenterMask | ExtCenterMask);

    EdgeMask = 0;

    for (i = 0; i < 8; i++) {
        SetBit(EdgeMask, a1 + i);
        SetBit(EdgeMask, a8 + i);
        SetBit(EdgeMask, a1 + 8 * i);
        SetBit(EdgeMask, h1 + 8 * i);
    }

    /*
    PrintBitBoard(CenterMask); printf("\n");
    PrintBitBoard(ExtCenterMask); printf("\n");
    PrintBitBoard(NoCenterMask);
    */

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
    /*
    PrintBitBoard(WhiteSquaresMask); printf("\n");
    PrintBitBoard(BlackSquaresMask);
    */

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
        /*
        printf("%c%c:\n", SQUARE(i));
        PrintBitBoard(KingSquareW[i]); printf("\n");
        PrintBitBoard(KingSquareB[i]); printf("\n");
        */
    }

    NotAFileMask = NotHFileMask = 0;
    for (i = 0; i < 7; i++) {
        NotAFileMask |= FileMask[i + 1];
        NotHFileMask |= FileMask[i];
    }
    /*
    PrintBitBoard(NotAFileMask); printf("\n");
    PrintBitBoard(NotHFileMask); printf("\n");
    */

    CornerMaskA1 = SetMask[a1] | SetMask[a2] | SetMask[b1] | SetMask[b2];
    CornerMaskA8 = SetMask[a8] | SetMask[a7] | SetMask[b8] | SetMask[b7];
    CornerMaskH1 = SetMask[h1] | SetMask[h2] | SetMask[g1] | SetMask[g2];
    CornerMaskH8 = SetMask[h8] | SetMask[h7] | SetMask[g8] | SetMask[g7];

    /*
    PrintBitBoard(CornerMaskA1); printf("\n");
    PrintBitBoard(CornerMaskA8); printf("\n");
    PrintBitBoard(CornerMaskH1); printf("\n");
    PrintBitBoard(CornerMaskH8); printf("\n");
    */

    WKingTrapsRook1 = SetMask[f1] | SetMask[g1];
    WRookTrapped1 = SetMask[g1] | SetMask[h1] | SetMask[h2];
    WKingTrapsRook2 = SetMask[c1] | SetMask[b1];
    WRookTrapped2 = SetMask[b1] | SetMask[a1] | SetMask[a2];
    BKingTrapsRook1 = SetMask[f8] | SetMask[g8];
    BRookTrapped1 = SetMask[g8] | SetMask[h8] | SetMask[h7];
    BKingTrapsRook2 = SetMask[c8] | SetMask[b8];
    BRookTrapped2 = SetMask[b8] | SetMask[a8] | SetMask[a7];

    for (i = 0; i < 64; i++) {
        StrongSquareW[i] = StrongSquareB[i] = 0;
        for (j = i; j < 64; j += 8)
            StrongSquareW[i] |= WPawnEPM[j];
        for (j = i; j >= 0; j -= 8)
            StrongSquareB[i] |= BPawnEPM[j];
        /*
        printf("%d\n", i);
        PrintBitBoard(StrongSquareW[i]);
        PrintBitBoard(StrongSquareB[i]);
        */
    }

    for (i = 0; i < 64; i++) {
        int rank = (i >> 3);
        int file = i & 7;
        int x, y;

        if (rank == 0)
            rank = 1;
        if (rank == 7)
            rank = 6;
        if (file == 0)
            file = 1;
        if (file == 7)
            file = 6;

        KingSafetyMask[i] = 0;
        for (x = rank - 1; x <= rank + 1; x++) {
            for (y = file - 1; y <= file + 1; y++) {
                KingSafetyMask[i] |= SetMask[x * 8 + y];
            }
        }
        /*
        printf("%c%c:\n", SQUARE(i));
        PrintBitBoard(KingSafetyMask[i]);
        */
    }

    WKingOpeningMask = SetMask[e1] | SetMask[d1];
    BKingOpeningMask = SetMask[e8] | SetMask[d8];
    WPawnOpeningMask = SetMask[e2] | SetMask[d2];
    BPawnOpeningMask = SetMask[e7] | SetMask[d7];

    Rook7thKingMask[White] = RankMask[7] | RankMask[6];
    Rook7thKingMask[Black] = RankMask[0] | RankMask[1];

    KingSideMask = FileMask[7] | FileMask[6] | FileMask[5] | FileMask[4];
    QueenSideMask = FileMask[0] | FileMask[1] | FileMask[2] | FileMask[3];

    WhitesHalf = RankMask[0] | RankMask[1] | RankMask[2] | RankMask[3];
    BlacksHalf = RankMask[4] | RankMask[5] | RankMask[6] | RankMask[7];

    FianchettoMaskWhiteKingSide = SetMask[f2] | SetMask[g3] | SetMask[h2];
    FianchettoMaskBlackKingSide = SetMask[f7] | SetMask[g6] | SetMask[h7];
    FianchettoMaskWhiteQueenSide = SetMask[c2] | SetMask[b3] | SetMask[a2];
    FianchettoMaskBlackQueenSide = SetMask[c7] | SetMask[b6] | SetMask[a7];
}

BitBoard ShiftUp(BitBoard x) { return (x << 8) & ShiftUpMask; }

BitBoard ShiftDown(BitBoard x) { return (x >> 8) & ShiftDownMask; }

BitBoard ShiftLeft(BitBoard x) { return (x << 1) & ShiftLeftMask; }

BitBoard ShiftRight(BitBoard x) { return (x >> 1) & ShiftRightMask; }

void InitAll(void) {
    InitMasks();
    InitPawnMasks();
    InitGeometry();
    InitMiscMasks();
}
