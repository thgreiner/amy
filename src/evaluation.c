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
 * evaluation.c - positional evaluation routines
 */

#include "amy.h"
#include "dbase.h"
#include "hashtable.h"
#include "init.h"
#include "inline.h"
#include "recog.h"
#include "utils.h"

#define REFLECT_X(a) ((a) ^ 0x38)

/**
 * Debugging stuff
 */

// #define DEBUG
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
 * Pawn scoring parameters
 */

int DoubledPawn = -70;
int BackwardPawn = -100;
int HiddenBackwardPawn = -70;
int PawnOutrunsKing = 6000;
int PawnDevelopmentBlocked = -100;
int PawnDuo = 15;
int PawnStorm = 10;
int CrampingPawn = -160;
int PawnMajority = 100;

int CoveredPassedPawn6th = 200;
int CoveredPassedPawn7th = 600;

int16_t PassedPawn[] = {0, 32, 64, 128, 256, 512, 1024, 0};

int16_t PassedPawnBlocked[] = {0, 16, 48, 96, 192, 384, 768, 0};

int16_t PassedPawnConnected[] = {0, 2, 6, 12, 24, 48, 96, 0};

int16_t IsolatedPawn[] = {-70, -80, -90, -100, -100, -90, -80, -70};

int16_t PawnAdvanceOpening[] = {-10, -10, 5, 10, 10, -20, -50, -50};

int16_t PawnAdvanceMiddlegame[] = {0, 0, 10, 15, 15, 10, 0, 0};

int16_t PawnAdvanceEndgame[] = {10, 10, 10, 10, 10, 10, 10, 10};

int16_t DistantPassedPawn[] = {500, 300, 300, 300, 200, 200, 150, 150, 150,
                               100, 100, 50,  50,  50,  0,   0,   0,   0,
                               0,   0,   0,   0,   0,   0,   0,   0,   0,
                               0,   0,   0,   0,   0,   0,   0,   0};

static int16_t WPawnPos[64];
static int16_t BPawnPos[64];

/**
 * Knight scoring parameters
 */

int KnightKingProximity = 7;
int KnightBlocksCPawn = -100;
int KnightEdgePenalty = -130;

// clang-format off
int16_t KnightPos[64] = {
     -30,  -30,  -30,  -30,  -30,  -30,  -30,  -30,
     -30,  -30,   60,   60,   60,   60,  -30,  -30,
     -30,   60,  130,  130,  130,  130,   60,  -30,
     -30,  130,  190,  190,  190,  190,  130,  -30,
       0,  130,  190,  250,  250,  190,  130,    0,
       0,  190,  250,  250,  250,  250,  190,    0,
       0,   90,  160,  160,  160,  160,   90,    0,
       0,    0,    0,    0,    0,    0,    0,    0
};

int16_t KnightOutpost[64] = {
    0, 0,  0,   0,   0,  0, 0, 0,
    0, 0,  0,   0,   0,  0, 0, 0,
    0, 0,  0,   0,   0,  0, 0, 0,
    0, 0,  0,  40,  40,  0, 0, 0,
    0, 0, 80, 100, 100, 80, 0, 0,
    0, 0, 80, 120, 120, 80, 0, 0,
    0, 0, 40,  80,  80, 40, 0, 0,
    0, 0,  0,   0,   0,  0, 0, 0
};
// clang-format on

/**
 * Bishop scoring parameters
 */

/*
 * The value of the bishop pair depends on the number of white pawns.
 */
int16_t BishopPair[] = {200, 200, 200, 200, 200, 200, 200, 150, 100};

int BishopMobility = 25;
int BishopKingProximity = 7;
int BishopTrapped = -1500;

// clang-format off
int16_t BishopPos[64] = {
     60,  60,  60,  60,  60,  60,  60,  60,
     60, 250,  60,  60,  60,  60, 250,  60,
     60, 160, 160, 160, 160, 160, 160,  60,
    160, 250, 280, 340, 340, 280, 250, 160,
    160, 250, 280, 340, 340, 280, 250, 160,
    160, 250, 280, 280, 280, 280, 250, 160,
    160, 250, 250, 250, 250, 250, 250, 160,
    160, 160, 160, 160, 160, 160, 160, 160
};
// clang-format on

/**
 * Rook scoring parameters
 */

int RookMobility = 10;

int RookOnOpenFile = 100;
int RookOnSemiOpenFile = 25;

int RookKingProximity = 5;
int RookConnected = 60;

int RookBehindPasser = 12; /* will be scaled by phase */

int RookOn7thRank = 300;

// clang-format off
int16_t RookPos[64] = {
      0,  90, 130, 220, 220, 130,  90,   0,
      0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,
    130, 130, 130, 130, 130, 130, 130, 130,
    200, 200, 200, 200, 200, 200, 200, 200,
    200, 200, 200, 200, 200, 200, 200, 200
};
// clang-format on

/**
 * Queen scoring parameters
 */

int QueenKingProximity = 8;

// clang-format off
int16_t QueenPos[64] = {
    0, 0,  0,  0,  0,  0,  0,  0,
    0, 30, 30, 30, 30, 30, 30, 0,
    0, 30, 60, 60, 60, 60, 30, 0,
    0, 30, 60, 90, 90, 60, 30, 0,
    0, 30, 60, 90, 90, 60, 30, 0,
    0, 30, 60, 60, 60, 60, 30, 0,
    0, 30, 30, 60, 60, 30, 30, 0,
    0, 0,  0,  0,  0,  0,  0,  0
};

int16_t QueenPosDevelopment[64] = {
    -200, -200,  0,  0,  0, 0, -200, -200,
    -200, -200, 30, 30, 30, 0, -200, -200,
    -200, -200,  0,  0,  0, 0, -200, -200,
    -200, -200,  0,  0,  0, 0, -200, -200,
    -200, -200,  0,  0,  0, 0, -200, -200,
    -200, -200,  0,  0,  0, 0, -200, -200,
    -200, -200,  0,  0,  0, 0, -200, -200,
    -200, -200,  0,  0,  0, 0, -200, -200,
};
// clang-format on

/**
 * King scoring parameters
 */

int KingBlocksRook = -300;
int KingInCenter = -100;
int KingSafetyScale = 1024;

// clang-format off
int16_t KingPosMiddlegame[64] = {
    -100, 0,    -200, -300, -300, -200,    0, -100,
    -100, -100, -200, -300, -300, -200, -100, -100,
    -300, -300, -300, -300, -300, -300, -300, -300,
    -400, -400, -400, -400, -400, -400, -400, -400,
    -500, -500, -500, -500, -500, -500, -500, -500,
    -600, -600, -600, -600, -600, -600, -600, -600,
    -700, -700, -700, -700, -700, -700, -700, -700,
    -800, -800, -800, -800, -800, -800, -800, -800};

int16_t KingPosEndgame[64] = {
    -300, -300, -300, -300, -300, -300, -300, -300,
    -300, -200, -100, -100, -100, -100, -200, -300,
    -300, -100,    0,  100,  100,    0, -100, -300,
    -300, -100,  100,  200,  200,  100, -100, -300,
    -300, -100,  200,  300,  300,  200, -100, -300,
    -300, -100,  200,  300,  300,  200, -100, -300,
    -300, -100, -100, -100, -100, -100, -100, -300,
    -300, -300, -300, -300, -300, -300, -300, -300};

int16_t KingPosEndgameQueenSide[64] = {
    -300, -300, -300, -300, -300, -400, -500, -600,
    -100, -100, -100, -100, -100, -200, -300, -600,
       0,  100,  100,    0, -100, -200, -300, -600,
     100,  200,  200,  100, -100, -200, -300, -600,
     200,  300,  300,  200, -100, -200, -300, -600,
     200,  300,  300,  200, -100, -200, -300, -600,
    -100, -100, -100, -100, -100, -200, -300, -600,
    -300, -300, -300, -300, -300, -400, -500, -600};
// clang-format on

// Calculated by mirroring KingPosEndgameQueenSide
static int16_t KingPosEndgameKingSide[64];

int16_t ScaleHalfOpenFilesMine[] = {0, 4, 7, 9, 11};

int16_t ScaleHalfOpenFilesYours[] = {0, 2, 3, 4, 5};

int16_t ScaleOpenFiles[] = {0, 8, 13, 16, 19};

static const int16_t ScaleUp[] = {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
                                  0,  0,  0,  1,  2,  4,  6,  8,  10, 12, 14,
                                  15, 16, 16, 16, 16, 16, 16, 16, 16, 16};

static const int16_t ScaleDown[] = {16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
                                    16, 16, 16, 15, 14, 12, 10, 8,  6,  4,  2,
                                    1,  0,  0,  0,  0,  0,  0,  0,  0,  0};

/*
 * MaxPos is the maximum difference between the material balance and
 * a positional evaluation.
 */

int MaxPos;
static const int MaxPosInit = 2000;

/*
 * These scoring parameters will be shared among function calls.
 */

static int RootGamePhase;

/**
 * Masks used in EvaluatePawns.
 */

static const BitBoard FianchettoMaskWhiteKingSide =
    SetMask(f2) | SetMask(g3) | SetMask(h2);
static const BitBoard FianchettoMaskBlackKingSide =
    SetMask(f7) | SetMask(g6) | SetMask(h7);
static const BitBoard FianchettoMaskWhiteQueenSide =
    SetMask(c2) | SetMask(b3) | SetMask(a2);
static const BitBoard FianchettoMaskBlackQueenSide =
    SetMask(c7) | SetMask(b6) | SetMask(a7);

/**
 * Masks used in EvaluateDevelopment.
 */
static const BitBoard WKingOpeningMask = SetMask(e1) | SetMask(d1);
static const BitBoard BKingOpeningMask = SetMask(e8) | SetMask(d8);

static const BitBoard WKingTrapsRook1 = SetMask(f1) | SetMask(g1);
static const BitBoard WRookTrapped1 = SetMask(g1) | SetMask(h1) | SetMask(h2);
static const BitBoard WKingTrapsRook2 = SetMask(c1) | SetMask(b1);
static const BitBoard WRookTrapped2 = SetMask(b1) | SetMask(a1) | SetMask(a2);
static const BitBoard BKingTrapsRook1 = SetMask(f8) | SetMask(g8);
static const BitBoard BRookTrapped1 = SetMask(g8) | SetMask(h8) | SetMask(h7);
static const BitBoard BKingTrapsRook2 = SetMask(c8) | SetMask(b8);
static const BitBoard BRookTrapped2 = SetMask(b8) | SetMask(a8) | SetMask(a7);

static void create_mirrored_piece_square_table(int16_t *, int16_t *);
static bool is_edge(unsigned int);

/**
 * Evaluate the pawn structure.
 *
 * This includes all scoring terms which are independent of piece placement
 * (i.e. only depend on pawn placement).
 *
 * Findings about pawnstructure (e.g. fianchetto pattern or king protection)
 * are stored in pawnFacts.
 *
 */

static int EvaluatePawns(const struct Position *p,
                         struct PawnFacts *pawnFacts) {
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

        if ((sq & 7) < 7 && TstBit(p->mask[White][Pawn], sq + 1)) {
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

        if ((sq & 7) < 7 && TstBit(p->mask[Black][Pawn], sq + 1)) {
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

    if (TstBit(p->mask[White][Pawn], d4) && TstBit(p->mask[Black][Pawn], d5)) {
        pawnFacts->pf_Flags |= QueensPawnOpening;
    }

#ifdef DEBUG
    if (DebugWhat & DebugPawnStructure) {
        Print(2, "EvaluatePawns returns %d\n", score);
    }
#endif

    return score;
}

/**
 * Look up current pawn structure in pawn hashtable. If not present,
 * use EvaluatePawns() to score the pawn structure. Store result in the
 * pawn hashtable.
 *
 */

static int EvaluatePawnsHashed(const struct Position *p,
                               struct PawnFacts *pawnFacts) {
    int score;

    PTry++;
    if (ProbePT(p->pkey, &score, pawnFacts) != Useful) {
        score = EvaluatePawns(p, pawnFacts);
        StorePT(p->pkey, score, pawnFacts);
    } else {
        PHit++;
    }

    return score;
}

/**
 * Evaluate passed pawns
 */

static int EvaluatePassedPawns(const struct Position *p, int wphase, int bphase,
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
#ifdef DEBUG
        int score_at_start = score;
#endif
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

#ifdef DEBUG
        if (DebugWhat & DebugPassedPawns) {
            int increment = score - score_at_start;
            Print(0, "score increment from %c%c: %d\n", SQUARE(sq), increment);
        }
#endif

        /* Evaluate covered passed pawns. */

        if (p->atkFr[sq] & p->mask[White][Pawn]) {
            if (rank == 5) {
                score += CoveredPassedPawn6th;
            }
            if (rank == 6) {
                score += CoveredPassedPawn7th;
            }
        }

#ifdef DEBUG
        if (DebugWhat & DebugPassedPawns) {
            int increment = score - score_at_start;
            Print(0, "score increment from %c%c: %d\n", SQUARE(sq), increment);
        }
#endif

        if (ConnectedMask[sq] & pawnFacts->pf_WhitePassers) {
            int max_rank = rank;
            BitBoard tmp2 = ConnectedMask[sq] & pawnFacts->pf_WhitePassers;
            while (tmp2) {
                int sq2 = FindSetBit(tmp2);
                tmp2 &= tmp2 - 1;
                int rank2 = sq2 >> 3;
                max_rank = MAX(rank2, max_rank);
            }
            score += ScaleDown[wphase] * PassedPawnConnected[max_rank] / 16;
        }

#ifdef DEBUG
        if (DebugWhat & DebugPassedPawns) {
            int increment = score - score_at_start;
            Print(0, "score increment from %c%c: %d\n", SQUARE(sq), increment);
        }
#endif

        /* Check for rook attacks 'from behind' */

        tmp = p->atkFr[sq] & ForwardRayB[sq];

        if (tmp & p->mask[White][Rook]) {
            score += ScaleDown[wphase] * RookBehindPasser;
        } else if (tmp & p->mask[Black][Rook]) {
            score -= ScaleDown[wphase] * RookBehindPasser;
        }

#ifdef DEBUG
        if (DebugWhat & DebugPassedPawns) {
            int increment = score - score_at_start;
            Print(0, "score increment from %c%c: %d\n", SQUARE(sq), increment);
        }
#endif

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
                Print(0, "white outside passed pawn on %c%c\n", SQUARE(sq));
            }
#endif

            wdistant = MAX(wdistant, rank);
        }

        if (file > 3 && !(allpawns & RightOf[file]) &&
            (allpawns & LeftOf[file]) &&
            !(p->mask[Black][Pawn] & RightOf[file - 2])) {
#ifdef DEBUG
            if (DebugWhat & DebugPassedPawns) {
                Print(0, "white outside passed pawn on %c%c\n", SQUARE(sq));
            }
#endif

            wdistant = MAX(wdistant, rank);
        }
    }

#ifdef DEBUG
    if (DebugWhat & DebugPassedPawns) {
        Print(0, "After white pawn basic scoring: %d\n", score);
    }
#endif

    pcs = pawnFacts->pf_BlackPassers;

    while (pcs) {
#ifdef DEBUG
        int score_at_start = score;
#endif
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

#ifdef DEBUG
        if (DebugWhat & DebugPassedPawns) {
            int increment = score - score_at_start;
            Print(0, "score increment from %c%c: %d\n", SQUARE(sq), increment);
        }
#endif

        /* Evaluate covered passed pawns. */

        if (p->atkFr[sq] & p->mask[Black][Pawn]) {
            if (rank == 5) {
                score -= CoveredPassedPawn6th;
            }
            if (rank == 6) {
                score -= CoveredPassedPawn7th;
            }
        }

#ifdef DEBUG
        if (DebugWhat & DebugPassedPawns) {
            int increment = score - score_at_start;
            Print(0, "score increment from %c%c: %d\n", SQUARE(sq), increment);
        }
#endif

        if (ConnectedMask[sq] & pawnFacts->pf_BlackPassers) {
            int max_rank = rank;
            BitBoard tmp2 = ConnectedMask[sq] & pawnFacts->pf_BlackPassers;
            while (tmp2) {
                int sq2 = FindSetBit(tmp2);
                tmp2 &= tmp2 - 1;
                int rank2 = 7 - (sq2 >> 3);
                max_rank = MAX(rank2, max_rank);
            }
            score -= ScaleDown[bphase] * PassedPawnConnected[max_rank] / 16;
        }

#ifdef DEBUG
        if (DebugWhat & DebugPassedPawns) {
            int increment = score - score_at_start;
            Print(0, "score increment from %c%c: %d\n", SQUARE(sq), increment);
        }
#endif

        /* Check for rook attacks 'from behind' */

        tmp = p->atkFr[sq] & ForwardRayW[sq];

        if (tmp & p->mask[White][Rook]) {
            score += ScaleDown[bphase] * RookBehindPasser;
        } else if (tmp & p->mask[Black][Rook]) {
            score -= ScaleDown[bphase] * RookBehindPasser;
        }

#ifdef DEBUG
        if (DebugWhat & DebugPassedPawns) {
            int increment = score - score_at_start;
            Print(0, "score increment from %c%c: %d\n", SQUARE(sq), increment);
        }
#endif

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
                Print(0, "black outside passed pawn on %c%c\n", SQUARE(sq));
            }
#endif

            bdistant = MAX(bdistant, rank);
        }

        if (file > 3 && !(allpawns & RightOf[file]) &&
            (allpawns & LeftOf[file]) &&
            !(p->mask[White][Pawn] & RightOf[file - 2])) {
#ifdef DEBUG
            if (DebugWhat & DebugPassedPawns) {
                Print(0, "black outside passed pawn on %c%c\n", SQUARE(sq));
            }
#endif

            bdistant = MAX(bdistant, rank);
        }
    }

#ifdef DEBUG
    if (DebugWhat & DebugPassedPawns) {
        Print(0, "After black pawn basic scoring: %d\n", score);
    }
#endif

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

#ifdef DEBUG
    if (DebugWhat & DebugPassedPawns) {
        Print(0, "Before runner scoring: wdistant: %d bdistant: %d\n", wdistant,
              bdistant);
    }
#endif

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

#ifdef DEBUG
    if (DebugWhat & DebugPassedPawns) {
        Print(0, "After runner scoring: %d\n", score);
    }
#endif

    return score;
}

/**
 * Evaluate the king safety
 */

static int EvaluateKingSafety(const struct Position *p, int wphase, int bphase,
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

    return (KingSafetyScale * score) / 256;
}

/**
 * Evaluate the development status
 */

static int EvaluateDevelopment(const struct Position *p) {
    int score = 0;
    BitBoard pcs;

    /*
     * Don't develop pieces to e3/d3 if they block a pawn
     */

    if (TstBit(p->mask[White][Pawn], e2) && TstBit(p->mask[White][0], e3)) {
        score += PawnDevelopmentBlocked;
    }
    if (TstBit(p->mask[White][Pawn], d2) && TstBit(p->mask[White][0], d3)) {
        score += PawnDevelopmentBlocked;
    }

    /*
     * Don't develop pieces to e6/d6 if they block a pawn
     */

    if (TstBit(p->mask[Black][Pawn], e7) && TstBit(p->mask[Black][0], e6)) {
        score -= PawnDevelopmentBlocked;
    }
    if (TstBit(p->mask[Black][Pawn], d7) && TstBit(p->mask[Black][0], d6)) {
        score -= PawnDevelopmentBlocked;
    }

    /*
     * Don't leave the king in the center
     */

    if (p->mask[White][King] & WKingOpeningMask) {
        score += KingInCenter;
    }

    if (p->mask[Black][King] & BKingOpeningMask) {
        score -= KingInCenter;
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
 * Evaluate the material balance.
 */

int MaterialBalance(const struct Position *p) {
    int score = p->material[White] - p->material[Black];

    return score;
}

/**
 * Evaluate the position from white points of view.
 */

static int EvaluatePositionForWhite(const struct Position *p) {
    int score;

    int wphase;
    int bphase;
    const int16_t *kingPST;
    int fastscore;
    int diff;

    int tmp, sq;
    BitBoard pcs;
    BitBoard tmpboard;
    struct PawnFacts pawnFacts;

    /*
     * Lookup the current position in the evaluation hashtable
     */

#ifndef DEBUG
    STry++;
    if (ProbeST(p->hkey, &score) == Useful) {
        SHit++;
        return score;
    }
#endif

    score = MaterialBalance(p);
    fastscore = score;

#ifdef DEBUG
    if (DebugWhat & DebugPieces) {
        Print(0, "After material balance: %d\n", score);
    }
#endif

    score += EvaluatePawnsHashed(p, &pawnFacts);

#ifdef DEBUG
    if (DebugWhat & DebugPieces) {
        Print(0, "After hashed pawns: %d\n", score);
    }
#endif

    /*************************************************************
     *
     * Kings
     *
     *************************************************************/

    wphase = MIN(31, p->nonPawn[Black] / Value[Pawn]);
    bphase = MIN(31, p->nonPawn[White] / Value[Pawn]);

    score += EvaluateKingSafety(p, wphase, bphase, &pawnFacts);

#ifdef DEBUG
    if (DebugWhat & DebugPieces) {
        Print(0, "After king safety: %d\n", score);
    }
#endif

    /*************************************************************
     *
     * Passed Pawns
     *
     *************************************************************/

    score += EvaluatePassedPawns(p, wphase, bphase, &pawnFacts);

#ifdef DEBUG
    if (DebugWhat & DebugPieces) {
        Print(0, "After passed pawns: %d\n", score);
    }
#endif

    /*************************************************************
     *
     * Trapped Bishops
     *
     *************************************************************/

    if (TstBit(p->mask[White][Bishop], a7) &&
        TstBit(p->mask[Black][Pawn], b6) &&
        (p->atkFr[b6] & p->mask[Black][Pawn])) {
        score += BishopTrapped;
    }
    if (TstBit(p->mask[White][Bishop], h7) &&
        TstBit(p->mask[Black][Pawn], g6) &&
        (p->atkFr[g6] & p->mask[Black][Pawn])) {
        score += BishopTrapped;
    }
    if (TstBit(p->mask[Black][Bishop], a2) &&
        TstBit(p->mask[White][Pawn], b3) &&
        (p->atkFr[b3] & p->mask[White][Pawn])) {
        score -= BishopTrapped;
    }
    if (TstBit(p->mask[Black][Bishop], h2) &&
        TstBit(p->mask[White][Pawn], g3) &&
        (p->atkFr[g3] & p->mask[White][Pawn])) {
        score -= BishopTrapped;
    }

#ifdef DEBUG
    if (DebugWhat & DebugPieces) {
        Print(0, "After trapped bishops: %d\n", score);
    }
#endif

    /*************************************************************
     *
     * Development
     *
     *************************************************************/

    if (RootGamePhase == Opening) {
        score += EvaluateDevelopment(p);
    }

#ifdef DEBUG
    if (DebugWhat & DebugPieces) {
        Print(0, "After development: %d\n", score);
    }
#endif

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
     * Evaluate white king
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
     * Evaluate black king
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

#ifdef DEBUG
    if (DebugWhat & DebugPieces) {
        Print(0, "After king: %d\n", score);
    }
#endif

    /*************************************************************
     *
     * Knights
     *
     *************************************************************/

    /*
     * Evaluate white knights
     */

    pcs = p->mask[White][Knight];
    while (pcs) {
        sq = FindSetBit(pcs);
        pcs &= pcs - 1;

        score += KnightPos[sq];

        if (is_edge(sq)) {
            score += KnightEdgePenalty;
        }

        if (!(p->mask[Black][Pawn] & OutpostMaskW[sq])) {
            score += KnightOutpost[sq];
        }

        score += (ScaleUp[wphase] * KnightKingProximity *
                  (4 - KingDist(sq, p->kingSq[Black]))) >>
                 4;

        if (sq == c3 && pawnFacts.pf_Flags & QueensPawnOpening &&
            TstBit(p->mask[White][Pawn], c2)) {
            score += KnightBlocksCPawn;
        }
    }

    /*
     * Evaluate black knights
     */

    pcs = p->mask[Black][Knight];
    while (pcs) {
        sq = FindSetBit(pcs);
        pcs &= pcs - 1;

        score -= KnightPos[REFLECT_X(sq)];

        if (is_edge(sq)) {
            score -= KnightEdgePenalty;
        }

        if (!(p->mask[White][Pawn] & OutpostMaskB[sq])) {
            score -= KnightOutpost[REFLECT_X(sq)];
        }

        score -= (ScaleUp[bphase] * KnightKingProximity *
                  (4 - KingDist(sq, p->kingSq[White]))) >>
                 4;

        if (sq == c6 && pawnFacts.pf_Flags & QueensPawnOpening &&
            TstBit(p->mask[Black][Pawn], c7)) {
            score -= KnightBlocksCPawn;
        }
    }

#ifdef DEBUG
    if (DebugWhat & DebugPieces) {
        Print(0, "After knights: %d\n", score);
    }
#endif

    /*************************************************************
     *
     * Bishops
     *
     *************************************************************/

    /*
     * Evaluate white bishops
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
     * Evaluate black bishops
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

#ifdef DEBUG
    if (DebugWhat & DebugPieces) {
        Print(0, "After bishops: %d\n", score);
    }
#endif

    /*************************************************************
     *
     * Rooks
     *
     *************************************************************/

    /*
     * Evaluate white rooks
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
     * Evaluate black rooks
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

#ifdef DEBUG
    if (DebugWhat & DebugPieces) {
        Print(0, "After rooks: %d\n", score);
    }
#endif

    /*************************************************************
     *
     * Queens
     *
     *************************************************************/

    /*
     * Evaluate white queens
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
     * Evaluate black queens
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

#ifdef DEBUG
    if (DebugWhat & DebugPieces) {
        Print(0, "After queens: %d\n", score);
    }
#endif

    /*
     * Check if both sides have only one bishops. If so, and they are opposite
     * colored, scale the score down.
     */

    if (((p->material_signature[White] & 0x1e) == SIGNATURE_BIT(Bishop)) &&
        ((p->material_signature[Black] & 0x1e) == SIGNATURE_BIT(Bishop))) {
        bool white_on_white =
            (p->mask[White][Bishop] & WhiteSquaresMask) != 0ULL;
        bool white_on_black =
            (p->mask[White][Bishop] & BlackSquaresMask) != 0ULL;
        bool black_on_white =
            (p->mask[Black][Bishop] & WhiteSquaresMask) != 0ULL;
        bool black_on_black =
            (p->mask[Black][Bishop] & BlackSquaresMask) != 0ULL;

        bool white_single_colored = white_on_white ^ white_on_black;
        bool black_single_colored = black_on_white ^ black_on_black;

        if (white_single_colored && black_single_colored) {
            if ((white_on_white && black_on_black) ||
                (white_on_black && black_on_white)) {
                score = 4 * score / 5;
            }
        }
    }

#ifdef DEBUG
    if (DebugWhat & DebugPieces) {
        Print(0, "After bishop adjustment: %d\n", score);
    }
#endif

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

    if (score >= 0) {
        score = (score + 7) & ~15;
    } else {
        score = -((-score + 7) & ~15);
    }

    StoreST(p->hkey, score);

    return score;
}

/**
 * This is just a convenience routine which handles sign switches
 * if it is not white to move.
 */

int EvaluatePosition(const struct Position *p) {
    if (p->turn == White)
        return EvaluatePositionForWhite(p);
    else
        return -EvaluatePositionForWhite(p);
}

/**
 * Do the pre-search initialization of evaluation.
 */

void InitEvaluation(const struct Position *p) {
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

    /*
     * Set up King piece square table.
     */
    create_mirrored_piece_square_table(KingPosEndgameQueenSide,
                                       KingPosEndgameKingSide);

#ifdef DEBUG
    DebugWhat = 255;
    EvaluatePositionForWhite(p);
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

static bool is_edge(unsigned int sq) {
    return ((sq & 7) == 0 || (sq & 7) == 7 || sq <= h1 || sq >= a8);
}

static void create_mirrored_piece_square_table(int16_t *src, int16_t *dest) {
    for (int rank = 0; rank < 8; rank++) {
        for (int file = 0; file < 8; file++) {
            int src_idx = 8 * rank + file;
            int dest_idx = 8 * rank + 7 - file;
            dest[dest_idx] = src[src_idx];
        }
    }
}
