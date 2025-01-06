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
 * dbase.c - global database manipulation routines
 */

#include "amy.h"
#include "heap.h"
#include "magic.h"

/*
 * Names of pieces (language dependent)
 */
char PieceName[] = {' ', 'P', 'N', 'B', 'R', 'Q', 'K'};

/*
 * material Values of Pieces
 */

int Value[] = {0,           PAWN_Value, KNIGHT_Value, BISHOP_Value, ROOK_Value,
               QUEEN_Value, 0};

/*
 * Masks for castle rights:
 */

const int8_t CastleMask[2][2] = {
    {0x01, 0x02}, /* White can castle king/queenp->turn */
    {0x04, 0x08}  /* dito for black */
};

/* local prototypes
 */

static void AtkSet(struct Position *, Piece, Color, Square);
static void AtkClr(struct Position *, Square);
static void GainAttack(struct Position *, Square, Square);
static void LooseAttack(struct Position *, Square, Square);
static void GainAttacks(struct Position *, Square);
static void LooseAttacks(struct Position *, Square);

/*
 * Routines to up/downdate the global database
 */

static void ShowMoveList(struct Position *p) {
    int ply;
    for (ply = 0; ply < p->ply; ply++) {
        int move = p->gameLog[ply].gl_Move;
        Print(0, "%s\n", ICS_SAN(move));
    }
}

static void Panic(struct Position *p) {
    ShowPosition(p);
    ShowMoveList(p);
    fflush(stdout);
    abort();
}

#ifdef DEBUG
static void DebugEngine(struct Position *p) {
    int kingSq = p->kingSq[White];
    int i, color;
    BitBoard temp;

    for (i = 0; i < 64; i++) {
        temp = p->atkTo[i];
        while (temp) {
            int sq = FindSetBit(temp);
            temp &= temp - 1;
            if (!TstBit(p->atkFr[sq], i)) {
                Print(0, "AtkFr or AtkTo is bad on %c%c or %c%c\n", SQUARE(i),
                      SQUARE(sq));
                ShowMoveList(p);
                ShowPosition(p);
                abort();
            }
        }
    }

    for (color = 0; color < 2; color++) {
        for (i = Pawn; i <= King; i++) {
            temp = p->mask[color][i];
            while (temp) {
                int sq = FindSetBit(temp);
                temp &= temp - 1;
                int pc = (1 - 2 * color) * i;
                if (p->piece[sq] != pc) {
                    Print(0, "Piece on %c%c is %d, expected %d!\n", SQUARE(sq),
                          p->piece[sq], pc);
                    ShowMoveList(p);
                    ShowPosition(p);
                    abort();
                }
            }
        }
    }

    if (p->atkTo[kingSq] != KingEPM[kingSq]) {
        Print(0, "White king is bad:\n");
        PrintBitBoard(p->atkTo[kingSq]);
        Print(0, "should be:\n");
        PrintBitBoard(KingEPM[kingSq]);
        ShowMoveList(p);
        ShowPosition(p);
        abort();
    }
    kingSq = p->kingSq[Black];
    if (p->atkTo[kingSq] != KingEPM[kingSq]) {
        Print(0, "Black king is bad:\n");
        PrintBitBoard(p->atkTo[kingSq]);
        Print(0, "should be:\n");
        PrintBitBoard(KingEPM[kingSq]);
        ShowMoveList(p);
        ShowPosition(p);
        abort();
    }
}
#endif

/*
 * Generate attacks for a piece "type" of "color" on square "square"
 */

static void AtkSet(struct Position *p, Piece type, Color color, Square square) {
    BitBoard attacks;

    switch (type) {
    case Pawn:
        attacks = PawnEPM[color][square];
        break;
    case Knight:
        attacks = KnightEPM[square];
        break;
    case Bishop:
        attacks = bishop_attacks(square, p->mask[0][0] | p->mask[1][0]);
        break;
    case Rook:
        attacks = rook_attacks(square, p->mask[0][0] | p->mask[1][0]);
        break;
    case Queen:
        attacks = bishop_attacks(square, p->mask[0][0] | p->mask[1][0]) |
                  rook_attacks(square, p->mask[0][0] | p->mask[1][0]);
        break;
    case King:
        attacks = KingEPM[square];
        break;
    default:
        printf("AtkSet(%d, %d, %d)\n", type, color, square);
        Panic(p);
        return; // never reached
    }

    p->atkTo[square] = attacks;
    while (attacks) {
        int i = FindSetBit(attacks);
        attacks &= attacks - 1;
        SetBit(p->atkFr[i], square);
    }
}

static void AtkClr(struct Position *p, Square square) {
    BitBoard tmp = p->atkTo[square];
    p->atkTo[square] = 0;

    while (tmp) {
        int i = FindSetBit(tmp);
        tmp &= tmp - 1;
        ClrBit(p->atkFr[i], square);
    }
}

/*
 * Recalculate Attacks from "from" to "to" after the piece on "to" has
 * been removed
 */

static void GainAttack(struct Position *p, const Square from, const Square to) {
    signed char *nsq = NextSQ[from];
    int sq = to;
    const BitBoard all = p->mask[0][0] | p->mask[1][0];

    for (;;) {
        sq = nsq[sq];
        if (sq < 0)
            break;

        SetBit(p->atkTo[from], sq);
        SetBit(p->atkFr[sq], from);

        if (TstBit(all, sq))
            break;
    }
}

/*
 * Recalculate Attacks from "from" to "to" after a piece has been put
 * onto "to"
 */

static void LooseAttack(struct Position *p, const Square from,
                        const Square to) {
    signed char *nsq = NextSQ[from];
    int sq = to;
    const BitBoard all = p->mask[0][0] | p->mask[1][0];

    for (;;) {
        sq = nsq[sq];
        if (sq < 0)
            break;

        ClrBit(p->atkTo[from], sq);
        ClrBit(p->atkFr[sq], from);

        if (TstBit(all, sq))
            break;
    }
}

/*
 * Recalculate all ray attacks which pass through square "to" after
 * the piece on this square has been removed
 */

static void GainAttacks(struct Position *p, Square to) {
    BitBoard tmp = p->atkFr[to] & p->slidingPieces;
    int i;

    while (tmp) {
        i = FindSetBit(tmp);
        tmp &= tmp - 1;
        GainAttack(p, i, to);
    }
}

/*
 * Recalculate all ray attacks which pass through square "to" after
 * a piece has been put onto this square
 */

static void LooseAttacks(struct Position *p, Square to) {
    BitBoard tmp = p->atkFr[to] & p->slidingPieces;
    int i;

    while (tmp) {
        i = FindSetBit(tmp);
        tmp &= tmp - 1;
        LooseAttack(p, i, to);
    }
}

/*
 * Determines if a piece of type tp is a sliding piece.
 */
static inline bool is_sliding(Piece tp) { return tp >= Bishop && tp <= Queen; }

/*
 * Make a castle move
 * I separated this routine from the normal DoMove routine since it has
 * to move two pieces
 */

static void DoCastle(struct Position *p, move_t move) {
    int from = M_FROM(move);
    int to = M_TO(move);
    int or = (move & M_SCASTLE) ? from + 3 : from - 4;
    int nr = (move & M_SCASTLE) ? from + 1 : from - 1;

    /* king looses its attacks */
    AtkClr(p, from);

    /* rook looses its attacks */
    AtkClr(p, or);

    /* move king on the board */
    p->piece[to] = p->piece[from];
    p->piece[from] = Neutral;
    ClrBit(p->mask[p->turn][0], from);
    ClrBit(p->mask[p->turn][King], from);
    SetBit(p->mask[p->turn][0], to);
    SetBit(p->mask[p->turn][King], to);

    /* move rook on the board */
    p->piece[nr] = p->piece[or];
    p->piece[or] = Neutral;
    ClrBit(p->mask[p->turn][0], or);
    ClrBit(p->mask[p->turn][Rook], or);
    ClrBit(p->slidingPieces, or);
    SetBit(p->mask[p->turn][0], nr);
    SetBit(p->mask[p->turn][Rook], nr);
    SetBit(p->slidingPieces, nr);

    /* re-calculate attacks through king-square
     * no need to do it for the rook, since it was on the edge of the board
     * For the same reason we don't have to LooseAttacks on any of the
     * new king/rook squares
     */

    GainAttacks(p, from);

    /* King and rook gain their attacks
     */

    AtkSet(p, King, p->turn, to);
    AtkSet(p, Rook, p->turn, nr);
    p->kingSq[p->turn] = to;

    /* update hashkey */
    /* Das koennte ich vorher berechnen! Ist dann nur eine Anweisung! */

    p->hkey ^= (HashKeys[p->turn][King][from] ^ HashKeys[p->turn][King][to] ^
                HashKeys[p->turn][Rook][or] ^ HashKeys[p->turn][Rook][nr]);
}

/*
 * Unmake a castle move
 */

static void UndoCastle(struct Position *p, move_t move) {
    int from = M_FROM(move);
    int to = M_TO(move);
    int or = (move & M_SCASTLE) ? from + 3 : from - 4;
    int nr = (move & M_SCASTLE) ? from + 1 : from - 1;

    /* king looses its attacks */
    AtkClr(p, to);

    /* rook looses its attacks */
    AtkClr(p, nr);

    /* re-calculate attacks through king-square
     * no need to do it for the rook, since it was on the edge of the board
     * For the same reason we don't have to LooseAttacks on any of the
     * new king/rook squares
     */
    LooseAttacks(p, from);

    /* move king on the board */
    p->piece[from] = p->piece[to];
    p->piece[to] = Neutral;
    ClrBit(p->mask[p->turn][0], to);
    ClrBit(p->mask[p->turn][King], to);
    SetBit(p->mask[p->turn][0], from);
    SetBit(p->mask[p->turn][King], from);

    /* move rook on the board */
    p->piece[or] = p->piece[nr];
    p->piece[nr] = Neutral;
    ClrBit(p->mask[p->turn][0], nr);
    ClrBit(p->mask[p->turn][Rook], nr);
    ClrBit(p->slidingPieces, nr);
    SetBit(p->mask[p->turn][0], or);
    SetBit(p->mask[p->turn][Rook], or);
    SetBit(p->slidingPieces, or);

    /* King and rook gain their attacks
     */

    AtkSet(p, King, p->turn, from);
    AtkSet(p, Rook, p->turn, or);
    p->kingSq[p->turn] = from;
}

/*
 * Make a move
 * updates the global database
 */

void DoMove(struct Position *p, move_t move) {
    int from = M_FROM(move);
    int to = M_TO(move);
    int tp = TYPE(p->piece[from]);

    /* save EnPassant and Castling */
    p->actLog->gl_EnPassant = p->enPassant;
    p->actLog->gl_Castle = p->castle;
    p->actLog->gl_HashKey = p->hkey;
    p->actLog->gl_PawnKey = p->pkey;

    if (move & M_CANY) {
        DoCastle(p, move);
        p->castle &= ~(CastleMask[p->turn][0] | CastleMask[p->turn][1]);
    } else {
        /* piece looses its attacks */
        AtkClr(p, from);

        if (tp == King) {
            p->kingSq[p->turn] = to;
        }

        /* remove it from the board */
        p->piece[from] = Neutral;
        ClrBit(p->mask[p->turn][0], from);
        ClrBit(p->mask[p->turn][tp], from);
        if (is_sliding(tp))
            ClrBit(p->slidingPieces, from);
        /* re-calculate attacks through from-square */
        GainAttacks(p, from);

        /* update hashkey */
        p->hkey ^= HashKeys[p->turn][tp][from];
        if (tp == Pawn)
            p->pkey ^= HashKeys[p->turn][Pawn][from];

        if (tp == King) {
            /* No more castling rights */
            p->castle &= ~(CastleMask[p->turn][0] | CastleMask[p->turn][1]);
        } else if (tp == Rook) {
            if (from == (p->turn == White ? h1 : h8))
                p->castle &= ~(CastleMask[p->turn][0]);
            if (from == (p->turn == White ? a1 : a8))
                p->castle &= ~(CastleMask[p->turn][1]);
        }
        if (move & M_CAPTURE) {
            int sp = TYPE(p->piece[to]);

            /* piece looses its attacks */
            AtkClr(p, to);

            /* remember type of captured piece */
            p->actLog->gl_Piece = p->piece[to];

            ClrBit(p->mask[OPP(p->turn)][0], to);
            ClrBit(p->mask[OPP(p->turn)][sp], to);
            if (is_sliding(sp))
                ClrBit(p->slidingPieces, to);

            /* Update oppponents material and PawnCount */
            p->material[OPP(p->turn)] -= Value[sp];
            if (sp != Pawn)
                p->nonPawn[OPP(p->turn)] -= Value[sp];

            /* update material signature */
            if (!(p->mask[OPP(p->turn)][sp])) {
                p->material_signature[OPP(p->turn)] &= ~SIGNATURE_BIT(sp);
            }

            /* update hashkey */
            p->hkey ^= HashKeys[OPP(p->turn)][sp][to];
            if (sp == Pawn)
                p->pkey ^= HashKeys[OPP(p->turn)][Pawn][to];
            if (to == (OPP(p->turn) == White ? h1 : h8)) {
                p->castle &= ~(CastleMask[OPP(p->turn)][0]);
            }
            if (to == (OPP(p->turn) == White ? a1 : a8)) {
                p->castle &= ~(CastleMask[OPP(p->turn)][1]);
            }
        } else if (move & M_ENPASSANT) {
            int so = to ^ 8;

            /* piece looses its attacks */
            AtkClr(p, so);

            /* captured piece must be a pawn */
            p->actLog->gl_Piece = ((OPP(p->turn) == White) ? Pawn : -Pawn);

            ClrBit(p->mask[OPP(p->turn)][0], so);
            ClrBit(p->mask[OPP(p->turn)][Pawn], so);

            /* re-calculate attacks through to-square */
            GainAttacks(p, so);

            /* remove captured pawn from the board */
            p->piece[so] = Neutral;

            /* Update oppponents material and PawnCount */
            p->material[OPP(p->turn)] -= Value[Pawn];

            /* update material signature */
            if (!(p->mask[OPP(p->turn)][Pawn])) {
                p->material_signature[OPP(p->turn)] &= ~SIGNATURE_BIT(Pawn);
            }

            /* update hashkey */
            p->hkey ^= HashKeys[OPP(p->turn)][Pawn][so];
            p->pkey ^= HashKeys[OPP(p->turn)][Pawn][so];

            /* re-calculate attacks through to-square */
            LooseAttacks(p, to);
        } else {
            /* re-calculate attacks through to-square */
            LooseAttacks(p, to);
        }

        if (move & M_PROMOTION_MASK) {
            /* Promote piece */
            tp = PromoType(move);

            /* Update own material */
            p->material[p->turn] += Value[tp] - Value[Pawn];
            p->nonPawn[p->turn] += Value[tp];

            if (!(p->mask[p->turn][Pawn])) {
                p->material_signature[p->turn] &= ~SIGNATURE_BIT(Pawn);
            }
            p->material_signature[p->turn] |= SIGNATURE_BIT(tp);
        }

        /* put it on the board again */
        p->piece[to] = (p->turn == White) ? tp : -tp;
        SetBit(p->mask[p->turn][0], to);
        SetBit(p->mask[p->turn][tp], to);
        if (is_sliding(tp))
            SetBit(p->slidingPieces, to);

        /* piece gains its attacks */
        AtkSet(p, tp, p->turn, to);

        /* update hashkey */
        p->hkey ^= HashKeys[p->turn][tp][to];
        if (tp == Pawn)
            p->pkey ^= HashKeys[p->turn][Pawn][to];
    }

    /* Check if loss of castling rights */
    if (p->castle != p->actLog->gl_Castle) {
        p->hkey ^= HashKeysCastle[p->actLog->gl_Castle];
        p->hkey ^= HashKeysCastle[p->castle];
    }

    /*
     * Check if double pawn push. There is a little trick here:
     * We only set the enPassant flag if there is a possibility
     * of an enPassant capture at all. This increases the efficiency of
     * the transposition table.
     */

    p->enPassant = 0;
    if (move & M_PAWND) {
        int tmpPassant = to ^ 8;
        if (p->atkFr[tmpPassant] & p->mask[OPP(p->turn)][Pawn]) {
            p->enPassant = tmpPassant;
        }
    }

    if (p->enPassant != p->actLog->gl_EnPassant) {
        p->hkey ^= HashKeysEP[p->actLog->gl_EnPassant];
        p->hkey ^= HashKeysEP[p->enPassant];
    }

    /* Update GameLog */
    p->actLog->gl_Move = move;
    p->ply++;

    /* Grow gameLog if needed. */
    if (p->ply >= p->gameLogSize) {
        p->gameLogSize *= 2;
        p->gameLog =
            realloc(p->gameLog, sizeof(struct GameLog) * p->gameLogSize);
        p->actLog = p->gameLog + p->ply;
    } else {
        p->actLog++;
    }

    /* Check if reversible move */
    if (move & (M_CAPTURE | M_PROMOTION_MASK | M_CANY) || tp == Pawn) {
        p->actLog->gl_IrrevCount = 0;
    } else {
        p->actLog->gl_IrrevCount = (p->actLog - 1)->gl_IrrevCount + 1;
    }

    /* Swap p->turns */
    p->turn = OPP(p->turn);
    p->hkey ^= STMKey;
}

void UndoMove(struct Position *p, move_t move) {
    int from = M_FROM(move);
    int to = M_TO(move);
    int tp = TYPE(p->piece[to]);

    /* Swap p->turns */
    p->turn = OPP(p->turn);

    /* Decrement ActLog */
    p->actLog--;
    p->ply--;

    if (move & M_CANY) {
        UndoCastle(p, move);
    } else {
        /* piece looses its attacks */
        AtkClr(p, to);

        if (tp == King) {
            p->kingSq[p->turn] = from;
        }

        /* update masks */
        ClrBit(p->mask[p->turn][0], to);
        ClrBit(p->mask[p->turn][tp], to);
        if (is_sliding(tp))
            ClrBit(p->slidingPieces, to);

        if (move & M_PROMOTION_MASK) {
            /* Update own material */
            p->material[p->turn] -= Value[tp] - Value[Pawn];
            p->nonPawn[p->turn] -= Value[tp];

            /* update material signature */
            if (!(p->mask[p->turn][tp])) {
                p->material_signature[p->turn] &= ~SIGNATURE_BIT(tp);
            }

            /* Unpromote piece */
            tp = Pawn;

            /* update material signature */
            p->material_signature[p->turn] |= SIGNATURE_BIT(Pawn);
        }

        if (move & M_CAPTURE) {
            int sp = p->actLog->gl_Piece;

            /* piece gains its attacks */
            AtkSet(p, TYPE(sp), OPP(p->turn), to);

            p->piece[to] = sp;
            sp = TYPE(sp);
            SetBit(p->mask[OPP(p->turn)][0], to);
            SetBit(p->mask[OPP(p->turn)][sp], to);
            if (is_sliding(sp))
                SetBit(p->slidingPieces, to);

            /* Update oppponents material and PawnCount */
            p->material[OPP(p->turn)] += Value[sp];
            if (sp != Pawn)
                p->nonPawn[OPP(p->turn)] += Value[sp];

            /* update material signature */
            p->material_signature[OPP(p->turn)] |= SIGNATURE_BIT(sp);
        } else if (move & M_ENPASSANT) {
            int so = to ^ 8;

            /* piece looses its attacks */
            AtkSet(p, Pawn, OPP(p->turn), so);

            SetBit(p->mask[OPP(p->turn)][0], so);
            SetBit(p->mask[OPP(p->turn)][Pawn], so);

            /* re-calculate attacks through to-square */
            LooseAttacks(p, so);

            /* remove captured pawn from the board */
            p->piece[so] = (OPP(p->turn) == White) ? Pawn : -Pawn;
            p->piece[to] = Neutral;

            /* re-calculate attacks through to-square */
            GainAttacks(p, to);

            /* Update oppponents material */
            p->material[OPP(p->turn)] += Value[Pawn];

            /* update material signature */
            p->material_signature[OPP(p->turn)] |= SIGNATURE_BIT(Pawn);
        } else {
            p->piece[to] = Neutral;

            /* re-calculate attacks through to-square */
            GainAttacks(p, to);
        }

        /* re-calculate attacks through from-square */
        LooseAttacks(p, from);

        /* put it on the board again */
        p->piece[from] = (p->turn == White) ? tp : -tp;
        SetBit(p->mask[p->turn][0], from);
        SetBit(p->mask[p->turn][tp], from);
        if (is_sliding(tp))
            SetBit(p->slidingPieces, from);

        /* piece gains its attacks */
        AtkSet(p, tp, p->turn, from);
    }

    /* restore EnPassant and Castling */
    p->enPassant = p->actLog->gl_EnPassant;
    p->castle = p->actLog->gl_Castle;

    p->hkey = p->actLog->gl_HashKey;
    p->pkey = p->actLog->gl_PawnKey;

    /*
    DebugEngine(move);
    */
}

/*
 * Make a null move, i.e. swap the p->turn on the move
 */

void DoNull(struct Position *p) {
    /* Update GameLog */
    p->actLog->gl_Move = M_NULL;
    p->actLog->gl_EnPassant = p->enPassant;
    p->actLog->gl_Castle = p->castle;
    p->actLog->gl_HashKey = p->hkey;
    p->enPassant = 0;

    if (p->enPassant != p->actLog->gl_EnPassant) {
        p->hkey ^= HashKeysEP[p->actLog->gl_EnPassant];
        p->hkey ^= HashKeysEP[p->enPassant];
    }

    p->ply++;

    /* Grow gameLog if needed. */
    if (p->ply >= p->gameLogSize) {
        p->gameLogSize *= 2;
        p->gameLog =
            realloc(p->gameLog, sizeof(struct GameLog) * p->gameLogSize);
        p->actLog = p->gameLog + p->ply;
    } else {
        p->actLog++;
    }

    /* treat null move as irreversible */
    p->actLog->gl_IrrevCount = 0;

    /* swap p->turns */
    p->turn = OPP(p->turn);
    p->hkey ^= STMKey;
}

/*
 * Unmake a null move
 */

void UndoNull(struct Position *p) {
    p->turn = OPP(p->turn);

    /* Decrement ActLog */
    p->actLog--;
    p->ply--;

    p->enPassant = p->actLog->gl_EnPassant;
    p->hkey = p->actLog->gl_HashKey;
}

/*
 * Given the Masks and the p->piece[] array, recalculate all necessary data
 */

void RecalcAttacks(struct Position *p) {
    int i;
    BitBoard tmp;

    for (i = 0; i < 64; i++) {
        p->atkTo[i] = p->atkFr[i] = 0;
    }

    for (i = Pawn; i <= King; i++) {
        p->mask[White][i] = p->mask[Black][i] = 0;
    }

    p->slidingPieces = 0;

    p->material[White] = p->material[Black] = p->nonPawn[White] =
        p->nonPawn[Black] = p->material_signature[White] =
            p->material_signature[Black] = p->hkey = p->pkey = 0;

    tmp = p->mask[White][0];
    while (tmp) {
        int i = FindSetBit(tmp);
        int pc = p->piece[i];
        tmp &= tmp - 1;
        SetBit(p->mask[White][pc], i);
        if (is_sliding(pc))
            SetBit(p->slidingPieces, i);
        p->material[White] += Value[pc];
        p->hkey ^= HashKeys[White][pc][i];
        if (pc != Pawn)
            p->nonPawn[White] += Value[pc];
        else {
            p->pkey ^= HashKeys[White][Pawn][i];
        }

        if (pc != King) {
            p->material_signature[White] |= SIGNATURE_BIT(pc);
        }
    }

    tmp = p->mask[Black][0];
    while (tmp) {
        int i = FindSetBit(tmp);
        int pc = -p->piece[i];
        tmp &= tmp - 1;
        SetBit(p->mask[Black][pc], i);
        if (is_sliding(pc))
            SetBit(p->slidingPieces, i);
        p->material[Black] += Value[pc];
        p->hkey ^= HashKeys[Black][pc][i];
        if (pc != Pawn)
            p->nonPawn[Black] += Value[pc];
        else {
            p->pkey ^= HashKeys[Black][Pawn][i];
        }

        if (pc != King) {
            p->material_signature[Black] |= SIGNATURE_BIT(pc);
        }
    }

    tmp = p->mask[White][0];
    while (tmp) {
        int i = FindSetBit(tmp);
        tmp &= tmp - 1;
        AtkSet(p, p->piece[i], White, i);
    }

    tmp = p->mask[Black][0];
    while (tmp) {
        int i = FindSetBit(tmp);
        tmp &= tmp - 1;
        AtkSet(p, -p->piece[i], Black, i);
    }

    p->kingSq[White] = FindSetBit(p->mask[White][King]);
    p->kingSq[Black] = FindSetBit(p->mask[Black][King]);

    p->hkey ^= HashKeysCastle[p->castle];
    if (p->turn == Black)
        p->hkey ^= STMKey;

    if (p->enPassant != 0) {
        p->hkey ^= HashKeysEP[p->enPassant];
    }
}

/*
 * Generate all capturing moves to a square "square"
 */
void GenTo(struct Position *p, Square square, heap_t heap) {
    BitBoard tmp = p->atkFr[square] & p->mask[p->turn][0];

    while (tmp) {
        int i = FindSetBit(tmp);
        tmp &= tmp - 1;
        if (TYPE(p->piece[i]) == Pawn && is_promo_square(square)) {
            append_to_heap(heap, make_promotion(i, square, Queen, M_CAPTURE));
            append_to_heap(heap, make_promotion(i, square, Knight, M_CAPTURE));
            append_to_heap(heap, make_promotion(i, square, Rook, M_CAPTURE));
            append_to_heap(heap, make_promotion(i, square, Bishop, M_CAPTURE));
        } else {
            append_to_heap(heap, make_move(i, square, M_CAPTURE));
        }
    }
}

void GenEnpas(struct Position *p, heap_t heap) {
    BitBoard tmp;

    if (!p->enPassant)
        return;

    tmp = p->atkFr[p->enPassant] & p->mask[p->turn][Pawn];
    while (tmp) {
        int i = FindSetBit(tmp);
        tmp &= tmp - 1;
        append_to_heap(heap, make_move(i, p->enPassant, M_ENPASSANT));
    }
}

/*
 * Generate all non-capturing moves from "square"
 */

void GenFrom(struct Position *p, Square square, heap_t heap) {
    if (TYPE(p->piece[square]) != Pawn) {
        BitBoard tmp;

        tmp = p->atkTo[square] & ~(p->mask[White][0] | p->mask[Black][0]);

        while (tmp) {
            int i = FindSetBit(tmp);
            tmp &= tmp - 1;
            append_to_heap(heap, make_move(square, i, 0));
        }

        /* Generate castling moves
         * we will check legality later...
         */

        if (TYPE(p->piece[square]) == King) {
            if (p->castle & CastleMask[p->turn][0]) {
                /* OK, we might castle king p->turn */
                append_to_heap(heap, make_move(p->turn == White ? e1 : e8,
                                               p->turn == White ? g1 : g8,
                                               M_SCASTLE));
            }
            if (p->castle & CastleMask[p->turn][1]) {
                append_to_heap(heap, make_move(p->turn == White ? e1 : e8,
                                               p->turn == White ? c1 : c8,
                                               M_LCASTLE));
            }
        }
    } else {
        int sq = (p->turn == White ? square + 8 : square - 8);

        if (p->piece[sq] == Neutral) {
            if (is_promo_square(sq)) {
                append_to_heap(heap, make_promotion(square, sq, Queen, 0));
                append_to_heap(heap, make_promotion(square, sq, Knight, 0));
                append_to_heap(heap, make_promotion(square, sq, Rook, 0));
                append_to_heap(heap, make_promotion(square, sq, Bishop, 0));
            } else {
                append_to_heap(heap, make_move(square, sq, 0));

                if ((p->turn == White && square <= h2) ||
                    (p->turn == Black && square >= a7)) {
                    sq = (p->turn == White ? sq + 8 : sq - 8);
                    if (p->piece[sq] == Neutral) {
                        append_to_heap(heap, make_move(square, sq, M_PAWND));
                    }
                }
            }
        }
    }
}

/*
 * Test if castling is legal
 */

bool MayCastle(struct Position *p, move_t move) {
    /* Sometimes there might be a legal castling move, but for the
       wrong p->turn, probably from the Countermove table */
    if (M_FROM(move) != ((p->turn == White) ? e1 : e8))
        return false;

    if (InCheck(p, p->turn))
        return false;

    /* king p->turn castling */
    if ((move & M_SCASTLE) && (p->castle & CastleMask[p->turn][0])) {
        int fs = (p->turn == White ? f1 : f8);
        int gs = (p->turn == White ? g1 : g8);

        /* Check if f and g square are empty */
        if (p->piece[fs] == Neutral && p->piece[gs] == Neutral) {
            /* Check if f and g square are not attacked by opponent */
            if ((p->atkFr[fs] | p->atkFr[gs]) & p->mask[OPP(p->turn)][0])
                return false;
            else
                return true;
        }
    }

    /* queen p->turn castling */
    if ((move & M_LCASTLE) && (p->castle & CastleMask[p->turn][1])) {
        int bs = (p->turn == White ? b1 : b8);
        int cs = (p->turn == White ? c1 : c8);
        int ds = (p->turn == White ? d1 : d8);

        /* Check if b, c and d square are empty */
        if (p->piece[bs] == Neutral && p->piece[cs] == Neutral &&
            p->piece[ds] == Neutral) {
            /* Check if c and d square are not attacked by opponent */
            if ((p->atkFr[cs] | p->atkFr[ds]) & p->mask[OPP(p->turn)][0])
                return false;
            else
                return true;
        }
    }

    return false;
}

/*
 * Test if a move is legal
 */

bool LegalMove(struct Position *p, move_t move) {
    int fr = M_FROM(move);
    int to = M_TO(move);

    if (move == M_NONE || move == M_NULL)
        return false;

    /* There must be a piece on the square */
    if (!SAME_COLOR(p->piece[fr], p->turn))
        return false;

    /* if a promotion, moving piece must be a pawn */
    if (move & M_PROMOTION_MASK && TYPE(p->piece[fr]) != Pawn)
        return false;

    /* if the move is a pawn move to the 1st/8th rank, it must be
     * be a promotion.
     */
    if ((to <= h1 || to >= a8) && TYPE(p->piece[fr]) == Pawn &&
        !(move & M_PROMOTION_MASK))
        return false;

    if (move & M_CAPTURE) {
        /* There must be an enemy piece on the target square, and we
         * must attack that square
         */

        if (!SAME_COLOR(p->piece[to], OPP(p->turn)) ||
            !TstBit(p->atkTo[fr], to)) {
            return false;
        }
        return true;
    } else if (move & M_ENPASSANT) {
        /* The moving piece must be a pawn, and the target square must be
         * the enpassant square
         */

        if (!p->enPassant)
            return false;
        if (TYPE(p->piece[fr]) != Pawn || to != p->enPassant)
            return false;
        if (!TstBit(p->atkTo[fr], to))
            return false;

        return true;
    } else if (move & M_CANY) {
        /* Call the castling test routine */
        return MayCastle(p, move);
    } else {
        /* target sqaure must be empty */
        if (p->piece[to] != Neutral)
            return false;

        if (TYPE(p->piece[fr]) != Pawn) {
            /* if no pawn, we must attack to square */
            if (!TstBit(p->atkTo[fr], to))
                return false;
            if (move & M_PAWND)
                return false;
            return true;
        } else {
            /* use NextPos array to check if legal move */
            int tt = (p->turn == White ? fr + 8 : fr - 8);
            if (move & M_PAWND) {
                if (p->piece[tt] != Neutral)
                    return false;
                tt = (p->turn == White ? tt + 8 : tt - 8);
            }
            if (tt != to)
                return false;

            if (p->turn == White && to >= a8 && !(move & M_PROMOTION_MASK))
                return false;
            if (p->turn == Black && to <= h1 && !(move & M_PROMOTION_MASK))
                return false;

            return true;
        }
    }
    /* return false; */ /* never reached */
}

/*
 * Test wether a move will give check
 */

bool IsCheckingMove(struct Position *p, move_t move) {
    int fr = M_FROM(move);
    int to = M_TO(move);
    int tp = TYPE(p->piece[fr]);
    int kp = FindSetBit(p->mask[OPP(p->turn)][King]);
    BitBoard tmp;

    /* Is it a direct check ? */

    if (move & M_PROMOTION_MASK)
        tp = PromoType(move);

    switch (tp) {
    case Knight:
        if (TstBit(KnightEPM[kp], to))
            return true;
        break;
    case Bishop:
        if (TstBit(BishopEPM[kp], to)) {
            if (!((p->mask[White][0] | p->mask[Black][0]) & InterPath[kp][to]))
                return true;
        }
        break;
    case Rook:
        if (TstBit(RookEPM[kp], to)) {
            if (!((p->mask[White][0] | p->mask[Black][0]) & InterPath[kp][to]))
                return true;
        }
        break;
    case Queen:
        if (TstBit(QueenEPM[kp], to)) {
            if (!((p->mask[White][0] | p->mask[Black][0]) & InterPath[kp][to]))
                return true;
        }
        break;
    case Pawn:
        if (p->turn == White) {
            if ((to & 7) < 7 && (to + 9) == kp)
                return true;
            if ((to & 7) > 0 && (to + 7) == kp)
                return true;
        } else {
            if ((to & 7) < 7 && (to - 9) == kp)
                return true;
            if ((to & 7) > 0 && (to - 7) == kp)
                return true;
        }
        break;
    }

    /*
     * No direct check...
     * Let's see if it might be a discovered check...
     */

    tmp = (p->mask[p->turn][Bishop] | p->mask[p->turn][Rook] |
           p->mask[p->turn][Queen]) &
          QueenEPM[kp];

    while (tmp) {
        int i = FindSetBit(tmp);
        BitBoard tmp2;

        tmp &= tmp - 1;
        if (TYPE(p->piece[i]) == Bishop && !TstBit(BishopEPM[kp], i))
            continue;
        if (TYPE(p->piece[i]) == Rook && !TstBit(RookEPM[kp], i))
            continue;

        tmp2 = (p->mask[White][0] | p->mask[Black][0]) & InterPath[kp][i];
        if (CountBits(tmp2) == 1 && FindSetBit(tmp2) == fr)
            return true;
    }

    return false;
}

/*
 * Generate all non-capturing checking moves. Actually this routine only
 * generates 'candidate' moves for checks. Some move generated here may
 * not be checks!
 */

void GenChecks(struct Position *p, heap_t heap) {
    BitBoard tmp;
    BitBoard fr;
    int kp = p->kingSq[OPP(p->turn)];
    BitBoard *ip = InterPath[kp];
    BitBoard fsq = p->mask[p->turn][0];
    BitBoard all = (p->mask[White][0] | p->mask[Black][0]);

    /* First find all blockers, i.e. pieces that give check when they move
     * from their current square
     */

    tmp = (p->mask[p->turn][Bishop] | p->mask[p->turn][Queen]) & BishopEPM[kp];

    while (tmp) {
        int i = FindSetBit(tmp);
        tmp &= tmp - 1;
        if (ip[i] && !(ip[i] & p->mask[OPP(p->turn)][0])) {
            BitBoard tmp2 = p->mask[p->turn][0] & ip[i];

            if (CountBits(tmp2) == 1) {
                int j = FindSetBit(tmp2);

                if (TstBit(fsq, j)) {
                    GenFrom(p, j, heap);
                    ClrBit(fsq, j);
                }
            }
        }
    }

    tmp = (p->mask[p->turn][Rook] | p->mask[p->turn][Queen]) & RookEPM[kp];

    while (tmp) {
        int i = FindSetBit(tmp);
        tmp &= tmp - 1;
        if (ip[i] && !(ip[i] & p->mask[OPP(p->turn)][0])) {
            BitBoard tmp2 = p->mask[p->turn][0] & ip[i];

            if (CountBits(tmp2) == 1) {
                int j = FindSetBit(tmp2);

                if (TstBit(fsq, j)) {
                    GenFrom(p, j, heap);
                    ClrBit(fsq, j);
                }
            }
        }
    }

    /* Find direct checks by Bishop or Queen */
    tmp = BishopEPM[kp];
    tmp &= ~all;

    fr = p->mask[p->turn][Bishop] | p->mask[p->turn][Queen];
    fr &= fsq;

    while (fr) {
        int sq = FindSetBit(fr);
        BitBoard tmp2 = p->atkTo[sq] & tmp;
        fr &= fr - 1;

        while (tmp2) {
            int sq2 = FindSetBit(tmp2);
            tmp2 &= tmp2 - 1;
            if (InterPath[kp][sq2] & all)
                continue;
            append_to_heap(heap, make_move(sq, sq2, 0));
        }
    }

    /* Find direct checks by Rook or Queen */
    tmp = RookEPM[kp];
    tmp &= ~all;

    fr = p->mask[p->turn][Rook] | p->mask[p->turn][Queen];
    fr &= fsq;

    while (fr) {
        int sq = FindSetBit(fr);
        BitBoard tmp2 = p->atkTo[sq] & tmp;
        fr &= fr - 1;

        while (tmp2) {
            int sq2 = FindSetBit(tmp2);
            tmp2 &= tmp2 - 1;
            if (InterPath[kp][sq2] & all)
                continue;
            append_to_heap(heap, make_move(sq, sq2, 0));
        }
    }

    /* Find direct checks by Knight */
    tmp = KnightEPM[kp];
    tmp &= ~all;

    fr = p->mask[p->turn][Knight];
    fr &= fsq;

    while (fr) {
        int sq = FindSetBit(fr);
        BitBoard tmp2;

        fr &= fr - 1;
        tmp2 = p->atkTo[sq] & tmp;

        while (tmp2) {
            int sq2 = FindSetBit(tmp2);
            tmp2 &= tmp2 - 1;
            append_to_heap(heap, make_move(sq, sq2, 0));
        }
    }

    /*
     * last find pawn checks
     */

    tmp = (p->turn == White) ? BPawnEPM[kp] : WPawnEPM[kp];
    tmp &= ~(p->mask[White][0] | p->mask[Black][0]);

    while (tmp) {
        int sq = FindSetBit(tmp);
        tmp &= tmp - 1;

        if (p->turn == White) {
            if (p->piece[sq - 8] == Pawn) {
                append_to_heap(heap, make_move(sq - 8, sq, 0));
            }
        } else {
            if (p->piece[sq + 8] == -Pawn) {
                append_to_heap(heap, make_move(sq + 8, sq, 0));
            }
        }
    }
}

/*
 * Repetition check
 * if mode = true, count the number of repetitions of current position
 * if mode = false, only check if current position is repeated
 */

int Repeated(struct Position *p, int mode) {
    int i, cnt = 0;
    struct GameLog *gl;

    if (p->ply == 0)
        return 0;

    if (p->actLog->gl_IrrevCount >= 100)
        return 3;

    gl = p->actLog - 1;
    for (i = p->actLog->gl_IrrevCount; i > 0; i--, gl--) {
        if (gl->gl_HashKey == p->hkey) {
            if (mode)
                cnt++;
            else
                return true;
        }
    }

    return cnt;
}

/*
 * Generate the SAN (Standard Algebraic Notation) for a move.
 *
 * Args:
 *   p: pointer to the current position
 *   move: the legal move in position to generate the SAN for
 *   buffer: a pointer to a buffer to place the generated string in.
 *           There is no bounds checking, so the buffer should be large
 *           enough to hold the generated SAN.
 *
 * Returns:
 *   the pointer to the generated string (buffer)
 */
char *SAN(struct Position *p, move_t move, char *buffer) {
    char *x = buffer;

    int to = M_TO(move);
    int fr = M_FROM(move);
    int tp = TYPE(p->piece[fr]);

    if (tp == Pawn) {
        if (move & (M_CAPTURE | M_ENPASSANT)) {
            *(x++) = 'a' + (fr & 7);
            *(x++) = 'x';
        }
        *(x++) = 'a' + (to & 7);
        *(x++) = '1' + (to >> 3);

        if (move & M_PROMOTION_MASK) {
            *(x++) = '=';
            *(x++) = PieceName[PromoType(move)];
        }
    } else if (move & M_CANY) {
        *(x++) = 'O';
        *(x++) = '-';
        *(x++) = 'O';
        if (move & M_LCASTLE) {
            *(x++) = '-';
            *(x++) = 'O';
        }
    } else {
        BitBoard tmp;
        bool aamb = false, /* set for ambigous moves */
            ramb = false,  /* set means ambigous rank */
            famb = false;  /* set means ambigous file */
        int i;

        tmp = p->atkFr[to] & p->mask[p->turn][tp];

        /* check for ambigous move */
        while (tmp) {
            i = FindSetBit(tmp);
            tmp &= tmp - 1;
            if (i != fr) {
                int incheck;
                int tmove = i | (to << 6);

                /* seems there is another piece of the same type which
                 * can move to the same destination square.
                 * Let's see wether it is pinned...
                 */

                if (p->piece[to])
                    tmove |= M_CAPTURE;

                DoMove(p, tmove);
                incheck = InCheck(p, OPP(p->turn));
                UndoMove(p, tmove);

                if (incheck)
                    continue;

                aamb = true;
                if ((i & 7) == (fr & 7))
                    famb = true;
                if ((i >> 3) == (fr >> 3))
                    ramb = true;
            }
        }

        *(x++) = PieceName[tp];
        if (aamb) {
            if (!famb)
                *(x++) = 'a' + (fr & 7);
            else {
                if (!ramb)
                    *(x++) = '1' + (fr >> 3);
                else {
                    *(x++) = 'a' + (fr & 7);
                    *(x++) = '1' + (fr >> 3);
                }
            }
        }

        if (move & (M_CAPTURE | M_ENPASSANT))
            *(x++) = 'x';

        *(x++) = 'a' + (to & 7);
        *(x++) = '1' + (to >> 3);
    }

    DoMove(p, move);
    if (InCheck(p, p->turn)) {
        if (!LegalMoves(p, NULL))
            *(x++) = '#';
        else
            *(x++) = '+';
    }
    UndoMove(p, move);

    *x = '\0';
    return buffer;
}

/*
 * Generate the ICS SAN for a move
 */

char *ICS_SAN(move_t move) {
    static char buffer[16];
    char *x = buffer;

    int to = M_TO(move);
    int fr = M_FROM(move);

    *(x++) = 'a' + (fr & 7);
    *(x++) = '1' + (fr >> 3);
    if (move & (M_CAPTURE | M_ENPASSANT)) {
        *(x++) = 'x';
    }
    *(x++) = 'a' + (to & 7);
    *(x++) = '1' + (to >> 3);
    if (move & M_PROMOTION_MASK) {
        *(x++) = PieceName[PromoType(move)];
    }
    *x = '\0';
    return buffer;
}

/*
 * Parse a move string in e2e4 notation
 */

move_t parse_gsan_internal(struct Position *p, char *san, heap_t heap) {
    int fr, to;
    int mask;

    if (!strncmp(san, "O-O-O", 5) || !strncmp(san, "o-o-o", 5) ||
        !strncmp(san, "0-0-0", 5)) {
        int move =
            M_LCASTLE | (p->turn == White ? (c1 << 6) + e1 : (c8 << 6) + e8);
        if (MayCastle(p, move))
            return move;
    }

    if (!strncmp(san, "O-O", 3) || !strncmp(san, "o-o", 3) ||
        !strncmp(san, "0-0", 3)) {
        int move =
            M_SCASTLE | (p->turn == White ? (g1 << 6) + e1 : (g8 << 6) + e8);
        if (MayCastle(p, move))
            return move;
    }

    (void)LegalMoves(p, heap);

    fr = *san - 'a' + 8 * (*(san + 1) - '1');
    to = *(san + 2) - 'a' + 8 * (*(san + 3) - '1');

    mask = fr + (to << 6);

    for (unsigned int i = heap->current_section->start;
         i < heap->current_section->end; i++) {
        move_t move = heap->data[i];
        if ((move & 4095) == mask) {
            if (move & M_PROMOTION_MASK) {
                char p = *(san + 4);
                move = move & (~M_PROMOTION_MASK);

                if (p == 'q' || p == 'Q')
                    return move | (Queen << M_PROMOTION_OFFSET);
                if (p == 'r' || p == 'R')
                    return move | (Rook << M_PROMOTION_OFFSET);
                if (p == 'n' || p == 'N')
                    return move | (Knight << M_PROMOTION_OFFSET);
                if (p == 'b' || p == 'B')
                    return move | (Bishop << M_PROMOTION_OFFSET);
            } else
                return move;
        }
    }
    return M_NONE;
}

move_t ParseGSAN(struct Position *p, char *san) {
    heap_t heap = allocate_heap();
    move_t move = parse_gsan_internal(p, san, heap);
    free_heap(heap);

    return move;
}

/*
 * Parse a move string in e2e4 notation against a supplied move list
 */

move_t ParseGSANList(char *san, Color side, move_t *mvs, int cnt) {
    int fr, to;
    int mask;
    int i;

    if (!strncmp(san, "O-O-O", 5) || !strncmp(san, "o-o-o", 5) ||
        !strncmp(san, "0-0-0", 5)) {
        int move =
            M_LCASTLE | (side == White ? (c1 << 6) + e1 : (c8 << 6) + e8);

        for (i = 0; i < cnt; i++)
            if (move == mvs[i])
                return move;
        return M_NONE;
    }

    if (!strncmp(san, "O-O", 3) || !strncmp(san, "o-o", 3) ||
        !strncmp(san, "0-0", 3)) {
        int move =
            M_SCASTLE | (side == White ? (g1 << 6) + e1 : (g8 << 6) + e8);

        for (i = 0; i < cnt; i++)
            if (move == mvs[i])
                return move;
        return M_NONE;
    }

    fr = *san - 'a' + 8 * (*(san + 1) - '1');
    to = *(san + 2) - 'a' + 8 * (*(san + 3) - '1');

    mask = fr + (to << 6);

    for (i = 0; i < cnt; i++) {
        if ((mvs[i] & 4095) == mask) {
            if (mvs[i] & M_PROMOTION_MASK) {
                char p = *(san + 4);
                int move = mvs[i] & (~M_PROMOTION_MASK);

                if (p == 'q' || p == 'Q')
                    return move | (Queen << M_PROMOTION_OFFSET);
                if (p == 'r' || p == 'R')
                    return move | (Rook << M_PROMOTION_OFFSET);
                if (p == 'n' || p == 'N')
                    return move | (Knight << M_PROMOTION_OFFSET);
                if (p == 'b' || p == 'B')
                    return move | (Bishop << M_PROMOTION_OFFSET);
            } else
                return mvs[i];
        }
    }
    return M_NONE;
}

/*
 * Test a pseudolegal move for legality
 */

static bool TryMove(struct Position *p, move_t move) {
    bool tmp;
    DoMove(p, move);
    tmp = InCheck(p, OPP(p->turn));
    UndoMove(p, move);

    return !tmp;
}

/*
 * Parse a move string (in SAN)
 */
static move_t parse_san_with_heap(struct Position *p, char *san, heap_t heap) {
    int tp = Neutral;
    int frk = -1, ffl = -1, trk = -1, tfl = -1;
    int pro = 0;
    unsigned int i;
    move_t move;

    /* Check castling first */

    if (!strncmp(san, "O-O-O", 5) || !strncmp(san, "o-o-o", 5) ||
        !strncmp(san, "0-0-0", 5)) {
        move = M_LCASTLE | (p->turn == White ? (c1 << 6) + e1 : (c8 << 6) + e8);
        if (MayCastle(p, move))
            return move;
        else
            return M_NONE;
    }

    if (!strncmp(san, "O-O", 3) || !strncmp(san, "o-o", 3) ||
        !strncmp(san, "0-0", 3)) {
        move = M_SCASTLE | (p->turn == White ? (g1 << 6) + e1 : (g8 << 6) + e8);
        if (MayCastle(p, move))
            return move;
        else
            return M_NONE;
    }

    PLegalMoves(p, heap);

    /* special handling of pawn captures a la 'cd' */
    if (strlen(san) == 2 && *san >= 'a' && *san <= 'h' && *(san + 1) >= 'a' &&
        *(san + 1) <= 'h') {
        ffl = *san - 'a';
        tfl = *(san + 1) - 'a';

        for (i = heap->current_section->start; i < heap->current_section->end;
             i++) {
            move = heap->data[i];
            int fr = M_FROM(move);
            int to = M_TO(move);

            if (TYPE(p->piece[fr]) == Pawn &&
                (move & (M_CAPTURE | M_ENPASSANT)) && (fr & 7) == ffl &&
                (to & 7) == tfl && TryMove(p, move))
                return move;
        }
        return M_NONE;
    }

    /* Next examine the string */
    for (; *san; san++) {
        switch (*san) {
        case 'a':
        case 'b':
        case 'c':
        case 'd':
        case 'e':
        case 'f':
        case 'g':
        case 'h': {
            ffl = tfl;
            tfl = *san - 'a';
            break;
        }
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8': {
            frk = trk;
            trk = *san - '1';
            break;
        }
        case 'N':
            tp = Knight;
            break;
        case 'B':
            tp = Bishop;
            break;
        case 'R':
            tp = Rook;
            break;
        case 'Q':
            tp = Queen;
            break;
        case 'K':
            tp = King;
            break;
        case '=':
            san++;
            if (*san == 'Q')
                pro = Queen;
            else if (*san == 'R')
                pro = Rook;
            else if (*san == 'B')
                pro = Bishop;
            else if (*san == 'N')
                pro = Knight;
            else
                return M_NONE;
            break;
        case 'x':
        case '+':
        case '#':
            break;
        default:
            return M_NONE;
        }
    }

    if (tp == Neutral)
        tp = Pawn;

    for (i = heap->current_section->start; i < heap->current_section->end;
         i++) {
        move = heap->data[i];
        int fr = M_FROM(move), to = M_TO(move);

        if (TYPE(p->piece[fr]) != tp)
            continue;
        if ((to & 7) != tfl || (to >> 3) != trk)
            continue;
        if (ffl != -1 && (fr & 7) != ffl)
            continue;
        if (frk != -1 && (fr >> 3) != frk)
            continue;
        if (pro && (PromoType(move) == pro))
            continue;
        if (!TryMove(p, move))
            continue;

        return move;
    }

    return M_NONE;
}

move_t ParseSAN(struct Position *p, char *san) {
    heap_t heap = allocate_heap();
    move_t move = parse_san_with_heap(p, san, heap);
    free_heap(heap);
    return move;
}

/*
 * Parse a move string (in SAN) against supplied move list
 */

move_t ParseSANList(char *san, Color side, move_t *mvs, int cnt, int *pmap) {
    int tp = Neutral;
    int frk = -1, ffl = -1, trk = -1, tfl = -1;
    int pro = 0;
    int move;
    int i;

    /* Check castling first */

    if (!strncmp(san, "O-O-O", 5) || !strncmp(san, "o-o-o", 5) ||
        !strncmp(san, "0-0-0", 5)) {
        move = M_LCASTLE | (side == White ? (c1 << 6) + e1 : (c8 << 6) + e8);
        for (i = 0; i < cnt; i++)
            if (move == mvs[i])
                return move;
        return M_NONE;
    }

    if (!strncmp(san, "O-O", 3) || !strncmp(san, "o-o", 3) ||
        !strncmp(san, "0-0", 3)) {
        move = M_SCASTLE | (side == White ? (g1 << 6) + e1 : (g8 << 6) + e8);
        for (i = 0; i < cnt; i++)
            if (move == mvs[i])
                return move;
        return M_NONE;
    }

    /* special handling of pawn captures a la 'cd' */
    if (strlen(san) == 2 && *san >= 'a' && *san <= 'h' && *(san + 1) >= 'a' &&
        *(san + 1) <= 'h') {
        ffl = *san - 'a';
        tfl = *(san + 1) - 'a';

        for (i = 0; i < cnt; i++) {
            int fr = M_FROM(mvs[i]);
            int to = M_TO(mvs[i]);

            if (pmap[fr] == Pawn && (mvs[i] & (M_CAPTURE | M_ENPASSANT)) &&
                (fr & 7) == ffl && (to & 7) == tfl)
                return mvs[i];
        }
        return M_NONE;
    }

    /* Next examine the string */
    for (; *san; san++) {
        switch (*san) {
        case 'a':
        case 'b':
        case 'c':
        case 'd':
        case 'e':
        case 'f':
        case 'g':
        case 'h': {
            ffl = tfl;
            tfl = *san - 'a';
            break;
        }
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8': {
            frk = trk;
            trk = *san - '1';
            break;
        }
        case 'N':
            tp = Knight;
            break;
        case 'B':
            tp = Bishop;
            break;
        case 'R':
            tp = Rook;
            break;
        case 'Q':
            tp = Queen;
            break;
        case 'K':
            tp = King;
            break;
        case '=':
            san++;
            if (*san == 'Q')
                pro = Queen;
            else if (*san == 'R')
                pro = Rook;
            else if (*san == 'B')
                pro = Bishop;
            else if (*san == 'N')
                pro = Knight;
            else
                return M_NONE;
            break;
        case 'x':
        case '+':
        case '#':
            break;
        default:
            return M_NONE;
        }
    }

    if (tp == Neutral)
        tp = Pawn;

    for (i = 0; i < cnt; i++) {
        int fr = M_FROM(mvs[i]), to = M_TO(mvs[i]);

        if (TYPE(pmap[fr]) != tp)
            continue;
        if ((to & 7) != tfl || (to >> 3) != trk)
            continue;
        if (ffl != -1 && (fr & 7) != ffl)
            continue;
        if (frk != -1 && (fr >> 3) != frk)
            continue;
        if (pro && (PromoType(mvs[i]) != pro))
            continue;

        return mvs[i];
    }

    return M_NONE;
}

/*
 * Generate all pseudolegal (!) moves
 * or test if there are any, if mvs = NULL
 */

void PLegalMoves(struct Position *p, heap_t heap) {
    BitBoard tmp;

    tmp = p->mask[OPP(p->turn)][0];
    while (tmp) {
        int j = FindSetBit(tmp);
        tmp &= tmp - 1;

        GenTo(p, j, heap);
    }

    tmp = p->mask[p->turn][0];
    while (tmp) {
        int j = FindSetBit(tmp);
        tmp &= tmp - 1;

        GenFrom(p, j, heap);
    }

    GenEnpas(p, heap);
}

/**
 * Generate all strictly legal moves.
 *
 * Returns:
 *     the number of generated moves
 */

void legal_moves_internal(struct Position *p, heap_t heap, heap_t tmp_heap) {
    BitBoard tmp;

    tmp = p->mask[OPP(p->turn)][0];
    while (tmp) {
        int j = FindSetBit(tmp);
        unsigned int i;
        tmp &= tmp - 1;

        push_section(tmp_heap);
        GenTo(p, j, tmp_heap);

        for (i = tmp_heap->current_section->start;
             i < tmp_heap->current_section->end; i++) {
            move_t move = tmp_heap->data[i];
            DoMove(p, move);
            if (!InCheck(p, OPP(p->turn))) {
                append_to_heap(heap, move);
            }
            UndoMove(p, move);
        }

        pop_section(tmp_heap);
    }

    tmp = p->mask[p->turn][0];
    while (tmp) {
        int j = FindSetBit(tmp);
        unsigned int i;
        tmp &= tmp - 1;

        push_section(tmp_heap);
        GenFrom(p, j, tmp_heap);

        for (i = tmp_heap->current_section->start;
             i < tmp_heap->current_section->end; i++) {
            move_t move = tmp_heap->data[i];
            if ((move & M_CANY) && !MayCastle(p, move))
                continue;

            DoMove(p, move);
            if (!InCheck(p, OPP(p->turn))) {
                append_to_heap(heap, move);
            }
            UndoMove(p, move);
        }

        pop_section(tmp_heap);
    }

    push_section(tmp_heap);
    GenEnpas(p, tmp_heap);
    {
        unsigned int i;
        for (i = tmp_heap->current_section->start;
             i < tmp_heap->current_section->end; i++) {
            move_t move = tmp_heap->data[i];
            DoMove(p, move);
            if (!InCheck(p, OPP(p->turn))) {
                append_to_heap(heap, move);
            }
            UndoMove(p, move);
        }
    }
    pop_section(tmp_heap);
}

int LegalMoves(struct Position *p, heap_t heap) {
    heap_t tmp_heap = allocate_heap();
    heap_t destination = heap;

    if (heap == NULL) {
        destination = allocate_heap();
    }

    legal_moves_internal(p, destination, tmp_heap);

    int cnt =
        destination->current_section->end - destination->current_section->start;

    if (heap == NULL) {
        free_heap(destination);
    }

    free_heap(tmp_heap);

    return cnt;
}

/*
 * Print the current position
 */

void ShowPosition(struct Position *p) {
    int rk, fl;

    Print(0, "        +---+---+---+---+---+---+---+---+\n");
    for (rk = 7; rk >= 0; rk--) {
        char indicator =
            ((rk == 7) && (p->turn)) || ((rk == 0) && (!p->turn)) ? '>' : ' ';
        Print(0, "    %c %c ", indicator, '1' + rk);
        for (fl = 0; fl < 8; fl++) {
            int i = (rk << 3) + fl;

            Print(0, "|");
            if (p->enPassant && i == p->enPassant)
                Print(0, "<E>");
            else {
                if (p->piece[i] < 0)
                    Print(0, "*");
                else
                    Print(0, " ");
                Print(0, "%c", PieceName[TYPE(p->piece[i])]);
                if (p->piece[i] < 0)
                    Print(0, "*");
                else
                    Print(0, " ");
            }
        }
        if (rk == 4) {
            int bit;
            Print(0, "|   Black (%5d, %5d)  ", p->material[Black],
                  p->nonPawn[Black]);
            for (bit = 0; bit < 5; bit++) {
                Print(0, "%c",
                      (p->material_signature[Black] & (1 << bit))
                          ? PieceName[bit + 1]
                          : '.');
            }
            Print(0, "\n");
        } else if (rk == 3) {
            int bit;
            Print(0, "|   White (%5d, %5d)  ", p->material[White],
                  p->nonPawn[White]);
            for (bit = 0; bit < 5; bit++) {
                Print(0, "%c",
                      (p->material_signature[White] & (1 << bit))
                          ? PieceName[bit + 1]
                          : '.');
            }
            Print(0, "\n");
        } else if (rk == 6) {
            Print(0, "|   Hashkey: %llx\n", p->hkey);
        } else if (rk == 1) {
            Print(0, "|   Index: %d\n", RECOGNIZER_INDEX(p));
        } else if (rk == 0) {
            Print(0, "|   MateThreat: %d %d\n", MateThreat(p, White),
                  MateThreat(p, Black));
        } else {
            Print(0, "|\n");
        }
        Print(0, "        +---+---+---+---+---+---+---+---+\n");
    }
    Print(0, "          a   b   c   d   e   f   g   h\n");
}

/*
 * Display all legal moves.
 */

void ShowMoves(struct Position *p) {
    unsigned int i;
    char san_buffer[16];

    heap_t heap = allocate_heap();

    push_section(heap);
    LegalMoves(p, heap);

    for (i = heap->current_section->start; i < heap->current_section->end;
         i++) {
        move_t move = heap->data[i];
        Print(0, "%s ", SAN(p, move, san_buffer));
        if (IsCheckingMove(p, move))
            Print(0, "(check) ");
        if (!LegalMove(p, move)) {
            Print(0, "(rejected?!) ");
        }
        if (move & (M_CAPTURE | M_ENPASSANT)) {
            Print(0, "(%d) ", SwapOff(p, move));
        }
    }
    Print(0, "\n");

    pop_section(heap);

    push_section(heap);
    GenChecks(p, heap);

    if (heap->current_section->end > heap->current_section->start) {
        Print(0, "Checks: ");
        for (i = heap->current_section->start; i < heap->current_section->end;
             i++) {
            move_t move = heap->data[i];
            Print(0, "%s ", SAN(p, move, san_buffer));
        }
        Print(0, "\n");
    }

    free_heap(heap);
}

/*
 * EPD stuff
 */

int goodmove[MAX_EPD_MOVES];
int badmove[MAX_EPD_MOVES];

/**
 * Read a position from an EPD string.
 */
static void ReadEPD(struct Position *p, char *x) {
    int rk = 7, fl = 0;
    int i;
    char *ops[MAX_EPD_OPS];
    char *line;
    char san_buffer[16];

    /* Make a copy of the input string, since it will be destroyed
     * due to the use of strtok, sorry :-)
     */

    line = malloc(strlen(x) + 1);
    strcpy(line, x);
    x = line;

    for (i = 0; i < 64; i++)
        p->piece[i] = Neutral;
    p->mask[White][0] = p->mask[Black][0] = 0;

    /* scan piece placement */
    while (rk >= 0) {
        switch (*x) {
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
            fl += (*x) - '0';
            break;
        case '-':
            fl += 1;
            break;
        case 'P':
            p->piece[fl + (rk << 3)] = Pawn;
            SetBit(p->mask[White][0], fl + (rk << 3));
            fl++;
            break;
        case 'N':
            p->piece[fl + (rk << 3)] = Knight;
            SetBit(p->mask[White][0], fl + (rk << 3));
            fl++;
            break;
        case 'B':
            p->piece[fl + (rk << 3)] = Bishop;
            SetBit(p->mask[White][0], fl + (rk << 3));
            fl++;
            break;
        case 'R':
            p->piece[fl + (rk << 3)] = Rook;
            SetBit(p->mask[White][0], fl + (rk << 3));
            fl++;
            break;
        case 'Q':
            p->piece[fl + (rk << 3)] = Queen;
            SetBit(p->mask[White][0], fl + (rk << 3));
            fl++;
            break;
        case 'K':
            p->piece[fl + (rk << 3)] = King;
            SetBit(p->mask[White][0], fl + (rk << 3));
            fl++;
            break;
        case 'p':
            p->piece[fl + (rk << 3)] = -Pawn;
            SetBit(p->mask[Black][0], fl + (rk << 3));
            fl++;
            break;
        case 'n':
            p->piece[fl + (rk << 3)] = -Knight;
            SetBit(p->mask[Black][0], fl + (rk << 3));
            fl++;
            break;
        case 'b':
            p->piece[fl + (rk << 3)] = -Bishop;
            SetBit(p->mask[Black][0], fl + (rk << 3));
            fl++;
            break;
        case 'r':
            p->piece[fl + (rk << 3)] = -Rook;
            SetBit(p->mask[Black][0], fl + (rk << 3));
            fl++;
            break;
        case 'q':
            p->piece[fl + (rk << 3)] = -Queen;
            SetBit(p->mask[Black][0], fl + (rk << 3));
            fl++;
            break;
        case 'k':
            p->piece[fl + (rk << 3)] = -King;
            SetBit(p->mask[Black][0], fl + (rk << 3));
            fl++;
            break;
        case '/':
            fl = 0;
            rk--;
            break;
        case ' ':
            rk = -1;
        }
        x++;
    }

    /* scan p->turn to move */
    if (*x == 'w') {
        p->turn = White;
    } else {
        p->turn = Black;
    }

    /* skip white space */
    while (*(++x) == ' ')
        ;

    /* scan castling status */
    p->castle = 0;
    if (*x != '-') {
        if (*x == 'K') {
            p->castle |= CastleMask[White][0];
            x++;
        }
        if (*x == 'Q') {
            p->castle |= CastleMask[White][1];
            x++;
        }
        if (*x == 'k') {
            p->castle |= CastleMask[Black][0];
            x++;
        }
        if (*x == 'q') {
            p->castle |= CastleMask[Black][1];
            x++;
        }
    }

    /* skip white space */
    while (*(++x) == ' ')
        ;

    /* scan enpassant status */
    p->enPassant = 0;
    if (*x != '-') {
        p->enPassant = *x - 'a' + ((*(x + 1) - '1') << 3);
        x++;
    }

    /* skip white space */
    while (*(++x) == ' ')
        ;

    RecalcAttacks(p);
    p->ply = 0;

    i = 0;
    ops[i] = strtok(x, ";");
    while (ops[i]) {
        i++;
        if (i >= MAX_EPD_OPS)
            break;
        ops[i] = strtok(NULL, ";");
    }

    goodmove[0] = M_NONE;
    badmove[0] = M_NONE;

    for (i = 0; ops[i] && i < (MAX_EPD_OPS - 1); i++) {
        char *op = strtok(ops[i], " ");

        if (op) {
            if (!strcmp(op, "bm")) {
                int cnt = 0;

                while ((op = strtok(NULL, " "))) {
                    int mv = ParseSAN(p, op);
                    if (mv != 0) {
                        goodmove[cnt] = mv;
                        Print(0, "best move is %s\n",
                              SAN(p, goodmove[cnt], san_buffer));
                        cnt++;
                        if (cnt >= MAX_EPD_MOVES - 1)
                            break;
                    }
                }
                goodmove[cnt] = M_NONE;
            } else if (!strcmp(op, "am")) {
                int cnt = 0;

                while ((op = strtok(NULL, " "))) {
                    int mv = ParseSAN(p, op);
                    if (mv != 0) {
                        badmove[cnt] = mv;
                        Print(0, "bad move is %s\n",
                              SAN(p, badmove[cnt], san_buffer));
                        cnt++;
                        if (cnt >= MAX_EPD_MOVES - 1)
                            break;
                    }
                }
                badmove[cnt] = M_NONE;
            }
        }
    }

    /* free the memory allocated
     */

    free(line);
}

/**
 * Create an EPD of the current position
 */

char *MakeEPD(struct Position *p) {
    static char epdbuffer[2048];
    char wname[] = " PNBRQK";
    char bname[] = " pnbrqk";
    char san_buffer[16];

    char *x = epdbuffer;
    int i, j, cnt;

    for (i = 7; i >= 0; i--) {
        cnt = 0;
        for (j = 0; j < 8; j++) {
            if (p->piece[i * 8 + j] == Neutral) {
                cnt++;
                if (j == 7)
                    *(x++) = '0' + cnt;
            } else {
                if (cnt)
                    *(x++) = '0' + cnt;
                cnt = 0;
                if (p->piece[i * 8 + j] > 0)
                    *(x++) = wname[TYPE(p->piece[i * 8 + j])];
                else
                    *(x++) = bname[TYPE(p->piece[i * 8 + j])];
            }
        }
        if (i == 0)
            *(x++) = ' ';
        else
            *(x++) = '/';
    }
    if (p->turn == White)
        *(x++) = 'w';
    else
        *(x++) = 'b';
    *(x++) = ' ';

    if (p->castle & CastleMask[White][0])
        *(x++) = 'K';
    if (p->castle & CastleMask[White][1])
        *(x++) = 'Q';
    if (p->castle & CastleMask[Black][0])
        *(x++) = 'k';
    if (p->castle & CastleMask[Black][1])
        *(x++) = 'q';
    if (!p->castle)
        *(x++) = '-';
    *(x++) = ' ';

    if (p->enPassant) {
        *(x++) = 'a' + (p->enPassant & 7);
        *(x++) = '1' + (p->enPassant >> 3);
    } else
        *(x++) = '-';
    *(x++) = '\0';

    if (goodmove[0] != M_NONE) {
        int i;
        strcat(epdbuffer, " bm");
        for (i = 0; goodmove[i] != M_NONE; i++) {
            strcat(epdbuffer, " ");
            strcat(epdbuffer, SAN(p, goodmove[i], san_buffer));
        }
        strcat(epdbuffer, ";");
    }

    if (badmove[0] != M_NONE) {
        int i;
        strcat(epdbuffer, " am");
        for (i = 0; badmove[i] != M_NONE; i++) {
            strcat(epdbuffer, " ");
            strcat(epdbuffer, SAN(p, badmove[i], san_buffer));
        }
        strcat(epdbuffer, ";");
    }
    return epdbuffer;
}

/*
 * Check if game is technically ended.
 *
 * Returns NULL if not, otherwise a descriptive string.
 *
 */

const char *GameEnd(struct Position *p) {
    if (p->actLog->gl_IrrevCount >= 100) {
        return "1/2-1/2 {50 move rule}";
    }

    if (Repeated(p, true) >= 2) {
        return "1/2-1/2 {Draw by repetition}";
    }

    if (p->material[White] == 0 && p->material[Black] == 0) {
        return "1/2-1/2 {Insufficient material}";
    }

    if (!LegalMoves(p, NULL)) {
        if (InCheck(p, p->turn)) {
            if (p->turn == Black) {
                return "1-0 {White mates}";
            } else {
                return "0-1 {Black mates}";
            }
        } else {
            return "1/2-1/2 {Stalemate}";
        }
    }

    return NULL;
}

/*
 * Check if this is a theoretical draw
 */

bool CheckDraw(const struct Position *p) {
    if (p->material[Black] == 0) {
        if (p->nonPawn[White] == 0) {
            if (!(p->mask[White][Pawn] & NotAFileMask)) {
                if (p->mask[Black][King] & CornerMaskA8)
                    return true;
            }
            if (!(p->mask[White][Pawn] & NotHFileMask)) {
                if (p->mask[Black][King] & CornerMaskH8)
                    return true;
            }
        } else if (p->nonPawn[White] == BISHOP_Value &&
                   p->mask[White][Bishop]) {
            if (!(p->mask[White][Pawn] & NotAFileMask) &&
                (p->mask[Black][King] & CornerMaskA8)) {
                if (p->mask[White][Bishop] & BlackSquaresMask)
                    return true;
            }
            if (!(p->mask[White][Pawn] & NotHFileMask) &&
                (p->mask[Black][King] & CornerMaskH8)) {
                if (p->mask[White][Bishop] & WhiteSquaresMask)
                    return true;
            }
        }
    }
    if (p->material[White] == 0) {
        if (p->nonPawn[Black] == 0) {
            if (!(p->mask[Black][Pawn] & NotAFileMask)) {
                if (p->mask[White][King] & CornerMaskA1)
                    return true;
            }
            if (!(p->mask[Black][Pawn] & NotHFileMask)) {
                if (p->mask[White][King] & CornerMaskH1)
                    return true;
            }
        } else if (p->nonPawn[Black] == BISHOP_Value &&
                   p->mask[Black][Bishop]) {
            if (!(p->mask[Black][Pawn] & NotAFileMask) &&
                (p->mask[White][King] & CornerMaskA1)) {
                if (p->mask[Black][Bishop] & WhiteSquaresMask)
                    return true;
            }
            if (!(p->mask[Black][Pawn] & NotHFileMask) &&
                (p->mask[White][King] & CornerMaskH1)) {
                if (p->mask[Black][Bishop] & BlackSquaresMask)
                    return true;
            }
        }
    }
    return false;
}

/*
 * Check if the pawn is passed
 */

bool IsPassed(const struct Position *p, int sq, int side) {
    if (side == White)
        return !(p->mask[Black][Pawn] & PassedMaskW[sq]);
    else
        return !(p->mask[White][Pawn] & PassedMaskB[sq]);
}

/**
 * Create a position from an EPD
 */

struct Position *CreatePositionFromEPD(char *epd) {
    struct Position *p = calloc(sizeof(struct Position), 1);
    if (!p) {
        Print(0, "Cannot allocate Position.\n");
        exit(1);
    }
    p->gameLogSize = INITIAL_GAME_LOG_SIZE;
    p->gameLog = calloc(sizeof(struct GameLog), p->gameLogSize);
    if (!p->gameLog) {
        Print(0, "Cannot allocate GameLog.\n");
        exit(1);
    }
    p->actLog = p->gameLog;
    ReadEPD(p, epd);
    p->actLog->gl_IrrevCount = 0;

    /* default for book usage is no book */
    p->outOfBookCnt[White] = p->outOfBookCnt[Black] = 3;

    return p;
}

/**
 * Create a position in the usual starting position
 */

struct Position *InitialPosition(void) {
    struct Position *p = CreatePositionFromEPD(
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -");

    /* we are 'in book' in the InitalPosition */
    p->outOfBookCnt[White] = p->outOfBookCnt[Black] = 0;

    return p;
}

struct Position *ClonePosition(struct Position *src) {
    struct Position *p = calloc(sizeof(struct Position), 1);
    if (!p) {
        Print(0, "Cannot allocated Position.\n");
        exit(1);
    }
    memcpy(p, src, sizeof(struct Position));

    p->gameLogSize = src->gameLogSize;
    p->gameLog = calloc(sizeof(struct GameLog), p->gameLogSize);
    if (!p->gameLog) {
        Print(0, "Cannot allocate GameLog.\n");
        exit(1);
    }
    memcpy(p->gameLog, src->gameLog, sizeof(struct GameLog) * p->gameLogSize);

    p->actLog = p->gameLog + (src->actLog - src->gameLog);

    return p;
}

/**
 * Release the resources connected with a Position
 */

void FreePosition(struct Position *p) {
    if (p) {
        free(p->gameLog);
        free(p);
    }
}
