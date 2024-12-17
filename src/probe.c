/*
 * probe.c - EGTB probing code
 *
 * This file is part of Amy, a chess program by Thorsten Greiner
 *
 * Amy is copyrighted by Thorsten Greiner
 *
 */

#include "amy.h"
#include <stdlib.h>

/*
 * This is from tbindex.cpp
 */

/* Some constants from SJE program */

#define pageL 65536

/* tablebase byte entry semispan length */

#define tbbe_ssL ((pageL - 4) / 2)

#define bev_broken (tbbe_ssL + 1) /* illegal or busted */

#define bev_mi1 tbbe_ssL /* mate in 1 move */
#define bev_mimin 1      /* mate in 126 moves */

#define bev_draw 0 /* draw */

#define bev_limax (-1)      /* mated in 125 moves */
#define bev_li0 (-tbbe_ssL) /* mated in 0 moves */

#define bev_limaxx (-tbbe_ssL - 1) /* mated in 126 moves */
#define bev_miminx (-tbbe_ssL - 2) /* mate in 127 moves */

#define EGTB_CACHE_SIZE 2 * 1024 * 1024

#define C_PIECES 3
#define XX 128
typedef unsigned int INDEX;
typedef int (*INDEX_FUNC)(int *, int *, int, int);

extern int IInitializeTb(char *);
extern int IDescFindFromCounters(int *);
extern int FRegisteredFun(int, int);
INDEX_FUNC PfnIndCalcFun(int, int);
extern int L_TbtProbeTable(int, int, INDEX);
extern int FTbSetCacheSize(void *buffer, unsigned long size);
extern int TB_CRC_CHECK;

static int EGTBMenCount;
int EGTBProbe, EGTBProbeSucc;

#if MP && HAVE_LIBPTHREAD
static pthread_mutex_t EGTBMutex = PTHREAD_MUTEX_INITIALIZER;
#endif

void InitEGTB(char *tbpath) {
    TB_CRC_CHECK = 0;
    EGTBMenCount = IInitializeTb(tbpath);
    if (EGTBMenCount != 0) {
        void *egtb_cache = malloc(EGTB_CACHE_SIZE);
        Print(0, "Found %d-men endgame table bases.\n", EGTBMenCount);
        FTbSetCacheSize(egtb_cache, EGTB_CACHE_SIZE);
    }
}

void InitializeCounters(int *pieceCounter, int *squares, int type,
                        BitBoard mask) {
    int count = 0;
    while (mask) {
        int index = FindSetBit(mask);
        mask &= mask - 1;
        squares[type * C_PIECES + count] = index;
        count++;
    }
    *pieceCounter = count;
}

int ProbeEGTB(const struct Position *p, int *score, int ply) {
    int pcCount[10];
    int wSquares[16], bSquares[16];
    int iTB;
    int color;
    int invert;
    int *wp, *bp;
    int ep;
    INDEX index;
    int value;
    int result;

    if (CountBits(p->mask[White][0] | p->mask[Black][0]) > EGTBMenCount)
        return 0;

    EGTBProbe++;
    InitializeCounters(pcCount, wSquares, 0, p->mask[White][Pawn]);
    InitializeCounters(pcCount + 1, wSquares, 1, p->mask[White][Knight]);
    InitializeCounters(pcCount + 2, wSquares, 2, p->mask[White][Bishop]);
    InitializeCounters(pcCount + 3, wSquares, 3, p->mask[White][Rook]);
    InitializeCounters(pcCount + 4, wSquares, 4, p->mask[White][Queen]);
    InitializeCounters(pcCount + 5, bSquares, 0, p->mask[Black][Pawn]);
    InitializeCounters(pcCount + 6, bSquares, 1, p->mask[Black][Knight]);
    InitializeCounters(pcCount + 7, bSquares, 2, p->mask[Black][Bishop]);
    InitializeCounters(pcCount + 8, bSquares, 3, p->mask[Black][Rook]);
    InitializeCounters(pcCount + 9, bSquares, 4, p->mask[Black][Queen]);

#if MP && HAVE_LIBPTHREAD
    pthread_mutex_lock(&EGTBMutex);
#endif

    do {
        iTB = IDescFindFromCounters(pcCount);
        if (iTB == 0) {
            result = 0;
            break;
        }

        wSquares[15] = p->kingSq[White];
        bSquares[15] = p->kingSq[Black];

        if (iTB > 0) {
            color = (p->turn == White) ? 0 : 1;
            invert = 0;
            wp = wSquares;
            bp = bSquares;
        } else {
            color = (p->turn == White) ? 1 : 0;
            invert = 1;
            wp = bSquares;
            bp = wSquares;
            iTB = -iTB;
        }

        if (!FRegisteredFun(iTB, color)) {
            result = 0;
            break;
        }

        ep = p->enPassant ? p->enPassant : XX;
        index = PfnIndCalcFun(iTB, color)(wp, bp, ep, invert);
        value = L_TbtProbeTable(iTB, color, index);
        if (value == bev_broken) {
            result = 0;
            break;
        }

        if (value > 0) {
            int distance = 32767 - value;
            value = (INF - (ply + 2 * distance + 1));
        } else if (value < 0) {
            int distance = 32766 + value;
            value = -INF + ply + 2 * distance;
        }

        *score = value;

        EGTBProbeSucc++;
        result = 1;
    } while (0);

#if MP && HAVE_LIBPTHREAD
    pthread_mutex_unlock(&EGTBMutex);
#endif

    return result;
}
