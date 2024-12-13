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
 * search.c - tree searching routines
 */

#include "amy.h"

#define NULLMOVE 1
#define FUTILITY 1
#define EXTENDED_FUTILITY 1
#define RAZORING 1

#define REVERSE "\x1B[7m"
#define NORMAL "\x1B[0m"

#define MAX_DEFERRED 128

/*
 * We use fractional ply extensions.
 * See D. Levy, D. Broughton and M. Taylor: The SEX Algorithm in Computer Chess
 * ICCA Journal, Volume 2, No. 1, pp. 10-22.
 */

enum {
    OnePly = 16,

    /*
     * Check extensions. Every check is extended one ply. Additional extensions
     * are awarded if there is only one legal reply or if it is a double or
     * discovered check.
     */

    ExtendInCheck = 14,
    ExtendDoubleCheck = 4,
    ExtendDiscoveredCheck = 4,
    ExtendSingularReply = 12,

    /*
     * A passed pawn push to the seventh rank is extended.
     */

    ExtendPassedPawn = 14,
    ExtendZugzwang = 12,

    /*
     * The tree below a null move is searched with reduced search depth.
     */

    ReduceNullMove = 48,
    ReduceNullMoveDeep = 65
};

/*
 * Captures and recaptures are extended.
 */

static const int ExtendRecapture[] = {0, 4, 6, 6, 8, 10};

static const int PVWindow = 250;
static const int ResearchWindow = 1500;

static const int MateDepth = 3;

/**
 * search tree data
 */

int MaxDepth;

unsigned long RCExt, ChkExt, DiscExt, DblExt, SingExt, PPExt, ZZExt;
unsigned int HardLimit, SoftLimit, SoftLimit2;
unsigned int StartTime, WallTimeStart;
unsigned int CurTime;
unsigned int FHTime;
int AbortSearch;
bool NeedTime = false;
int PrintOK;
int MaxSearchDepth = MAX_TREE_SIZE - 1;
int DoneAtRoot;
static int EGTBDepth = 0;

static int NodesPerCheck;

static OPTIONAL_ATOMIC unsigned long TotalNodes;

/*
 * Search stati
 */

enum {
    Searching = 1,
    Pondering = 2,
    Puzzling = 3,
    Analyzing = 4,
    Interrupted = 5
};

enum { PVNode = 0, AllNode = 1, CutNode = 2, CutNodeNoNull = 3 };

static int SearchMode = Searching;

/* Permanent Brain Variables */
int PBMove, PBActMove;
int PBHit;
int PBAltMove;

char BestLine[2048];
char ShortBestLine[2048];
char AnalysisLine[4096];

OPTIONAL_ATOMIC unsigned long HTry, HHit, PTry, PHit, STry, SHit;

/* prototypes for search routines */

static int quies(struct SearchData *, int, int, int);
#if MP
static int negascout(struct SearchData *, int, int, int, int, int);
#else
static int negascout(struct SearchData *, int, int, int, int);
#endif

/* search routines */

/*
 * Check if search should be terminated
 *
 * Here we also handle the case that we are in Permanent Brain and have to
 * check for user input.
 *
 */
static bool TerminateSearch(struct SearchData *sd) {
    if ((sd->nodes_cnt + sd->qnodes_cnt) > sd->check_nodes_cnt) {
        unsigned int now = GetTime();

        sd->check_nodes_cnt = sd->nodes_cnt + sd->qnodes_cnt + NodesPerCheck;
        if (AbortSearch)
            return true;

        CurTime = now;
        if (CurTime > (StartTime + ONE_SECOND))
            PrintOK = true;

        if (sd->master && InputReady()) {
            char buffer[64];
            struct Command *theCommand;

            ReadLine(buffer, 64);

            /*
             * the '.' command can only be handled here
             */

            if (buffer[0] == '.') {
                PrintNoLog(0, "stat01: %d %ld %d %d %d\n",
                           (CurTime - StartTime), TotalNodes, sd->depth,
                           sd->nrootmoves - sd->movenum - 1, sd->nrootmoves);
            }

            theCommand = ParseInput(buffer);

            if (theCommand) {
                if (SearchMode == Pondering && theCommand->move != M_NONE) {
                    if (theCommand->move == PBActMove) {
                        PBHit = true;
                        SearchMode = Searching;
                        Print(1, "OK!\n");
                        WallTimeStart = now;

                        if (CurTime >= HardLimit)
                            return true;
                        if (DoneAtRoot)
                            return true;

                        return false;
                    } else {
                        PBHit = false;
                        PBAltMove = theCommand->move;
                        return true;
                    }
                }

                if (SearchMode == Puzzling && theCommand->move != M_NONE) {
                    PBAltMove = theCommand->move;
                    return true;
                }

                if (SearchMode == Analyzing && theCommand->move != M_NONE) {
                    ExecuteCommand(theCommand);
                    return true;
                }

                if (theCommand->allowed_during_search) {
                    ExecuteCommand(theCommand);

                    if (theCommand->interrupts_search) {
                        SearchMode = Interrupted;
                        return true;
                    }
                }
            }
        }

        if (SearchMode == Searching) {
            if (CurTime >= HardLimit)
                return true;
        }
    }
    return false;
}

/*
 * Support routine for recpature extensions
 */

static int IsRecapture(int piece1, int piece2) {
    switch (TYPE(piece1)) {
    case Knight:
    case Bishop:
        return (TYPE(piece2) == Knight || TYPE(piece2) == Bishop);
    default:
        return TYPE(piece1) == TYPE(piece2);
    }
}

/*
 * Decide wether to extend the check due to the following conditions:
 *  - double check
 *  - discovered check
 *  - check with only one legal response
 *
 */

static int CheckExtend(struct Position *p) {
    int kp = p->kingSq[p->turn];
    BitBoard att;

    att = p->atkFr[kp] & p->mask[OPP(p->turn)][0];

    if (CountBits(att) > 1) {

        /*
         * double check, the king has to move
         * count no. of flight squares, if only one, extend deeper
         *
         */

        BitBoard ff;

        int i;
        int cnt = 0;

        DblExt++;

        ff = KingEPM[kp] & ~p->mask[p->turn][0];
        att &= p->slidingPieces;

        while (att) {
            i = FindSetBit(att);
            att &= att - 1;
            ff &= ~Ray[i][kp];
        }

        while (ff) {
            i = FindSetBit(ff);
            ff &= ff - 1;
            if (!(p->atkFr[i] & p->mask[OPP(p->turn)][0]))
                cnt++;
            if (cnt > 1)
                return ExtendDoubleCheck;
        }
    } else {
        BitBoard ff;
        BitBoard def;
        BitBoard tmp;

        int atp = FindSetBit(att);
        int cnt = 0;
        int i;
        int nd = 0;

        /* discovered check */
        if (atp != M_TO((p->actLog - 1)->gl_Move)) {
            DiscExt++;
            nd = ExtendDiscoveredCheck;
        }

        ff = KingEPM[kp] & ~p->mask[p->turn][0];

        i = FindSetBit(att);
        if (TstBit(p->slidingPieces, i))
            ff &= ~Ray[i][kp];

        /* check for king flight squares */
        while (ff) {
            i = FindSetBit(ff);
            ff &= ff - 1;
            if (!(p->atkFr[i] & p->mask[OPP(p->turn)][0]))
                cnt++;
            if (cnt > 1)
                return nd;
        }

        /* Find all non-pinned defenders */
        def = p->mask[p->turn][0] & ~p->mask[p->turn][King];

        tmp = (p->mask[OPP(p->turn)][Bishop] | p->mask[OPP(p->turn)][Queen]) &
              BishopEPM[kp];
        while (tmp) {
            BitBoard tmp2;
            i = FindSetBit(tmp);
            tmp &= tmp - 1;
            tmp2 = InterPath[i][kp];
            if (tmp2 && !(p->mask[OPP(p->turn)][0] & tmp2)) {
                tmp2 &= p->mask[p->turn][0];
                if (CountBits(tmp2) == 1) {
                    ClrBit(def, FindSetBit(tmp2));
                }
            }
        }

        tmp = (p->mask[OPP(p->turn)][Rook] | p->mask[OPP(p->turn)][Queen]) &
              RookEPM[kp];
        while (tmp) {
            BitBoard tmp2;
            i = FindSetBit(tmp);
            tmp &= tmp - 1;
            tmp2 = InterPath[i][kp];
            if (tmp2 && !(p->mask[OPP(p->turn)][0] & tmp2)) {
                tmp2 &= p->mask[p->turn][0];
                if (CountBits(tmp2) == 1) {
                    ClrBit(def, FindSetBit(tmp2));
                }
            }
        }

        /* All non-pinned defenders are in 'def' */
        tmp = p->atkFr[atp] & def;

        cnt += CountBits(tmp);
        if (cnt > 1)
            return nd;

        /* if possible, try an interposition */
        if (TstBit(p->slidingPieces, atp)) {
            tmp = InterPath[atp][kp];
            while (tmp) {
                BitBoard tmp2;
                i = FindSetBit(tmp);
                tmp &= tmp - 1;
                if ((tmp2 = p->atkFr[i] & def)) {
                    cnt += CountBits(tmp2);
                }
                if (p->turn == White && (i - 8) > 0 &&
                    TstBit(p->mask[White][Pawn], i - 8) && TstBit(def, i - 8))
                    cnt++;
                if (p->turn == Black && (i + 8) < 64 &&
                    TstBit(p->mask[Black][Pawn], i + 8) && TstBit(def, i + 8))
                    cnt++;
                if (cnt > 1)
                    return nd;
            }
        }
    }

    /* If we get here, we have only one legal move. */

    SingExt++;
    return ExtendSingularReply;
}

/*
 * Compute an optimistic score for a move.
 */

static int ScoreMove(struct Position *p, move_t move) {
    int score = 0;

    if (move & M_CAPTURE)
        score += Value[TYPE(p->piece[M_TO(move)])];
    if (move & M_PANY)
        score += Value[PromoType(move)] - Value[Pawn];
    else if (TYPE(p->piece[M_FROM(move)]) == Pawn) {
        if (p->turn == White && M_TO(move) >= a7) {
            score += Value[Bishop];
        }
        if (p->turn == Black && M_TO(move) <= h2) {
            score += Value[Bishop];
        }
    }

    if (move & M_ENPASSANT)
        score += Value[Pawn];

    return score;
}

/*
 * Store the result of the full width search
 */

static void StoreResult(struct SearchData *sd, int score, int alpha, int beta,
                        move_t move, int depth, int threat) {
    struct Position *p = sd->position;

    if (!(move & M_TACTICAL) && score > alpha) {
        sd->historyTab[p->turn][move & 4095] += depth * depth;
    }

    StoreHT(p->hkey, score, alpha, beta, move, depth, threat, sd->ply
#if MP
            ,
            sd->localHashTable
#endif
    );
}

/*
 * The quiescence search.
 *
 * we only do a full width search if the side to move was in check since
 * the horizon, otherwise we do only a capture search.
 *
 */

static int quies(struct SearchData *sd, int alpha, int beta, int depth) {
    struct Position *p = sd->position;
    int best;
    move_t move;
    int talpha;
    int tmp;

    EnterNode(sd);

    sd->qnodes_cnt++;
    TotalNodes++;

    /* max search depth reached */
    if (sd->ply >= MaxDepth || Repeated(p, false)) {
        best = 0;
        goto EXIT;
    }

    /*
     * Probe recognizers. If the probe is successful, use the
     * recognizer score as evaluation score.
     *
     * Otherwise, use ScorePosition()
     */

    switch (ProbeRecognizer(p, &tmp)) {
    case ExactScore:
        best = tmp;
        goto EXIT;
    case LowerBound:
        best = tmp;
        if (best >= beta) {
            goto EXIT;
        }
        break;
    case UpperBound:
        best = tmp;
        if (best <= alpha) {
            goto EXIT;
        }
        break;
    default:
        best = ScorePosition(p);
        break;
    }

    if (best >= beta) {
        goto EXIT;
    }

    talpha = MAX(alpha, best);

    while ((move = NextMoveQ(sd, alpha)) != M_NONE) {
        DoMove(p, move);
        if (InCheck(p, OPP(p->turn)))
            UndoMove(p, move);
        else {
            tmp = -quies(sd, -beta, -talpha, depth - 1);
            UndoMove(p, move);
            if (tmp >= beta) {
                best = tmp;
                goto EXIT;
            }
            if (tmp > best) {
                best = tmp;
                if (best > talpha) {
                    talpha = best;
                }
            }
        }
    }

EXIT:

    LeaveNode(sd);
    return best;
}

/*
 * The main negascout routine with handles things like
 * search extensions, hash table lookup etc.
 *
 * It recursively calls itself until depth is exhausted.
 * Than `quies` is called.
 *
 * This is modified to perform an 'ABDADA' search if MP is defined.
 * See Jean-Christophe Weill, "The ABDADA Distributed Minimax-Search Algorithm"
 * ICCA Journal, Volume 19, No. 1, pp. 3-16
 */

static int negascout(struct SearchData *sd, int alpha, int beta,
                     const int depth, int node_type
#if MP
                     ,
                     int exclusiveP
#endif /* MP  */
) {
    struct Position *p = sd->position;
    struct SearchStatus *st;
    int best = -INF;
    move_t bestm = M_NONE;
    int tmp;
    int talpha;
    int incheck;
    int lmove;
    move_t move;
    int extend = 0;
    bool threat = false;
    int reduce_extensions;
    int next_type;
    bool was_futile = false;
#if FUTILITY
    int is_futile;
    int optimistic = 0;
#endif

#if MP
    int deferred_cnt = 0;
    int deferred_list[MAX_DEFERRED];
    int deferred_depth[MAX_DEFERRED];
#endif

    EnterNode(sd);

    sd->nodes_cnt++;
    TotalNodes++;

    /* check for search termination */
    if (sd->master && TerminateSearch(sd)) {
        AbortSearch = true;
        goto EXIT;
    }

    /* max search depth reached */
    if (sd->ply >= MaxDepth)
        goto EXIT;

    /*
     * Check for insufficent material or theoretical draw.
     */

    if (/* InsufMat(p) || CheckDraw(p) || */ Repeated(p, false)) {
        best = 0;
        goto EXIT;
    }

    /*
     * check extension
     */

    incheck = InCheck(p, p->turn);
    if (incheck && p->material[p->turn] > 0) {
        extend += CheckExtend(p);
        ChkExt++;
    }

    /*
     * Check the hashtable
     */

    st = sd->current;

    HTry++;
#if MP
    switch (ProbeHT(p->hkey, &tmp, depth, &(st->st_hashmove), &threat, sd->ply,
                    exclusiveP, sd->localHashTable))
#else
    switch (ProbeHT(p->hkey, &tmp, depth, &(st->st_hashmove), &threat, sd->ply))
#endif /* MP */
    {
    case ExactScore:
        HHit++;
        best = tmp;
        goto EXIT;
    case UpperBound:
        if (tmp <= alpha) {
            HHit++;
            best = tmp;
            goto EXIT;
        }
        break;
    case LowerBound:
        if (tmp >= beta) {
            HHit++;
            best = tmp;
            goto EXIT;
        }
        break;
    case Useless:
        threat = !incheck && MateThreat(p, OPP(p->turn));
        break;
#if MP
    case OnEvaluation:
        best = -ON_EVALUATION;
        goto EXIT;
#endif
    }

    /*
     * Probe EGTB
     */

    if (depth > EGTBDepth && ProbeEGTB(p, &tmp, sd->ply)) {
        best = tmp;
        goto EXIT;
    }

    /*
     * Probe recognizers
     */

    switch (ProbeRecognizer(p, &tmp)) {
    case ExactScore:
        best = tmp;
        goto EXIT;
    case LowerBound:
        if (tmp >= beta) {
            best = tmp;
            goto EXIT;
        }
        break;
    case UpperBound:
        if (tmp <= alpha) {
            best = tmp;
            goto EXIT;
        }
        break;
    }

#if NULLMOVE

    /*
     * Null move search.
     * See Christian Donninger, "Null Move and Deep Search"
     * ICCA Journal Volume 16, No. 3, pp. 137-143
     */

    if (!incheck && node_type == CutNode && !threat) {
        int next_depth;
        int nms;

        next_depth = depth - ReduceNullMove;

        if (next_depth > 0) {
            next_depth = depth - ReduceNullMoveDeep;
        }

        DoNull(p);
        if (next_depth < 0) {
            nms = -quies(sd, -beta, -beta + 1, 0);
        } else {
#if MP
            nms = -negascout(sd, -beta, -beta + 1, next_depth, AllNode, 0);
#else
            nms = -negascout(sd, -beta, -beta + 1, next_depth, AllNode);
#endif
        }
        UndoNull(p);

        if (AbortSearch)
            goto EXIT;
        if (nms >= beta) {
            if (p->nonPawn[p->turn] >= Value[Queen]) {
                best = nms;
                goto EXIT;
            } else {
                if (next_depth < 0) {
                    nms = quies(sd, beta - 1, beta, 0);
                } else {
#if MP
                    nms = negascout(sd, beta - 1, beta, next_depth,
                                    CutNodeNoNull, 0);
#else
                    nms = negascout(sd, beta - 1, beta, next_depth,
                                    CutNodeNoNull);
#endif
                }

                if (nms >= beta) {
                    best = nms;
                    goto EXIT;
                } else {
                    extend += ExtendZugzwang;
                    ZZExt++;
                }
            }
        } else if (nms <= -CMLIMIT) {
            threat = true;
        }
    }
#endif /* NULLMOVE */

    lmove = (p->actLog - 1)->gl_Move;
    reduce_extensions = (sd->ply > 2 * sd->depth);
    talpha = alpha;

    switch (node_type) {
    case AllNode:
        next_type = CutNode;
        break;
    case CutNode:
    case CutNodeNoNull:
        next_type = AllNode;
        break;
    default:
        next_type = PVNode;
        break;
    }

#if FUTILITY
    is_futile = !incheck && !threat && alpha < CMLIMIT && alpha > -CMLIMIT;
    if (is_futile) {
        if (p->turn == White) {
            optimistic = MaterialBalance(p) + MaxPos;
        } else {
            optimistic = -MaterialBalance(p) + MaxPos;
        }
    }
#endif /* FUTILITY */

    /*
     * Internal iterative deepening. If we do not have a move, we try
     * a shallow search to find a good candidate.
     */

    if (depth > 2 * OnePly && ((alpha + 1) != beta) &&
        !LegalMove(p, st->st_hashmove)) {
#if MP
        negascout(sd, alpha, beta, depth - 2 * OnePly, PVNode, 0);
#else
        negascout(sd, alpha, beta, depth - 2 * OnePly, PVNode);
#endif
        st->st_hashmove = sd->pv_save[sd->ply + 1];
    }

    /*
     * Search all legal moves
     */

    while ((move = incheck ? NextEvasion(sd) : NextMove(sd)) != M_NONE) {
        int next_depth = extend;

        if (move & M_CANY && !MayCastle(p, move))
            continue;

        /*
         * recapture extension
         */

        if ((move & M_CAPTURE) && (lmove & M_CAPTURE) &&
            M_TO(move) == M_TO(lmove) &&
            IsRecapture(p->piece[M_TO(move)], (p->actLog - 1)->gl_Piece)) {
            RCExt += 1;
            next_depth += ExtendRecapture[TYPE(p->piece[M_TO(move)])];
        }

        /*
         * passed pawn push extension
         */

        if (TYPE(p->piece[M_FROM(move)]) == Pawn &&
            p->nonPawn[OPP(p->turn)] <= Value[Queen]) {

            int to = M_TO(move);

            if (((p->turn == White && to >= a7) ||
                 (p->turn == Black && to <= h2)) &&
                IsPassed(p, to, p->turn) && SwapOff(p, move) >= 0) {
                next_depth += ExtendPassedPawn;
                PPExt += 1;
            }
        }

        /*
         * limit extensions to sensible range.
         */

        if (reduce_extensions)
            next_depth /= 2;

        next_depth += depth - OnePly;

#if FUTILITY

        /*
         * Futility cutoffs
         */

        if (is_futile) {
            if (next_depth < 0 && !IsCheckingMove(p, move)) {
                tmp = optimistic + ScoreMove(p, move);
                if (tmp <= alpha) {
                    if (tmp > best) {
                        best = tmp;
                        bestm = move;
                        was_futile = true;
                    }
                    continue;
                }
            }
#if EXTENDED_FUTILITY

            /*
             * Extended futility cutoffs and limited razoring.
             * See Ernst A. Heinz, "Extended Futility Pruning"
             * ICCA Journal Volume 21, No. 2, pp 75-83
             */

            else if (next_depth >= 0 && next_depth < OnePly &&
                     !IsCheckingMove(p, move)) {
                tmp = optimistic + ScoreMove(p, move) + (3 * Value[Pawn]);
                if (tmp <= alpha) {
                    if (tmp > best) {
                        best = tmp;
                        bestm = move;
                        was_futile = true;
                    }
                    continue;
                }
            }
#if RAZORING
            else if (next_depth >= OnePly && next_depth < 2 * OnePly &&
                     !IsCheckingMove(p, move)) {
                tmp = optimistic + ScoreMove(p, move) + (6 * Value[Pawn]);
                if (tmp <= alpha) {
                    next_depth -= OnePly;
                }
            }
#endif /* RAZORING */
#endif /* EXTENDED_FUTILITY */
        }

#endif /* FUTILITY */

        DoMove(p, move);
        if (InCheck(p, OPP(p->turn))) {
            UndoMove(p, move);
        } else {
            /*
             * Check extension
             */

            if (p->material[p->turn] > 0 && InCheck(p, p->turn)) {
                next_depth +=
                    (reduce_extensions) ? ExtendInCheck >> 1 : ExtendInCheck;
            }

            /*
             * Recursively search this position. If depth is exhausted, use
             * quies, otherwise use negascout.
             */

            if (next_depth < 0) {
                tmp = -quies(sd, -beta, -talpha, 0);
            } else if (bestm != M_NONE && !was_futile) {
#if MP
                tmp = -negascout(sd, -talpha - 1, -talpha, next_depth,
                                 next_type, bestm != M_NONE);
                if (tmp != ON_EVALUATION && tmp > talpha && tmp < beta) {
                    tmp = -negascout(sd, -beta, -tmp, next_depth,
                                     node_type == PVNode ? PVNode : AllNode,
                                     bestm != M_NONE);
                }
#else
                tmp =
                    -negascout(sd, -talpha - 1, -talpha, next_depth, next_type);
                if (tmp > talpha && tmp < beta) {
                    tmp = -negascout(sd, -beta, -tmp, next_depth,
                                     node_type == PVNode ? PVNode : AllNode);
                }
#endif /* MP */
            } else {
#if MP
                tmp = -negascout(sd, -beta, -talpha, next_depth, next_type,
                                 bestm != M_NONE);
#else
                tmp = -negascout(sd, -beta, -talpha, next_depth, next_type);
#endif /* MP */
            }

            UndoMove(p, move);

            if (AbortSearch)
                goto EXIT;

#if MP
            if (tmp == ON_EVALUATION) {

                /*
                 * This child is ON_EVALUATION. Remember move and
                 * depth.
                 */

                deferred_list[deferred_cnt] = move;
                deferred_depth[deferred_cnt] = next_depth;
                deferred_cnt++;

            } else {
#endif /* MP */

                /*
                 * beta cutoff, enter move in Killer/Countermove table
                 */

                if (tmp >= beta) {
                    if (!(move & M_TACTICAL)) {
                        PutKiller(sd, move);
                        sd->counterTab[p->turn][lmove & 4095] = move;
                    }
                    StoreResult(sd, tmp, alpha, beta, move, depth, threat);
                    best = tmp;
                    goto EXIT;
                }

                /*
                 * Improvement on best move to date
                 */

                if (tmp > best) {
                    best = tmp;
                    bestm = move;
                    was_futile = false;

                    if (best > talpha) {
                        talpha = best;
                    }
                }

                next_type = CutNode;
#if MP
            }
#endif /* MP */
        }
    }

#if MP

    /*
     * Now search all moves which were ON_EVALUATION in pass one.
     */

    while (deferred_cnt) {
        int next_depth = deferred_depth[--deferred_cnt];
        move = deferred_list[deferred_cnt];

        DoMove(p, move);

        tmp = -negascout(sd, -talpha - 1, -talpha, next_depth, next_type, 0);

        if (tmp == ON_EVALUATION) {
            printf("Oops...\n");
        }

        if (tmp > talpha && tmp < beta) {
            tmp = -negascout(sd, -beta, -talpha, next_depth,
                             node_type == PVNode ? PVNode : AllNode, 0);
        }

        UndoMove(p, move);

        /*
         * beta cutoff, enter move in Killer/Countermove table
         */

        if (tmp >= beta) {
            if (!(move & M_TACTICAL)) {
                PutKiller(sd, move);
                sd->counterTab[p->turn][lmove & 4095] = move;
            }
            StoreResult(sd, tmp, alpha, beta, move, depth, threat);
            best = tmp;
            goto EXIT;
        }

        /*
         * Improvement on best move to date
         */

        if (tmp > best) {
            best = tmp;
            bestm = move;
            was_futile = false;

            if (best > talpha) {
                talpha = best;
            }
        }

        next_type = CutNode;
    }
#endif /* MP */

    /*
     * If we get here, no legal move was found.
     * Score this position as mate or draw.
     */

    if (bestm == M_NONE) {
        if (incheck)
            best = -INF + sd->ply;
        else
            best = 0;
    }

    if (!was_futile) {
        StoreResult(sd, best, alpha, beta, bestm, depth, threat);
    }

EXIT:

    if (node_type == PVNode) {
        sd->pv_save[sd->ply] = bestm;
    }

    LeaveNode(sd);
    return best;
}

/**
 * Print the SAN of a move prefixed by the move number.
 */
static char *NumberedSAN(struct Position *p, int move, char *buffer,
                         size_t len) {
    char san_buffer[16];
    if (p->turn == White)
        snprintf(buffer, len, "%d. %s", 1 + (p->ply + 1) / 2,
                 SAN(p, move, san_buffer));
    else
        snprintf(buffer, len, "%d. .. %s", 1 + p->ply / 2,
                 SAN(p, move, san_buffer));

    return buffer;
}

/*
 * Analyze the hashtable to find the principal variation.
 */

static void AnaLoop(struct Position *p, int depth) {
    move_t move;
    bool dummy = false;
    int score;

#if MP
    if (ProbeHT(p->hkey, &score, 0, &move, &dummy, 0, 0, NULL) == Useless)
        return;
#else
    if (ProbeHT(p->hkey, &score, 0, &move, &dummy, 0) == Useless)
        return;
#endif

    if (Repeated(p, true) >= 2)
        return;

    if (LegalMove(p, move)) {
        int incheck;
        char buffer[16];

        DoMove(p, move);
        incheck = InCheck(p, OPP(p->turn));
        UndoMove(p, move);

        if (p->turn == White) {
            snprintf(buffer, sizeof(buffer), "%d. ", 1 + (p->ply + 1) / 2);
            strcat(BestLine, buffer);
        }

        if (incheck) {
            strcat(BestLine, "<ill>");
            strcat(ShortBestLine, "<ill>");
            return;
        }

        char *san = SAN(p, move, buffer);
        strcat(BestLine, san);
        strcat(BestLine, " ");
        strcat(ShortBestLine, san);
        strcat(ShortBestLine, " ");

        /* save move to ponder on ... */
        if (depth == 1)
            PBMove = move;

        DoMove(p, move);
        AnaLoop(p, depth + 1);
        UndoMove(p, move);
    } else if (move == M_HASHED) {
        strcat(BestLine, "..");
        strcat(ShortBestLine, "..");
    } else if (move == M_NULL) {
        strcat(BestLine, "<null>");
        strcat(ShortBestLine, "<null>");
    }
}

static void AnalyzeHT(struct Position *p, move_t move) {
    NumberedSAN(p, move, BestLine, sizeof(BestLine));
    strcat(BestLine, " ");
    char san_buffer[16];
    strcpy(ShortBestLine, SAN(p, move, san_buffer));
    strcat(ShortBestLine, " ");
    DoMove(p, move);
    AnaLoop(p, 1);
    UndoMove(p, move);
}

/**
 * Initialize the search variables.
 */
static void InitSearch(struct SearchData *sd) {
    sd->ply = 0;
    sd->nodes_cnt = sd->qnodes_cnt = sd->check_nodes_cnt = 0;
    RCExt = ChkExt = DiscExt = DblExt = SingExt = PPExt = ZZExt = 0;
    PrintOK = (SearchMode == Analyzing) ? true : false;
    DoneAtRoot = false;
    EGTBProbe = EGTBProbeSucc = 0;

    /* Initialize scoring tables */

    HTry = HHit = PTry = PHit = STry = SHit = 0;
}

// Marcin Ciura's gap sequence for shell sort
static int gaps[] = {57, 23, 10, 4, 1};

/**
 * Resort the root move list. Keeps the first element unchanged,
 * and sorts the remaining moves by number of nodes searched
 * in decreasing order.
 */
static void ResortMovesList(int cnt, move_t *mvs, unsigned long *nodes) {
    if (cnt <= 0)
        return;

    // Skip over the first element
    cnt -= 1;
    mvs++;
    nodes++;

    for (int gap_index = 0; gap_index < 5; gap_index++) {
        int gap = gaps[gap_index];
        for (int i = gap; i < cnt; i++) {
            int j;
            move_t mvs_tmp = mvs[i];
            unsigned long nodes_tmp = nodes[i];

            for (j = i; (j >= gap) && (nodes[j - gap] < nodes_tmp); j -= gap) {
                nodes[j] = nodes[j - gap];
                mvs[j] = mvs[j - gap];
            }
            nodes[j] = nodes_tmp;
            mvs[j] = mvs_tmp;
        }
    }
}

/*
 * This routine searches a chess position. It uses iterative deepening,
 * aspiration window and scout search.
 */

static void *IterateInt(void *x) {
    int best;
    unsigned long nodes[256];
    int last = 0;
    double elapsed;
    struct SearchData *sd = x;
    struct Position *p;
    bool any_pv_printed = false;
    bool pv_valid = false;

    if (!sd->master) {
        usleep(50 + 100 * Random());
    }
    p = sd->position;

    InitSearch(sd);
    sd->nrootmoves = LegalMoves(p, sd->heap);

    move_t *mvs = sd->heap->data + sd->heap->current_section->start;

    best = p->material[p->turn] - p->material[OPP(p->turn)];

    if (!(mvs[0] & M_TACTICAL))
        PutKiller(sd, mvs[0]);

    MaxDepth = MAX_TREE_SIZE - 1;

    for (sd->depth = 1; sd->depth < MaxSearchDepth; sd->depth++) {
        int alpha = best - PVWindow;
        int beta = best + PVWindow;
        bool is_pv = true;
        bool pv_stable = true;

        for (sd->movenum = 0; sd->movenum < sd->nrootmoves; sd->movenum++) {
            int tmp;
            int next_depth = (sd->depth - 2) * OnePly;
            move_t move = mvs[sd->movenum];

            nodes[sd->movenum] = sd->nodes_cnt;

            if (sd->master && PrintOK) {
                char time_buffer[16];
                char san_buffer[32];

                PrintNoLog(
                    2, "%2d  %s   %2d/%2d  %s      \r", sd->depth,
                    FormatTime(CurTime - StartTime, time_buffer,
                               sizeof(time_buffer)),
                    sd->movenum + 1, sd->nrootmoves,
                    NumberedSAN(p, move, san_buffer, sizeof(san_buffer)));
            }

            DoMove(p, move);
            if (InCheck(p, p->turn))
                next_depth += ExtendInCheck;

            if (next_depth >= 0) {
#if MP
                tmp = -negascout(sd, -beta, -alpha, next_depth,
                                 is_pv ? PVNode : CutNode, 0);
#else
                tmp = -negascout(sd, -beta, -alpha, next_depth,
                                 is_pv ? PVNode : CutNode);
#endif
            } else {
                tmp = -quies(sd, -beta, -alpha, 0);
            }
            UndoMove(p, move);
            if (AbortSearch)
                goto final;

            if (is_pv && tmp <= alpha) {

                /*
                 * Fail low on principal variation.
                 * Open window, take some time, and re-search.
                 */

                pv_stable = false;

                if (sd->master && PrintOK) {
                    char san_buffer[32];
                    SearchOutputFailHighLow(
                        sd->depth, CurTime - StartTime, false,
                        NumberedSAN(p, move, san_buffer, sizeof(san_buffer)),
                        sd->nodes_cnt + sd->qnodes_cnt);
                }

                NeedTime = true;

                beta = tmp;
                alpha = tmp - ResearchWindow;

                DoMove(p, move);
                if (next_depth >= 0) {
#if MP
                    tmp = -negascout(sd, -beta, -alpha, next_depth, PVNode, 0);
#else
                    tmp = -negascout(sd, -beta, -alpha, next_depth, PVNode);
#endif
                } else {
                    tmp = -quies(sd, -beta, -alpha, 0);
                }
                UndoMove(p, move);
                if (AbortSearch)
                    goto final;

                if (tmp <= alpha) {
                    beta = tmp;
                    alpha = -INF;

                    DoMove(p, move);
                    if (next_depth >= 0) {
#if MP
                        tmp = -negascout(sd, -beta, -alpha, next_depth, PVNode,
                                         0);
#else
                        tmp = -negascout(sd, -beta, -alpha, next_depth, PVNode);
#endif
                    } else {
                        tmp = -quies(sd, -beta, -alpha, 0);
                    }
                    UndoMove(p, move);
                    if (AbortSearch)
                        goto final;
                }
                nodes[sd->movenum] = sd->nodes_cnt - nodes[sd->movenum];
            } else if (tmp >= beta) {

                /*
                 * Fail high.
                 * Re-search with open window.
                 */

                pv_stable = false;

                if (sd->movenum != 0) {
                    int tn = nodes[sd->movenum];
                    int j;

                    for (j = sd->movenum; j > 0; j--) {
                        mvs[j] = mvs[j - 1];
                        nodes[j] = nodes[j - 1];
                    }
                    mvs[0] = move;
                    nodes[0] = tn;

                    if (!(move & M_TACTICAL))
                        PutKiller(sd, move);
                    PBMove = M_NONE;
                    is_pv = true;

                    FHTime = (CurTime - StartTime) / ONE_SECOND;
                }

                if (sd->master && PrintOK) {
                    char san_buffer[32];
                    SearchOutputFailHighLow(
                        sd->depth, CurTime - StartTime, true,
                        NumberedSAN(p, mvs[0], san_buffer, sizeof(san_buffer)),
                        sd->nodes_cnt + sd->qnodes_cnt);
                }

                alpha = tmp;
                beta = tmp + ResearchWindow;

                DoMove(p, move);
                if (next_depth >= 0) {
#if MP
                    tmp = -negascout(sd, -beta, -alpha, next_depth, PVNode, 0);
#else
                    tmp = -negascout(sd, -beta, -alpha, next_depth, PVNode);
#endif
                } else {
                    tmp = -quies(sd, -beta, -alpha, 0);
                }
                UndoMove(p, move);
                if (AbortSearch)
                    goto final;

                if (tmp >= beta) {
                    alpha = tmp;
                    beta = INF;

                    DoMove(p, move);
                    if (next_depth >= 0) {
#if MP
                        tmp = -negascout(sd, -beta, -alpha, next_depth, PVNode,
                                         0);
#else
                        tmp = -negascout(sd, -beta, -alpha, next_depth, PVNode);
#endif
                    } else {
                        tmp = -quies(sd, -beta, -alpha, 0);
                    }
                    UndoMove(p, move);
                    if (AbortSearch)
                        goto final;
                }
                nodes[0] = sd->nodes_cnt - nodes[0];
            } else {
                nodes[sd->movenum] = sd->nodes_cnt - nodes[sd->movenum];
            }

            if (AbortSearch)
                goto final;

            if (is_pv) {
                best = tmp;

                if (sd->master) {
                    char score_as_text[16];
                    AnalyzeHT(p, mvs[0]);
                    pv_valid = true;

                    snprintf(
                        AnalysisLine, sizeof(AnalysisLine), "%2d: (%7s) %s",
                        sd->depth,
                        FormatScore(best, score_as_text, sizeof(score_as_text)),
                        BestLine);

                    if (PrintOK) {
                        SearchOutput(sd->depth, CurTime - StartTime,
                                     (p->turn) ? -best : best, BestLine,
                                     sd->nodes_cnt + sd->qnodes_cnt);

                        any_pv_printed = true;
                    }
                }

                alpha = best;
                beta = best + 1;
                is_pv = false;
            }

            if (sd->master && sd->movenum == 0 && !NeedTime &&
                CurTime > SoftLimit) {
                if (SearchMode == Searching) {
                    AbortSearch = true;
                    goto final;
                } else if (SearchMode == Pondering) {
                    DoneAtRoot = true;
                }
            }
        }

        if (sd->master && (PrintOK || (sd->depth > MateDepth &&
                                       (best < -CMLIMIT || best > CMLIMIT)))) {
            SearchOutput(sd->depth, CurTime - StartTime,
                         (p->turn) ? -best : best, BestLine,
                         sd->nodes_cnt + sd->qnodes_cnt);

            any_pv_printed = true;
        }

        if (best < -CMLIMIT || best > CMLIMIT) {
            if (last > CMLIMIT && best >= last && sd->depth > MateDepth) {
                if (SearchMode == Searching)
                    break;
                else
                    DoneAtRoot = true;
            }
            if (SearchMode == Searching && last < CMLIMIT && best <= last &&
                sd->depth > MateDepth)
                break;
            last = best;
        }

        NeedTime = false;
        ResortMovesList(sd->nrootmoves, mvs, nodes);

        /*
            if(Depth > 5 && pv_stable) {
                double pv_percentage;
                int matval = NPVal[Side] / Value[Knight];
                double easy_threshold;
                if(matval > 10) matval = 10;

                easy_threshold = 0.95 - matval * 0.017;

                nodes_per_iteration = Nodes - nodes_per_iteration;
                pv_percentage = (double) nodes[0] /
                                (double) (nodes_per_iteration);

                if(pv_percentage >= easy_threshold) {
                    Print(1, "                    -> easy move\n");

                    SoftLimit = StartTime +
                                2 * (SoftLimit - StartTime) / 3;
                }
            }
        */

        CurTime = GetTime();

        if (sd->depth > 3) {
            /*
             * Do ten checks per second.
             */

            elapsed = (double)(CurTime - StartTime) / (double)ONE_SECOND;

            NodesPerCheck =
                (elapsed == 0.0)
                    ? 1000
                    : (int)((sd->nodes_cnt + sd->qnodes_cnt) / elapsed / 10);
        }

        if (SearchMode == Puzzling && sd->depth > 4)
            break;

        if (sd->master &&
            ((CurTime > SoftLimit) || (pv_stable && CurTime > SoftLimit2))) {
            if (SearchMode == Searching) {
                AbortSearch = true;
                break;
            } else if (SearchMode == Pondering) {
                DoneAtRoot = true;
            }
        }
    }

final:

    if (CurTime <= StartTime)
        StartTime--;
    elapsed = (double)(CurTime - StartTime) / (double)ONE_SECOND;

    if (sd->master) {
        if (pv_valid && !any_pv_printed) {
            // Make sure there is a PV printed
            SearchOutput(sd->depth, CurTime - StartTime,
                         (p->turn) ? -best : best, BestLine,
                         sd->nodes_cnt + sd->qnodes_cnt);
        }

        char buf1[16], buf2[16], buf3[16], buf4[16], buf5[16], buf6[16],
            buf7[16];

        unsigned long nps = (unsigned long)(TotalNodes / elapsed);

        Print(2, "Nodes = %s, QPerc: %d %%, time = %g secs, %s nodes/s\n",
              FormatCount(TotalNodes, buf1, sizeof(buf1)),
              Percentage(sd->qnodes_cnt, sd->nodes_cnt + sd->qnodes_cnt),
              elapsed, FormatCount(nps, buf2, sizeof(buf2)));

        Print(2,
              "Extensions: Check: %s  DblChk: %s  DiscChk: %s  SingReply: %s\n"
              "            Recapture: %s   Passed Pawn: %s   Zugzwang: %s\n",
              FormatCount(ChkExt, buf1, sizeof(buf1)),
              FormatCount(DblExt, buf2, sizeof(buf2)),
              FormatCount(DiscExt, buf3, sizeof(buf3)),
              FormatCount(SingExt, buf4, sizeof(buf4)),
              FormatCount(RCExt, buf5, sizeof(buf5)),
              FormatCount(PPExt, buf6, sizeof(buf6)),
              FormatCount(ZZExt, buf7, sizeof(buf7)));

        Print(2,
              "Hashing: Trans: %s/%s = %d %%   Pawn: %s/%s = %d %%\n"
              "         Eval: %s/%s = %d %%\n",
              FormatCount(HHit, buf1, sizeof(buf1)),
              FormatCount(HTry, buf2, sizeof(buf2)), Percentage(HHit, HTry),
              FormatCount(PHit, buf3, sizeof(buf3)),
              FormatCount(PTry, buf4, sizeof(buf4)), Percentage(PHit, PTry),
              FormatCount(SHit, buf5, sizeof(buf5)),
              FormatCount(STry, buf6, sizeof(buf6)), Percentage(SHit, STry));

        if (EGTBProbe != 0) {
            Print(2, "EGTB Hits/Probes = %s/%s\n",
                  FormatCount(EGTBProbeSucc, buf1, sizeof(buf1)),
                  FormatCount(EGTBProbe, buf2, sizeof(buf2)));
        }

        ShowHashStatistics();
    }

    sd->best_move = mvs[0];

    if (!sd->master) {
        FreePosition(sd->position);
        FreeSearchData(sd);
    }

    return NULL;
}

#if MP && HAVE_LIBPTHREAD
pthread_t *tids = NULL;
#endif /* MP && HAVE_LIBPTHREAD */

#if MP

/*
 * In parallel search stop all helper threads
 */

void StopHelpers(void) {
#if HAVE_LIBPTHREAD
    if (tids) {
        int nthread;
        void *dummy;

        AbortSearch = true;
        for (nthread = 0; nthread < (NumberOfCPUs - 1); nthread++) {
            pthread_join(tids[nthread], &dummy);
        }
        free(tids);
        tids = NULL;
    }
#endif /* HAVE_LIBPTHREAD */
}

/*
 * In parallel search start up all helper threads
 */

static void StartHelpers(struct Position *p) {
#if HAVE_LIBPTHREAD
    pthread_attr_t attr;
    int nthread;

    if (tids) {
        StopHelpers();
    }

    if (NumberOfCPUs < 2)
        return;

    tids = calloc(NumberOfCPUs - 1, sizeof(pthread_t));

    if (tids == NULL) {
        Print(0, "Cannot allocate memory for helpers.\n");
        Print(0, "Will try to search sequential.\n");
        return;
    }

    pthread_attr_init(&attr);
    pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);

    /*
     * Start up the helper threads.
     */

    for (nthread = 0; nthread < (NumberOfCPUs - 1); nthread++) {
        struct SearchData *sd = CreateSearchData(ClonePosition(p));
        sd->master = false;
        pthread_create(tids + nthread, &attr, &IterateInt, sd);
    }

#endif /* HAVE_LIBPTHREAD */
}

#endif /* MP */

/**
 * The basic root iteration procedure.
 */
int Iterate(struct Position *p) {
    float soft, hard;
    int cnt;
    struct SearchData *sd;

    FHTime = 0;

    StartTime = GetTime();
    CurTime = StartTime;
    WallTimeStart = StartTime;

    CalcTime(p, &soft, &hard);

    heap_t heap = allocate_heap();
    cnt = LegalMoves(p, heap);

    AbortSearch = false;
    NeedTime = false;

    TotalNodes = 0;

    /*
     * Check if we need to start searching at all
     */

    if (cnt == 0) {
        free_heap(heap);
        if (!InCheck(p, p->turn))
            strcpy(AnalysisLine, "stalemate");
        else
            strcpy(AnalysisLine, "mate");
        return M_NONE;
    } else if (cnt == 1 && SearchMode != Analyzing) {
        move_t only_move = heap->data[heap->current_section->start];
        free_heap(heap);
        strcpy(AnalysisLine, "forced move");
        return only_move;
    }

    free_heap(heap);

    SoftLimit = StartTime + (int)(soft * ONE_SECOND);
    SoftLimit2 = StartTime + (int)(85 * soft);
    HardLimit = StartTime + (int)(hard * ONE_SECOND);

    InitScore(p);
    AgeHashTable();
    SearchHeader();

#if MP
    StartHelpers(p);
#endif /* MP */

    sd = CreateSearchData(p);
    sd->master = true;
    IterateInt(sd);

    move_t best_move = sd->best_move;
    FreeSearchData(sd);

#if MP
    StopHelpers();
#endif /* MP */

    return best_move;
}

/**
 * Search the root node.
 */
void SearchRoot(struct Position *p) {
    move_t move = M_NONE;
    struct Position *q;

    SearchMode = Searching;

    /* Test book first */
    if (p->outOfBookCnt[p->turn] < 3) {
        move = SelectBook(p);

        if (move != M_NONE) {
            char san_buffer[32];
            Print(1, "Book move found: %s\n",
                  NumberedSAN(p, move, san_buffer, sizeof(san_buffer)));
            p->outOfBookCnt[p->turn] = 0;
        } else {
            p->outOfBookCnt[p->turn] += 1;
        }
    }

    if (move == M_NONE) {
        q = ClonePosition(p);
        move = Iterate(q);
        FreePosition(q);
    }

    if (move != M_NONE) {
        double elapsed = (double)(CurTime - StartTime) / (double)ONE_SECOND;
        DoTC(p, (int)(elapsed + 0.5));

        char san_buffer[16];
        Print(0, REVERSE "%s(%d): %s" NORMAL "\n",
              p->turn == White ? "White" : "Black", (p->ply / 2) + 1,
              SAN(p, move, san_buffer));

        if (XBoardMode)
            PrintNoLog(0, "move %s\n", ICS_SAN(move));

        DoMove(p, move);
    }
}

/**
 * Implements the permanent brain.
 */
int PermanentBrain(struct Position *p) {
    if (!LegalMove(p, PBMove)) {
        struct Position *q;

        q = ClonePosition(p);
        SearchMode = Puzzling;
        PBAltMove = M_NONE;

        Print(2, "Puzzling over a move to ponder on...\n");
        PBMove = Iterate(q);
        FreePosition(q);

        if (SearchMode == Interrupted) {
            return PB_NO_PB_MOVE;
        }

        if (PBAltMove != M_NONE) {
            DoMove(p, PBAltMove);
            return PB_NO_PB_HIT;
        }

        if (!LegalMove(p, PBMove)) {
            Print(0, "No PB move.\n");
            return PB_NO_PB_MOVE;
        }
    }

    if (LegalMove(p, PBMove)) {
        move_t move = M_NONE;
        struct Position *q;
        bool inbook = false;
        char san_buffer[16];

        q = ClonePosition(p);

        PBActMove = PBMove;
        PBAltMove = M_NONE;
        PBHit = false;

        Print(0, "%s(%d): %s (in Permanent Brain)\n",
              p->turn == White ? "White" : "Black", (p->ply / 2) + 1,
              SAN(p, PBActMove, san_buffer));

        DoMove(q, PBActMove);

        if (q->outOfBookCnt[q->turn] < 3) {
            move = SelectBook(q);
            if (move != M_NONE) {
                PBHit = false;
                PBAltMove = M_NONE;
                inbook = true;
            }
        }

        SearchMode = Pondering;

        if (!inbook) {
            move = Iterate(q);
        }

        FreePosition(q);

        if (SearchMode == Interrupted) {
            return PB_NO_PB_MOVE;
        }

        if (PBHit) {
            double elapsed =
                (double)(CurTime - WallTimeStart) / (double)ONE_SECOND;
            Print(2, "PB Hit! (elapsed %g secs)\n", elapsed);

            Print(0, "%s(%d): %s\n", p->turn == White ? "White" : "Black",
                  (p->ply / 2) + 1, SAN(p, PBActMove, san_buffer));

            DoMove(p, PBActMove);
            DoTC(p, (int)(elapsed + 0.5));

            Print(0, REVERSE "%s(%d): %s" NORMAL "\n",
                  p->turn == White ? "White" : "Black", (p->ply / 2) + 1,
                  SAN(p, move, san_buffer));

            if (XBoardMode) {
                PrintNoLog(0, "move %s\n", ICS_SAN(move));
            }

            DoMove(p, move);

            return PB_HIT;
        } else if (!PBHit && PBAltMove != M_NONE) {
            Print(2, "PB not Hit! Alternate move is %s\n",
                  SAN(p, PBAltMove, san_buffer));

            DoMove(p, PBAltMove);

            return PB_NO_PB_HIT;
        }
    }

    return PB_NO_PB_MOVE;
}

/**
 * Analysis mode for xboard.
 */
void AnalysisMode(struct Position *p) {
    struct Position *q;

    SearchMode = Analyzing;

    q = ClonePosition(p);
    Iterate(q);
    FreePosition(q);
}
