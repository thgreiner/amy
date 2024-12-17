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
 * magic.c - precalculates the tables needed for magic bitboards.
 */

#include "amy.h"
#include "assert.h"

uint16_t rook_table_offsets[64];
uint64_t rook_table[102400];
uint16_t bishop_table_offsets[64];
uint64_t bishop_table[5248];

const uint64_t rook_blocker_mask[] = {
    0x101010101017e,    0x202020202027c,    0x404040404047a,
    0x8080808080876,    0x1010101010106e,   0x2020202020205e,
    0x4040404040403e,   0x8080808080807e,   0x1010101017e00,
    0x2020202027c00,    0x4040404047a00,    0x8080808087600,
    0x10101010106e00,   0x20202020205e00,   0x40404040403e00,
    0x80808080807e00,   0x10101017e0100,    0x20202027c0200,
    0x40404047a0400,    0x8080808760800,    0x101010106e1000,
    0x202020205e2000,   0x404040403e4000,   0x808080807e8000,
    0x101017e010100,    0x202027c020200,    0x404047a040400,
    0x8080876080800,    0x1010106e101000,   0x2020205e202000,
    0x4040403e404000,   0x8080807e808000,   0x1017e01010100,
    0x2027c02020200,    0x4047a04040400,    0x8087608080800,
    0x10106e10101000,   0x20205e20202000,   0x40403e40404000,
    0x80807e80808000,   0x17e0101010100,    0x27c0202020200,
    0x47a0404040400,    0x8760808080800,    0x106e1010101000,
    0x205e2020202000,   0x403e4040404000,   0x807e8080808000,
    0x7e010101010100,   0x7c020202020200,   0x7a040404040400,
    0x76080808080800,   0x6e101010101000,   0x5e202020202000,
    0x3e404040404000,   0x7e808080808000,   0x7e01010101010100,
    0x7c02020202020200, 0x7a04040404040400, 0x7608080808080800,
    0x6e10101010101000, 0x5e20202020202000, 0x3e40404040404000,
    0x7e80808080808000,
};
const uint64_t bishop_blocker_mask[] = {
    0x40201008040200, 0x402010080400,   0x4020100a00,     0x40221400,
    0x2442800,        0x204085000,      0x20408102000,    0x2040810204000,
    0x20100804020000, 0x40201008040000, 0x4020100a0000,   0x4022140000,
    0x244280000,      0x20408500000,    0x2040810200000,  0x4081020400000,
    0x10080402000200, 0x20100804000400, 0x4020100a000a00, 0x402214001400,
    0x24428002800,    0x2040850005000,  0x4081020002000,  0x8102040004000,
    0x8040200020400,  0x10080400040800, 0x20100a000a1000, 0x40221400142200,
    0x2442800284400,  0x4085000500800,  0x8102000201000,  0x10204000402000,
    0x4020002040800,  0x8040004081000,  0x100a000a102000, 0x22140014224000,
    0x44280028440200, 0x8500050080400,  0x10200020100800, 0x20400040201000,
    0x2000204081000,  0x4000408102000,  0xa000a10204000,  0x14001422400000,
    0x28002844020000, 0x50005008040200, 0x20002010080400, 0x40004020100800,
    0x20408102000,    0x40810204000,    0xa1020400000,    0x142240000000,
    0x284402000000,   0x500804020000,   0x201008040200,   0x402010080400,
    0x2040810204000,  0x4081020400000,  0xa102040000000,  0x14224000000000,
    0x28440200000000, 0x50080402000000, 0x20100804020000, 0x40201008040200,
};

const uint8_t rook_index_bits[64] = {
    12, 11, 11, 11, 11, 11, 11, 12, 11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11, 11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11, 11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11, 12, 11, 11, 11, 11, 11, 11, 12};

const uint8_t bishop_index_bits[64] = {
    6, 5, 5, 5, 5, 5, 5, 6, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 7, 7, 7, 7,
    5, 5, 5, 5, 7, 9, 9, 7, 5, 5, 5, 5, 7, 9, 9, 7, 5, 5, 5, 5, 7, 7,
    7, 7, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 5, 5, 5, 5, 5, 5, 6};

const uint64_t rook_magics[64] = {
    0xa8002c000108020ULL,  0x6c00049b0002001ULL,  0x100200010090040ULL,
    0x2480041000800801ULL, 0x280028004000800ULL,  0x900410008040022ULL,
    0x280020001001080ULL,  0x2880002041000080ULL, 0xa000800080400034ULL,
    0x4808020004000ULL,    0x2290802004801000ULL, 0x411000d00100020ULL,
    0x402800800040080ULL,  0xb000401004208ULL,    0x2409000100040200ULL,
    0x1002100004082ULL,    0x22878001e24000ULL,   0x1090810021004010ULL,
    0x801030040200012ULL,  0x500808008001000ULL,  0xa08018014000880ULL,
    0x8000808004000200ULL, 0x201008080010200ULL,  0x801020000441091ULL,
    0x800080204005ULL,     0x1040200040100048ULL, 0x120200402082ULL,
    0xd14880480100080ULL,  0x12040280080080ULL,   0x100040080020080ULL,
    0x9020010080800200ULL, 0x813241200148449ULL,  0x491604001800080ULL,
    0x100401000402001ULL,  0x4820010021001040ULL, 0x400402202000812ULL,
    0x209009005000802ULL,  0x810800601800400ULL,  0x4301083214000150ULL,
    0x204026458e001401ULL, 0x40204000808000ULL,   0x8001008040010020ULL,
    0x8410820820420010ULL, 0x1003001000090020ULL, 0x804040008008080ULL,
    0x12000810020004ULL,   0x1000100200040208ULL, 0x430000a044020001ULL,
    0x280009023410300ULL,  0xe0100040002240ULL,   0x200100401700ULL,
    0x2244100408008080ULL, 0x8000400801980ULL,    0x2000810040200ULL,
    0x8010100228810400ULL, 0x2000009044210200ULL, 0x4080008040102101ULL,
    0x40002080411d01ULL,   0x2005524060000901ULL, 0x502001008400422ULL,
    0x489a000810200402ULL, 0x1004400080a13ULL,    0x4000011008020084ULL,
    0x26002114058042ULL,
};

const uint64_t bishop_magics[64] = {
    0x89a1121896040240ULL, 0x2004844802002010ULL, 0x2068080051921000ULL,
    0x62880a0220200808ULL, 0x4042004000000ULL,    0x100822020200011ULL,
    0xc00444222012000aULL, 0x28808801216001ULL,   0x400492088408100ULL,
    0x201c401040c0084ULL,  0x840800910a0010ULL,   0x82080240060ULL,
    0x2000840504006000ULL, 0x30010c4108405004ULL, 0x1008005410080802ULL,
    0x8144042209100900ULL, 0x208081020014400ULL,  0x4800201208ca00ULL,
    0xf18140408012008ULL,  0x1004002802102001ULL, 0x841000820080811ULL,
    0x40200200a42008ULL,   0x800054042000ULL,     0x88010400410c9000ULL,
    0x520040470104290ULL,  0x1004040051500081ULL, 0x2002081833080021ULL,
    0x400c00c010142ULL,    0x941408200c002000ULL, 0x658810000806011ULL,
    0x188071040440a00ULL,  0x4800404002011c00ULL, 0x104442040404200ULL,
    0x511080202091021ULL,  0x4022401120400ULL,    0x80c0040400080120ULL,
    0x8040010040820802ULL, 0x480810700020090ULL,  0x102008e00040242ULL,
    0x809005202050100ULL,  0x8002024220104080ULL, 0x431008804142000ULL,
    0x19001802081400ULL,   0x200014208040080ULL,  0x3308082008200100ULL,
    0x41010500040c020ULL,  0x4012020c04210308ULL, 0x208220a202004080ULL,
    0x111040120082000ULL,  0x6803040141280a00ULL, 0x2101004202410000ULL,
    0x8200000041108022ULL, 0x21082088000ULL,      0x2410204010040ULL,
    0x40100400809000ULL,   0x822088220820214ULL,  0x40808090012004ULL,
    0x910224040218c9ULL,   0x402814422015008ULL,  0x90014004842410ULL,
    0x1000042304105ULL,    0x10008830412a00ULL,   0x2520081090008908ULL,
    0x40102000a0a60140ULL,
};

static int poplsb(BitBoard *x) {
    int lsb = FindSetBit(*x);
    *x &= *x - 1;
    return lsb;
}

static uint64_t rook_attack_mask(int sq, uint64_t blockers) {
    uint64_t x = 0;

    uint64_t m = ShiftLeft(SetMask(sq));
    while (m) {
        x |= m;
        m &= ~blockers;
        m = ShiftLeft(m);
    }

    m = ShiftRight(SetMask(sq));
    while (m) {
        x |= m;
        m &= ~blockers;
        m = ShiftRight(m);
    }

    m = ShiftUp(SetMask(sq));
    while (m) {
        x |= m;
        m &= ~blockers;
        m = ShiftUp(m);
    }

    m = ShiftDown(SetMask(sq));
    while (m) {
        x |= m;
        m &= ~blockers;
        m = ShiftDown(m);
    }

    return x;
}

static uint64_t bishop_attack_mask(int sq, uint64_t blockers) {
    uint64_t x = 0;

    uint64_t m = ShiftUp(ShiftLeft(SetMask(sq)));
    while (m) {
        x |= m;
        m &= ~blockers;
        m = ShiftUp(ShiftLeft(m));
    }

    m = ShiftUp(ShiftRight(SetMask(sq)));
    while (m) {
        x |= m;
        m &= ~blockers;
        m = ShiftUp(ShiftRight(m));
    }

    m = ShiftDown(ShiftLeft(SetMask(sq)));
    while (m) {
        x |= m;
        m &= ~blockers;
        m = ShiftDown(ShiftLeft(m));
    }

    m = ShiftDown(ShiftRight(SetMask(sq)));
    while (m) {
        x |= m;
        m &= ~blockers;
        m = ShiftDown(ShiftRight(m));
    }

    return x;
}

static BitBoard blockers_from_index(int index, BitBoard mask) {
    uint64_t blockers = 0;
    int bits = __builtin_popcountll(mask);
    for (int i = 0; i < bits; i++) {
        int bit = poplsb(&mask);
        if (index & (1 << i)) {
            SetBit(blockers, bit);
        }
    }
    return blockers;
}

static void init_rook_table(void) {
    int offset = 0;

    for (int sq = 0; sq < 64; sq++) {
        int bits = rook_index_bits[sq];

        rook_table_offsets[sq] = offset >> 1;

        for (int index = 0; index < (1 << bits); index++) {
            uint64_t blockers =
                blockers_from_index(index, rook_blocker_mask[sq]);
            int magic_index =
                (blockers * rook_magics[sq]) >> (64 - rook_index_bits[sq]);
            rook_table[offset + magic_index] = rook_attack_mask(sq, blockers);
        }

        offset += 1 << bits;
    }

    assert(offset == 102400);
}

static void init_bishop_table(void) {
    int offset = 0;

    for (int sq = 0; sq < 64; sq++) {
        int bits = bishop_index_bits[sq];

        bishop_table_offsets[sq] = offset;

        for (int index = 0; index < (1 << bits); index++) {
            uint64_t blockers =
                blockers_from_index(index, bishop_blocker_mask[sq]);
            int magic_index =
                (blockers * bishop_magics[sq]) >> (64 - bishop_index_bits[sq]);
            bishop_table[offset + magic_index] =
                bishop_attack_mask(sq, blockers);
        }

        offset += 1 << bits;
    }

    assert(offset == 5248);
}

/**
 * Initializes the tables needed for attack generation with magic bitboards.
 */
void InitMagic(void) {
    init_rook_table();
    init_bishop_table();
}
