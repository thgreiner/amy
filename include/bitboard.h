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

#ifndef BITBOARD_H
#define BITBOARD_H

#include <stdint.h>

#define bitboard_set_mask(i) (1ULL << (i))
#define bitboard_clr_mask(i) (~bitboard_set_mask(i))

#define bitboard_set_bit(b, i) ((b) |= bitboard_set_mask(i))
#define bitboard_clr_bit(b, i) ((b) &= bitboard_clr_mask(i))
#define bitboard_tst_bit(b, i) ((b) & bitboard_set_mask(i))

typedef uint64_t BitBoard;

#if HAVE___BUILTIN_POPCOUNTLL
#define bitboard_count_bits(x) __builtin_popcountll(x)
#else
int bitboard_count_bits(BitBoard);
#endif

#if HAVE___BUILTIN_CTZLL
#define bitboard_find_set_bit(x) __builtin_ctzll(x)
#else
int bitboard_find_set_bit(BitBoard);
#endif

void bitboard_print(BitBoard);

#endif
