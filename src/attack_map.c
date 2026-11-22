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

#include <stdio.h>
#include <stdlib.h>

#define OFFSET_DEFENDER 28

static int am_index(int attacker, int victim, int from, int to, int offset) {
    int plane = (attacker - 1) * 5 + victim - 2;
    int sq = from * 64 + to;

    return 64 * 64 * (plane + offset) + sq;
}

/*
 * The side's canonical frame flip: black squares are rank-flipped
 * (0x38), and the frame is additionally mirrored horizontally (7) when
 * the side's king stands on files e..h.  The two masks commute.
 */
static int am_side_mirror(const Position *p, int side) {
    int mirror = (p->kingSq[side] & 7) > 3 ? 7 : 0;
    if (side == Black) {
        mirror |= 0x38;
    }
    return mirror;
}

static void am_compute_attacks(int from, int attacker, int victim,
                               BitBoard attacks, IntSetSorted *indexes,
                               int mirror, int offset) {
    while (attacks) {
        int to = bitboard_find_set_bit(attacks) ^ mirror;
        attacks &= attacks - 1;

        int index = am_index(attacker, victim, from ^ mirror, to, offset);

        /* The set capacity is a hard bound on the number of attack
         * features per side; reaching it means a real position has
         * more attacks than the network can represent.  Fail loudly
         * instead of silently truncating the map (and with it the
         * accumulator contribution).
         */
        if (intset_sorted_add(indexes, index) == INTSET_SORTED_FULL) {
            print(0,
                  "Attack map overflow: more than %d attack "
                  "features in one position.\n",
                  INTSET_SORTED_MAX);
            exit(1);
        }
    }
}

static void am_compute_all_attacks_of_side(const Position *p, int attacker,
                                           int defender, IntSetSorted *indexes,
                                           int mirror, int offset) {

    // Pawn attacks - include all pawn attacks to pieces
    BitBoard attackers = p->mask[attacker][Pawn];
    while (attackers) {
        int from = bitboard_find_set_bit(attackers);
        attackers &= attackers - 1;

        am_compute_attacks(from, Pawn, Knight,
                           p->atkTo[from] & p->mask[defender][Knight], indexes,
                           mirror, offset);
        am_compute_attacks(from, Pawn, Bishop,
                           p->atkTo[from] & p->mask[defender][Bishop], indexes,
                           mirror, offset);
        am_compute_attacks(from, Pawn, Rook,
                           p->atkTo[from] & p->mask[defender][Rook], indexes,
                           mirror, offset);
        am_compute_attacks(from, Pawn, Queen,
                           p->atkTo[from] & p->mask[defender][Queen], indexes,
                           mirror, offset);
    }

    // Knight attacks - include all knight attacks to pieces and pawns
    attackers = p->mask[attacker][Knight];
    while (attackers) {
        int from = bitboard_find_set_bit(attackers);
        attackers &= attackers - 1;

        am_compute_attacks(from, Knight, Pawn,
                           p->atkTo[from] & p->mask[defender][Pawn], indexes,
                           mirror, offset);
        am_compute_attacks(from, Knight, Knight,
                           p->atkTo[from] & p->mask[defender][Knight], indexes,
                           mirror, offset);
        am_compute_attacks(from, Knight, Bishop,
                           p->atkTo[from] & p->mask[defender][Bishop], indexes,
                           mirror, offset);
        am_compute_attacks(from, Knight, Rook,
                           p->atkTo[from] & p->mask[defender][Rook], indexes,
                           mirror, offset);
        am_compute_attacks(from, Knight, Queen,
                           p->atkTo[from] & p->mask[defender][Queen], indexes,
                           mirror, offset);
    }

    // Bishop attacks - include all bishop attacks to pieces and pawns, exclude
    // attacks on pawns that already attack the bishop
    attackers = p->mask[attacker][Bishop];
    while (attackers) {
        int from = bitboard_find_set_bit(attackers);
        attackers &= attackers - 1;

        // exclude attacks on pawns that already attack the bishop
        BitBoard excl = ~PawnEPM[!defender][from];
        am_compute_attacks(from, Bishop, Pawn,
                           p->atkTo[from] & excl & p->mask[defender][Pawn],
                           indexes, mirror, offset);
        am_compute_attacks(from, Bishop, Knight,
                           p->atkTo[from] & p->mask[defender][Knight], indexes,
                           mirror, offset);
        am_compute_attacks(from, Bishop, Bishop,
                           p->atkTo[from] & p->mask[defender][Bishop], indexes,
                           mirror, offset);
        am_compute_attacks(from, Bishop, Rook,
                           p->atkTo[from] & p->mask[defender][Rook], indexes,
                           mirror, offset);
        am_compute_attacks(from, Bishop, Queen,
                           p->atkTo[from] & p->mask[defender][Queen], indexes,
                           mirror, offset);
    }

    // Rook attacks - include all rook attacks to pieces and pawns
    attackers = p->mask[attacker][Rook];
    while (attackers) {
        int from = bitboard_find_set_bit(attackers);
        attackers &= attackers - 1;

        am_compute_attacks(from, Rook, Pawn,
                           p->atkTo[from] & p->mask[defender][Pawn], indexes,
                           mirror, offset);
        am_compute_attacks(from, Rook, Knight,
                           p->atkTo[from] & p->mask[defender][Knight], indexes,
                           mirror, offset);
        am_compute_attacks(from, Rook, Bishop,
                           p->atkTo[from] & p->mask[defender][Bishop], indexes,
                           mirror, offset);
        am_compute_attacks(from, Rook, Rook,
                           p->atkTo[from] & p->mask[defender][Rook], indexes,
                           mirror, offset);
        am_compute_attacks(from, Rook, Queen,
                           p->atkTo[from] & p->mask[defender][Queen], indexes,
                           mirror, offset);
    }

    // Queen attacks - include all queen attacks to pieces and pawns, exclude
    // those where the pawn, bishop or rook already attack the queen
    attackers = p->mask[attacker][Queen];
    while (attackers) {
        int from = bitboard_find_set_bit(attackers);
        attackers &= attackers - 1;

        // exclude attacks on pawns that already attack the queen
        BitBoard excl = ~PawnEPM[!defender][from];
        am_compute_attacks(from, Queen, Pawn,
                           p->atkTo[from] & excl & p->mask[defender][Pawn],
                           indexes, mirror, offset);
        am_compute_attacks(from, Queen, Knight,
                           p->atkTo[from] & p->mask[defender][Knight], indexes,
                           mirror, offset);
        // Only include queen attacks on bishops via rook moves
        am_compute_attacks(from, Queen, Bishop,
                           p->atkTo[from] & RookEPM[from] &
                               p->mask[defender][Bishop],
                           indexes, mirror, offset);
        // Only include queen attacks on rooks via bishop moves
        am_compute_attacks(from, Queen, Rook,
                           p->atkTo[from] & BishopEPM[from] &
                               p->mask[defender][Rook],
                           indexes, mirror, offset);
        am_compute_attacks(from, Queen, Queen,
                           p->atkTo[from] & p->mask[defender][Queen], indexes,
                           mirror, offset);
    }

    int from = bitboard_find_set_bit(p->mask[attacker][King]);

    am_compute_attacks(from, King, Pawn,
                       p->atkTo[from] & p->mask[defender][Pawn], indexes,
                       mirror, offset);
    am_compute_attacks(from, King, Knight,
                       p->atkTo[from] & p->mask[defender][Knight], indexes,
                       mirror, offset);
    // Only include king attacks on bishops via rook moves
    am_compute_attacks(from, King, Bishop,
                       p->atkTo[from] & RookEPM[from] &
                           p->mask[defender][Bishop],
                       indexes, mirror, offset);
    // Only include king attacks on rooks via bishop moves
    am_compute_attacks(from, King, Rook,
                       p->atkTo[from] & BishopEPM[from] &
                           p->mask[defender][Rook],
                       indexes, mirror, offset);
}

void am_compute_attack_maps(const Position *p, IntSetSorted *indexes_w,
                            IntSetSorted *indexes_b) {
    am_compute_all_attacks_of_side(p, White, White, indexes_w,
                                   am_side_mirror(p, White), OFFSET_DEFENDER);
    am_compute_all_attacks_of_side(p, White, Black, indexes_b,
                                   am_side_mirror(p, Black), 0);
    am_compute_all_attacks_of_side(p, Black, White, indexes_w,
                                   am_side_mirror(p, White), 0);
    am_compute_all_attacks_of_side(p, Black, Black, indexes_b,
                                   am_side_mirror(p, Black), OFFSET_DEFENDER);
}

/*
 * Compute the attack feature indices of the given side (own pieces
 * attacking opponent pieces/pawns) into out.  Returns the count, or -1
 * if it would exceed cap.  Shared with nn_extract.c.
 */
int am_compute_attack_indices(const Position *p, int side, int *out, int cap) {
    IntSetSorted indexes;

    intset_sorted_init(&indexes);
    am_compute_all_attacks_of_side(p, side, side, &indexes,
                                   am_side_mirror(p, side), OFFSET_DEFENDER);
    am_compute_all_attacks_of_side(p, !side, side, &indexes,
                                   am_side_mirror(p, side), 0);

    if ((int)indexes.count > cap) {
        return -1;
    }
    for (size_t i = 0; i < indexes.count; i++) {
        out[i] = indexes.data[i];
    }
    return (int)indexes.count;
}

#define RIGHT_ARROW_UTF8 "\xe2\x86\x92"

/*
 * For the given index, return a textual representation in the given position,
 * e.g. "Ne5->f7".
 */
void am_attack_index_to_string(const Position *p, int side, int index,
                               char *buffer) {
    int mirror = am_side_mirror(p, side);
    int to = (index & 63) ^ mirror;
    int from = ((index >> 6) & 63) ^ mirror;
    int plane = index / 4096;

    const char *symbol = "x";

    if (plane >= OFFSET_DEFENDER) {
        plane -= OFFSET_DEFENDER;
        symbol = RIGHT_ARROW_UTF8;
    }

    int attacker = (plane + 1) / 5 + 1;

    snprintf(buffer, 9, "%c%c%c%s%c%c", PieceName[attacker], SQUARE(from),
             symbol, SQUARE(to));
}
