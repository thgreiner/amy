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
 * next.c - move selection routines
 */

#include "amy.h"

#define MAX_TREE_SIZE 64 /* maximum depth we will search to */

enum SearchPhases {
    HashMove,
    GenerateCaptures,
    GainingCapture,
    Killer1,
    Killer2,
    CounterMv,
    Killer3,
    GenerateRest,
    LoosingCapture,
    HistoryMoves,
    Done,
    GenerateQChecks,
    QChecks
};

enum GeneratedPhases { Nothing = 0, Captures = 1, NonCaptures = 2, Checks = 4 };

extern int PromoSquare[];

struct SearchData *CreateSearchData(struct Position *p) {
    struct SearchData *sd = calloc(sizeof(struct SearchData), 1);
    if (!sd) {
        Print(0, "Cannot allocate SearchData.\n");
        exit(1);
    }

    sd->position = p;

    sd->statusTable = calloc(sizeof(struct SearchStatus), MAX_TREE_SIZE);
    if (!sd->statusTable) {
        Print(0, "Cannot allocate SearchStatus.\n");
        exit(1);
    }
    sd->current = sd->statusTable;

    sd->killerTable = calloc(sizeof(struct KillerEntry), MAX_TREE_SIZE);
    if (!sd->killerTable) {
        Print(0, "Cannot allocate KillerEntry.\n");
        exit(1);
    }
    sd->killer = sd->killerTable;

    sd->moveHeap = calloc(sizeof(move_t), MAX_SEARCH_HEAP);
    if (!sd->moveHeap) {
        Print(0, "Cannot allocate moveHeap.\n");
        exit(1);
    }

    sd->dataHeap = calloc(sizeof(int), MAX_SEARCH_HEAP);
    if (!sd->dataHeap) {
        Print(0, "Cannot allocate dataHeap.\n");
        exit(1);
    }

#if MP
    sd->localHashTable = calloc(sizeof(struct HTEntry), L_HT_Size);
    if (!sd->localHashTable) {
        Print(0, "Cannot allocate thread-local hashtable.\n");
        exit(1);
    }
#endif

    sd->ply = 0;

    return sd;
}

void FreeSearchData(struct SearchData *sd) {
    free(sd->statusTable);
    free(sd->killerTable);
    free(sd->moveHeap);
    free(sd->dataHeap);
#if MP
    free(sd->localHashTable);
#endif

    free(sd);
}

void EnterNode(struct SearchData *sd) {
    struct SearchStatus *old = (sd->current);
    struct SearchStatus *st;

    st = ++(sd->current);

    st->st_phase = HashMove;
    sd->ply++;
    sd->killer++;

    if (sd->ply > 0)
        st->st_first = st->st_last = old->st_last;
    else
        st->st_first = st->st_last = 0;
}

void LeaveNode(struct SearchData *sd) {
    sd->current--;
    sd->killer--;
    sd->ply--;
}

int NextMove(struct SearchData *sd) {
    struct SearchStatus *st = sd->current;
    struct Position *p = sd->position;
    int i;
    move_t move;

    switch (st->st_phase) {
    case HashMove:
#ifdef VERBOSE
        Print(9, "HashMove\n");
#endif
        if (LegalMove(p, st->st_hashmove)) {
            st->st_phase = GenerateCaptures;
            return st->st_hashmove;
        } else {
            st->st_hashmove = M_NONE;
        }
    /* fall through */
    case GenerateCaptures: {
        BitBoard targets;
        int cnt;
#ifdef VERBOSE
        Print(9, "GenerateCaptures\n");
#endif

        /*
         * Generate captures.
         */

        targets = p->mask[OPP(p->turn)][0];
        while (targets) {
            int j;

            i = FindSetBit(targets);
            targets &= targets - 1;
            cnt = GenTo(p, i, sd->moveHeap + st->st_last);
            for (j = 0; j < cnt; j++) {
                sd->dataHeap[st->st_last] =
                    SwapOff(p, sd->moveHeap[st->st_last]);
                st->st_last++;
            }
        }

        targets = p->mask[p->turn][Pawn] & SeventhRank[p->turn];
        while (targets) {
            int j;
            i = FindSetBit(targets);
            targets &= targets - 1;
            cnt = GenFrom(p, i, sd->moveHeap + st->st_last);

            for (j = 0; j < cnt; j++) {
                sd->dataHeap[st->st_last] =
                    SwapOff(p, sd->moveHeap[st->st_last]);
                st->st_last++;
            }
        }

        cnt = GenEnpas(p, sd->moveHeap + st->st_last);
        for (i = 0; i < cnt; i++) {
            sd->dataHeap[st->st_last] = 0;
            st->st_last++;
        }

        st->st_phase = GainingCapture;
    }
    /* fall through */
    case GainingCapture:
#ifdef VERBOSE
        Print(9, "GainingCapture\n");
#endif
        while (st->st_last > st->st_first) {
            int besti = st->st_first;
            int best = sd->dataHeap[besti];

            for (i = st->st_first + 1; i < st->st_last; i++) {
                if (sd->dataHeap[i] > best) {
                    best = sd->dataHeap[i];
                    besti = i;
                }
            }
            if (best >= 0) {
                move = sd->moveHeap[besti];
                st->st_last--;

                sd->moveHeap[besti] = sd->moveHeap[st->st_last];
                sd->dataHeap[besti] = sd->dataHeap[st->st_last];

                if (move == st->st_hashmove)
                    continue;

                return move;
            } else
                break;
        }
    /* fall through */
    case Killer1: {
        move = sd->killer->killer1;
#ifdef VERBOSE
        Print(9, "Killer1\n");
#endif
        st->st_k1 = M_NONE;
        if (move != st->st_hashmove && LegalMove(p, move)) {
            st->st_phase = Killer2;
            st->st_k1 = move;

            return move;
        }
    }
    /* fall through */
    case Killer2: {
        move = sd->killer->killer2;
#ifdef VERBOSE
        Print(9, "Killer2\n");
#endif
        st->st_k2 = M_NONE;
        if (move != st->st_hashmove && LegalMove(p, move)) {
            st->st_phase = CounterMv;
            st->st_k2 = move;

            return move;
        }
    }
    /* fall through */
    case CounterMv: {
        move_t lmove = (p->actLog - 1)->gl_Move;

#ifdef VERBOSE
        Print(9, "CounterMv\n");
#endif
        st->st_cm = M_NONE;
        if (lmove != M_NULL) {
            move = sd->counterTab[p->turn][lmove & 4095];

            if (move != M_NONE && move != st->st_hashmove &&
                move != st->st_k1 && move != st->st_k2 && LegalMove(p, move)) {
                st->st_phase = Killer3;
                st->st_cm = move;

                return move;
            }
        }
    }
    /* fallthrough */
    case Killer3:
#ifdef VERBOSE
        Print(9, "Killer3\n");
#endif
        st->st_k3 = M_NONE;
        if (sd->ply >= 2) {
            move = (sd->killer - 2)->killer1;

            if (move == st->st_hashmove || move == st->st_k1 ||
                move == st->st_k2 || move == st->st_cm || !LegalMove(p, move))
                move = (sd->killer - 2)->killer2;

            if (move != st->st_hashmove && move != st->st_k1 &&
                move != st->st_k2 && move != st->st_cm && LegalMove(p, move)) {
                st->st_phase = /* GenerateRest */ LoosingCapture;
                st->st_k3 = move;

                return move;
            }
        }
        /* fallthrough */

    case LoosingCapture:
#ifdef VERBOSE
        Print(9, "LoosingCapture\n");
#endif
        while (st->st_last > st->st_first) {
            int besti = st->st_first;
            int best = sd->dataHeap[besti];

            for (i = st->st_first + 1; i < st->st_last; i++) {
                if (sd->dataHeap[i] > best) {
                    best = sd->dataHeap[i];
                    besti = i;
                }
            }
            move = sd->moveHeap[besti];

            sd->moveHeap[besti] = sd->moveHeap[st->st_first];
            sd->dataHeap[besti] = sd->dataHeap[st->st_first];
            st->st_first++;

            st->st_phase = LoosingCapture;

            if (move == st->st_hashmove)
                continue;

            return move;
        }
        /* fallthrough */

    case GenerateRest: {
#ifdef VERBOSE
        Print(9, "GenerateRest\n");
#endif
        BitBoard tmp, tmp2;
        BitBoard excl;

        int i, j;

        excl = p->mask[White][0] | p->mask[Black][0];

        st->st_nc_first = st->st_last;

        if (p->castle & CastleMask[p->turn][0]) {
            sd->moveHeap[st->st_last++] = (p->turn == White ? e1 : e8) |
                                          ((p->turn == White ? g1 : g8) << 6) |
                                          M_SCASTLE;
        }
        if (p->castle & CastleMask[p->turn][1]) {
            sd->moveHeap[st->st_last++] = (p->turn == White ? e1 : e8) |
                                          ((p->turn == White ? c1 : c8) << 6) |
                                          M_LCASTLE;
        }

        tmp = p->mask[p->turn][0] & ~p->mask[p->turn][Pawn];

        while (tmp) {
            i = FindSetBit(tmp);
            tmp &= tmp - 1;
            tmp2 = p->atkTo[i] & ~excl;
            while (tmp2) {
                j = FindSetBit(tmp2);
                tmp2 &= tmp2 - 1;
                sd->moveHeap[st->st_last++] = i | (j << 6);
            }
        }

        tmp = p->mask[p->turn][Pawn] & ~SeventhRank[p->turn];

        if (p->turn == White)
            tmp = ShiftUp(tmp);
        else
            tmp = ShiftDown(tmp);

        tmp2 = tmp &= ~excl;

        while (tmp2) {
            int fr;
            i = FindSetBit(tmp2);
            tmp2 &= tmp2 - 1;
            fr = (p->turn == White) ? i - 8 : i + 8;
            if (PromoSquare[i]) {
                sd->moveHeap[st->st_last++] = fr | (i << 6) | M_PQUEEN;
                sd->moveHeap[st->st_last++] = fr | (i << 6) | M_PKNIGHT;
                sd->moveHeap[st->st_last++] = fr | (i << 6) | M_PROOK;
                sd->moveHeap[st->st_last++] = fr | (i << 6) | M_PBISHOP;
            } else
                sd->moveHeap[st->st_last++] = fr | (i << 6);
        }

        tmp &= ThirdRank[p->turn];

        if (p->turn == White)
            tmp = ShiftUp(tmp);
        else
            tmp = ShiftDown(tmp);

        tmp &= ~excl;

        while (tmp) {
            int fr;
            i = FindSetBit(tmp);
            tmp &= tmp - 1;
            fr = (p->turn == White) ? i - 16 : i + 16;
            sd->moveHeap[st->st_last++] = fr | (i << 6) | M_PAWND;
        }

        st->st_phase = HistoryMoves;
    }

    case HistoryMoves:
#ifdef VERBOSE
        Print(9, "HistoryMoves\n");
#endif
        while (st->st_last > st->st_nc_first) {
            int besti = st->st_nc_first;
            int best = sd->historyTab[p->turn][sd->moveHeap[besti] & 4095];

            for (i = st->st_nc_first + 1; i < st->st_last; i++) {
                int hval = sd->historyTab[p->turn][sd->moveHeap[i] & 4095];
                if (hval > best) {
                    best = hval;
                    besti = i;
                }
            }
            move = sd->moveHeap[besti];

            st->st_last--;
            sd->moveHeap[besti] = sd->moveHeap[st->st_last];

            if (move == st->st_hashmove || move == st->st_k1 ||
                move == st->st_k2 || move == st->st_k3 || move == st->st_cm)
                continue;

            return move;
        }
    }

    return M_NONE;
}

int NextEvasion(struct SearchData *sd) {
    struct SearchStatus *st = sd->current;
    struct Position *p = sd->position;
    int i;
    move_t move;

    switch (st->st_phase) {
    case HashMove:
#ifdef VERBOSE
        Print(9, "HashMove\n");
#endif
        if (LegalMove(p, st->st_hashmove)) {
            st->st_phase = GenerateCaptures;
            return st->st_hashmove;
        } else {
            st->st_hashmove = M_NONE;
        }
        /* fall through */
    case GenerateCaptures: {
        BitBoard targets;
        int cnt;

#ifdef VERBOSE
        Print(9, "GainingCapture\n");
#endif

        /*
         * Generate captures. If in check, generate only
         * captures by the king or to pieces which give
         * check
         */

        int kp = p->kingSq[p->turn];

        targets = (p->atkFr[kp] | p->atkTo[kp]) & p->mask[OPP(p->turn)][0];

        while (targets) {
            int j;
            i = FindSetBit(targets);
            targets &= targets - 1;
            cnt = GenTo(p, i, sd->moveHeap + st->st_last);
            for (j = 0; j < cnt; j++) {
                sd->dataHeap[st->st_last] =
                    SwapOff(p, sd->moveHeap[st->st_last]);
                st->st_last++;
            }
        }

        cnt = GenEnpas(p, sd->moveHeap + st->st_last);
        for (i = 0; i < cnt; i++) {
            sd->dataHeap[st->st_last] = 0;
            st->st_last++;
        }
    }
        /* fall through */
    case GainingCapture:
        while (st->st_last > st->st_first) {
            int besti = st->st_first;
            int best = sd->dataHeap[besti];

            for (i = st->st_first + 1; i < st->st_last; i++) {
                if (sd->dataHeap[i] > best) {
                    best = sd->dataHeap[i];
                    besti = i;
                }
            }
            if (best >= 0) {
                move = sd->moveHeap[besti];
                st->st_last--;

                sd->moveHeap[besti] = sd->moveHeap[st->st_last];
                sd->dataHeap[besti] = sd->dataHeap[st->st_last];

                st->st_phase = GainingCapture;

                if (move == st->st_hashmove)
                    continue;

                return move;
            } else
                break;
        }
        /* fall through */
    case Killer1: {
        move = sd->killer->killer1;
#ifdef VERBOSE
        Print(9, "Killer1\n");
#endif
        st->st_k1 = M_NONE;
        if (move != st->st_hashmove && LegalMove(p, move)) {
            st->st_phase = Killer2;
            st->st_k1 = move;

            return move;
        }
    }
        /* fall through */
    case Killer2: {
        move = sd->killer->killer2;
#ifdef VERBOSE
        Print(9, "Killer2\n");
#endif
        st->st_k2 = M_NONE;
        if (move != st->st_hashmove && LegalMove(p, move)) {
            st->st_phase = CounterMv;
            st->st_k2 = move;

            return move;
        }
    }
        /* fall through */
    case CounterMv: {
        move_t lmove = (p->actLog - 1)->gl_Move;

#ifdef VERBOSE
        Print(9, "CounterMv\n");
#endif
        st->st_cm = M_NONE;
        if (lmove != M_NULL) {
            move = sd->counterTab[p->turn][lmove & 4095];

            if (move != M_NONE && move != st->st_hashmove &&
                move != st->st_k1 && move != st->st_k2 && LegalMove(p, move)) {
                st->st_phase = Killer3;
                st->st_cm = move;

                return move;
            }
        }
    }
        /* fall through */
    case Killer3:
#ifdef VERBOSE
        Print(9, "Killer3\n");
#endif
        st->st_k3 = M_NONE;
        if (sd->ply >= 2) {
            move = (sd->killer - 2)->killer1;

            if (move == st->st_hashmove || move == st->st_k1 ||
                move == st->st_k2 || move == st->st_cm || !LegalMove(p, move))
                move = (sd->killer - 2)->killer2;

            if (move != st->st_hashmove && move != st->st_k1 &&
                move != st->st_k2 && move != st->st_cm && LegalMove(p, move)) {
                st->st_phase = /* HistoryMoves; */ LoosingCapture;
                st->st_k3 = move;

                return move;
            }
        }
        /* fall through */
    case LoosingCapture:
#ifdef VERBOSE
        Print(9, "LoosingCapture\n");
#endif
        while (st->st_last > st->st_first) {
            int besti = st->st_first;
            int best = sd->dataHeap[besti];

            for (i = st->st_first + 1; i < st->st_last; i++) {
                if (sd->dataHeap[i] > best) {
                    best = sd->dataHeap[i];
                    besti = i;
                }
            }
            move = sd->moveHeap[besti];

            sd->moveHeap[besti] = sd->moveHeap[st->st_first];
            sd->dataHeap[besti] = sd->dataHeap[st->st_first];
            st->st_first++;

            st->st_phase = LoosingCapture;

            if (move == st->st_hashmove)
                continue;

            return move;
        }

        /* fall through */
    case GenerateRest: {
        BitBoard tmp, tmp2;
        BitBoard excl;
        int kp = p->kingSq[p->turn]; /* FindSetBit(Mask[Side][King]); */
        BitBoard att, des;
        int i;
#ifdef VERBOSE
        Print(9, "HistoryMoves\n");
#endif

        excl = p->mask[White][0] | p->mask[Black][0];

        st->st_nc_first = st->st_last;

        tmp = p->atkTo[kp] & ~excl;

        while (tmp) {
            i = FindSetBit(tmp);
            tmp &= tmp - 1;
            if (!(p->atkFr[i] & p->mask[OPP(p->turn)][0]))
                sd->moveHeap[st->st_last++] = kp | (i << 6);
        }

        att = (p->mask[OPP(p->turn)][Bishop] | p->mask[OPP(p->turn)][Rook] |
               p->mask[OPP(p->turn)][Queen]) &
              p->atkFr[kp];
        des = 0;

        while (att) {
            i = FindSetBit(att);
            att &= att - 1;
            des = InterPath[kp][i];
        }

        tmp = (p->mask[p->turn][0] & ~p->mask[p->turn][King]) &
              ~p->mask[p->turn][Pawn];

        while (tmp) {
            int j;
            BitBoard mto;

            i = FindSetBit(tmp);
            tmp &= tmp - 1;
            mto = (p->atkTo[i] & ~excl) & des;

            while (mto) {
                j = FindSetBit(mto);
                mto &= mto - 1;
                sd->moveHeap[st->st_last++] = i | (j << 6);
            }
        }

        tmp = p->mask[p->turn][Pawn];

        if (p->turn == White)
            tmp = ShiftUp(tmp);
        else
            tmp = ShiftDown(tmp);

        tmp2 = tmp = (tmp & ~excl);

        while (tmp2) {
            int fr;
            i = FindSetBit(tmp2);
            tmp2 &= tmp2 - 1;
            fr = (p->turn == White) ? i - 8 : i + 8;

            if (PromoSquare[i]) {
                sd->moveHeap[st->st_last++] = fr | (i << 6) | M_PQUEEN;
                sd->moveHeap[st->st_last++] = fr | (i << 6) | M_PKNIGHT;
                sd->moveHeap[st->st_last++] = fr | (i << 6) | M_PROOK;
                sd->moveHeap[st->st_last++] = fr | (i << 6) | M_PBISHOP;
            } else
                sd->moveHeap[st->st_last++] = fr | (i << 6);
        }

        tmp &= ThirdRank[p->turn];

        if (p->turn == White)
            tmp = ShiftUp(tmp);
        else
            tmp = ShiftDown(tmp);

        tmp &= ~excl;

        while (tmp) {
            int fr;
            i = FindSetBit(tmp);
            tmp &= tmp - 1;
            fr = (p->turn == White) ? i - 16 : i + 16;
            sd->moveHeap[st->st_last++] = fr | (i << 6) | M_PAWND;
        }
    }

        /* fall through */
    case HistoryMoves:
        while (st->st_last > st->st_nc_first) {
            int besti = st->st_nc_first;
            int m = sd->moveHeap[besti];
            unsigned int best = sd->historyTab[p->turn][m & 4095];
            for (i = st->st_nc_first + 1; i < st->st_last; i++) {
                m = sd->moveHeap[i];
                if (sd->historyTab[p->turn][m & 4095] > best) {
                    best = sd->historyTab[p->turn][m & 4095];
                    besti = i;
                }
            }
            move = sd->moveHeap[besti];

            st->st_last--;
            sd->moveHeap[besti] = sd->moveHeap[st->st_last];

            st->st_phase = HistoryMoves;

            if (move == st->st_hashmove || move == st->st_k1 ||
                move == st->st_k2 || move == st->st_k3 || move == st->st_cm)
                continue;

            return move;
        }
    }

    return M_NONE;
}

static void GenerateQCaptures(struct SearchData *sd, int alpha) {
    struct SearchStatus *st = sd->current;
    struct Position *p = sd->position;
    BitBoard pwn7th;
    BitBoard att, def;
    int score;
    int i;

    att = p->mask[p->turn][0];

    /* Handle pawn promotions first */
    pwn7th = p->mask[p->turn][Pawn] & SeventhRank[p->turn];
    att &= ~pwn7th;

    while (pwn7th) {
        int next;
        int j;
        BitBoard tmp;

        i = FindSetBit(pwn7th);
        pwn7th &= pwn7th - 1;
        next = (p->turn == White) ? i + 8 : i - 8;

        if (p->piece[next] == Neutral) {
            int move = i | (next << 6);
            int sw;
            if ((sw = SwapOff(p, move | M_PQUEEN)) >= 0) {
                sd->moveHeap[st->st_last] = move | M_PQUEEN;
                sd->dataHeap[st->st_last] = sw;
                st->st_last++;
            }
        }

        tmp = p->atkTo[i] & p->mask[OPP(p->turn)][0];
        while (tmp) {
            move_t move;
            int sw;
            j = FindSetBit(tmp);
            tmp &= tmp - 1;
            move = i | (j << 6) | M_CAPTURE;
            if ((sw = SwapOff(p, move | M_PQUEEN)) >= 0) {
                sd->moveHeap[st->st_last] = move | M_PQUEEN;
                sd->dataHeap[st->st_last] = sw;
                st->st_last++;
            }
        }
    }

    if (p->turn == White) {
        score = MaterialBalance(p) + MaxPos;
    } else {
        score = -MaterialBalance(p) + MaxPos;
    }

    if (score + Value[Queen] <= alpha)
        return;
    def = p->mask[OPP(p->turn)][Queen];
    while (def) {
        BitBoard tmp2;
        int j;
        i = FindSetBit(def);
        def &= def - 1;
        tmp2 = p->atkFr[i] & att;
        while (tmp2) {
            int move, sw;
            j = FindSetBit(tmp2);
            tmp2 &= tmp2 - 1;
            move = j | i << 6 | M_CAPTURE;
            sw = SwapOff(p, move);
            if (sw >= 0) {
                sd->moveHeap[st->st_last] = move;
                sd->dataHeap[st->st_last] = sw;
                st->st_last++;
            }
        }
    }
    if (score + Value[Rook] <= alpha)
        return;
    def = p->mask[OPP(p->turn)][Rook];
    while (def) {
        BitBoard tmp2;
        int j;
        i = FindSetBit(def);
        def &= def - 1;
        tmp2 = p->atkFr[i] & att;
        while (tmp2) {
            int move, sw;
            j = FindSetBit(tmp2);
            tmp2 &= tmp2 - 1;
            move = j | i << 6 | M_CAPTURE;
            sw = SwapOff(p, move);
            if (sw >= 0) {
                sd->moveHeap[st->st_last] = move;
                sd->dataHeap[st->st_last] = sw;
                st->st_last++;
            }
        }
    }
    if (score + Value[Bishop] <= alpha)
        return;
    def = p->mask[OPP(p->turn)][Bishop] | p->mask[OPP(p->turn)][Knight];
    while (def) {
        BitBoard tmp2;
        int j;
        i = FindSetBit(def);
        def &= def - 1;
        tmp2 = p->atkFr[i] & att;
        while (tmp2) {
            int move, sw;
            j = FindSetBit(tmp2);
            tmp2 &= tmp2 - 1;
            move = j | i << 6 | M_CAPTURE;
            sw = SwapOff(p, move);
            if (sw >= 0) {
                sd->moveHeap[st->st_last] = move;
                sd->dataHeap[st->st_last] = sw;
                st->st_last++;
            }
        }
    }
    if (score + Value[Pawn] <= alpha)
        return;
    def = p->mask[OPP(p->turn)][Pawn];
    while (def) {
        BitBoard tmp2;
        int j;
        i = FindSetBit(def);
        def &= def - 1;
        tmp2 = p->atkFr[i] & att;
        while (tmp2) {
            int move, sw;
            j = FindSetBit(tmp2);
            tmp2 &= tmp2 - 1;
            move = j | i << 6 | M_CAPTURE;
            sw = SwapOff(p, move);
            if (sw >= 0) {
                sd->moveHeap[st->st_last] = move;
                sd->dataHeap[st->st_last] = sw;
                st->st_last++;
            }
        }
    }
}

int NextMoveQ(struct SearchData *sd, int alpha) {
    struct SearchStatus *st = sd->current;
    int i;
    move_t move;

    switch (st->st_phase) {
    case HashMove:
    case GenerateCaptures:
#ifdef VERBOSE
        Print(9, "GenerateCaptures\n");
#endif
        GenerateQCaptures(sd, alpha);
        st->st_phase = GainingCapture;

        /* fall through */
    case GainingCapture:
#ifdef VERBOSE
        Print(9, "GainingCapture\n");
#endif
        if (st->st_last > st->st_first) {
            int besti = st->st_first;
            int best = sd->dataHeap[besti];

            for (i = st->st_first + 1; i < st->st_last; i++) {
                if (sd->dataHeap[i] > best) {
                    best = sd->dataHeap[i];
                    besti = i;
                }
            }

            move = sd->moveHeap[besti];
            st->st_last--;
            sd->moveHeap[besti] = sd->moveHeap[st->st_last];
            sd->dataHeap[besti] = sd->dataHeap[st->st_last];

            return move;
        }
    }

    return M_NONE;
}

/*
 * Enter move in Killertable
 */

void PutKiller(struct SearchData *sd, move_t m) {
    struct KillerEntry *k = sd->killer;

    if (m == k->killer1) {
        k->kcount1 += 1;
    } else if (m == k->killer2) {
        k->kcount2 += 1;
        if (k->kcount2 > k->kcount1) {
            int tmp;

            tmp = k->kcount1;
            k->kcount1 = k->kcount2;
            k->kcount2 = tmp;

            tmp = k->killer1;
            k->killer1 = k->killer2;
            k->killer2 = tmp;
        }
    } else {
        if (k->killer1 == M_NONE) {
            k->killer1 = m;
            k->kcount1 = 1;
        } else {
            k->killer2 = m;
            k->kcount2 = 1;
        }
    }
}
