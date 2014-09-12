/*

    Amy - a chess playing program
    Copyright (C) 2002 Thorsten Greiner

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/

/*
 * movedata.c - move data precalculation stuff
 *
 * $Id: movedata.c 27 2003-02-11 22:39:17Z thorsten $
 *
 */

#include <stdio.h>
#include "amy.h"

static int conv[128];

signed char NextSQ[64][64];
struct MoveData NextSquare[8][64][64];

static int KnightDirs[] = {
    14, 31, 33, 18, -14, -31, -33, -18
};

static int KingDirs[] = {
    -1, 15, 16, 17, 1, -17, -16, -15
};

static int BishopDirs[] = {
    15, 17, -15, -17
};

static int RookDirs[] = {
    16, 1, -16, -1
};

static int QueenDirs[] = {
    16, 1, -16, -1, 15, 17, -15, -17
};

void InitMoves(void)
{
    int sq, sq2;
    int pc;

    for(sq=0; sq<128; sq++) {
        conv[sq] = 127;
    }
    for(sq=0; sq<128; sq++) {
        if(!(sq & 0x88)) {
            sq2 = (sq & 7) | (sq & 0x70) >> 1;
            conv[sq] = sq2;
        }
    }

    for(sq = 0; sq < 64; sq++) {
        for(sq2 = 0; sq2 < 64; sq2++) {
            for(pc = Pawn; pc <= BPawn; pc++) {
                NextSquare[pc][sq][sq2].nextPos = -1;
                NextSquare[pc][sq][sq2].nextDir = -1;
            }
            NextSQ[sq][sq2] = -1;
        }
    }

    /*
     * Pawns
     */

    for(sq = 0; sq < 128; sq++) {
        int next;
        int next2;

        if(sq & 0x88) continue;

        next = sq + 0x11;
        if(!(next & 0x88)) {
            NextSquare[Pawn][conv[sq]][conv[sq]].nextPos = conv[next];
            NextSquare[Pawn][conv[sq]][conv[sq]].nextDir = conv[next];
        }
        else {
            next = sq;
        }

        next2 = sq + 0x0f;
        if(!(next2 & 0x88)) {
            NextSquare[Pawn][conv[sq]][conv[next]].nextPos = conv[next2];
            NextSquare[Pawn][conv[sq]][conv[next]].nextDir = conv[next2];
        }
    }

    for(sq = 0; sq < 128; sq++) {
        int next;
        int next2;

        if(sq & 0x88) continue;

        next = sq - 0x11;
        if(!(next & 0x88)) {
            NextSquare[BPawn][conv[sq]][conv[sq]].nextPos = conv[next];
            NextSquare[BPawn][conv[sq]][conv[sq]].nextDir = conv[next];
        }
        else {
            next = sq;
        }

        next2 = sq - 0x0f;
        if(!(next2 & 0x88)) {
            NextSquare[BPawn][conv[sq]][conv[next]].nextPos = conv[next2];
            NextSquare[BPawn][conv[sq]][conv[next]].nextDir = conv[next2];
        }
    }

    /*
     * Knight
     */

    for(sq = 0; sq < 128; sq++) {
        int next;
        int i;
        if(sq & 0x88) continue;

        next = sq;

        for(i=0; i<8; i++) {
            int next2 = sq + KnightDirs[i];
            if(next2 & 0x88) continue;

            NextSquare[Knight][conv[sq]][conv[next]].nextPos = conv[next2];
            NextSquare[Knight][conv[sq]][conv[next]].nextDir = conv[next2];

            next = next2;
        }
    }

    /*
     * King
     */

    for(sq = 0; sq < 128; sq++) {
        int next;
        int i;
        if(sq & 0x88) continue;

        next = sq;

        for(i=0; i<8; i++) {
            int next2 = sq + KingDirs[i];
            if(next2 & 0x88) continue;

            NextSquare[King][conv[sq]][conv[next]].nextPos = conv[next2];
            NextSquare[King][conv[sq]][conv[next]].nextDir = conv[next2];

            next = next2;
        }
    }

    /*
     * Bishops
     */

    for(sq = 0; sq < 128; sq++) {
        int dir, nextdir;
        int next;
        int start = TRUE;

        if(sq & 0x88) continue;

        dir = 0;

        while(dir < 4) {
            int next2 = -1;

            next = sq + BishopDirs[dir];
            if(next & 0x88) {
                dir++;
                continue;
            }

            nextdir = dir+1;
            while(nextdir < 4) {
                next2 = sq + BishopDirs[nextdir];
                if(!(next2 & 0x88)) break;
                nextdir++;
            }

            if(start) {
                NextSquare[Bishop][conv[sq]][conv[sq]].nextPos = conv[next];
                start = FALSE;
            }

            for(;;) {
                int next3 = next + BishopDirs[dir];

                if(next3 & 0x88) {
                    if(nextdir < 4) {
                        NextSquare[Bishop][conv[sq]][conv[next]].nextPos = 
                            conv[next2];
                        NextSquare[Bishop][conv[sq]][conv[next]].nextDir = 
                            conv[next2];
                    }
                    break;
                }
                else {
                    NextSquare[Bishop][conv[sq]][conv[next]].nextPos = 
                        conv[next3];
                    if(nextdir < 4) {
                        NextSquare[Bishop][conv[sq]][conv[next]].nextDir = 
                            conv[next2];
                    }
                }
                next = next3;
            }
            dir++;
        }
    }

    /*
     * Rooks
     */

    for(sq = 0; sq < 128; sq++) {
        int dir, nextdir;
        int next;
        int start = TRUE;

        if(sq & 0x88) continue;

        dir = 0;

        while(dir < 4) {
            int next2 = -1;

            next = sq + RookDirs[dir];
            if(next & 0x88) {
                dir++;
                continue;
            }

            nextdir = dir+1;
            while(nextdir < 4) {
                next2 = sq + RookDirs[nextdir];
                if(!(next2 & 0x88)) break;
                nextdir++;
            }

            if(start) {
                NextSquare[Rook][conv[sq]][conv[sq]].nextPos = conv[next];
                start = FALSE;
            }

            for(;;) {
                int next3 = next + RookDirs[dir];

                if(next3 & 0x88) {
                    if(nextdir < 4) {
                        NextSquare[Rook][conv[sq]][conv[next]].nextPos = 
                            conv[next2];
                        NextSquare[Rook][conv[sq]][conv[next]].nextDir = 
                            conv[next2];
                    }
                    break;
                }
                else {
                    NextSquare[Rook][conv[sq]][conv[next]].nextPos = 
                        conv[next3];
                    if(nextdir < 4) {
                        NextSquare[Rook][conv[sq]][conv[next]].nextDir = 
                            conv[next2];
                    }
                }
                next = next3;
            }
            dir++;
        }
    }

    /*
     * Queens
     */

    for(sq = 0; sq < 128; sq++) {
        int dir, nextdir;
        int next;
        int start = TRUE;

        if(sq & 0x88) continue;

        dir = 0;

        while(dir < 8) {
            int next2 = -1;

            next = sq + QueenDirs[dir];
            if(next & 0x88) {
                dir++;
                continue;
            }

            nextdir = dir+1;
            while(nextdir < 8) {
                next2 = sq + QueenDirs[nextdir];
                if(!(next2 & 0x88)) break;
                nextdir++;
            }

            if(start) {
                NextSquare[Queen][conv[sq]][conv[sq]].nextPos = conv[next];
                start = FALSE;
            }

            for(;;) {
                int next3 = next + QueenDirs[dir];

                if(next3 & 0x88) {
                    if(nextdir < 8) {
                        NextSquare[Queen][conv[sq]][conv[next]].nextPos = 
                            conv[next2];
                        NextSquare[Queen][conv[sq]][conv[next]].nextDir = 
                            conv[next2];
                    }
                    break;
                }
                else {
                    NextSquare[Queen][conv[sq]][conv[next]].nextPos = 
                        conv[next3];
                    if(nextdir < 8) {
                        NextSquare[Queen][conv[sq]][conv[next]].nextDir = 
                            conv[next2];
                    }
                }
                next = next3;
            }
            dir++;
        }
    }

    /*
     * Inititialize NextSQ
     */

    for(sq = 0; sq < 128; sq++) {
        int dir;

        if(sq & 0x88) continue;

        for(dir = 0; dir < 8; dir++) {
            int next, next2;

            next = sq + QueenDirs[dir];
            if(next & 0x88) continue;

            for(;;) {
                next2 = next + QueenDirs[dir];
                if(next2 & 0x88) break;
                NextSQ[conv[sq]][conv[next]] = conv[next2];
                next = next2;
            }
        }
    }
}
