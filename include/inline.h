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

#ifndef INLINE_H
#define INLINE_H

extern BitBoard ShiftUpMask, ShiftDownMask;
extern BitBoard ShiftLeftMask, ShiftRightMask;

static inline BitBoard ShiftUp(BitBoard x) { return (x << 8) & ShiftUpMask; }

static inline BitBoard ShiftDown(BitBoard x) {
    return (x >> 8) & ShiftDownMask;
}

static inline BitBoard ShiftLeft(BitBoard x) {
    return (x << 1) & ShiftLeftMask;
}

static inline BitBoard ShiftRight(BitBoard x) {
    return (x >> 1) & ShiftRightMask;
}

/**
 * Calculate the 'king distance' between two squares.
 * This the number of king moves to go from sq1 to sq2.
 */
static inline int KingDist(int sq1, int sq2) {
    int file_dist = ABS((sq1 & 7) - (sq2 & 7));
    int rank_dist = ABS((sq1 >> 3) - (sq2 >> 3));

    return MAX(file_dist, rank_dist);
}

/**
 * Calculate the 'minimum distance' between two squares.
 * This the minimum of the file and rank distances.
 */
static inline int MinDist(int sq1, int sq2) {
    int file_dist = ABS((sq1 & 7) - (sq2 & 7));
    int rank_dist = ABS((sq1 >> 3) - (sq2 >> 3));

    return MIN(file_dist, rank_dist);
}

/**
 * Calculate the 'Manhattan distance' between two squares.
 */
static inline int ManhattanDist(int sq1, int sq2) {
    int file_dist = ABS((sq1 & 7) - (sq2 & 7));
    int rank_dist = ABS((sq1 >> 3) - (sq2 >> 3));

    return file_dist + rank_dist;
}

static inline int FileDist(int sq1, int sq2) {
    return ABS((sq1 & 7) - (sq2 & 7));
}

/**
 * Calculate the distance of 'sq' to any edge on the chessboard
 */
static inline int EdgeDist(int sq) {
    int filedist = MIN(sq & 7, 7 - (sq & 7));
    int rankdist = MIN(sq >> 3, 7 - (sq >> 3));

    return MAX(filedist, rankdist);
}

/**
 * Create a move from from square, to square and flags.
 */
static inline int make_move(int from, int to, int flags) {
    return (move_t)(from | (to << 6) | flags);
}

/**
 * Create a promotion move from from square, to square and flags.
 */
static inline int make_promotion(int from, int to, int type, int flags) {
    return (move_t)(from | (to << 6) | (type << M_PROMOTION_OFFSET) | flags);
}

/**
 * Returns if the square is a promotion square.
 */
static inline bool is_promo_square(int sq) {
    int rank = sq >> 3;
    return rank == 0 || rank == 7;
}

/*
 * Test whether a side is in check
 */

static inline bool InCheck(struct Position *p, int side) {
    int sq = p->kingSq[side];
    return (p->atkFr[sq] & p->mask[!side][0]);
}

/*
 * Determine type of promotion from move
 */

static inline int PromoType(move_t move) {
    return (move & M_PROMOTION_MASK) >> M_PROMOTION_OFFSET;
}

#endif /* INLINE_H */
