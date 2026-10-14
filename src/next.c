/*

    Amy - a chess playing program

    Copyright (c) 2002-2026, Thorsten Greiner
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

#include "next.h"
#include "dbase.h"
#include "evaluation.h"
#include "heap.h"
#include "init.h"
#include "inline.h"
#include "safe_malloc.h"
#include "search.h"
#include "swap.h"
#include "utils.h"
#if MP
#include "hashtable.h"
#endif

struct SearchData *next_create_data(struct Position *p) {
    struct SearchData *sd = safe_calloc(1, sizeof(struct SearchData));

    sd->position = p;
    sd->statusTable = safe_calloc(MAX_TREE_SIZE, sizeof(struct SearchStatus));
    sd->current = sd->statusTable;
    sd->killerTable = safe_calloc(MAX_TREE_SIZE, sizeof(struct KillerEntry));
    sd->killer = sd->killerTable;

    sd->heap = heap_allocate();

    sd->data_heap = NULL;
    sd->data_heap_size = 0;

#if MP
    sd->localHashTable = safe_calloc(L_HT_Size, sizeof(struct HTEntry));
    sd->deferred_heap = heap_allocate();
#endif

    sd->ply = 0;

    return sd;
}

void next_free_data(struct SearchData *sd) {
    free(sd->statusTable);
    free(sd->killerTable);
    free(sd->data_heap);
    heap_free(sd->heap);

#if MP
    free(sd->localHashTable);
    heap_free(sd->deferred_heap);
#endif

    free(sd);
}

void next_enter_node(struct SearchData *sd) {
    struct SearchStatus *st;

    st = ++(sd->current);

    st->st_phase = HashMove;
    sd->ply++;
    sd->killer++;

    heap_push_section(sd->heap);
#if MP
    heap_push_section(sd->deferred_heap);
#endif
}

void next_leave_node(struct SearchData *sd) {
    heap_pop_section(sd->heap);
    sd->current--;
    sd->killer--;
    sd->ply--;
#if MP
    heap_pop_section(sd->deferred_heap);
#endif
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

move_t next_move(struct SearchData *sd) {
    heap_section_t section = sd->heap->current_section;
    struct SearchStatus *st = sd->current;
    struct Position *p = sd->position;
    move_t move;

    switch (st->st_phase) {
    case HashMove:
#ifdef VERBOSE
        print(9, "HashMove\n");
#endif
        if (dbase_legal_move(p, st->st_hashmove)) {
            st->st_phase = GenerateCaptures;
            return st->st_hashmove;
        } else {
            st->st_hashmove = M_NONE;
        }
    /* fall through */
    case GenerateCaptures: {
#ifdef VERBOSE
        print(9, "GenerateCaptures\n");
#endif

        /*
         * Generate captures.
         */
        BitBoard targets = p->mask[OPP(p->turn)][0];
        while (targets) {
            int to = bitboard_find_set_bit(targets);
            targets &= targets - 1;

            dbase_gen_to(p, to, sd->heap);
        }

        BitBoard promoting_pawns =
            p->mask[p->turn][Pawn] & SeventhRank[p->turn];
        while (promoting_pawns) {
            int from = bitboard_find_set_bit(promoting_pawns);
            promoting_pawns &= promoting_pawns - 1;

            dbase_gen_from(p, from, sd->heap);
        }

        grow_data_heap(sd);
        for (unsigned int j = section->start; j < section->end; j++) {
            sd->data_heap[j] = swap_off(p, sd->heap->data[j]);
        }

        unsigned int last_end = section->end;
        dbase_gen_enpas(p, sd->heap);
        grow_data_heap(sd);

        for (unsigned int j = last_end; j < section->end; j++) {
            sd->data_heap[j] = 0;
        }

        st->st_phase = GainingCapture;
    }
    /* fall through */
    case GainingCapture:
#ifdef VERBOSE
        print(9, "GainingCapture\n");
#endif
        while (section->end > section->start) {
            unsigned int besti = section->start;
            int best = sd->data_heap[besti];

            for (unsigned int i = section->start + 1; i < section->end; i++) {
                if (sd->data_heap[i] > best) {
                    best = sd->data_heap[i];
                    besti = i;
                }
            }
            if (best >= 0) {
                move = sd->heap->data[besti];
                section->end--;

                sd->heap->data[besti] = sd->heap->data[section->end];
                sd->data_heap[besti] = sd->data_heap[section->end];

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
        print(9, "Killer1\n");
#endif
        st->st_k1 = M_NONE;
        if (move != st->st_hashmove && dbase_legal_move(p, move)) {
            st->st_phase = Killer2;
            st->st_k1 = move;

            return move;
        }
    }
    /* fall through */
    case Killer2: {
        move = sd->killer->killer2;
#ifdef VERBOSE
        print(9, "Killer2\n");
#endif
        st->st_k2 = M_NONE;
        if (move != st->st_hashmove && dbase_legal_move(p, move)) {
            st->st_phase = CounterMv;
            st->st_k2 = move;

            return move;
        }
    }
    /* fall through */
    case CounterMv: {
        move_t lmove = (p->actLog - 1)->gl_Move;

#ifdef VERBOSE
        print(9, "CounterMv\n");
#endif
        st->st_cm = M_NONE;
        if (lmove != M_NULL) {
            move = sd->counterTab[p->turn][lmove & 4095];

            if (move != M_NONE && move != st->st_hashmove &&
                move != st->st_k1 && move != st->st_k2 && dbase_legal_move(p, move)) {
                st->st_phase = Killer3;
                st->st_cm = move;

                return move;
            }
        }
    }
    /* fallthrough */
    case Killer3:
#ifdef VERBOSE
        print(9, "Killer3\n");
#endif
        st->st_k3 = M_NONE;
        if (sd->ply >= 2) {
            move = (sd->killer - 2)->killer1;

            if (move == st->st_hashmove || move == st->st_k1 ||
                move == st->st_k2 || move == st->st_cm || !dbase_legal_move(p, move))
                move = (sd->killer - 2)->killer2;

            if (move != st->st_hashmove && move != st->st_k1 &&
                move != st->st_k2 && move != st->st_cm && dbase_legal_move(p, move)) {
                st->st_phase = LoosingCapture;
                st->st_k3 = move;

                return move;
            }
        }
        /* fallthrough */

    case LoosingCapture:
#ifdef VERBOSE
        print(9, "LoosingCapture\n");
#endif
        while (section->end > section->start) {
            unsigned int besti = section->start;
            int best = sd->data_heap[besti];

            for (unsigned int i = section->start + 1; i < section->end; i++) {
                if (sd->data_heap[i] > best) {
                    best = sd->data_heap[i];
                    besti = i;
                }
            }

            move = sd->heap->data[besti];
            section->end--;

            sd->heap->data[besti] = sd->heap->data[section->end];
            sd->data_heap[besti] = sd->data_heap[section->end];

            st->st_phase = LoosingCapture;

            if (move == st->st_hashmove)
                continue;

            return move;
        }
        /* fallthrough */

    case GenerateRest: {
#ifdef VERBOSE
        print(9, "GenerateRest\n");
#endif
        const BitBoard empty = ~(p->mask[White][0] | p->mask[Black][0]);

        if (p->castle & CastleMask[p->turn][0]) {
            heap_append(sd->heap,
                           make_move(p->turn == White ? e1 : e8,
                                     p->turn == White ? g1 : g8, M_SCASTLE));
        }
        if (p->castle & CastleMask[p->turn][1]) {
            heap_append(sd->heap,
                           make_move(p->turn == White ? e1 : e8,
                                     p->turn == White ? c1 : c8, M_LCASTLE));
        }

        BitBoard non_pawn = p->mask[p->turn][0] & ~p->mask[p->turn][Pawn];

        while (non_pawn) {
            int from = bitboard_find_set_bit(non_pawn);
            non_pawn &= non_pawn - 1;
            BitBoard attacks = p->atkTo[from] & empty;
            while (attacks) {
                int to = bitboard_find_set_bit(attacks);
                attacks &= attacks - 1;
                heap_append(sd->heap, make_move(from, to, 0));
            }
        }

        BitBoard tmp = p->mask[p->turn][Pawn] & ~SeventhRank[p->turn];

        if (p->turn == White)
            tmp = bitboard_shift_up(tmp);
        else
            tmp = bitboard_shift_down(tmp);

        BitBoard tmp2 = tmp &= empty;

        while (tmp2) {
            int to = bitboard_find_set_bit(tmp2);
            tmp2 &= tmp2 - 1;

            int fr = (p->turn == White) ? to - 8 : to + 8;
            heap_append(sd->heap, make_move(fr, to, 0));
        }

        tmp &= ThirdRank[p->turn];

        if (p->turn == White)
            tmp = bitboard_shift_up(tmp);
        else
            tmp = bitboard_shift_down(tmp);

        tmp &= empty;

        while (tmp) {
            int to = bitboard_find_set_bit(tmp);
            tmp &= tmp - 1;

            int fr = (p->turn == White) ? to - 16 : to + 16;
            heap_append(sd->heap, make_move(fr, to, M_PAWND));
        }

        st->st_phase = HistoryMoves;
    }

    case HistoryMoves:
#ifdef VERBOSE
        print(9, "HistoryMoves\n");
#endif
        while (section->end > section->start) {
            int besti = section->start;
            int best = sd->historyTab[p->turn][sd->heap->data[besti] & 4095];

            for (unsigned int i = section->start + 1; i < section->end; i++) {
                int hval = sd->historyTab[p->turn][sd->heap->data[i] & 4095];
                if (hval > best) {
                    best = hval;
                    besti = i;
                }
            }
            move = sd->heap->data[besti];

            section->end--;
            sd->heap->data[besti] = sd->heap->data[section->end];

            if (move == st->st_hashmove || move == st->st_k1 ||
                move == st->st_k2 || move == st->st_k3 || move == st->st_cm)
                continue;

            return move;
        }

    default:
        break;
    }

    return M_NONE;
}

move_t next_evasion(struct SearchData *sd) {
    heap_section_t section = sd->heap->current_section;
    struct SearchStatus *st = sd->current;
    struct Position *p = sd->position;
    move_t move;

    switch (st->st_phase) {
    case HashMove:
#ifdef VERBOSE
        print(9, "HashMove\n");
#endif
        if (dbase_legal_move(p, st->st_hashmove)) {
            st->st_phase = GenerateCaptures;
            return st->st_hashmove;
        } else {
            st->st_hashmove = M_NONE;
        }
        /* fall through */
    case GenerateCaptures: {
#ifdef VERBOSE
        print(9, "GainingCapture\n");
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
            int to = bitboard_find_set_bit(targets);
            targets &= targets - 1;
            dbase_gen_to(p, to, sd->heap);
        }

        grow_data_heap(sd);
        for (unsigned int j = section->start; j < section->end; j++) {
            sd->data_heap[j] = swap_off(p, sd->heap->data[j]);
        }

        unsigned int last_end = section->end;
        dbase_gen_enpas(p, sd->heap);
        grow_data_heap(sd);

        for (unsigned int j = last_end; j < section->end; j++) {
            sd->data_heap[j] = 0;
        }
    }
        /* fall through */
    case GainingCapture:
        while (section->end > section->start) {
            unsigned int besti = section->start;
            int best = sd->data_heap[besti];

            for (unsigned int i = section->start + 1; i < section->end; i++) {
                if (sd->data_heap[i] > best) {
                    best = sd->data_heap[i];
                    besti = i;
                }
            }
            if (best >= 0) {
                move = sd->heap->data[besti];
                section->end--;

                sd->heap->data[besti] = sd->heap->data[section->end];
                sd->data_heap[besti] = sd->data_heap[section->end];

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
        print(9, "Killer1\n");
#endif
        st->st_k1 = M_NONE;
        if (move != st->st_hashmove && dbase_legal_move(p, move)) {
            st->st_phase = Killer2;
            st->st_k1 = move;

            return move;
        }
    }
        /* fall through */
    case Killer2: {
        move = sd->killer->killer2;
#ifdef VERBOSE
        print(9, "Killer2\n");
#endif
        st->st_k2 = M_NONE;
        if (move != st->st_hashmove && dbase_legal_move(p, move)) {
            st->st_phase = CounterMv;
            st->st_k2 = move;

            return move;
        }
    }
        /* fall through */
    case CounterMv: {
        move_t lmove = (p->actLog - 1)->gl_Move;

#ifdef VERBOSE
        print(9, "CounterMv\n");
#endif
        st->st_cm = M_NONE;
        if (lmove != M_NULL) {
            move = sd->counterTab[p->turn][lmove & 4095];

            if (move != M_NONE && move != st->st_hashmove &&
                move != st->st_k1 && move != st->st_k2 && dbase_legal_move(p, move)) {
                st->st_phase = Killer3;
                st->st_cm = move;

                return move;
            }
        }
    }
        /* fall through */
    case Killer3:
#ifdef VERBOSE
        print(9, "Killer3\n");
#endif
        st->st_k3 = M_NONE;
        if (sd->ply >= 2) {
            move = (sd->killer - 2)->killer1;

            if (move == st->st_hashmove || move == st->st_k1 ||
                move == st->st_k2 || move == st->st_cm || !dbase_legal_move(p, move))
                move = (sd->killer - 2)->killer2;

            if (move != st->st_hashmove && move != st->st_k1 &&
                move != st->st_k2 && move != st->st_cm && dbase_legal_move(p, move)) {
                st->st_phase = /* HistoryMoves; */ LoosingCapture;
                st->st_k3 = move;

                return move;
            }
        }
        /* fall through */
    case LoosingCapture:
#ifdef VERBOSE
        print(9, "LoosingCapture\n");
#endif
        while (section->end > section->start) {
            unsigned int besti = section->start;
            int best = sd->data_heap[besti];

            for (unsigned int i = section->start + 1; i < section->end; i++) {
                if (sd->data_heap[i] > best) {
                    best = sd->data_heap[i];
                    besti = i;
                }
            }
            move = sd->heap->data[besti];

            section->end--;
            sd->heap->data[besti] = sd->heap->data[section->end];
            sd->data_heap[besti] = sd->data_heap[section->end];

            st->st_phase = LoosingCapture;

            if (move == st->st_hashmove)
                continue;

            return move;
        }

        /* fall through */
    case GenerateRest: {
#ifdef VERBOSE
        print(9, "HistoryMoves\n");
#endif

        const int kp = p->kingSq[p->turn]; /* bitboard_find_set_bit(Mask[Side][King]); */
        const BitBoard empty = ~(p->mask[White][0] | p->mask[Black][0]);

        BitBoard king_flight_squares = p->atkTo[kp] & empty;

        while (king_flight_squares) {
            int to = bitboard_find_set_bit(king_flight_squares);
            king_flight_squares &= king_flight_squares - 1;
            if (!(p->atkFr[to] & p->mask[OPP(p->turn)][0]))
                heap_append(sd->heap, make_move(kp, to, 0));
        }

        BitBoard sliding_attackers =
            (p->mask[OPP(p->turn)][Bishop] | p->mask[OPP(p->turn)][Rook] |
             p->mask[OPP(p->turn)][Queen]) &
            p->atkFr[kp];

        BitBoard interpositions = 0;

        while (sliding_attackers) {
            int attacker_sq = bitboard_find_set_bit(sliding_attackers);
            sliding_attackers &= sliding_attackers - 1;
            interpositions = InterPath[kp][attacker_sq];
        }

        BitBoard non_pawns = (p->mask[p->turn][0] & ~p->mask[p->turn][King]) &
                             ~p->mask[p->turn][Pawn];

        while (non_pawns) {
            int from = bitboard_find_set_bit(non_pawns);
            non_pawns &= non_pawns - 1;
            BitBoard blocking = p->atkTo[from] & empty & interpositions;

            while (blocking) {
                int to = bitboard_find_set_bit(blocking);
                blocking &= blocking - 1;
                heap_append(sd->heap, make_move(from, to, 0));
            }
        }

        BitBoard pawns = p->mask[p->turn][Pawn];

        if (p->turn == White)
            pawns = bitboard_shift_up(pawns);
        else
            pawns = bitboard_shift_down(pawns);

        BitBoard pawns_to = pawns = (pawns & empty);

        while (pawns_to) {
            int to = bitboard_find_set_bit(pawns_to);
            pawns_to &= pawns_to - 1;
            int fr = (p->turn == White) ? to - 8 : to + 8;

            if (is_promo_square(to)) {
                heap_append(sd->heap, make_promotion(fr, to, Queen, 0));
                heap_append(sd->heap, make_promotion(fr, to, Knight, 0));
                heap_append(sd->heap, make_promotion(fr, to, Rook, 0));
                heap_append(sd->heap, make_promotion(fr, to, Bishop, 0));
            } else
                heap_append(sd->heap, make_move(fr, to, 0));
        }

        pawns &= ThirdRank[p->turn];

        if (p->turn == White)
            pawns = bitboard_shift_up(pawns);
        else
            pawns = bitboard_shift_down(pawns);

        pawns &= empty;

        while (pawns) {
            int to = bitboard_find_set_bit(pawns);
            pawns &= pawns - 1;
            int fr = (p->turn == White) ? to - 16 : to + 16;
            heap_append(sd->heap, make_move(fr, to, M_PAWND));
        }

        st->st_phase = HistoryMoves;
    }

        /* fall through */
    case HistoryMoves:
#ifdef VERBOSE
        print(9, "HistoryMoves\n");
#endif
        while (section->end > section->start) {
            unsigned int besti = section->start;
            int best = sd->historyTab[p->turn][sd->heap->data[besti] & 4095];

            for (unsigned int i = section->start + 1; i < section->end; i++) {
                int hval = sd->historyTab[p->turn][sd->heap->data[i] & 4095];
                if (hval > best) {
                    best = hval;
                    besti = i;
                }
            }
            move = sd->heap->data[besti];

            section->end--;
            sd->heap->data[besti] = sd->heap->data[section->end];

            if (move == st->st_hashmove || move == st->st_k1 ||
                move == st->st_k2 || move == st->st_k3 || move == st->st_cm)
                continue;

            return move;
        }
    default:
        break;
    }

    return M_NONE;
}

static void next_generate_qcaptures(struct SearchData *sd, int alpha) {
    heap_section_t section = sd->heap->current_section;
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

        i = bitboard_find_set_bit(pwn7th);
        pwn7th &= pwn7th - 1;
        next = (p->turn == White) ? i + 8 : i - 8;

        if (p->piece[next] == Neutral) {
            move_t move = make_promotion(i, next, Queen, 0);
            int sw;
            if ((sw = swap_off(p, move)) >= 0) {
                heap_append(sd->heap, move);
                grow_data_heap(sd);
                sd->data_heap[section->end - 1] = sw;
            }
        }

        tmp = p->atkTo[i] & p->mask[OPP(p->turn)][0];
        while (tmp) {
            int sw;
            j = bitboard_find_set_bit(tmp);
            tmp &= tmp - 1;
            move_t move = make_promotion(i, j, Queen, M_CAPTURE);
            if ((sw = swap_off(p, move)) >= 0) {
                heap_append(sd->heap, move);
                grow_data_heap(sd);
                sd->data_heap[section->end - 1] = sw;
            }
        }
    }

    if (p->turn == White) {
        score = eval_material_balance(p) + MaxPos;
    } else {
        score = -eval_material_balance(p) + MaxPos;
    }

    if (score + Value[Queen] <= alpha)
        return;
    def = p->mask[OPP(p->turn)][Queen];
    while (def) {
        BitBoard tmp2;
        int j;
        i = bitboard_find_set_bit(def);
        def &= def - 1;
        tmp2 = p->atkFr[i] & att;
        while (tmp2) {
            j = bitboard_find_set_bit(tmp2);
            tmp2 &= tmp2 - 1;
            move_t move = make_move(j, i, M_CAPTURE);
            int sw = swap_off(p, move);
            if (sw >= 0) {
                heap_append(sd->heap, move);
                grow_data_heap(sd);
                sd->data_heap[section->end - 1] = sw;
            }
        }
    }
    if (score + Value[Rook] <= alpha)
        return;
    def = p->mask[OPP(p->turn)][Rook];
    while (def) {
        BitBoard tmp2;
        int j;
        i = bitboard_find_set_bit(def);
        def &= def - 1;
        tmp2 = p->atkFr[i] & att;
        while (tmp2) {
            j = bitboard_find_set_bit(tmp2);
            tmp2 &= tmp2 - 1;
            move_t move = make_move(j, i, M_CAPTURE);
            int sw = swap_off(p, move);
            if (sw >= 0) {
                heap_append(sd->heap, move);
                grow_data_heap(sd);
                sd->data_heap[section->end - 1] = sw;
            }
        }
    }
    if (score + Value[Bishop] <= alpha)
        return;
    def = p->mask[OPP(p->turn)][Bishop] | p->mask[OPP(p->turn)][Knight];
    while (def) {
        BitBoard tmp2;
        int j;
        i = bitboard_find_set_bit(def);
        def &= def - 1;
        tmp2 = p->atkFr[i] & att;
        while (tmp2) {
            j = bitboard_find_set_bit(tmp2);
            tmp2 &= tmp2 - 1;
            move_t move = make_move(j, i, M_CAPTURE);
            int sw = swap_off(p, move);
            if (sw >= 0) {
                heap_append(sd->heap, move);
                grow_data_heap(sd);
                sd->data_heap[section->end - 1] = sw;
            }
        }
    }
    if (score + Value[Pawn] <= alpha)
        return;
    def = p->mask[OPP(p->turn)][Pawn];
    while (def) {
        BitBoard tmp2;
        int j;
        i = bitboard_find_set_bit(def);
        def &= def - 1;
        tmp2 = p->atkFr[i] & att;
        while (tmp2) {
            j = bitboard_find_set_bit(tmp2);
            tmp2 &= tmp2 - 1;
            move_t move = make_move(j, i, M_CAPTURE);
            int sw = swap_off(p, move);
            if (sw >= 0) {
                heap_append(sd->heap, move);
                grow_data_heap(sd);
                sd->data_heap[section->end - 1] = sw;
            }
        }
    }
}

move_t next_move_q(struct SearchData *sd, int alpha) {
    heap_section_t section = sd->heap->current_section;
    struct SearchStatus *st = sd->current;
    move_t move;

    switch (st->st_phase) {
    case HashMove:
    case GenerateCaptures:
#ifdef VERBOSE
        print(9, "GenerateCaptures\n");
#endif
        next_generate_qcaptures(sd, alpha);
        st->st_phase = GainingCapture;

        /* fall through */
    case GainingCapture:
#ifdef VERBOSE
        print(9, "GainingCapture\n");
#endif
        while (section->end > section->start) {
            unsigned int besti = section->start;
            int best = sd->data_heap[besti];

            for (unsigned int i = section->start + 1; i < section->end; i++) {
                if (sd->data_heap[i] > best) {
                    best = sd->data_heap[i];
                    besti = i;
                }
            }

            move = sd->heap->data[besti];
            section->end--;
            sd->heap->data[besti] = sd->heap->data[section->end];
            sd->data_heap[besti] = sd->data_heap[section->end];

            return move;
        }
    default:
        break;
    }

    return M_NONE;
}

/*
 * Enter move in Killertable
 */

void next_put_killer(struct SearchData *sd, move_t m) {
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
    next_enter_node(sd);

    while (true) {
        move_t move = func(sd);
        if (move == M_NONE)
            break;

        if (dbase_legal_move(sd->position, move)) {
            if (comma) {
                print(0, ", ");
            }
            char san_buffer[16];
            print(0, "%s", dbase_san(sd->position, move, san_buffer));
            comma = true;
        }
    }
    next_leave_node(sd);
    print(0, "\n");
}

move_t next_move_q_fixed_alpha(struct SearchData *sd) {
    return next_move_q(sd, -500000);
}

void next_test_generators(struct Position *p) {
    struct SearchData *sd = next_create_data(p);
    print(0, "next_move:\n");
    test_next_move(sd, &next_move);
    print(0, "\nNextEvasion:\n");
    test_next_move(sd, &next_evasion);
    print(0, "\nNextMoveQ:\n");
    test_next_move(sd, &next_move_q_fixed_alpha);
}
