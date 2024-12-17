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

#endif /* INLINE_H */
