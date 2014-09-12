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
 * recog.c - interior node recognizers
 *
 * $Id: recog.c 27 2003-02-11 22:39:17Z thorsten $
 *
 */

/*
 * See Ernst A. Heinz, "Efficient Interior-Node Recognition"
 * ICCA Journal Volume 21, No. 3, pp 156-167
 */

#include "amy.h"

typedef int RECOGNIZER(struct Position *, int *score);

static RECOGNIZER *Recognizers[64];
static int         RecognizerAvailable[32];

static RECOGNIZER RecognizerKK;
static RECOGNIZER RecognizerKBK;
static RECOGNIZER RecognizerKBNK;
static RECOGNIZER RecognizerKNK;
static RECOGNIZER RecognizerKBKP;
static RECOGNIZER RecognizerKNKP;

static void RegisterRecognizer(RECOGNIZER *funct, int white_sig, int black_sig)
{
    Recognizers[CALCULATE_INDEX(white_sig, black_sig)] = funct;

    RecognizerAvailable[white_sig] |= (1 << black_sig);
    RecognizerAvailable[black_sig] |= (1 << white_sig);
}

static int sig(int pawn, int knight, int bishop, int rook, int queen)
{
    return pawn | (knight << 1) | (bishop << 2) | (rook << 3) | (queen << 4);
}

void RecogInit(void)
{
    int i;

    for(i=0; i<64; i++) {
	Recognizers[i] = NULL;
    }

    for(i=0; i<32; i++) {
	RecognizerAvailable[i] = 0;
    }

    /*
     *                                       P  N  B  R  Q       P  N  B  R  Q
     */

    RegisterRecognizer( RecognizerKK   , sig(0, 0, 0, 0, 0), sig(0, 0, 0, 0, 0));
    RegisterRecognizer( RecognizerKBK  , sig(0, 0, 1, 0, 0), sig(0, 0, 0, 0, 0));
    RegisterRecognizer( RecognizerKBNK , sig(0, 1, 1, 0, 0), sig(0, 0, 0, 0, 0));
    RegisterRecognizer( RecognizerKBNK , sig(0, 0, 1, 0, 0), sig(0, 1, 0, 0, 0));
    RegisterRecognizer( RecognizerKNK  , sig(0, 1, 0, 0, 0), sig(0, 0, 0, 0, 0));
    RegisterRecognizer( RecognizerKNK  , sig(0, 0, 0, 0, 0), sig(0, 1, 0, 0, 0));
    RegisterRecognizer( RecognizerKNK  , sig(0, 1, 0, 0, 0), sig(0, 1, 0, 0, 0));
    RegisterRecognizer( RecognizerKBKP , sig(0, 0, 1, 0, 0), sig(1, 0, 0, 0, 0));
    RegisterRecognizer( RecognizerKBKP , sig(1, 0, 1, 0, 0), sig(0, 0, 0, 0, 0));
    RegisterRecognizer( RecognizerKBKP , sig(1, 0, 1, 0, 0), sig(1, 0, 0, 0, 0));
    RegisterRecognizer( RecognizerKNKP , sig(0, 1, 0, 0, 0), sig(1, 0, 0, 0, 0));
}

int ProbeRecognizer(struct Position *p, int *score)
{
    RECOGNIZER *rec = Recognizers[RECOGNIZER_INDEX(p)];
    if( rec != NULL ) {
	if(RecognizerAvailable[p->material_signature[White]] &
	     (1 << p->material_signature[Black])) {
	    return rec(p, score);
	}
    }

    return Useless;
}

static int RecognizerKK(struct Position *p, int *score)
{
    *score = 0;

    return ExactScore;
}

static int RecognizerKBK(struct Position *p, int *score)
{
    BitBoard pcs;
    int color = White;

    if(p->material_signature[Black]) {
	color = Black;
    }

    pcs = p->mask[color][Bishop];

    /*
     * drawn if there is only one bishop
     */

    if(CountBits(pcs) < 2) {
	*score = 0;
	return ExactScore;
    }

    /*
     * drawn if the bishops are all of the same color
     */

    if(!((pcs & WhiteSquaresMask) && (pcs & BlackSquaresMask))) {
	*score = 0;
	return ExactScore;
    }

    /*
     * do not recognize when losers king attacks a piece
     */

    if(p->atkTo[p->kingSq[OPP(color)]] & p->mask[color][0]) {
	return Useless;
    }

    /*
     * do not recognize when losers king is on the and the winners king
     * is close enough to stalemate
     */

    if( p->turn != color &&
	(p->mask[OPP(color)][King] & EdgeMask) &&
	(KingDist(p->kingSq[White], p->kingSq[Black]) == 2) ) {
	return Useless;
    }

    /*
     * This is a win. Calculate a score which guarantuess progress.
     */

    *score = p->material[color] + 2 * Value[Pawn]
           - 250 * EdgeDist(p->kingSq[OPP(color)])
           - 125 * KingDist(p->kingSq[White], p->kingSq[Black]);

    if(p->turn != color) {
	*score = -*score;
	return UpperBound;
    }


    return LowerBound;
}

static int KBNKTab[] = {
    500, 450, 425, 400, 375, 350, 325, 300,
    450, 300, 300, 300, 300, 300, 300, 325,
    425, 300, 100, 100, 100, 100, 300, 350,
    400, 300, 100,   0,   0, 100, 300, 375,
    375, 300, 100,   0,   0, 100, 300, 400,
    350, 300, 100, 100, 100, 100, 300, 425,
    325, 300, 300, 300, 300, 300, 300, 450,
    300, 325, 350, 375, 400, 425, 450, 500
};

static int RecognizerKBNK(struct Position *p, int *score)
{
    if(p->material_signature[White] && p->material_signature[Black]) {

	/*
	 * This is knkb
	 */

	if(CountBits(p->mask[White][0] | p->mask[Black][0]) > 4) {
	    return Useless;
	}

	if(EdgeMask & (p->mask[White][King] | p->mask[Black][King])) {
	    return Useless;
	}

	*score = 0;
	return ExactScore;
    } else {

	/*
	 * This is kbnk
	 */

	BitBoard atkd = 0;
	int color = White;
	int sqx = 0;

	if(p->material_signature[Black]) {
	    color = Black;
	}

	/*
	 * do not recognize when losers king attacks a piece
	 */

	atkd = p->atkTo[p->kingSq[OPP(color)]] & p->mask[color][0];
	if(atkd) {
	    if(p->turn != color || CountBits(atkd) > 1) {
		return Useless;
	    }
	}

	/*
	 * do not recognize when losers king is on the and the winners king
	 * is close enough to stalemate
	 */

	if( p->turn != color &&
	    (p->mask[OPP(color)][King] & EdgeMask) &&
	    (KingDist(p->kingSq[White], p->kingSq[Black]) == 2) ) {
	    return Useless;
	}

	/*
	 * This is a win. Calculate a score which guarantuess progress.
	 */

	if(p->mask[color][Bishop] & BlackSquaresMask) {
            sqx = KBNKTab[p->kingSq[OPP(color)]];
	}

	if(p->mask[color][Bishop] & WhiteSquaresMask) {
            sqx = KBNKTab[7 ^ p->kingSq[OPP(color)]];
	}

	*score = p->material[color] + 3 * Value[Pawn]
	       + sqx
	       - 125 * KingDist(p->kingSq[White], p->kingSq[Black]);

	if(p->turn != color) {
	    *score = -*score;
	    return UpperBound;
	}


	return LowerBound;
    }
}

static int RecognizerKNK(struct Position *p, int *score)
{
    if(p->material_signature[White] && p->material_signature[Black]) {
	return Useless;
    } else {
	int cnt;

	if(p->material_signature[White]) {
	    cnt = CountBits(p->mask[White][Knight]);
	} else {
	    cnt = CountBits(p->mask[Black][Knight]);
	}

	if(cnt < 3) {
	    *score = 0;
	    return ExactScore;
	}

	return Useless;
    }
}

static int RecognizerKBKP(struct Position *p, int *score)
{
    if(p->material_signature[White] && p->material_signature[Black]) {

        /*
         * This is KBKP or KBPKP
         */

        int color = White;

        if(p->material_signature[Black] & SIGNATURE_BIT(Bishop)) {
            color = Black;
        }

        if(p->material_signature[color] & SIGNATURE_BIT(Pawn)) {
            if(color == White) {
                if( !(p->mask[White][Pawn] & NotAFileMask)
                    && !(p->mask[White][Bishop] & WhiteSquaresMask)
                    && (p->mask[Black][King] & CornerMaskA8) ) {
                    *score = 0;
                    return (p->turn == White) ? UpperBound : LowerBound;
                }
                if( !(p->mask[White][Pawn] & NotHFileMask)
                    && !(p->mask[White][Bishop] & BlackSquaresMask)
                    && (p->mask[Black][King] & CornerMaskH8) ) {
                    *score = 0;
                    return (p->turn == White) ? UpperBound : LowerBound;
                }
            } else {
                if( !(p->mask[Black][Pawn] & NotAFileMask)
                    && !(p->mask[Black][Bishop] & BlackSquaresMask)
                    && (p->mask[White][King] & CornerMaskA1) ) {
                    *score = 0;
                    return (p->turn == Black) ? UpperBound : LowerBound;
                }
                if( !(p->mask[Black][Pawn] & NotHFileMask)
                    && !(p->mask[Black][Bishop] & WhiteSquaresMask)
                    && (p->mask[White][King] & CornerMaskH1) ) {
                    *score = 0;
                    return (p->turn == Black) ? UpperBound : LowerBound;
                }
            }

            return Useless;
        } else {
            if(CountBits(p->mask[color][Bishop]) > 1
                || p->mask[OPP(color)][King] & EdgeMask) {
                return Useless;
            }

            *score = 0;

            if(color == p->turn) {
                return UpperBound;
            } else {
                return LowerBound;
            }
        }
    } else {

        /*
         * This is KBPK
         *
         * Check for draws because of wrongly colored bishop
         */

        if(p->material_signature[White]) {
            if( !(p->mask[White][Pawn] & NotAFileMask)
                && !(p->mask[White][Bishop] & WhiteSquaresMask)
                && (p->mask[Black][King] & CornerMaskA8) ) {
                *score = 0;
                return ExactScore;
            }
            if( !(p->mask[White][Pawn] & NotHFileMask)
                && !(p->mask[White][Bishop] & BlackSquaresMask)
                && (p->mask[Black][King] & CornerMaskH8) ) {
                *score = 0;
                return ExactScore;
            }
        } else {
            if( !(p->mask[Black][Pawn] & NotAFileMask)
                && !(p->mask[Black][Bishop] & BlackSquaresMask)
                && (p->mask[White][King] & CornerMaskA1) ) {
                *score = 0;
                return ExactScore;
            }
            if( !(p->mask[Black][Pawn] & NotHFileMask)
                && !(p->mask[Black][Bishop] & WhiteSquaresMask)
                && (p->mask[White][King] & CornerMaskH1) ) {
                *score = 0;
                return ExactScore;
            }
        }

        return Useless;
    }
}

static int RecognizerKNKP(struct Position *p, int *score)
{
    int color = White;

    if(p->material_signature[Black] & SIGNATURE_BIT(Knight)) {
        color = Black;
    }

    if(CountBits(p->mask[color][Knight]) > 1
            || p->mask[OPP(color)][King] & EdgeMask) {
        return Useless;
    }

    *score = 0;

    if(color == p->turn) {
        return UpperBound;
    } else {
        return LowerBound;
    }
}
