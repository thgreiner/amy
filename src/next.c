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
#include "heap.h"
#include "inline.h"

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

    sd->heap = allocate_heap();

    sd->data_heap = NULL;
    sd->data_heap_size = 0;

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
    free(sd->data_heap);
    free_heap(sd->heap);

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

    push_section(sd->heap);
}

void LeaveNode(struct SearchData *sd) {
    pop_section(sd->heap);
    sd->current--;
    sd->killer--;
    sd->ply--;
}

static inline void grow_data_heap(struct SearchData *sd) {
    if (sd->heap->current_section->end > sd->data_heap_size) {
        sd->data_heap_size = sd->heap->current_section->end + 256;
        sd->data_heap =
            realloc(sd->data_heap, sd->data_heap_size * sizeof(int32_t));
        if (sd->data_heap == NULL) {
            perror("Cannot grow data_heap");
            exit(1);
        }
    }
}

move_t NextMove(struct SearchData *sd) {
    struct SearchStatus *st = sd->current;
    struct Position *p = sd->position;
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
#ifdef VERBOSE
        Print(9, "GenerateCaptures\n");
#endif

        /*
         * Generate captures.
         */
        BitBoard targets = p->mask[OPP(p->turn)][0];
        while (targets) {
            int to = FindSetBit(targets);
            targets &= targets - 1;

            GenTo(p, to, sd->heap);
        }

        BitBoard promoting_pawns =
            p->mask[p->turn][Pawn] & SeventhRank[p->turn];
        while (promoting_pawns) {
            int from = FindSetBit(promoting_pawns);
            promoting_pawns &= promoting_pawns - 1;

            GenFrom(p, from, sd->heap);
        }

        grow_data_heap(sd);
        for (unsigned int j = sd->heap->current_section->start;
             j < sd->heap->current_section->end; j++) {
            sd->data_heap[j] = SwapOff(p, sd->heap->data[j]);
        }

        unsigned int last_end = sd->heap->current_section->end;
        GenEnpas(p, sd->heap);
        grow_data_heap(sd);

        for (unsigned int j = last_end; j < sd->heap->current_section->end;
             j++) {
            sd->data_heap[j] = 0;
        }

        st->st_first = sd->heap->current_section->start;
        st->st_last = sd->heap->current_section->end;
        st->st_nc_first = st->st_last;

        st->st_phase = GainingCapture;
    }
    /* fall through */
    case GainingCapture:
#ifdef VERBOSE
        Print(9, "GainingCapture\n");
#endif
        while (st->st_last > st->st_first) {
            int besti = st->st_first;
            int best = sd->data_heap[besti];

            for (unsigned int i = st->st_first + 1; i < st->st_last; i++) {
                if (sd->data_heap[i] > best) {
                    best = sd->data_heap[i];
                    besti = i;
                }
            }
            if (best >= 0) {
                move = sd->heap->data[besti];
                st->st_last--;

                sd->heap->data[besti] = sd->heap->data[st->st_last];
                sd->data_heap[besti] = sd->data_heap[st->st_last];

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
                st->st_phase = LoosingCapture;
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
            int best = sd->data_heap[besti];

            for (unsigned int i = st->st_first + 1; i < st->st_last; i++) {
                if (sd->data_heap[i] > best) {
                    best = sd->data_heap[i];
                    besti = i;
                }
            }
            move = sd->heap->data[besti];

            sd->heap->data[besti] = sd->heap->data[st->st_first];
            sd->data_heap[besti] = sd->data_heap[st->st_first];
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
        const BitBoard empty = ~(p->mask[White][0] | p->mask[Black][0]);

        if (p->castle & CastleMask[p->turn][0]) {
            append_to_heap(sd->heap,
                           make_move(p->turn == White ? e1 : e8,
                                     p->turn == White ? g1 : g8, M_SCASTLE));
        }
        if (p->castle & CastleMask[p->turn][1]) {
            append_to_heap(sd->heap,
                           make_move(p->turn == White ? e1 : e8,
                                     p->turn == White ? c1 : c8, M_LCASTLE));
        }

        BitBoard non_pawn = p->mask[p->turn][0] & ~p->mask[p->turn][Pawn];

        while (non_pawn) {
            int from = FindSetBit(non_pawn);
            non_pawn &= non_pawn - 1;
            BitBoard attacks = p->atkTo[from] & empty;
            while (attacks) {
                int to = FindSetBit(attacks);
                attacks &= attacks - 1;
                append_to_heap(sd->heap, make_move(from, to, 0));
            }
        }

        BitBoard tmp = p->mask[p->turn][Pawn] & ~SeventhRank[p->turn];

        if (p->turn == White)
            tmp = ShiftUp(tmp);
        else
            tmp = ShiftDown(tmp);

        BitBoard tmp2 = tmp &= empty;

        while (tmp2) {
            int to = FindSetBit(tmp2);
            tmp2 &= tmp2 - 1;

            int fr = (p->turn == White) ? to - 8 : to + 8;
            append_to_heap(sd->heap, make_move(fr, to, 0));
        }

        tmp &= ThirdRank[p->turn];

        if (p->turn == White)
            tmp = ShiftUp(tmp);
        else
            tmp = ShiftDown(tmp);

        tmp &= empty;

        while (tmp) {
            int to = FindSetBit(tmp);
            tmp &= tmp - 1;

            int fr = (p->turn == White) ? to - 16 : to + 16;
            append_to_heap(sd->heap, make_move(fr, to, M_PAWND));
        }

        st->st_phase = HistoryMoves;
        st->st_last = sd->heap->current_section->end;
    }

    case HistoryMoves:
#ifdef VERBOSE
        Print(9, "HistoryMoves\n");
#endif
        while (st->st_last > st->st_nc_first) {
            int besti = st->st_nc_first;
            int best = sd->historyTab[p->turn][sd->heap->data[besti] & 4095];

            for (unsigned int i = st->st_nc_first + 1; i < st->st_last; i++) {
                int hval = sd->historyTab[p->turn][sd->heap->data[i] & 4095];
                if (hval > best) {
                    best = hval;
                    besti = i;
                }
            }
            move = sd->heap->data[besti];

            st->st_last--;
            sd->heap->data[besti] = sd->heap->data[st->st_last];

            if (move == st->st_hashmove || move == st->st_k1 ||
                move == st->st_k2 || move == st->st_k3 || move == st->st_cm)
                continue;

            return move;
        }
    }

    return M_NONE;
}

move_t NextEvasion(struct SearchData *sd) {
    struct SearchStatus *st = sd->current;
    struct Position *p = sd->position;
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
#ifdef VERBOSE
        Print(9, "GainingCapture\n");
#endif

        /*
         * Generate captures. If in check, generate only
         * captures by the king or to pieces which give
         * check
         */

        int kp = p->kingSq[p->turn];

        BitBoard targets =
            (p->atkFr[kp] | p->atkTo[kp]) & p->mask[OPP(p->turn)][0];

        while (targets) {
            int to = FindSetBit(targets);
            targets &= targets - 1;
            GenTo(p, to, sd->heap);
        }

        grow_data_heap(sd);
        for (unsigned int j = sd->heap->current_section->start;
             j < sd->heap->current_section->end; j++) {
            sd->data_heap[j] = SwapOff(p, sd->heap->data[j]);
        }

        unsigned int last_end = sd->heap->current_section->end;
        GenEnpas(p, sd->heap);
        grow_data_heap(sd);

        for (unsigned int j = last_end; j < sd->heap->current_section->end;
             j++) {
            sd->data_heap[j] = 0;
        }

        st->st_first = sd->heap->current_section->start;
        st->st_last = sd->heap->current_section->end;
        st->st_nc_first = st->st_last;
    }
        /* fall through */
    case GainingCapture:
        while (st->st_last > st->st_first) {
            int besti = st->st_first;
            int best = sd->data_heap[besti];

            for (unsigned int i = st->st_first + 1; i < st->st_last; i++) {
                if (sd->data_heap[i] > best) {
                    best = sd->data_heap[i];
                    besti = i;
                }
            }
            if (best >= 0) {
                move = sd->heap->data[besti];
                st->st_last--;

                sd->heap->data[besti] = sd->heap->data[st->st_last];
                sd->data_heap[besti] = sd->data_heap[st->st_last];

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
            int best = sd->data_heap[besti];

            for (unsigned int i = st->st_first + 1; i < st->st_last; i++) {
                if (sd->data_heap[i] > best) {
                    best = sd->data_heap[i];
                    besti = i;
                }
            }
            move = sd->heap->data[besti];

            sd->heap->data[besti] = sd->heap->data[st->st_first];
            sd->data_heap[besti] = sd->data_heap[st->st_first];
            st->st_first++;

            st->st_phase = LoosingCapture;

            if (move == st->st_hashmove)
                continue;

            return move;
        }

        /* fall through */
    case GenerateRest: {
#ifdef VERBOSE
        Print(9, "HistoryMoves\n");
#endif

        const int kp = p->kingSq[p->turn]; /* FindSetBit(Mask[Side][King]); */
        const BitBoard empty = ~(p->mask[White][0] | p->mask[Black][0]);

        BitBoard king_flight_squares = p->atkTo[kp] & empty;

        while (king_flight_squares) {
            int to = FindSetBit(king_flight_squares);
            king_flight_squares &= king_flight_squares - 1;
            if (!(p->atkFr[to] & p->mask[OPP(p->turn)][0]))
                append_to_heap(sd->heap, make_move(kp, to, 0));
        }

        BitBoard sliding_attackers =
            (p->mask[OPP(p->turn)][Bishop] | p->mask[OPP(p->turn)][Rook] |
             p->mask[OPP(p->turn)][Queen]) &
            p->atkFr[kp];

        BitBoard interpositions = 0;

        while (sliding_attackers) {
            int attacker_sq = FindSetBit(sliding_attackers);
            sliding_attackers &= sliding_attackers - 1;
            interpositions = InterPath[kp][attacker_sq];
        }

        BitBoard non_pawns = (p->mask[p->turn][0] & ~p->mask[p->turn][King]) &
                             ~p->mask[p->turn][Pawn];

        while (non_pawns) {
            int from = FindSetBit(non_pawns);
            non_pawns &= non_pawns - 1;
            BitBoard blocking = p->atkTo[from] & empty & interpositions;

            while (blocking) {
                int to = FindSetBit(blocking);
                blocking &= blocking - 1;
                append_to_heap(sd->heap, make_move(from, to, 0));
            }
        }

        BitBoard pawns = p->mask[p->turn][Pawn];

        if (p->turn == White)
            pawns = ShiftUp(pawns);
        else
            pawns = ShiftDown(pawns);

        BitBoard pawns_to = pawns = (pawns & empty);

        while (pawns_to) {
            int to = FindSetBit(pawns_to);
            pawns_to &= pawns_to - 1;
            int fr = (p->turn == White) ? to - 8 : to + 8;

            if (is_promo_square(to)) {
                append_to_heap(sd->heap, make_move(fr, to, M_PQUEEN));
                append_to_heap(sd->heap, make_move(fr, to, M_PKNIGHT));
                append_to_heap(sd->heap, make_move(fr, to, M_PROOK));
                append_to_heap(sd->heap, make_move(fr, to, M_PBISHOP));
            } else
                append_to_heap(sd->heap, make_move(fr, to, 0));
        }

        pawns &= ThirdRank[p->turn];

        if (p->turn == White)
            pawns = ShiftUp(pawns);
        else
            pawns = ShiftDown(pawns);

        pawns &= empty;

        while (pawns) {
            int to = FindSetBit(pawns);
            pawns &= pawns - 1;
            int fr = (p->turn == White) ? to - 16 : to + 16;
            append_to_heap(sd->heap, make_move(fr, to, M_PAWND));
        }

        st->st_phase = HistoryMoves;
        st->st_last = sd->heap->current_section->end;
    }

        /* fall through */
    case HistoryMoves:
#ifdef VERBOSE
        Print(9, "HistoryMoves\n");
#endif
        while (st->st_last > st->st_nc_first) {
            int besti = st->st_nc_first;
            int best = sd->historyTab[p->turn][sd->heap->data[besti] & 4095];

            for (unsigned int i = st->st_nc_first + 1; i < st->st_last; i++) {
                int hval = sd->historyTab[p->turn][sd->heap->data[i] & 4095];
                if (hval > best) {
                    best = hval;
                    besti = i;
                }
            }
            move = sd->heap->data[besti];

            st->st_last--;
            sd->heap->data[besti] = sd->heap->data[st->st_last];

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

    st->st_first = sd->heap->current_section->start;

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
            move_t move = make_move(i, next, M_PQUEEN);
            int sw;
            if ((sw = SwapOff(p, move)) >= 0) {
                st->st_last = sd->heap->current_section->end;
                append_to_heap(sd->heap, move);
                grow_data_heap(sd);
                sd->data_heap[st->st_last] = sw;
                st->st_last++;
            }
        }

        tmp = p->atkTo[i] & p->mask[OPP(p->turn)][0];
        while (tmp) {
            int sw;
            j = FindSetBit(tmp);
            tmp &= tmp - 1;
            move_t move = make_move(i, j, M_CAPTURE | M_PQUEEN);
            if ((sw = SwapOff(p, move)) >= 0) {
                st->st_last = sd->heap->current_section->end;
                append_to_heap(sd->heap, move);
                grow_data_heap(sd);
                sd->data_heap[st->st_last] = sw;
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
            j = FindSetBit(tmp2);
            tmp2 &= tmp2 - 1;
            move_t move = make_move(j, i, M_CAPTURE);
            int sw = SwapOff(p, move);
            if (sw >= 0) {
                st->st_last = sd->heap->current_section->end;
                append_to_heap(sd->heap, move);
                grow_data_heap(sd);
                sd->data_heap[st->st_last] = sw;
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
            j = FindSetBit(tmp2);
            tmp2 &= tmp2 - 1;
            move_t move = make_move(j, i, M_CAPTURE);
            int sw = SwapOff(p, move);
            if (sw >= 0) {
                st->st_last = sd->heap->current_section->end;
                append_to_heap(sd->heap, move);
                grow_data_heap(sd);
                sd->data_heap[st->st_last] = sw;
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
            j = FindSetBit(tmp2);
            tmp2 &= tmp2 - 1;
            move_t move = make_move(j, i, M_CAPTURE);
            int sw = SwapOff(p, move);
            if (sw >= 0) {
                st->st_last = sd->heap->current_section->end;
                append_to_heap(sd->heap, move);
                grow_data_heap(sd);
                sd->data_heap[st->st_last] = sw;
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
            j = FindSetBit(tmp2);
            tmp2 &= tmp2 - 1;
            move_t move = make_move(j, i, M_CAPTURE);
            int sw = SwapOff(p, move);
            if (sw >= 0) {
                st->st_last = sd->heap->current_section->end;
                append_to_heap(sd->heap, move);
                grow_data_heap(sd);
                sd->data_heap[st->st_last] = sw;
                st->st_last++;
            }
        }
    }
}

move_t NextMoveQ(struct SearchData *sd, int alpha) {
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
            int best = sd->data_heap[besti];

            for (i = st->st_first + 1; i < st->st_last; i++) {
                if (sd->data_heap[i] > best) {
                    best = sd->data_heap[i];
                    besti = i;
                }
            }

            move = sd->heap->data[besti];
            st->st_last--;
            sd->heap->data[besti] = sd->heap->data[st->st_last];
            sd->data_heap[besti] = sd->data_heap[st->st_last];

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

static void test_next_move(struct SearchData *sd,
                           move_t (*func)(struct SearchData *)) {
    bool comma = false;
    EnterNode(sd);

    while (true) {
        move_t move = func(sd);
        if (move == M_NONE)
            break;

        if (LegalMove(sd->position, move)) {
            if (comma) {
                Print(0, ", ");
            }
            char san_buffer[16];
            Print(0, "%s", SAN(sd->position, move, san_buffer));
            comma = true;
        }
    }
    LeaveNode(sd);
    Print(0, "\n");
}

move_t next_move_q_fixed_alpha(struct SearchData *sd) {
    return NextMoveQ(sd, -500000);
}

void TestNextGenerators(struct Position *p) {
    struct SearchData *sd = CreateSearchData(p);
    Print(0, "NextMove:\n");
    test_next_move(sd, &NextMove);
    Print(0, "\nNextEvasion:\n");
    test_next_move(sd, &NextEvasion);
    Print(0, "\nNextMoveQ:\n");
    test_next_move(sd, &next_move_q_fixed_alpha);
}
