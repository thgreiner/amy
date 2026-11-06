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

#include "bitboard.h"
#include "dbase.h"
#include "init.h"
#include "intset_sorted.h"
#include "utils.h"

#include <stdlib.h>

static int am_index(int attacker, int victim, int from, int to) {
    int plane = (attacker - 1) * 5 + victim - 2;
    int sq = from * 64 + to;

    return 64 * 64 * plane + sq;
}

static void am_compute_attacks(int from, int attacker, int victim,
                            BitBoard attacks, IntSetSorted *indexes,
                            int mirror) {
    while (attacks) {
        int to = bitboard_find_set_bit(attacks) ^ mirror;
        attacks &= attacks - 1;

        int index = am_index(attacker, victim, from ^ mirror, to);
        /* The set capacity is a hard bound on the number of attack
         * features per side; reaching it means a real position has
         * more attacks than the network can represent.  Fail loudly
         * instead of silently truncating the map (and with it the
         * accumulator contribution).
         */
        if (intset_sorted_add(indexes, index) == INTSET_SORTED_FULL) {
            print(0, "Attack map overflow: more than %d attack "
                     "features in one position.\n", INTSET_SORTED_MAX);
            exit(1);
        }
    }
}

static void am_compute_all_attacks_of_side(struct Position *p, int side,
                                        IntSetSorted *indexes, int mirror) {

    // Pawn attacks - include all pawn attacks to pieces
    BitBoard attackers = p->mask[side][Pawn];
    while (attackers) {
        int from = bitboard_find_set_bit(attackers);
        attackers &= attackers - 1;

        am_compute_attacks(from, Pawn, Knight,
                        p->atkTo[from] & p->mask[!side][Knight], indexes,
                        mirror);
        am_compute_attacks(from, Pawn, Bishop,
                        p->atkTo[from] & p->mask[!side][Bishop], indexes,
                        mirror);
        am_compute_attacks(from, Pawn, Rook, p->atkTo[from] & p->mask[!side][Rook],
                        indexes, mirror);
        am_compute_attacks(from, Pawn, Queen,
                        p->atkTo[from] & p->mask[!side][Queen], indexes,
                        mirror);
    }

    // Knight attacks - include all knight attacks to pieces and pawns
    attackers = p->mask[side][Knight];
    while (attackers) {
        int from = bitboard_find_set_bit(attackers);
        attackers &= attackers - 1;

        am_compute_attacks(from, Knight, Pawn,
                        p->atkTo[from] & p->mask[!side][Pawn], indexes, mirror);
        am_compute_attacks(from, Knight, Knight,
                        p->atkTo[from] & p->mask[!side][Knight], indexes,
                        mirror);
        am_compute_attacks(from, Knight, Bishop,
                        p->atkTo[from] & p->mask[!side][Bishop], indexes,
                        mirror);
        am_compute_attacks(from, Knight, Rook,
                        p->atkTo[from] & p->mask[!side][Rook], indexes, mirror);
        am_compute_attacks(from, Knight, Queen,
                        p->atkTo[from] & p->mask[!side][Queen], indexes,
                        mirror);
    }

    // Bishop attacks - include all bishop attacks to pieces and pawns, exclude
    // attacks on pawns that already attack the bishop
    attackers = p->mask[side][Bishop];
    while (attackers) {
        int from = bitboard_find_set_bit(attackers);
        attackers &= attackers - 1;

        // exclude attacks on pawns that already attack the bishop
        BitBoard excl = ~PawnEPM[side][from];
        am_compute_attacks(from, Bishop, Pawn,
                        p->atkTo[from] & excl & p->mask[!side][Pawn], indexes,
                        mirror);
        am_compute_attacks(from, Bishop, Knight,
                        p->atkTo[from] & p->mask[!side][Knight], indexes,
                        mirror);
        am_compute_attacks(from, Bishop, Bishop,
                        p->atkTo[from] & p->mask[!side][Bishop], indexes,
                        mirror);
        am_compute_attacks(from, Bishop, Rook,
                        p->atkTo[from] & p->mask[!side][Rook], indexes, mirror);
        am_compute_attacks(from, Bishop, Queen,
                        p->atkTo[from] & p->mask[!side][Queen], indexes,
                        mirror);
    }

    // Rook attacks - include all rook attacks to pieces and pawns
    attackers = p->mask[side][Rook];
    while (attackers) {
        int from = bitboard_find_set_bit(attackers);
        attackers &= attackers - 1;

        am_compute_attacks(from, Rook, Pawn, p->atkTo[from] & p->mask[!side][Pawn],
                        indexes, mirror);
        am_compute_attacks(from, Rook, Knight,
                        p->atkTo[from] & p->mask[!side][Knight], indexes,
                        mirror);
        am_compute_attacks(from, Rook, Bishop,
                        p->atkTo[from] & p->mask[!side][Bishop], indexes,
                        mirror);
        am_compute_attacks(from, Rook, Rook, p->atkTo[from] & p->mask[!side][Rook],
                        indexes, mirror);
        am_compute_attacks(from, Rook, Queen,
                        p->atkTo[from] & p->mask[!side][Queen], indexes,
                        mirror);
    }

    // Queen attacks - include all queen attacks to pieces and pawns, exclude
    // those where the pawn, bishop or rook already attack the queen
    attackers = p->mask[side][Queen];
    while (attackers) {
        int from = bitboard_find_set_bit(attackers);
        attackers &= attackers - 1;

        // exclude attacks on pawns that already attack the queen
        BitBoard excl = ~PawnEPM[side][from];
        am_compute_attacks(from, Queen, Pawn,
                        p->atkTo[from] & excl & p->mask[!side][Pawn], indexes,
                        mirror);
        am_compute_attacks(from, Queen, Knight,
                        p->atkTo[from] & p->mask[!side][Knight], indexes,
                        mirror);
        // Only include queen attacks on bishops via rook moves
        am_compute_attacks(from, Queen, Bishop,
                        p->atkTo[from] & RookEPM[from] & p->mask[!side][Bishop],
                        indexes, mirror);
        // Only include queen attacks on rooks via bishop moves
        am_compute_attacks(from, Queen, Rook,
                        p->atkTo[from] & BishopEPM[from] & p->mask[!side][Rook],
                        indexes, mirror);
        am_compute_attacks(from, Queen, Queen,
                        p->atkTo[from] & p->mask[!side][Queen], indexes,
                        mirror);
    }
}

void am_compute_attack_maps(struct Position *p, IntSetSorted *indexes_w,
                        IntSetSorted *indexes_b) {
    int king_sq_w = p->kingSq[White];
    int king_sq_b = p->kingSq[Black];

    int mirror_w = (king_sq_w & 7) > 3 ? 7 : 0;
    int mirror_b = (king_sq_b & 7) > 3 ? 0x3f : 0x38;

    am_compute_all_attacks_of_side(p, White, indexes_w, mirror_w);
    am_compute_all_attacks_of_side(p, Black, indexes_b, mirror_b);
}
