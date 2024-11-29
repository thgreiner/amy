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
 * score.c - positional scoring routines
 */

#include "amy.h"

#define REFLECT_X(a) ((a) ^ 0x38)

/**
 * Debugging stuff
 */

/* #define DEBUG */
#ifdef DEBUG

enum {
    DebugPawnStructure = 1,
    DebugKingSafety = 2,
    DebugPassedPawns = 4,
    DebugPieces = 8
};

static int DebugWhat = 0;

#endif

/**
 * Some constants for pawn structure
 */

enum {
    PawnsOnKingSide = (1 << 0),
    PawnsOnQueenSide = (1 << 1),
    FianchettoWhiteKingSide = (1 << 2),
    FianchettoWhiteQueenSide = (1 << 3),
    FianchettoBlackKingSide = (1 << 4),
    FianchettoBlackQueenSide = (1 << 5),
    QueensPawnOpening = (1 << 6)
};

/**
 * Some constants for RootGamePhase
 */

enum { Opening, Middlegame, Endgame };

char *GamePhaseName[] = {"Opening", "Middlegame", "Endgame"};

/**
 * General scoring paramters
 */

const static int Development = -100;

/**
 * Pawn scoring parameters
 */

const static int DoubledPawn = -70;
const static int BackwardPawn = -100;
const static int HiddenBackwardPawn = -70;
const static int PawnOutrunsKing = 6000;
const static int PawnDevelopmentBlocked = -100;
const static int PawnDuo = 15;
const static int PawnStorm = 10;
const static int CrampingPawn = -160;
const static int PawnMajority = 100;

const static int CoveredPassedPawn6th = 200;
const static int CoveredPassedPawn7th = 600;

const static int PassedPawn[] = {0, 32, 64, 128, 256, 512, 1024, 0};

const static int PassedPawnBlocked[] = {0, 16, 48, 96, 192, 384, 768, 0};

const static int PassedPawnConnected[] = {0, 4, 12, 24, 48, 96, 192, 0};

const static int IsolatedPawn[] = {-70, -80, -90, -100, -100, -90, -80, -70};

const static int PawnAdvanceOpening[] = {-10, -10, 5, 10, 10, -20, -50, -50};

const static int PawnAdvanceMiddlegame[] = {0, 0, 10, 15, 15, 10, 0, 0};

const static int PawnAdvanceEndgame[] = {10, 10, 10, 10, 10, 10, 10, 10};

static int WPawnPos[64];
static int BPawnPos[64];

const static int DistantPassedPawn[] = {
    500, 300, 300, 300, 200, 200, 150, 150, 150, 100, 100, 50,
    50,  50,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0};

/**
 * Knight scoring parameters
 */

const static int KnightKingProximity = 7;
const static int KnightBlocksCPawn = -100;

const static int KnightPos[] = {
    -160, -160, -160, -160, -160, -160, -160, -160, -160, -30,  60,   60,   60,
    60,   -30,  -160, -160, 60,   130,  130,  130,  130,  60,   -160, -160, 130,
    190,  190,  190,  190,  130,  -160, -130, 130,  190,  250,  250,  190,  130,
    -130, -130, 190,  250,  250,  250,  250,  190,  -130, -130, 90,   160,  160,
    160,  160,  90,   -130, -130, -130, -130, -130, -130, -130, -130, -130};

const static int KnightOutpost[] = {
    0, 0, 0,  0,   0,   0,  0, 0, 0, 0, 0,  0,   0,   0,  0, 0,
    0, 0, 0,  0,   0,   0,  0, 0, 0, 0, 0,  40,  40,  0,  0, 0,
    0, 0, 80, 100, 100, 80, 0, 0, 0, 0, 80, 120, 120, 80, 0, 0,
    0, 0, 40, 80,  80,  40, 0, 0, 0, 0, 0,  0,   0,   0,  0, 0};

/**
 * Bishop scoring parameters
 */

/*
 * The value of the bishop pair depends on the number of white pawns.
 */
const static int BishopPair[] = {200, 200, 200, 200, 200, 200, 200, 150, 100};

const static int BishopMobility = 25;
const static int BishopKingProximity = 7;
const static int BishopTrapped = -1500;

const static int BishopPos[] = {
    60,  60,  60,  60,  60,  60,  60,  60,  60,  250, 60,  60,  60,
    60,  250, 60,  60,  160, 160, 160, 160, 160, 160, 60,  160, 250,
    280, 340, 340, 280, 250, 160, 160, 250, 280, 340, 340, 280, 250,
    160, 160, 250, 280, 280, 280, 280, 250, 160, 160, 250, 250, 250,
    250, 250, 250, 160, 160, 160, 160, 160, 160, 160, 160, 160};

/**
 * Rook scoring parameters
 */

const static int RookMobility = 10;

const static int RookOnOpenFile = 100;
const static int RookOnSemiOpenFile = 25;

const static int RookKingProximity = 5;
const static int RookConnected = 60;

const static int RookBehindPasser = 12; /* will be scaled by phase */

const static int RookOn7thRank = 300;

const static int RookPos[] = {
    0,   90,  130, 220, 220, 130, 90,  0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   130, 130, 130, 130, 130, 130, 130, 130, 200, 200, 200, 200,
    200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200};

/**
 * Queen scoring parameters
 */

const static int QueenKingProximity = 8;

const static int QueenPos[] = {
    0, 0,  0,  0,  0,  0,  0,  0, 0, 30, 30, 30, 30, 30, 30, 0,
    0, 30, 60, 60, 60, 60, 30, 0, 0, 30, 60, 90, 90, 60, 30, 0,
    0, 30, 60, 90, 90, 60, 30, 0, 0, 30, 60, 60, 60, 60, 30, 0,
    0, 30, 30, 60, 60, 30, 30, 0, 0, 0,  0,  0,  0,  0,  0,  0};

const static int QueenPosDevelopment[] = {
    -200, -200, 0, 0, 0, 0, -200, -200, -200, -200, 30, 30, 30, 0, -200, -200,
    -200, -200, 0, 0, 0, 0, -200, -200, -200, -200, 0,  0,  0,  0, -200, -200,
    -200, -200, 0, 0, 0, 0, -200, -200, -200, -200, 0,  0,  0,  0, -200, -200,
    -200, -200, 0, 0, 0, 0, -200, -200, -200, -200, 0,  0,  0,  0, -200, -200,
};

/**
 * King scoring parameters
 */

const static int KingBlocksRook = -300;

const static int KingPosMiddlegame[] = {
    -100, 0,    -200, -300, -300, -200, 0,    -100 - 100, -100, -200, -300,
    -300, -200, -100, -100, -300, -300, -300, -300,       -300, -300, -300,
    -300, -400, -400, -400, -400, -400, -400, -400,       -400, -500, -500,
    -500, -500, -500, -500, -500, -500, -600, -600,       -600, -600, -600,
    -600, -600, -600, -700, -700, -700, -700, -700,       -700, -700, -700,
    -800, -800, -800, -800, -800, -800, -800, -800};

const static int KingPosEndgame[] = {
    -300, -300, -300, -300, -300, -300, -300, -300, -300, -200, -100,
    -100, -100, -100, -200, -300, -300, -100, 0,    100,  100,  0,
    -100, -300, -300, -100, 100,  200,  200,  100,  -100, -300, -300,
    -100, 200,  300,  300,  200,  -100, -300, -300, -100, 200,  300,
    300,  200,  -100, -300, -300, -100, -100, -100, -100, -100, -100,
    -300, -300, -300, -300, -300, -300, -300, -300, -300};

const static int KingPosEndgameQueenSide[] = {
    -300, -300, -300, -300, -300, -400, -500, -600, -100, -100, -100,
    -100, -100, -200, -300, -600, 0,    100,  100,  0,    -100, -200,
    -300, -600, 100,  200,  200,  100,  -100, -200, -300, -600, 200,
    300,  300,  200,  -100, -200, -300, -600, 200,  300,  300,  200,
    -100, -200, -300, -600, -100, -100, -100, -100, -100, -200, -300,
    -600, -300, -300, -300, -300, -300, -400, -300, -600};

const static int KingPosEndgameKingSide[] = {
    -600, -500, -400, -300, -300, -300, -300, -300, -600, -300, -200,
    -100, -100, -100, -100, -100, -600, -300, -200, -100, 0,    100,
    100,  0,    -600, -300, -200, -100, 100,  200,  200,  100,  -600,
    -300, -200, -100, 200,  300,  300,  200,  -600, -300, -200, -100,
    200,  300,  300,  200,  -600, -300, -200, -100, -100, -100, -100,
    -100, -600, -500, -400, -300, -300, -300, -300, -300};

const static int ScaleHalfOpenFilesMine[] = {0, 4, 7, 9, 11};

const static int ScaleHalfOpenFilesYours[] = {0, 2, 3, 4, 5};

const static int ScaleOpenFiles[] = {0, 8, 13, 16, 19};

const static int ScaleUp[] = {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
                              0,  0,  0,  1,  2,  4,  6,  8,  10, 12, 14,
                              15, 16, 16, 16, 16, 16, 16, 16, 16, 16};

const static int ScaleDown[] = {16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
                                16, 16, 16, 15, 14, 12, 10, 8,  6,  4,  2,
                                1,  0,  0,  0,  0,  0,  0,  0,  0,  0};

/*
 * MaxPos is the maximum difference between the material balance and
 * a positional evaluation.
 */

int MaxPos;
const static int MaxPosInit = 2000;

/*
 * These scoring parameters will be shared among function calls.
 */

static int RootGamePhase;

/**
 * Masks used in ScorePawns.
 */

const static BitBoard FianchettoMaskWhiteKingSide =
    SetMask(f2) | SetMask(g3) | SetMask(h2);
const static BitBoard FianchettoMaskBlackKingSide =
    SetMask(f7) | SetMask(g6) | SetMask(h7);
const static BitBoard FianchettoMaskWhiteQueenSide =
    SetMask(c2) | SetMask(b3) | SetMask(a2);
const static BitBoard FianchettoMaskBlackQueenSide =
    SetMask(c7) | SetMask(b6) | SetMask(a7);

/**
 * Masks used in ScoreDevelopment.
 */
const static BitBoard WKingOpeningMask = SetMask(e1) | SetMask(d1);
const static BitBoard BKingOpeningMask = SetMask(e8) | SetMask(d8);

const static BitBoard WKingTrapsRook1 = SetMask(f1) | SetMask(g1);
const static BitBoard WRookTrapped1 = SetMask(g1) | SetMask(h1) | SetMask(h2);
const static BitBoard WKingTrapsRook2 = SetMask(c1) | SetMask(b1);
const static BitBoard WRookTrapped2 = SetMask(b1) | SetMask(a1) | SetMask(a2);
const static BitBoard BKingTrapsRook1 = SetMask(f8) | SetMask(g8);
const static BitBoard BRookTrapped1 = SetMask(g8) | SetMask(h8) | SetMask(h7);
const static BitBoard BKingTrapsRook2 = SetMask(c8) | SetMask(b8);
const static BitBoard BRookTrapped2 = SetMask(b8) | SetMask(a8) | SetMask(a7);

/**
 * Score the pawn structure.
 *
 * This includes all scoring terms which are independent of piece placement
 * (i.e. only depend on pawn placement).
 *
 * Findings about pawnstructure (e.g. fianchetto pattern or king protection)
 * are stored in pawnFacts.
 *
 */

static int ScorePawns(struct Position *p, struct PawnFacts *pawnFacts) {
    BitBoard pcs = 0;
    int score = 0;
    int file = 0;
    int tmp_w, tmp_b;

    int kside_open_files = 0;
    int qside_open_files = 0;
    int kside_hopen_files_w = 0;
    int kside_hopen_files_b = 0;
    int qside_hopen_files_w = 0;
    int qside_hopen_files_b = 0;

    int kside_pawns_w = 0;
    int qside_pawns_w = 0;
    int kside_pawns_b = 0;
    int qside_pawns_b = 0;

    pawnFacts->pf_WhitePassers = 0;

    pcs = p->mask[White][Pawn];
    while (pcs) {
        int sq = FindSetBit(pcs);
        pcs &= pcs - 1;

        score += WPawnPos[sq];

        /*
         * check if passed
         */

        if (!(p->mask[Black][Pawn] & PassedMaskW[sq])) {
            SetBit(pawnFacts->pf_WhitePassers, sq);
        }

        /*
         * check if doubled
         */

        if (p->mask[White][Pawn] & ForwardRayW[sq])
            score += DoubledPawn;

        /*
         * check if isolated or backward
         */

        if (!(p->mask[White][Pawn] & IsoMask[sq & 7])) {
            score += IsolatedPawn[sq & 7];
#ifdef DEBUG
            if (DebugWhat & DebugPawnStructure)
                Print(2, "isolated pawn on %c%c\n", SQUARE(sq));
#endif
        } else if (!(p->mask[White][Pawn] & WPawnBackwardMask[sq]) &&
                   (p->atkFr[sq + 8] & p->mask[Black][Pawn])) {
            if (p->mask[Black][Pawn] & ForwardRayW[sq]) {
                score += HiddenBackwardPawn;
#ifdef DEBUG
                if (DebugWhat & DebugPawnStructure)
                    Print(2, "hidden backward pawn on %c%c\n", SQUARE(sq));
#endif
            } else {
#ifdef DEBUG
                if (DebugWhat & DebugPawnStructure)
                    Print(2, "backward pawn on %c%c\n", SQUARE(sq));
#endif
                score += BackwardPawn;
            }
        }

        if ((sq & 7) < 7 && p->piece[sq + 1] == Pawn) {
            score += PawnDuo;
        }
    }

    pawnFacts->pf_BlackPassers = 0;
    pcs = p->mask[Black][Pawn];

    while (pcs) {
        int sq = FindSetBit(pcs);

        pcs &= pcs - 1;
        score -= BPawnPos[sq];

        /*
         * check if passed
         */

        if (!(p->mask[White][Pawn] & PassedMaskB[sq])) {
            SetBit(pawnFacts->pf_BlackPassers, sq);
        }

        /*
         * check if doubled
         */

        if (p->mask[Black][Pawn] & ForwardRayB[sq])
            score -= DoubledPawn;

        /*
         * check if isolated or backward
         */

        if (!(p->mask[Black][Pawn] & IsoMask[sq & 7])) {
            score -= IsolatedPawn[sq & 7];
#ifdef DEBUG
            if (DebugWhat & DebugPawnStructure)
                Print(2, "isolated pawn on %c%c\n", SQUARE(sq));
#endif
        } else if (!(p->mask[Black][Pawn] & BPawnBackwardMask[sq]) &&
                   (p->atkFr[sq - 8] & p->mask[White][Pawn])) {
            if (p->mask[White][Pawn] & ForwardRayB[sq]) {
                score -= HiddenBackwardPawn;
#ifdef DEBUG
                if (DebugWhat & DebugPawnStructure)
                    Print(2, "hidden backward pawn on %c%c\n", SQUARE(sq));
#endif
            } else {
                score -= BackwardPawn;
#ifdef DEBUG
                if (DebugWhat & DebugPawnStructure)
                    Print(2, "backward pawn on %c%c\n", SQUARE(sq));
#endif
            }
        }

        if ((sq & 7) < 7 && p->piece[sq + 1] == -Pawn) {
            score -= PawnDuo;
        }
    }

    /*
     * Check for pawn majorities. We only count 'real' majorities, i.e.
     * without doubled pawns.
     */

    tmp_w = CountBits(p->mask[White][Pawn] & KingSideMask);
    tmp_b = CountBits(p->mask[Black][Pawn] & KingSideMask);

    if (tmp_w != tmp_b) {
        tmp_w = tmp_b = 0;
        for (file = 0; file < 4; file++) {
            if (p->mask[White][Pawn] & FileMask[file])
                tmp_w++;
            if (p->mask[Black][Pawn] & FileMask[file])
                tmp_b++;
        }

        if (tmp_w > tmp_b) {
            score += PawnMajority;
        } else if (tmp_b > tmp_w) {
            score -= PawnMajority;
        }
    }

    tmp_w = CountBits(p->mask[White][Pawn] & QueenSideMask);
    tmp_b = CountBits(p->mask[Black][Pawn] & QueenSideMask);

    if (tmp_w != tmp_b) {
        tmp_w = tmp_b = 0;
        for (file = 4; file < 8; file++) {
            if (p->mask[White][Pawn] & FileMask[file])
                tmp_w++;
            if (p->mask[Black][Pawn] & FileMask[file])
                tmp_b++;
        }

        if (tmp_w > tmp_b) {
            score += PawnMajority;
        } else if (tmp_b > tmp_w) {
            score -= PawnMajority;
        }
    }

    for (file = 0; file < 3; file++) {
        int open_w = !(p->mask[White][Pawn] & FileMask[file]);
        int open_b = !(p->mask[Black][Pawn] & FileMask[file]);

        /*
         * Check the queen side
         */

        if (open_w && open_b) {
            qside_open_files++;
        } else {
            if (open_w) {
                qside_hopen_files_w++;
            } else {
                if (!TstBit(p->mask[White][Pawn], a2 + file)) {
                    qside_pawns_w++;
                    if (!TstBit(p->mask[White][Pawn], a3 + file)) {
                        qside_pawns_w++;
                    }
                }
            }
            if (open_b) {
                qside_hopen_files_b++;
            } else {
                if (!TstBit(p->mask[Black][Pawn], a7 + file)) {
                    qside_pawns_b++;
                    if (!TstBit(p->mask[Black][Pawn], a6 + file)) {
                        qside_pawns_b++;
                    }
                }
            }
        }

        /*
         * Check the king side
         */

        open_w = !(p->mask[White][Pawn] & FileMask[7 - file]);
        open_b = !(p->mask[Black][Pawn] & FileMask[7 - file]);

        if (open_w && open_b) {
            kside_open_files++;
        } else {
            if (open_w) {
                kside_hopen_files_w++;
            } else {
                if (!TstBit(p->mask[White][Pawn], h2 - file)) {
                    kside_pawns_w++;
                    if (!TstBit(p->mask[White][Pawn], h3 - file)) {
                        kside_pawns_w++;
                    }
                }
            }

            if (open_b) {
                kside_hopen_files_b++;
            } else {
                if (!TstBit(p->mask[Black][Pawn], h7 - file)) {
                    kside_pawns_b++;
                    if (!TstBit(p->mask[Black][Pawn], h6 - file)) {
                        kside_pawns_b++;
                    }
                }
            }
        }
    }

#ifdef DEBUG
    if (DebugWhat & DebugPawnStructure) {
        Print(0, "open : %d %d hopen k: %d %d hopen q: %d %d\n",
              kside_open_files, qside_open_files, kside_hopen_files_w,
              kside_hopen_files_b, qside_hopen_files_w, qside_hopen_files_b);
        Print(0, "pawns w: %d %d pawns b: %d %d\n", kside_pawns_w,
              qside_pawns_w, kside_pawns_b, qside_pawns_b);
    }
#endif /* DEBUG */

    pawnFacts->pf_WhiteKingSide = kside_pawns_w +
                                  ScaleHalfOpenFilesMine[kside_hopen_files_w] +
                                  ScaleHalfOpenFilesYours[kside_hopen_files_b] +
                                  ScaleOpenFiles[kside_open_files];

    pawnFacts->pf_BlackKingSide = kside_pawns_b +
                                  ScaleHalfOpenFilesMine[kside_hopen_files_b] +
                                  ScaleHalfOpenFilesYours[kside_hopen_files_w] +
                                  ScaleOpenFiles[kside_open_files];

    pawnFacts->pf_WhiteQueenSide =
        qside_pawns_w + ScaleHalfOpenFilesMine[qside_hopen_files_w] +
        ScaleHalfOpenFilesYours[qside_hopen_files_b] +
        ScaleOpenFiles[qside_open_files];

    pawnFacts->pf_BlackQueenSide =
        qside_pawns_b + ScaleHalfOpenFilesMine[qside_hopen_files_b] +
        ScaleHalfOpenFilesYours[qside_hopen_files_w] +
        ScaleOpenFiles[qside_open_files];

#ifdef DEBUG
    if (DebugWhat & DebugPawnStructure) {
        Print(0, "king safety white: %d %d\n", pawnFacts->pf_WhiteKingSide,
              pawnFacts->pf_WhiteQueenSide);

        Print(0, "king safety black: %d %d\n", pawnFacts->pf_BlackKingSide,
              pawnFacts->pf_BlackQueenSide);
    }
#endif /* DEBUG */

    pawnFacts->pf_Flags = 0;

    pcs = p->mask[White][Pawn] | p->mask[Black][Pawn];

    if (pcs & KingSideMask) {
        pawnFacts->pf_Flags |= PawnsOnKingSide;
    }

    if (pcs & QueenSideMask) {
        pawnFacts->pf_Flags |= PawnsOnQueenSide;
    }

    if ((p->mask[White][Pawn] & FianchettoMaskWhiteKingSide) ==
        FianchettoMaskWhiteKingSide) {
        pawnFacts->pf_Flags |= FianchettoWhiteKingSide;
    }

    if ((p->mask[Black][Pawn] & FianchettoMaskBlackKingSide) ==
        FianchettoMaskBlackKingSide) {
        pawnFacts->pf_Flags |= FianchettoBlackKingSide;
    }

    if ((p->mask[White][Pawn] & FianchettoMaskWhiteQueenSide) ==
        FianchettoMaskWhiteQueenSide) {
        pawnFacts->pf_Flags |= FianchettoWhiteQueenSide;
    }

    if ((p->mask[Black][Pawn] & FianchettoMaskBlackQueenSide) ==
        FianchettoMaskBlackQueenSide) {
        pawnFacts->pf_Flags |= FianchettoBlackQueenSide;
    }

    if (p->piece[d4] == Pawn && p->piece[d5] == -Pawn) {
        pawnFacts->pf_Flags |= QueensPawnOpening;
    }

#ifdef DEBUG
    if (DebugWhat & DebugPawnStructure) {
        Print(2, "ScorePawns returns %d\n", score);
    }
#endif

    return score;
}

/**
 * Look up current pawn structure in pawn hashtable. If not present,
 * use ScorePawns() to score the pawn structure. Store result in the
 * pawn hashtable.
 *
 */

static int ScorePawnsHashed(struct Position *p, struct PawnFacts *pawnFacts) {
    int score;

    PTry++;
    if (ProbePT(p->pkey, &score, pawnFacts) != Useful) {
        score = ScorePawns(p, pawnFacts);
        StorePT(p->pkey, score, pawnFacts);
    } else {
        PHit++;
    }

    return score;
}

/**
 * Score passed pawns
 */

static int ScorePassedPawns(struct Position *p, int wphase, int bphase,
                            struct PawnFacts *pawnFacts) {
    int score = 0;
    int wdistant = 0;
    int bdistant = 0;

    BitBoard pcs;
    BitBoard tmp;
    BitBoard allpawns = p->mask[White][Pawn] | p->mask[Black][Pawn];

    BitBoard wrunner = 0;
    BitBoard brunner = 0;

    pcs = pawnFacts->pf_WhitePassers;

    while (pcs) {
        int sq = FindSetBit(pcs);
        int rank = sq >> 3;
        int file = sq & 7;

        pcs &= pcs - 1;

        /* Basic score */

        if (!TstBit(p->mask[Black][0], sq + 8)) {
            score += ScaleDown[wphase] * PassedPawn[rank] / 16;
        } else {
            score += ScaleDown[wphase] * PassedPawnBlocked[rank] / 16;
        }

        /* Score covered passed pawns. */

        if (p->atkFr[sq] & p->mask[White][Pawn]) {
            if (rank == 5) {
                score += CoveredPassedPawn6th;
            }
            if (rank == 6) {
                score += CoveredPassedPawn7th;
            }
        }

        if (ConnectedMask[sq] & pawnFacts->pf_WhitePassers) {
            int rank2 =
                FindSetBit(ConnectedMask[sq] & pawnFacts->pf_WhitePassers) >> 3;

            score +=
                ScaleDown[wphase] * PassedPawnConnected[MAX(rank, rank2)] / 16;
        }

        /* Check for rook attacks 'from behind' */

        tmp = p->atkFr[sq] & ForwardRayB[sq];

        if (tmp & p->mask[White][Rook]) {
            score += ScaleDown[wphase] * RookBehindPasser;
        } else if (tmp & p->mask[Black][Rook]) {
            score -= ScaleDown[wphase] * RookBehindPasser;
        }

        /* Check if pawn is out of the king's square */
        if (p->nonPawn[Black] == 0) {
            int sq2 = (p->turn == White) ? sq : sq - 8;
            if (!(p->mask[Black][King] & KingSquareW[sq2])) {
                SetBit(wrunner, sq);
#ifdef DEBUG
                if (DebugWhat & DebugPassedPawns) {
                    Print(0, "white runner on %c%c\n", SQUARE(sq));
                }
#endif
            }
        }

        /* Check if 'distant' passed pawn */
        if (file < 4 && !(allpawns & LeftOf[file]) &&
            (allpawns & RightOf[file]) &&
            !(p->mask[Black][Pawn] & LeftOf[file + 2])) {
#ifdef DEBUG
            if (DebugWhat & DebugPassedPawns) {
                Print(0, "white outside passend pawn on %c%c\n", SQUARE(sq));
            }
#endif

            wdistant = rank;
        }

        if (file > 3 && !(allpawns & RightOf[file]) &&
            (allpawns & LeftOf[file]) &&
            !(p->mask[Black][Pawn] & RightOf[file - 2])) {
#ifdef DEBUG
            if (DebugWhat & DebugPassedPawns) {
                Print(0, "white outside passend pawn on %c%c\n", SQUARE(sq));
            }
#endif

            wdistant = rank;
        }
    }

    pcs = pawnFacts->pf_BlackPassers;

    while (pcs) {
        int sq = FindSetBit(pcs);
        int rank = 7 - (sq >> 3);
        int file = sq & 7;

        pcs &= pcs - 1; //(pcs, sq);

        /* Basic score */

        if (!TstBit(p->mask[White][0], sq - 8)) {
            score -= ScaleDown[bphase] * PassedPawn[rank] / 16;
        } else {
            score -= ScaleDown[bphase] * PassedPawnBlocked[rank] / 16;
        }

        /* Score covered passed pawns. */

        if (p->atkFr[sq] & p->mask[Black][Pawn]) {
            if (rank == 5) {
                score -= CoveredPassedPawn6th;
            }
            if (rank == 6) {
                score -= CoveredPassedPawn7th;
            }
        }

        if (ConnectedMask[sq] & pawnFacts->pf_BlackPassers) {
            int rank2 = 7 - (FindSetBit(ConnectedMask[sq] &
                                        pawnFacts->pf_BlackPassers) >>
                             3);

            score -=
                ScaleDown[bphase] * PassedPawnConnected[MAX(rank, rank2)] / 16;
        }

        /* Check for rook attacks 'from behind' */

        tmp = p->atkFr[sq] & ForwardRayW[sq];

        if (tmp & p->mask[White][Rook]) {
            score += ScaleDown[bphase] * RookBehindPasser;
        } else if (tmp & p->mask[Black][Rook]) {
            score -= ScaleDown[bphase] * RookBehindPasser;
        }

        /* Check if pawn is out of the king's square */
        if (p->nonPawn[White] == 0) {
            int sq2 = (p->turn == Black) ? sq : sq + 8;
            if (!(p->mask[White][King] & KingSquareB[sq2])) {
                SetBit(brunner, sq);
#ifdef DEBUG
                if (DebugWhat & DebugPassedPawns) {
                    Print(0, "black runner on %c%c\n", SQUARE(sq));
                }
#endif
            }
        }

        /* Check if 'distant' passed pawn */
        if (file < 4 && !(allpawns & LeftOf[file]) &&
            (allpawns & RightOf[file]) &&
            !(p->mask[White][Pawn] & LeftOf[file + 2])) {
#ifdef DEBUG
            if (DebugWhat & DebugPassedPawns) {
                Print(0, "black outside passend pawn on %c%c\n", SQUARE(sq));
            }
#endif

            bdistant = rank;
        }

        if (file > 3 && !(allpawns & RightOf[file]) &&
            (allpawns & LeftOf[file]) &&
            !(p->mask[White][Pawn] & RightOf[file - 2])) {
#ifdef DEBUG
            if (DebugWhat & DebugPassedPawns) {
                Print(0, "black outside passend pawn on %c%c\n", SQUARE(sq));
            }
#endif

            bdistant = rank;
        }
    }

    /*
     * Evaluate pawns that can outrun the king.
     */

    if (wrunner) {
        if (!brunner) {
            score += PawnOutrunsKing;
        } else {

            /*
             * Both sides have pawns that can outrun the king.
             * Check who comes first.
             */

            int wdist = 5;
            int bdist = 5;

            while (wrunner) {
                int sq = FindSetBit(wrunner);
                int dist = 7 - (sq >> 3);
                wrunner &= wrunner - 1;

                if (dist < wdist) {
                    wdist = dist;
                }
            }

            while (brunner) {
                int sq = FindSetBit(brunner);
                int dist = (sq >> 3);
                brunner &= brunner - 1;

                if (dist < bdist) {
                    bdist = dist;
                }
            }

            if (p->turn == White) {
                if (wdist < bdist) {
                    score += PawnOutrunsKing;
                } else if (bdist <= (wdist - 2)) {
                    score -= PawnOutrunsKing;
                }
            } else {
                if (bdist < wdist) {
                    score -= PawnOutrunsKing;
                } else if (wdist <= (bdist - 2)) {
                    score += PawnOutrunsKing;
                }
            }
        }
    } else if (brunner) {
        score -= PawnOutrunsKing;
    }

    /*
     * Evaluate distant passed pawns
     */

    if (wdistant && bdistant) {
        if (wdistant > bdistant) {
            bdistant = 0;
        } else if (bdistant > wdistant) {
            wdistant = 0;
        } else {
            wdistant = bdistant = 0;
        }
    }

    if (wdistant && !bdistant) {
        score += DistantPassedPawn[wphase];
    } else if (bdistant && !wdistant) {
        score -= DistantPassedPawn[bphase];
    } else if (wdistant && bdistant) {
        if (wdistant > bdistant) {
            score += DistantPassedPawn[wphase];
        } else if (bdistant > wdistant) {
            score -= DistantPassedPawn[bphase];
        }
    }

    return score;
}

/**
 * Score the king safety
 */

static int ScoreKingSafety(struct Position *p, int wphase, int bphase,
                           struct PawnFacts *pawnFacts) {
    int score = 0;

    int king_safety_w = 0;
    int king_safety_b = 0;

    /*
     * white king safety
     */

    if ((p->kingSq[White] & 7) >= 4) {

        /* king side */

        king_safety_w = pawnFacts->pf_WhiteKingSide;

        /* test for fianchetto */

        if (pawnFacts->pf_Flags & FianchettoWhiteKingSide) {
            if (TstBit(p->mask[White][Bishop], g2)) {
                king_safety_w -= 1;
            } else if (!(p->mask[White][Bishop] & WhiteSquaresMask) &&
                       (p->mask[Black][Bishop] & WhiteSquaresMask)) {
                king_safety_w += 2;
            }
        }

    } else {

        /* queen side */

        king_safety_w = pawnFacts->pf_WhiteQueenSide;

        /* test for fianchetto */

        if (pawnFacts->pf_Flags & FianchettoWhiteQueenSide) {
            if (TstBit(p->mask[White][Bishop], b2)) {
                king_safety_w -= 1;
            } else if (!(p->mask[White][Bishop] & BlackSquaresMask) &&
                       (p->mask[Black][Bishop] & BlackSquaresMask)) {
                king_safety_w += 2;
            }
        }
    }

    if (p->mask[Black][Queen]) {
        king_safety_w *= 2;
    }

    /*
     * black king safety
     */

    if ((p->kingSq[Black] & 7) >= 4) {

        /* king side */

        king_safety_b = pawnFacts->pf_BlackKingSide;

        /* test for fianchetto, which is ok */

        if (pawnFacts->pf_Flags & FianchettoBlackKingSide) {
            if (TstBit(p->mask[Black][Bishop], g7)) {
                king_safety_b -= 1;
            } else if (!(p->mask[Black][Bishop] & BlackSquaresMask) &&
                       (p->mask[White][Bishop] & BlackSquaresMask)) {
                king_safety_b += 2;
            }
        }

    } else {

        /* queen side */

        king_safety_b = pawnFacts->pf_BlackQueenSide;

        /* test for fianchetto, which is ok */

        if (pawnFacts->pf_Flags & FianchettoBlackQueenSide) {
            if (TstBit(p->mask[Black][Bishop], b7)) {
                king_safety_b -= 1;
            } else if (!(p->mask[Black][Bishop] & WhiteSquaresMask) &&
                       (p->mask[White][Bishop] & WhiteSquaresMask)) {
                king_safety_b += 2;
            }
        }
    }

    if (p->mask[White][Queen]) {
        king_safety_b *= 2;
    }

    score = -ScaleUp[wphase] * king_safety_w + ScaleUp[bphase] * king_safety_b;

#ifdef DEBUG
    if (DebugWhat & DebugKingSafety) {
        Print(0, "king safety w: %d b: %d total: %d\n", king_safety_w,
              king_safety_b, score);
    }
#endif

    return 4 * score;
}

/**
 * Score the development status
 */

static int ScoreDevelopment(struct Position *p) {
    int score = 0;
    BitBoard pcs;

    /*
     * Don't develop pieces to e3/d3 if they block a pawn
     */

    if (p->piece[e2] == Pawn && p->piece[e3] != Neutral) {
        score += PawnDevelopmentBlocked;
    }
    if (p->piece[d2] == Pawn && p->piece[d3] != Neutral) {
        score += PawnDevelopmentBlocked;
    }

    /*
     * Don't develop pieces to e6/d6 if they block a pawn
     */

    if (p->piece[e7] == -Pawn && p->piece[e6] != Neutral) {
        score -= PawnDevelopmentBlocked;
    }
    if (p->piece[d7] == -Pawn && p->piece[d6] != Neutral) {
        score -= PawnDevelopmentBlocked;
    }

    /*
     * Don't leave the king in the center
     */

    if (p->mask[White][King] & WKingOpeningMask) {
        score += Development;
    }

    if (p->mask[Black][King] & BKingOpeningMask) {
        score -= Development;
    }

    /*
     * Don't make early queen moves
     */

    pcs = p->mask[White][Queen];
    while (pcs) {
        int sq = FindSetBit(pcs);
        pcs &= pcs - 1;
        score += QueenPosDevelopment[sq];
    }

    pcs = p->mask[Black][Queen];
    while (pcs) {
        int sq = FindSetBit(pcs);
        pcs &= pcs - 1;
        score -= QueenPosDevelopment[REFLECT_X(sq)];
    }

    return score;
}

/**
 * Score the material balance.
 */

int MaterialBalance(struct Position *p) {
    int score = p->material[White] - p->material[Black];

    return score;
}

/**
 * Score the position from white points of view.
 */

static int ScorePositionForWhite(struct Position *p, int alpha, int beta) {
    int score;

    int wphase;
    int bphase;
    const int *kingPST;
    int fastscore;
    int diff;

    int tmp, sq;
    BitBoard pcs;
    BitBoard tmpboard;
    struct PawnFacts pawnFacts;

    /*
     * Lookup the current position in the evaluation hashtable
     */

    STry++;
    if (ProbeST(p->hkey, &score) == Useful) {
        SHit++;
        return score;
    }

    score = MaterialBalance(p);
    fastscore = score;

    score += ScorePawnsHashed(p, &pawnFacts);

    /*************************************************************
     *
     * Kings
     *
     *************************************************************/

    wphase = MIN(31, p->nonPawn[Black] / Value[Pawn]);
    bphase = MIN(31, p->nonPawn[White] / Value[Pawn]);

    score += ScoreKingSafety(p, wphase, bphase, &pawnFacts);

    /*************************************************************
     *
     * Passed Pawns
     *
     *************************************************************/

    score += ScorePassedPawns(p, wphase, bphase, &pawnFacts);

    /*************************************************************
     *
     * Trapped Bishops
     *
     *************************************************************/

    if (p->piece[a7] == Bishop && p->piece[b6] == -Pawn &&
        (p->atkFr[b6] & p->mask[Black][Pawn])) {
        score += BishopTrapped;
    }
    if (p->piece[h7] == Bishop && p->piece[g6] == -Pawn &&
        (p->atkFr[g6] & p->mask[Black][Pawn])) {
        score += BishopTrapped;
    }
    if (p->piece[a2] == -Bishop && p->piece[b3] == Pawn &&
        (p->atkFr[b3] & p->mask[White][Pawn])) {
        score -= BishopTrapped;
    }
    if (p->piece[h2] == -Bishop && p->piece[g3] == Pawn &&
        (p->atkFr[g3] & p->mask[White][Pawn])) {
        score -= BishopTrapped;
    }

    /*************************************************************
     *
     * Development
     *
     *************************************************************/

    if (RootGamePhase == Opening) {
        score += ScoreDevelopment(p);
    }

    /*************************************************************
     *
     * Kings
     *
     *************************************************************/

    /*
     * Determine which piece/square table to use for kings in the endgame.
     */

    tmp = pawnFacts.pf_Flags & (PawnsOnKingSide | PawnsOnQueenSide);

    if (tmp == PawnsOnKingSide) {
        kingPST = KingPosEndgameKingSide;
    } else if (tmp == PawnsOnQueenSide) {
        kingPST = KingPosEndgameQueenSide;
    } else {
        kingPST = KingPosEndgame;
    }

    /*
     * Score white king
     */

    score += (KingPosMiddlegame[p->kingSq[White]] * ScaleUp[wphase] +
              kingPST[p->kingSq[White]] * ScaleDown[wphase]) >>
             4;

    /*
     * Check if a king which did not castle blocks a rook in a corner
     */

    if (((p->mask[White][King] & WKingTrapsRook1) &&
         (p->mask[White][Rook] & WRookTrapped1)) ||
        ((p->mask[White][King] & WKingTrapsRook2) &&
         (p->mask[White][Rook] & WRookTrapped2))) {
        score += KingBlocksRook;
    };

    /*
     * Score black king
     */

    score -= (KingPosMiddlegame[REFLECT_X(p->kingSq[Black])] * ScaleUp[bphase] +
              kingPST[REFLECT_X(p->kingSq[Black])] * ScaleDown[bphase]) >>
             4;

    /*
     * Check if a king which did not castle blocks a rook in a corner
     */

    if (((p->mask[Black][King] & BKingTrapsRook1) &&
         (p->mask[Black][Rook] & BRookTrapped1)) ||
        ((p->mask[Black][King] & BKingTrapsRook2) &&
         (p->mask[Black][Rook] & BRookTrapped2))) {
        score -= KingBlocksRook;
    };

    /*************************************************************
     *
     * Knights
     *
     *************************************************************/

    /*
     * Score white knights
     */

    pcs = p->mask[White][Knight];
    while (pcs) {
        sq = FindSetBit(pcs);
        pcs &= pcs - 1;

        score += KnightPos[sq];

        if (!(p->mask[Black][Pawn] & OutpostMaskW[sq])) {
            score += KnightOutpost[sq];
        }

        score += (ScaleUp[wphase] * KnightKingProximity *
                  (4 - KingDist(sq, p->kingSq[Black]))) >>
                 4;

        if (sq == c3 && pawnFacts.pf_Flags & QueensPawnOpening &&
            p->piece[c2] == Pawn) {
            score += KnightBlocksCPawn;
        }
    }

    /*
     * Score black knights
     */

    pcs = p->mask[Black][Knight];
    while (pcs) {
        sq = FindSetBit(pcs);
        pcs &= pcs - 1;

        score -= KnightPos[REFLECT_X(sq)];

        if (!(p->mask[White][Pawn] & OutpostMaskB[sq])) {
            score -= KnightOutpost[REFLECT_X(sq)];
        }

        score -= (ScaleUp[bphase] * KnightKingProximity *
                  (4 - KingDist(sq, p->kingSq[White]))) >>
                 4;

        if (sq == c6 && pawnFacts.pf_Flags & QueensPawnOpening &&
            p->piece[c7] == -Pawn) {
            score -= KnightBlocksCPawn;
        }
    }

    /*************************************************************
     *
     * Bishops
     *
     *************************************************************/

    /*
     * Score white bishops
     */

    pcs = p->mask[White][Bishop];

    if ((pcs & WhiteSquaresMask) && (pcs & BlackSquaresMask)) {
        score += BishopPair[CountBits(p->mask[White][Pawn])];
    }

    while (pcs) {
        sq = FindSetBit(pcs);
        pcs &= pcs - 1;

        score += (ScaleUp[wphase] * BishopPos[sq]) >> 4;

        tmp = CountBits(p->atkTo[sq] & ~p->mask[White][0]);
        score += BishopMobility * (tmp - 7);

        score += (ScaleUp[wphase] * BishopKingProximity *
                  (4 - KingDist(sq, p->kingSq[Black]))) >>
                 4;
    }

    /*
     * Score black bishops
     */

    pcs = p->mask[Black][Bishop];

    if ((pcs & WhiteSquaresMask) && (pcs & BlackSquaresMask)) {
        score -= BishopPair[CountBits(p->mask[Black][Pawn])];
    }

    while (pcs) {
        sq = FindSetBit(pcs);
        pcs &= pcs - 1;

        score -= (ScaleUp[bphase] * BishopPos[REFLECT_X(sq)]) >> 4;

        tmp = CountBits(p->atkTo[sq] & ~p->mask[Black][0]);
        score -= BishopMobility * (tmp - 7);

        score -= (ScaleUp[bphase] * BishopKingProximity *
                  (4 - KingDist(sq, p->kingSq[White]))) >>
                 4;
    }

    /*************************************************************
     *
     * Rooks
     *
     *************************************************************/

    /*
     * Score white rooks
     */

    pcs = p->mask[White][Rook];
    while (pcs) {
        int file;

        sq = FindSetBit(pcs);
        pcs &= pcs - 1;
        file = sq & 7;

        score += (ScaleUp[wphase] * RookPos[sq]) >> 4;

        tmp = CountBits(p->atkTo[sq] & ~p->mask[White][0]);
        score += RookMobility * (tmp - 7);

        if (!(FileMask[file] & p->mask[White][Pawn])) {
            if (!(FileMask[file] & p->mask[Black][Pawn])) {
                score += RookOnOpenFile;
            } else {
                score += RookOnSemiOpenFile;
            }
        }

        score += (ScaleUp[wphase] * RookKingProximity *
                  (4 - KingDist(sq, p->kingSq[Black]))) >>
                 4;

        tmpboard = p->atkTo[sq] & ForwardRayW[sq];
        if (tmpboard & p->mask[White][Rook]) {
            score += RookConnected;
        }

        if ((sq >> 3) == 6 && p->kingSq[Black] >= a8) {
            score += RookOn7thRank;
        }
    }

    /*
     * Score black rooks
     */

    pcs = p->mask[Black][Rook];
    while (pcs) {
        int file;
        sq = FindSetBit(pcs);
        pcs &= pcs - 1;
        file = sq & 7;

        score -= (ScaleUp[bphase] * RookPos[REFLECT_X(sq)]) >> 4;

        tmp = CountBits(p->atkTo[sq] & ~p->mask[Black][0]);
        score -= RookMobility * (tmp - 7);

        if (!(FileMask[file] & p->mask[Black][Pawn])) {
            if (!(FileMask[file] & p->mask[White][Pawn])) {
                score -= RookOnOpenFile;
            } else {
                score -= RookOnSemiOpenFile;
            }
        }

        score -= (ScaleUp[bphase] * RookKingProximity *
                  (4 - KingDist(sq, p->kingSq[White]))) >>
                 4;

        tmpboard = p->atkTo[sq] & ForwardRayB[sq];
        if (tmpboard & p->mask[Black][Rook]) {
            score -= RookConnected;
        }

        if ((sq >> 3) == 1 && p->kingSq[White] <= h1) {
            score -= RookOn7thRank;
        }
    }

    /*************************************************************
     *
     * Queens
     *
     *************************************************************/

    /*
     * Score white queens
     */

    pcs = p->mask[White][Queen];
    while (pcs) {
        sq = FindSetBit(pcs);
        pcs &= pcs - 1;

        score += QueenPos[sq];

        score += (ScaleUp[wphase] * QueenKingProximity *
                  (4 - KingDist(sq, p->kingSq[Black]))) >>
                 4;
    }

    /*
     * Score black queens
     */

    pcs = p->mask[Black][Queen];
    while (pcs) {
        sq = FindSetBit(pcs);
        pcs &= pcs - 1;

        score -= QueenPos[REFLECT_X(sq)];

        score -= (ScaleUp[bphase] * QueenKingProximity *
                  (4 - KingDist(sq, p->kingSq[White]))) >>
                 4;
    }

    /*
     * Check if both sides have only one bishops. If so, and they are opposite
     * colored, scale the score down.
     */

    if (((p->material_signature[White] & 0x1e) == SIGNATURE_BIT(Bishop)) &&
        ((p->material_signature[Black] & 0x1e) == SIGNATURE_BIT(Bishop))) {
        int white = (p->mask[White][Bishop] & WhiteSquaresMask) != 0;
        int black = (p->mask[Black][Bishop] & WhiteSquaresMask) != 0;

        if ((!white && black) || (white && !black)) {
            score = 4 * score / 5;
        }
    }

    /*
     * Adjust MaxPos if necessary. If the current difference is greater than
     * MaxPos, adjust MaxPos. Otherwise slowly tune MaxPos down to MaxPosInit.
     */

    diff = ABS(score - fastscore);
    if (diff > MaxPos) {
        MaxPos = MAX(diff, MaxPos + 100);
    } else {
        MaxPos = (MaxPosInit + 31 * MaxPos) >> 5;
    }

    score = (score + 7) & ~15;

    StoreST(p->hkey, score);

    return score;
}

/**
 * This is just a convenience routine which handles sign switches
 * if it is not white to move.
 */

int ScorePosition(struct Position *p, int alpha, int beta) {
    if (p->turn == White)
        return ScorePositionForWhite(p, alpha, beta);
    else
        return -ScorePositionForWhite(p, -beta, -alpha);
}

/**
 * Do the pre-search initialization of scoring.
 */

void InitScore(struct Position *p) {
    int sq;

    int eg_threshold = Value[Queen] + Value[Bishop];

    int npmat = (p->nonPawn[White] + p->nonPawn[Black]) / Value[Pawn];

    int wkfile = p->kingSq[White] & 7;
    int bkfile = p->kingSq[Black] & 7;
    int pawnstorm = 0;

    if (wkfile < 3 && bkfile > 4) {
        pawnstorm = 1;
    } else if (wkfile > 4 && bkfile < 3) {
        pawnstorm = 2;
    }

    /*
     * Setup pawn piece/square tables
     */

    for (sq = a2; sq <= h7; sq++) {
        int wrank = (sq >> 3) - 1;
        int brank = 6 - (sq >> 3);
        int wfile = sq & 7;
        int bfile = sq & 7;

        if ((p->kingSq[White] & 7) < 4)
            wfile = 7 - wfile;
        if ((p->kingSq[Black] & 7) < 4)
            bfile = 7 - bfile;

        if (p->nonPawn[Black] < eg_threshold) {
            WPawnPos[sq] = PawnAdvanceEndgame[wfile] * wrank;
        } else if (p->castle & 3) {
            WPawnPos[sq] = PawnAdvanceOpening[wfile] * wrank;
        } else {
            WPawnPos[sq] = PawnAdvanceMiddlegame[wfile] * wrank;
            if (pawnstorm == 1 && wfile > 4) {
                WPawnPos[sq] += PawnStorm * wrank;
            } else if (pawnstorm == 2 && wfile < 3) {
                WPawnPos[sq] += PawnStorm * wrank;
            }
        }

        if (p->nonPawn[White] < eg_threshold) {
            BPawnPos[sq] = PawnAdvanceEndgame[bfile] * brank;
        } else if (p->castle & 12) {
            BPawnPos[sq] = PawnAdvanceOpening[bfile] * brank;
        } else {
            BPawnPos[sq] = PawnAdvanceMiddlegame[bfile] * brank;
            if (pawnstorm == 1 && bfile < 3) {
                BPawnPos[sq] += PawnStorm * brank;
            } else if (pawnstorm == 2 && bfile > 4) {
                BPawnPos[sq] += PawnStorm * brank;
            }
        }
    }

    WPawnPos[d2] += CrampingPawn;
    WPawnPos[e2] += CrampingPawn;
    BPawnPos[d7] += CrampingPawn;
    BPawnPos[e7] += CrampingPawn;

    ClearPawnHashTable();

#ifdef DEBUG
    DebugWhat = DebugKingSafety;
    ScorePositionForWhite(p, -999999, 999999);
    DebugWhat = 0;
#endif

    /*
     * Determine if we are still in the development phase
     */

    RootGamePhase = Middlegame;
    if (npmat >= 38) {
        bool devel = (p->castle != 0);
        int backrank =
            CountBits((p->mask[White][Knight] | p->mask[White][Bishop]) &
                      RankMask[0]) +
            CountBits((p->mask[Black][Knight] | p->mask[Black][Bishop]) &
                      RankMask[7]);
        if (backrank > 0)
            devel = true;

        if (devel)
            RootGamePhase = Opening;
    }

    Print(2, "GamePhase: %s\n", GamePhaseName[RootGamePhase]);

    MaxPos = MaxPosInit;
}
