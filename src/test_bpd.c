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

#include "bpd.h"
#include "inline.h"

#include <assert.h>
#include <string.h>

/*
 * Return the nibble of square sq in the binary position bp.
 */
static uint8_t bpd_nibble(const BinaryPosition *bp, int sq) {
    return (uint8_t)((bp->pos[sq >> 4] >> (4 * (sq & 15))) & 15);
}

/*
 * Compare all position fields of a and b. The book counters
 * (outOfBookCnt) are deliberately not compared: dbase_create_
 * position_from_epd() sets them to 3 while bpd_decode() leaves
 * them at 0.
 */
static void test_compare(const struct Position *a, const struct Position *b) {
    int i;
    int j;

    for (i = 0; i < 64; i++) {
        assert(a->piece[i] == b->piece[i]);
        assert(a->atkTo[i] == b->atkTo[i]);
        assert(a->atkFr[i] == b->atkFr[i]);
    }
    for (i = 0; i < 2; i++) {
        for (j = 0; j < 7; j++) {
            assert(a->mask[i][j] == b->mask[i][j]);
        }
        assert(a->material[i] == b->material[i]);
        assert(a->nonPawn[i] == b->nonPawn[i]);
        assert(a->material_signature[i] == b->material_signature[i]);
        assert(a->kingSq[i] == b->kingSq[i]);
    }
    assert(a->slidingPieces == b->slidingPieces);
    assert(a->hkey == b->hkey);
    assert(a->pkey == b->pkey);
    assert(a->castle == b->castle);
    assert(a->enPassant == b->enPassant);
    assert(a->turn == b->turn);
    assert(a->ply == b->ply);
}

/*
 * Encode the position, decode it again and verify that the result
 * is identical to the original.
 */
static void test_roundtrip(char *epd, int score) {
    struct Position *p;
    struct Position *q;
    BinaryPosition bp;

    p = dbase_create_position_from_epd(epd);
    bpd_encode(p, score, &bp);
    q = bpd_decode(&bp);

    assert(q != NULL);
    assert(bp.score == score);
    assert(bp.turn == p->turn);
    test_compare(p, q);

    dbase_free_position(p);
    dbase_free_position(q);
}

static void test_roundtrip_initial(void) {
    test_roundtrip("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -", 0);
}

/* White to move, white pawn on f5 can capture the pawn on e5 en
 * passant (fxe6). */
static void test_roundtrip_enpassant_white(void) {
    test_roundtrip("rnbqkbnr/pppp1ppp/8/4pP2/8/8/PPPPP1PP/RNBQKBNR w KQkq e6",
                   1000);
}

/* Black to move, black pawn on e4 can capture the pawn on d4 en
 * passant (exd3). */
static void test_roundtrip_enpassant_black(void) {
    test_roundtrip("rnbqkbnr/pppp1ppp/8/8/3Pp3/8/PPP1PPPP/RNBQKBNR b KQkq d3",
                   -1000);
}

/* Partial castling rights: white can only castle king side, black
 * only queen side. */
static void test_roundtrip_partial_castling(void) {
    test_roundtrip("r3k2r/8/8/8/8/8/8/R3K2R b Kq -", 42);
}

/* Rook on a1 without castling right. */
static void test_roundtrip_rook_no_castling(void) {
    test_roundtrip("4k3/8/8/8/8/8/8/R3K3 w - -", 0);
}

/* En passant flag taken over from the EPD although no en passant
 * capture is possible at all. */
static void test_roundtrip_stale_enpassant(void) {
    test_roundtrip("r1bqkbnr/pppp1ppp/8/4p3/4P3/8/PPPP1PPP/RNBQKBNR w KQkq e6",
                   0);
}

/* Verify that the castling rook nibbles are used as intended. */
static void test_encode_castling_nibbles(void) {
    struct Position *p = dbase_create_position_from_epd(
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -");
    BinaryPosition bp;

    bpd_encode(p, 0, &bp);

    assert(bpd_nibble(&bp, a1) == PN_W_ROOK_CASTLE);
    assert(bpd_nibble(&bp, h1) == PN_W_ROOK_CASTLE);
    assert(bpd_nibble(&bp, a8) == PN_B_ROOK_CASTLE);
    assert(bpd_nibble(&bp, h8) == PN_B_ROOK_CASTLE);
    assert(bp.turn == White);

    dbase_free_position(p);
}

/* Verify that rooks without castling rights use the plain rook
 * nibbles. */
static void test_encode_partial_castling_nibbles(void) {
    struct Position *p =
        dbase_create_position_from_epd("r3k2r/8/8/8/8/8/8/R3K2R b Kq -");
    BinaryPosition bp;

    bpd_encode(p, 0, &bp);

    assert(bpd_nibble(&bp, a1) == PN_W_ROOK);
    assert(bpd_nibble(&bp, h1) == PN_W_ROOK_CASTLE);
    assert(bpd_nibble(&bp, a8) == PN_B_ROOK_CASTLE);
    assert(bpd_nibble(&bp, h8) == PN_B_ROOK);
    assert(bp.turn == Black);

    dbase_free_position(p);
}

/* Verify that the en passant target square is encoded as
 * PN_EN_PASSANT. */
static void test_encode_enpassant_nibble(void) {
    struct Position *p = dbase_create_position_from_epd(
        "rnbqkbnr/pppp1ppp/8/4pP2/8/8/PPPPP1PP/RNBQKBNR w KQkq e6");
    BinaryPosition bp;

    bpd_encode(p, 0, &bp);

    assert(bpd_nibble(&bp, e6) == PN_EN_PASSANT);
    assert(bpd_nibble(&bp, e5) == PN_B_PAWN);
    assert(bpd_nibble(&bp, f5) == PN_W_PAWN);
    assert(bp.turn == White);

    dbase_free_position(p);
}

/* A position without a black king must be rejected. */
static void test_decode_rejects_missing_king(void) {
    BinaryPosition bp;

    memset(&bp, 0, sizeof(bp));
    bp.pos[0] = PN_W_KING;                 /* king on a1 */
    bp.pos[0] |= (uint64_t)PN_W_KING << 4; /* king on b1 */
    bp.turn = White;

    assert(bpd_decode(&bp) == NULL);
}

void test_all_bpd(void) {
    test_roundtrip_initial();
    test_roundtrip_enpassant_white();
    test_roundtrip_enpassant_black();
    test_roundtrip_partial_castling();
    test_roundtrip_rook_no_castling();
    test_roundtrip_stale_enpassant();
    test_encode_castling_nibbles();
    test_encode_partial_castling_nibbles();
    test_encode_enpassant_nibble();
    test_decode_rejects_missing_king();
}
