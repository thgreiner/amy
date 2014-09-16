/*

    Amy - a chess playing program
    Copyright (C) 2002-2004 Thorsten Greiner

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
 * search.c - tree searching routines
 *
 * $Id: search.c 456 2004-03-04 21:11:26Z thorsten $
 *
 */

#include "amy.h"

#define NULLMOVE          1
#define FUTILITY          1
#define EXTENDED_FUTILITY 1
#define RAZORING          1

#define REVERSE "\x1B[7m"
#define NORMAL  "\x1B[0m"

/*
 * We use fractional ply extensions.
 * See D. Levy, D. Broughton and M. Taylor: The SEX Algorithm in Computer Chess
 * ICCA Journal, Volume 2, No. 1, pp. 10-22.
 */

enum {
    OnePly                 = 16,

    /*
     * Check extensions. Every check is extended one ply. Additional extensions
     * are awarded if there is only one legal reply or if it is a double or
     * discovered check.
     */

    ExtendInCheck          = 14,
    ExtendDoubleCheck      =  4,
    ExtendDiscoveredCheck  =  4,
    ExtendSingularReply    = 12,

    /*
     * A passed pawn push to the seventh rank is extended.
     */

    ExtendPassedPawn       = 14,
    ExtendZugzwang         = 12,

    /*
     * The tree below a null move is searched with reduced search depth.
     */

    ReduceNullMove         = 48,
    ReduceNullMoveDeep     = 64
};

/*
 * Captures and recaptures are extended.
 */

static const int ExtendRecapture[]      = { 0, 4, 6, 6, 8, 10 };

static const int PVWindow               = 250;
static const int ResearchWindow         = 1500;

static const int MateDepth              = 3;

/**
 * Version info
 */

char RcsId_search_c[] = "$Id: search.c 456 2004-03-04 21:11:26Z thorsten $";

/**
 * search tree data
 */

int MaxDepth;
int Nodes, QNodes, ChkNodes;
int RCExt, ChkExt, DiscExt, DblExt, SingExt, PPExt, ZZExt;
unsigned int HardLimit, SoftLimit, SoftLimit2;
unsigned int StartTime, WallTimeStart;
unsigned int CurTime;
int FHTime;
int AbortSearch;
int NeedTime = FALSE;
int PrintOK;
int MaxSearchDepth = MAX_TREE_SIZE-1;
int DoneAtRoot;
static int EGTBDepth = 0;

static int NodesPerCheck;

/*
 * Search stati
 */

enum {
    Searching =   1,
    Pondering =   2,
    Puzzling  =   3,
    Analyzing =   4,
    Interrupted = 5
};

enum {
    PVNode = 0,
    AllNode = 1,
    CutNode = 2,
    CutNodeNoNull = 3
};

static int SearchMode = Searching;

/* Permanent Brain Variables */
int PBMove, PBActMove;
int PBHit;
int PBAltMove;

char BestLine[2048];
char ShortBestLine[2048];
char AnalysisLine[2048];

int HTry, HHit;
int PTry, PHit;
int STry, SHit;

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
static int TerminateSearch(struct SearchData *sd)
{
    if((Nodes+QNodes) > ChkNodes) {
        unsigned int now = GetTime();

        ChkNodes = Nodes+QNodes+NodesPerCheck;
        if(AbortSearch) return TRUE;

        CurTime = now;
        if(CurTime > (StartTime+ONE_SECOND)) PrintOK = TRUE;

        if(InputReady()) {
            char buffer[64];
            struct Command *theCommand;

            ReadLine(buffer, 64);

            /*
             * the '.' command can only be handled here
             */

            if(buffer[0] == '.') {
                PrintNoLog(0,
                    "stat01: %d %d %d %d %d\n",
                    (CurTime - StartTime),
                    Nodes + QNodes,
                    sd->depth,
                    sd->nrootmoves - sd->movenum -1,
                    sd->nrootmoves);
            }

            theCommand = ParseInput(buffer);

            if(theCommand) {
                if(SearchMode == Pondering && theCommand->move != M_NONE) {
                    if(theCommand->move == PBActMove) {
                        PBHit = TRUE;
                        SearchMode = Searching;
                        Print(1, "OK!\n");
                        WallTimeStart = now;

                        if(CurTime >= HardLimit) return TRUE;
                        if(DoneAtRoot) return TRUE;

                        return FALSE;
                    }
                    else {
                        PBHit = FALSE;
                        PBAltMove = theCommand->move;
                        return TRUE;
                    }
                }

                if(SearchMode == Puzzling && theCommand->move != M_NONE) {
                    PBAltMove = theCommand->move;
                    return TRUE;
                }

                if(SearchMode == Analyzing && theCommand->move != M_NONE) {
                    ExecuteCommand(theCommand);
                    return TRUE;
                }

                if(theCommand->allowed_during_search) {
                    ExecuteCommand(theCommand);

                    if(theCommand->interrupts_search) {
                        SearchMode = Interrupted;
                        return TRUE;
                    }
                }
            }
        }

        if(SearchMode == Searching) {
            if(CurTime >= HardLimit) return TRUE;
        }
    }
    return FALSE;
}

/* 
 * Check for draw because of insufficient material 
 */

static int InsufMat(struct Position *p)
{
    if(p->material[White] == 0 && p->material[Black] == p->nonPawn[Black] &&
       p->material[Black] < Value[Rook]) return TRUE;
    if(p->material[Black] == 0 && p->material[White] == p->nonPawn[White] &&
       p->material[White] < Value[Rook]) return TRUE;
    return FALSE;
}

/* 
 * Support routine for recpature extensions 
 */

static int IsRecapture(int piece1, int piece2)
{
    switch(TYPE(piece1)) {
        case Pawn:
            return TYPE(piece2) == Pawn;
        case Knight:
        case Bishop:
            return (TYPE(piece2) == Knight || TYPE(piece2) == Bishop);
        case Rook:
            return TYPE(piece2) == Rook;
        case Queen:
            return TYPE(piece2) == Queen;
    }

    return FALSE;
}

/*
 * Decide wether to extend the check due to the following conditions:
 *  - double check
 *  - discovered check
 *  - check with only one legal response
 *
 */

static int CheckExtend(struct Position *p)
{
    int kp = p->kingSq[p->turn];
    BitBoard att;

    att = p->atkFr[kp] & p->mask[OPP(p->turn)][0];

    if(CountBits(att) > 1) {

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

        while(att) {
            i=FindSetBit(att); ClrBit(att, i);
            ff &= ~Ray[i][kp];
        }

        while(ff) {
            i=FindSetBit(ff); ClrBit(ff, i);
            if(!(p->atkFr[i] & p->mask[OPP(p->turn)][0])) cnt++;
            if(cnt > 1) return ExtendDoubleCheck;
        }
    }
    else {
        BitBoard ff;
        BitBoard def;
        BitBoard tmp;

        int atp = FindSetBit(att);
        int cnt = 0;
        int i;
        int nd = 0;

        /* discovered check */
        if(atp != M_TO((p->actLog-1)->gl_Move)) {
            DiscExt++;
            nd = ExtendDiscoveredCheck;
        }

        ff = KingEPM[kp] & ~p->mask[p->turn][0];

        i = FindSetBit(att);
        if(Sliding[TYPE(p->piece[i])]) ff &= ~Ray[i][kp];

        /* check for king flight squares */
        while(ff) {
            i=FindSetBit(ff); ClrBit(ff, i);
            if(!(p->atkFr[i] & p->mask[OPP(p->turn)][0])) cnt++;
            if(cnt > 1) return nd;
        }

        /* Find all non-pinned defenders */
        def = p->mask[p->turn][0] & ~p->mask[p->turn][King];

        tmp = (p->mask[OPP(p->turn)][Bishop] | p->mask[OPP(p->turn)][Queen]) 
            & BishopEPM[kp];
        while(tmp) {
            BitBoard tmp2;
            i=FindSetBit(tmp); ClrBit(tmp, i);
            tmp2= InterPath[i][kp];
            if(tmp2 && !(p->mask[OPP(p->turn)][0] & tmp2)) {
                tmp2 &= p->mask[p->turn][0];
                if(CountBits(tmp2) == 1) {
                    ClrBit(def, FindSetBit(tmp2));
                }
            }
        }

        tmp = (p->mask[OPP(p->turn)][Rook] | p->mask[OPP(p->turn)][Queen]) 
            & RookEPM[kp];
        while(tmp) {
            BitBoard tmp2;
            i=FindSetBit(tmp); ClrBit(tmp, i);
            tmp2= InterPath[i][kp];
            if(tmp2 && !(p->mask[OPP(p->turn)][0] & tmp2)) {
                tmp2 &= p->mask[p->turn][0];
                if(CountBits(tmp2) == 1) {
                    ClrBit(def, FindSetBit(tmp2));
                }
            }
        }

        /* All non-pinned defenders are in 'def' */
        tmp = p->atkFr[atp] & def;

        cnt += CountBits(tmp);
        if(cnt > 1) return nd;

        /* if possible, try an interposition */
        if(Sliding[TYPE(p->piece[atp])]) {
            tmp = InterPath[atp][kp];
            while(tmp) {
                BitBoard tmp2;
                i=FindSetBit(tmp); ClrBit(tmp, i);
                if((tmp2 = p->atkFr[i] & def)) {
                    cnt+=CountBits(tmp2);
                }
                if(p->turn == White 
                    && (i-8) > 0
                    && TstBit(p->mask[White][Pawn], i-8) 
                    && TstBit(def, i-8)) cnt++;
                if(p->turn == Black 
                    && (i+8) < 64
                    && TstBit(p->mask[Black][Pawn], i+8) 
                    && TstBit(def, i+8)) cnt++;
                if(cnt > 1) return nd;
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

static int ScoreMove(struct Position *p, int move)
{
    int score = 0;

    if(move & M_CAPTURE)   score += Value[TYPE(p->piece[M_TO(move)])];
    if(move & M_PANY)      score += Value[PromoType(move)]-Value[Pawn];
    else if(TYPE(p->piece[M_FROM(move)]) == Pawn) {
        if(p->turn == White && M_TO(move) >= a7) {
            score += Value[Bishop];
        }
        if(p->turn == Black && M_TO(move) <= h2) {
            score += Value[Bishop];
        }
    }

    if(move & M_ENPASSANT) score += Value[Pawn];
 
    return score;
}
 
/*
 * Store the result of the full width search
 */

static void StoreResult(
    struct SearchData *sd, int score, int alpha, int beta,
    int move, int depth, int threat)
{
    struct Position *p = sd->position;

    if(!(move & M_TACTICAL) && score > alpha) {
        sd->historyTab[p->turn][move & 4095] += depth*depth;
    }

    StoreHT(p->hkey, score, alpha, beta, move, depth, threat, sd->ply);
}

/*
 * The quiescence search.
 *
 * we only do a full width search if the side to move was in check since
 * the horizon, otherwise we do only a capture search.
 *
 */

static int quies(struct SearchData *sd, int alpha, int beta, int depth)
{
    struct Position *p = sd->position;
    int best;
    int move;
    int talpha;
    int tmp;
    QNodes++;

    EnterNode(sd);

    /* max search depth reached */
    if(sd->ply >= MaxDepth || Repeated(p, FALSE)) {
        best = 0;
        goto EXIT;
    }

    /*
     * Probe recognizers. If the probe is successful, use the
     * recognizer score as evaluation score.
     *
     * Otherwise, use ScorePosition()
     */

    switch(ProbeRecognizer(p, &tmp)) {
        case ExactScore:
            best = tmp;
            goto EXIT;
        case LowerBound:
            best = tmp;
            if(best >= beta) {
                goto EXIT;
            }
            break;
        case UpperBound:
            best = tmp;
            if(best <= alpha) {
                goto EXIT;
            }
            break;
        default:
            best = ScorePosition(p, alpha, beta);
            break;
    }

    if(best >= beta) {
        goto EXIT;
    }

    talpha = MAX(alpha, best);

    while((move = NextMoveQ(sd, alpha) ) != M_NONE) {
        DoMove(p, move);
        if(InCheck(p, OPP(p->turn))) UndoMove(p, move);
        else {
            tmp = -quies(sd, -beta, -talpha, depth-1);
            UndoMove(p, move);
            if(tmp >= beta) {
                best = tmp;
                goto EXIT;
            }
            if(tmp > best) {
                best = tmp;
                if(best > talpha) {
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

static int negascout(struct SearchData *sd,
              int alpha,
              int beta,
              const int depth,
              int node_type
#if MP
             ,int exclusiveP
#endif /* MP  */
)
{
    struct Position *p = sd->position;
    struct SearchStatus *st;
    int best = -INF;
    int bestm = M_NONE;
    int tmp;
    int talpha;
    int incheck;
    int lmove;
    int move;
    int extend = 0;
    int threat = FALSE;
    int reduce_extensions;
    int next_type;
    int was_futile = FALSE;
#if FUTILITY
    int is_futile;
    int optimistic = 0;
#endif

#if MP
    int deferred_cnt = 0;
    int deferred_list[64];
    int deferred_depth[64];
#endif

    EnterNode(sd);

    Nodes++;

    /* check for search termination */
    if(sd->master && TerminateSearch(sd)) {
        AbortSearch = TRUE;
        goto EXIT;
    }

    /* max search depth reached */
    if(sd->ply >= MaxDepth) goto EXIT;

    /*
     * Check for insufficent material or theoretical draw.
     */

    if( /* InsufMat(p) || CheckDraw(p) || */  Repeated(p, FALSE)) {
        best = 0;
        goto EXIT;
    }

    /*
     * check extension 
     */

    incheck = InCheck(p, p->turn);
    if(incheck && p->material[p->turn] > 0) {
        extend += CheckExtend(p);
        ChkExt++;
    }

    /*
     * Check the hashtable 
     */

    st = sd->current;

    HTry++;
#if MP
    switch(ProbeHT(p->hkey, &tmp, depth, &(st->st_hashmove), &threat, sd->ply,
                    exclusiveP))
#else
    switch(ProbeHT(p->hkey, &tmp, depth, &(st->st_hashmove), &threat, sd->ply))
#endif /* MP */
    {
        case ExactScore:
            HHit++;
            best = tmp;
            goto EXIT;
        case UpperBound:
            if(tmp <= alpha) {
                HHit++;
                best = tmp;
                goto EXIT;
            }
            break;
        case LowerBound:
            if(tmp >= beta) {
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

    if(depth > EGTBDepth && ProbeEGTB(p, &tmp, sd->ply)) {
        best = tmp;
        goto EXIT;
    }

    /*
     * Probe recognizers
     */

    switch(ProbeRecognizer(p, &tmp)) {
        case ExactScore:
            best = tmp;
            goto EXIT;
        case LowerBound:
            if(tmp >= beta) {
                best = tmp;
                goto EXIT;
            }
            break;
        case UpperBound:
            if(tmp <= alpha) {
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

    if(!incheck && node_type == CutNode && !threat) {
        int next_depth;
        int nms;

        next_depth = depth - ReduceNullMove;

	if (next_depth > 0) {
	    next_depth = depth - ReduceNullMoveDeep;
	}

        DoNull(p);
        if(next_depth < 0) {
            nms = -quies(sd, -beta, -beta+1, 0);
        } else {
#if MP
            nms = -negascout(sd, -beta, -beta+1, next_depth, AllNode, 0);
#else
            nms = -negascout(sd, -beta, -beta+1, next_depth, AllNode);
#endif
        }
        UndoNull(p);

        if(AbortSearch) goto EXIT;
        if(nms >= beta) {
            if(p->nonPawn[p->turn] >= Value[Queen]) {
                best = nms;
                goto EXIT;
            } else {
                if(next_depth < 0) {
                    nms = quies(sd, beta-1, beta, 0);
                } else {
#if MP
                    nms = negascout(sd, beta-1, beta, next_depth, CutNodeNoNull,
                    0);
#else
                    nms = negascout(sd, beta-1, beta, next_depth, 
                                    CutNodeNoNull);
#endif
                }

                if(nms >= beta) {
                    best = nms;
                    goto EXIT;
                } else {
                    extend += ExtendZugzwang;
                    ZZExt++;
                }
            }
        } else if(nms <= -CMLIMIT) {
            threat = TRUE;
        }
    }
#endif /* NULLMOVE */

    lmove = (p->actLog-1)->gl_Move;
    reduce_extensions = (sd->ply > 2*sd->depth);
    talpha = alpha;

    switch(node_type) {
        case AllNode: next_type = CutNode; break;
        case CutNode:
        case CutNodeNoNull:
                      next_type = AllNode; break;
        default:      next_type = PVNode;  break;
    }

#if FUTILITY
    is_futile = !incheck && !threat && alpha < CMLIMIT && alpha > -CMLIMIT;
    if(is_futile) {
        if(p->turn == White) {
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

    if(depth > 2*OnePly && alpha+1 != beta && !LegalMove(p, st->st_hashmove)) {
        int useless;
#if MP
        useless = negascout(sd, alpha, beta, depth-2*OnePly, PVNode, 0);
#else
        useless = negascout(sd, alpha, beta, depth-2*OnePly, PVNode);
#endif
        st->st_hashmove = sd->pv_save[sd->ply+1];
    }

    /*
     * Search all legal moves
     */

    while((move = incheck ? NextEvasion(sd) : NextMove(sd)) != M_NONE) {
        int next_depth = extend;

        if(move & M_CANY && !MayCastle(p, move)) continue;

        /* 
         * recapture extension 
         */

        if((move & M_CAPTURE) && (lmove & M_CAPTURE) && 
                M_TO(move) == M_TO(lmove) &&
                IsRecapture(p->piece[M_TO(move)], (p->actLog-1)->gl_Piece)) {
            RCExt += 1;
            next_depth += ExtendRecapture[TYPE(p->piece[M_TO(move)])];
        }

        /* 
         * passed pawn push extension 
         */

        if(TYPE(p->piece[M_FROM(move)]) == Pawn && 
            p->nonPawn[OPP(p->turn)] <= Value[Queen]) {

            int to = M_TO(move);

            if(((p->turn == White && to >= a7)
                        || (p->turn == Black && to <= h2))
                    && IsPassed(p, to, p->turn) && SwapOff(p, move) >= 0) {
                next_depth += ExtendPassedPawn;
                PPExt += 1;
            }
        }

        /* 
         * limit extensions to sensible range.
         */

        if(reduce_extensions) next_depth /= 2;

        next_depth += depth - OnePly;

#if FUTILITY

        /*
         * Futility cutoffs
         */

        if(is_futile) {
            if(next_depth < 0 && !IsCheckingMove(p, move)) {
                tmp = optimistic + ScoreMove(p, move);
                if(tmp <= alpha) {
                    if(tmp > best) {
                        best = tmp;
                        bestm = move;
                        was_futile = TRUE;
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

            else if(next_depth >= 0 && next_depth < OnePly 
                                    && !IsCheckingMove(p, move)) {
                tmp = optimistic + ScoreMove(p, move) + (3*Value[Pawn]);
                if(tmp <= alpha) {
                    if(tmp > best) {
                        best = tmp;
                        bestm = move;
                        was_futile = TRUE;
                    }
                    continue;
                }
            }
#if RAZORING
            else if(next_depth >= OnePly && next_depth < 2*OnePly 
                                         && !IsCheckingMove(p, move)) {
                tmp = optimistic + ScoreMove(p, move) + (6*Value[Pawn]);
                if(tmp <= alpha) {
                    next_depth -= OnePly;
                }
            }
#endif /* RAZORING */
#endif /* EXTENDED_FUTILITY */
        }

#endif /* FUTILITY */

        DoMove(p, move);
        if(InCheck(p, OPP(p->turn))) {
            UndoMove(p, move);
        }
        else {
            /*
             * Check extension
             */

            if(p->material[p->turn] > 0 && InCheck(p, p->turn)) {
                next_depth += (reduce_extensions) ? 
                    ExtendInCheck>>1 : ExtendInCheck;
            }

            /*
             * Recursively search this position. If depth is exhausted, use
             * quies, otherwise use negascout.
             */

            if(next_depth < 0) {
                tmp = -quies(sd, -beta, -talpha, 0);
            }
            else if(bestm != M_NONE && !was_futile) {
#if MP
                tmp = -negascout(sd, -talpha-1, -talpha, next_depth, next_type,
                                    bestm != M_NONE);
                if(tmp != ON_EVALUATION && tmp > talpha && tmp < beta) {
                    tmp = -negascout(sd, -beta, -tmp, next_depth, 
                                     node_type == PVNode ? PVNode : AllNode,
                                     bestm != M_NONE);
                }
#else
                tmp = -negascout(sd, -talpha-1, -talpha, next_depth, next_type);
                if(tmp > talpha && tmp < beta) {
                    tmp = -negascout(sd, -beta, -tmp, next_depth, 
                                     node_type == PVNode ? PVNode : AllNode);
                }
#endif /* MP */
            }
            else {
#if MP
                tmp = -negascout(sd, -beta, -talpha, next_depth, next_type,
                                 bestm != M_NONE);
#else
                tmp = -negascout(sd, -beta, -talpha, next_depth, next_type);
#endif /* MP */
            }

            UndoMove(p, move);

            if(AbortSearch) goto EXIT;

#if MP
            if(tmp == ON_EVALUATION) {

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

                if(tmp >= beta) {
                    if(!(move & M_TACTICAL)) {
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

                if(tmp > best) {
                    best = tmp;
                    bestm = move;
                    was_futile = FALSE;

                    if(best > talpha) {
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

    while(deferred_cnt) {
        int next_depth = deferred_depth[--deferred_cnt];
        move = deferred_list[deferred_cnt];

        DoMove(p, move);

        tmp = -negascout(sd, -talpha-1, -talpha, next_depth, next_type, 0);
        if(tmp > talpha && tmp < beta) {
            tmp = -negascout(sd, -beta, -talpha, next_depth, 
                             node_type == PVNode ? PVNode : AllNode, 0);
        }

        UndoMove(p, move);

        /*
         * beta cutoff, enter move in Killer/Countermove table
         */

	if(tmp >= beta) {
	    if(!(move & M_TACTICAL)) {
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

	if(tmp > best) {
	    best = tmp;
	    bestm = move;
	    was_futile = FALSE;

	    if(best > talpha) {
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

    if(bestm == M_NONE) {
        if(incheck) best = -INF + sd->ply;
        else        best = 0;
    }

    if(!was_futile) {
        StoreResult(sd, best, alpha, beta, bestm, depth, threat);
    }

    EXIT:

    if(node_type == PVNode) {
        sd->pv_save[sd->ply] = bestm;
    }

    LeaveNode(sd);
    return best;
}

static char *NSAN(struct Position *p, int move)
{
    static char tmp[32];

    if(p->turn == White) sprintf(tmp, "%d. %s", 1+(p->ply+1)/2, SAN(p, move));
    else                 sprintf(tmp, "%d. .. %s", 1+p->ply/2, SAN(p, move));

    return tmp;
}

/*
 * Analyze the hashtable to find the principal variation.
 */

static void AnaLoop(struct Position *p, int depth)
{
    int move;
    int dummy = 0;
    int score;

#if MP
    if(ProbeHT(p->hkey, &score, 0, &move, &dummy, 0, 0) == Useless) return;
#else
    if(ProbeHT(p->hkey, &score, 0, &move, &dummy, 0) == Useless) return;
#endif

    if(Repeated(p, TRUE) >= 2) return;

    if(LegalMove(p, move)) {
        int incheck;

        DoMove(p, move);
        incheck = InCheck(p, OPP(p->turn));
        UndoMove(p, move);

        if(p->turn == White) {
            char tmp[8];
            sprintf(tmp, "%d. ", 1+(p->ply+1)/2);
            strcat(BestLine, tmp);
        }

        if(incheck) {
            strcat(BestLine, "<ill>");
            strcat(ShortBestLine, "<ill>");
            return;
        }

        strcat(BestLine, SAN(p, move));
        strcat(BestLine, " ");
        strcat(ShortBestLine, SAN(p, move));
        strcat(ShortBestLine, " ");

        /* save move to ponder on ... */
        if(depth == 1) PBMove = move;

        DoMove(p, move);
        AnaLoop(p, depth+1);
        UndoMove(p, move);
    }
    else if(move == M_HASHED) {
        strcat(BestLine, "..");
        strcat(ShortBestLine, "..");
    }
    else if(move == M_NULL) {
        strcat(BestLine, "<null>");
        strcat(ShortBestLine, "<null>");
    }
}

static void AnalyzeHT(struct Position *p, int move)
{
    strcpy(BestLine, NSAN(p, move));
    strcat(BestLine, " ");
    strcpy(ShortBestLine, SAN(p, move));
    strcat(ShortBestLine, " ");
    DoMove(p, move);
    AnaLoop(p, 1);
    UndoMove(p, move);
}

/**
 * Initialize the search variables.
 */
static void InitSearch(struct SearchData *sd)
{
    sd->ply = 0;
    Nodes = QNodes = ChkNodes = 0;
    RCExt = ChkExt = DiscExt = DblExt = SingExt = PPExt = ZZExt = 0;
    PrintOK = (SearchMode == Analyzing) ? TRUE : FALSE;
    DoneAtRoot = FALSE;
    EGTBProbe = EGTBProbeSucc = 0;

    /* Initialize scoring tables */

    HTry = HHit = PTry = PHit = STry = SHit = 0;
}

/**
 * Resort the root move list.
 */
static void ResortMovesList(int cnt, int *mvs, int *nodes)
{
    int i;
    for(i=1; i<cnt-1; i++) {
        int besti = i;
        int bestn = nodes[i];
        int j;
        for(j=i+1; j<cnt; j++) {
            if(nodes[j] > bestn) {
                bestn = nodes[j];
                besti = j;
            }
        }

        if(besti != i) {
            int tmp = mvs[i];
            mvs[i] = mvs[besti];
            mvs[besti] = tmp;
            nodes[besti] = nodes[i];
        }
    }
}

/*
 * This routine searches a chess position. It uses iterative deepening,
 * aspiration window and scout search.
 */

static void *IterateInt(void *x)
{
    int best;
    int mvs[256];
    int nodes[256];
    int last = 0;
    double elapsed;
    struct SearchData *sd = x;
    struct Position *p;

    p = sd->position;

    InitSearch(sd);
    sd->nrootmoves = LegalMoves(p, mvs);
    best = p->material[p->turn] - p->material[OPP(p->turn)];

    if(!(mvs[0] & M_TACTICAL)) PutKiller(sd, mvs[0]);

    MaxDepth = MAX_TREE_SIZE-1;

    for(sd->depth=1; sd->depth < MaxSearchDepth; sd->depth++) {
        int alpha = best - PVWindow;
        int beta  = best + PVWindow;
        int is_pv = TRUE;
        int pv_stable = TRUE;
        /* int nodes_per_iteration = Nodes; */

        for(sd->movenum=0; sd->movenum < sd->nrootmoves; sd->movenum++) {
            int tmp;
            int next_depth = (sd->depth-2)*OnePly;

            nodes[sd->movenum] = Nodes;

            if(sd->master && PrintOK) {
                PrintNoLog(2, "%2d  %s   %2d/%2d  %s      \r", 
                            sd->depth,
                            TimeToText(CurTime-StartTime),
                            sd->movenum+1, sd->nrootmoves,
                            NSAN(p, mvs[sd->movenum]));
            }

            DoMove(p, mvs[sd->movenum]);
            if(InCheck(p, p->turn)) next_depth += ExtendInCheck;

            if(next_depth >= 0) {
#if MP
                tmp = -negascout(sd, -beta, -alpha, next_depth, 
                                 is_pv ? PVNode : CutNode, 0);
#else
                tmp = -negascout(sd, -beta, -alpha, next_depth, 
                                 is_pv ? PVNode : CutNode);
#endif
            }
            else {
                tmp = -quies(sd, -beta, -alpha, 0);
            }
            UndoMove(p, mvs[sd->movenum]);
            if(AbortSearch) goto final;

            if(is_pv && tmp <= alpha) {

                /* 
                 * Fail low on principal variation.
                 * Open window, take some time, and re-search.
                 */

                pv_stable = FALSE;

                if(sd->master && PrintOK) {
                    SearchOutputFailHighLow(
                        sd->depth, CurTime-StartTime, FALSE, NSAN(p, mvs[0]),
                        Nodes + QNodes);
                }

                NeedTime = TRUE;

                beta = tmp;
                alpha = tmp - ResearchWindow;

                DoMove(p, mvs[sd->movenum]);
                if(next_depth >= 0) {
#if MP
                    tmp = -negascout(sd, -beta, -alpha, next_depth, PVNode, 0);
#else
                    tmp = -negascout(sd, -beta, -alpha, next_depth, PVNode);
#endif
                }
                else {
                    tmp = -quies(sd, -beta, -alpha, 0);
                }
                UndoMove(p, mvs[sd->movenum]);
                if(AbortSearch) goto final;

                if(tmp <= alpha) {
                    beta = tmp;
                    alpha = -INF;

                    DoMove(p, mvs[sd->movenum]);
                    if(next_depth >= 0) {
#if MP
                        tmp = -negascout(sd, -beta, -alpha, next_depth, PVNode, 0);
#else
                        tmp = -negascout(sd, -beta, -alpha, next_depth, PVNode);
#endif
                    }
                    else {
                        tmp = -quies(sd, -beta, -alpha, 0);
                    }
                    UndoMove(p, mvs[sd->movenum]);
                    if(AbortSearch) goto final;
                }
                nodes[sd->movenum] = Nodes-nodes[sd->movenum];
            }
            else if(tmp >= beta) {

                /* 
                 * Fail high.
                 * Re-search with open window.
                 */

                pv_stable = FALSE;

                if(sd->movenum != 0) {
                    int tm = mvs[sd->movenum];
                    int tn = nodes[sd->movenum];
                    int j;

                    for(j=sd->movenum; j>0; j--) { 
                        mvs[j] = mvs[j-1];
                        nodes[j] = nodes[j-1];
                    }
                    mvs[0] = tm;
                    nodes[0] = tn;

                    if(!(mvs[0] & M_TACTICAL)) PutKiller(sd, mvs[0]);
                    PBMove = M_NONE;
                    is_pv = TRUE;

                    FHTime = (CurTime - StartTime)/ONE_SECOND;
                }

                if(sd->master && PrintOK) {
                    SearchOutputFailHighLow(
                        sd->depth, CurTime-StartTime, TRUE, NSAN(p, mvs[0]),
			Nodes + QNodes);
                }

                alpha = tmp;
                beta = tmp+ResearchWindow;

                DoMove(p, mvs[0]);
                if(next_depth >= 0) {
#if MP
                    tmp = -negascout(sd, -beta, -alpha, next_depth, PVNode, 0);
#else
                    tmp = -negascout(sd, -beta, -alpha, next_depth, PVNode);
#endif
                }
                else {
                    tmp = -quies(sd, -beta, -alpha, 0);
                }
                UndoMove(p, mvs[0]);
                if(AbortSearch) goto final;

                if(tmp >= beta) {
                    alpha = tmp;
                    beta = INF;

                    DoMove(p, mvs[0]);
                    if(next_depth >= 0) {
#if MP
                        tmp = -negascout(sd, -beta, -alpha, next_depth, PVNode, 0);
#else
                        tmp = -negascout(sd, -beta, -alpha, next_depth, PVNode);
#endif
                    }
                    else {
                        tmp = -quies(sd, -beta, -alpha, 0);
                    }
                    UndoMove(p, mvs[0]);
                    if(AbortSearch) goto final;
                }
                nodes[0] = Nodes-nodes[0];
            }
            else {
                nodes[sd->movenum] = Nodes-nodes[sd->movenum];
            }

            if(AbortSearch) goto final;

            if(is_pv) {
                best = tmp;


                if(sd->master) {
                    AnalyzeHT(p, mvs[0]);

                    sprintf(AnalysisLine, "%2d: (%7s) %s",
                        sd->depth, ScoreToText(best), BestLine);

                    if(PrintOK) {
                        SearchOutput(
                            sd->depth,
                            CurTime-StartTime,
                            (p->turn) ? -best : best,
                            BestLine,
                            Nodes+QNodes);
                    }

                }

                alpha = best;
                beta  = best+1;
                is_pv = FALSE;
            }

            if(sd->master && sd->movenum == 0 && !NeedTime &&
                CurTime > SoftLimit) {
                if(SearchMode == Searching) {
                    AbortSearch = TRUE;
                    goto final;
                } else if(SearchMode == Pondering) {
                    DoneAtRoot = TRUE;
                }
            }
        }

        if(sd->master && (PrintOK ||
           (sd->depth > MateDepth && (best < -CMLIMIT || best > CMLIMIT))) ) {
                SearchOutput(
                    sd->depth,
                    CurTime-StartTime,
                    (p->turn) ? -best : best,
                    BestLine,
                    Nodes+QNodes);
            }

        if(best < -CMLIMIT || best > CMLIMIT) {
            if(last > CMLIMIT && best >= last && sd->depth > MateDepth) {
                if(SearchMode == Searching) break;
                else DoneAtRoot = TRUE;
            }
            if(SearchMode == Searching && last < CMLIMIT && best <= last && 
                sd->depth > MateDepth) break;
            last = best;
        }

        NeedTime = FALSE;
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

	if(sd->depth > 3) {
	    /*
	    * Do ten checks per second.
	    */

	    elapsed = (double)(CurTime - StartTime) / (double) ONE_SECOND;

	    NodesPerCheck = (elapsed == 0.0) ?
		1000 :
		(int)((Nodes + QNodes) / elapsed / 10);
        }

        if(SearchMode == Puzzling && sd->depth > 4) break;

        if(sd->master && ( (CurTime > SoftLimit) ||
	                   (pv_stable && CurTime > SoftLimit2))) {
            if(SearchMode == Searching) {
                AbortSearch = TRUE;
                break;
            } else if(SearchMode == Pondering) {
                DoneAtRoot = TRUE;
            }
        }

    }

final:

    if(CurTime <= StartTime) StartTime--;
    elapsed = (double)(CurTime-StartTime)/(double)ONE_SECOND;

    if(sd->master) {
	Print(2, "Nodes = %d, QPerc: %d %%, time = %g secs, "
	  "%.1f kN/s\n", Nodes+QNodes, 
	  QNodes/((Nodes+QNodes)/100 +1), 
	  elapsed,
	  (Nodes+QNodes)/1000.0/elapsed);

	Print(2, "Extensions: Check: %d  DblChk: %d  DiscChk: %d  SingReply: %d\n"
	     "            Recapture: %d   Passed Pawn: %d   Zugzwang: %d\n",
	  ChkExt, DblExt, DiscExt, SingExt, RCExt, PPExt, ZZExt);

	Print(2, "Hashing: Trans: %d/%d = %d %%   Pawn: %d/%d = %d %%\n"
             "         Eval: %d/%d = %d %%\n",
          HHit, HTry, (HTry) ? (100*HHit/HTry) : 0,
          PHit, PTry, (PTry) ? (100*PHit/PTry) : 0,
	  SHit, STry, (STry) ? (100*SHit/STry) : 0 );

        if(EGTBProbe != 0) {
            Print(2, "EGTB Hits/Probes = %d/%d\n", EGTBProbeSucc, EGTBProbe);
        }

	ShowHashStatistics();
    }

    sd->best_move = mvs[0];

    if(!sd->master) {
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

void StopHelpers(void)
{
#if HAVE_LIBPTHREAD
    if(tids) {
	int nthread;
	void *dummy;

	AbortSearch = TRUE;
	for(nthread = 0; nthread < (NumberOfCPUs-1); nthread++) {
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

static void StartHelpers(struct Position *p)
{
#if HAVE_LIBPTHREAD
    pthread_attr_t attr;
    int nthread;

    if(tids) {
	StopHelpers();
    }

    if(NumberOfCPUs < 2) return;

    tids = calloc(NumberOfCPUs-1, sizeof(pthread_t));

    if(tids == NULL) {
	Print(0, "Cannot allocate memory for helpers.\n");
	Print(0, "Will try to search sequential.\n");
	return;
    }

    pthread_attr_init(&attr);
    pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);

    /*
     * Start up the helper threads.
     */

    for(nthread = 0; nthread < (NumberOfCPUs-1); nthread++) {
        struct SearchData *sd = CreateSearchData(ClonePosition(p));
        sd->master = FALSE;
        pthread_create(tids + nthread, &attr, &IterateInt, sd);
    }

#endif /* HAVE_LIBPTHREAD */
}

#endif /* MP */

/**
 * The basic root iteration procedure.
 */
int Iterate(struct Position *p)
{
    float soft, hard;
    int cnt;
    int mvs[256];
    struct SearchData *sd;
    
    FHTime = 0;

    StartTime = GetTime();
    CurTime = StartTime;
    WallTimeStart = StartTime;

    CalcTime(p, &soft, &hard);                      

    cnt = LegalMoves(p, mvs);

    AbortSearch = FALSE;
    NeedTime = FALSE;
    
    /*
     * Check if we need to start searching at all
     */

    if(cnt == 0) {
        if(!InCheck(p, p->turn)) strcpy(AnalysisLine, "stalemate");
        else               strcpy(AnalysisLine, "mate");
        return M_NONE;
    }
    else if(cnt == 1 && SearchMode != Analyzing) {
        strcpy(AnalysisLine, "forced move");
        return mvs[0];
    }

    SoftLimit  = StartTime + soft*ONE_SECOND;
    SoftLimit2 = StartTime + 85*soft;
    HardLimit  = StartTime + hard*ONE_SECOND;

    InitScore(p);
    AgeHashTable();
    SearchHeader();

#if MP 
    StartHelpers(p);
#endif /* MP */

    sd = CreateSearchData(p);
    sd->master = TRUE;
    IterateInt(sd);

    mvs[0] = sd->best_move;
    FreeSearchData(sd);

#if MP
    StopHelpers();
#endif /* MP */

    return mvs[0];
}

/**
 * Search the root node.
 */
void SearchRoot(struct Position *p)
{
    int move = M_NONE;
    struct Position *q;

    SearchMode = Searching;

    /* Test book first */
    if(p->outOfBookCnt[p->turn] < 3) {
        move = SelectBook(p);

        if(move != M_NONE) {
            Print(1, "Book move found: %s\n", NSAN(p, move));
            p->outOfBookCnt[p->turn] = 0;
        }
        else {
            p->outOfBookCnt[p->turn] += 1;
        }
    }

    if(move == M_NONE) {
	q = ClonePosition(p);
	move = Iterate(q);
	FreePosition(q);
    }

    if(move != M_NONE) {
        double elapsed = (double)(CurTime-StartTime)/(double)ONE_SECOND;
        DoTC(p, (int)(elapsed+0.5));
 
        Print(0, REVERSE "%s(%d): %s" NORMAL "\n", 
            p->turn == White ? "White":"Black", 
            (p->ply/2)+1,
            SAN(p, move));		
	
        if(XBoardMode) PrintNoLog(0, "move %s\n", ICS_SAN(move));

        DoMove(p, move);
    }
}

/**
 * Implements the permanent brain.
 */
int PermanentBrain(struct Position *p)
{
    if(!LegalMove(p, PBMove)) {
        struct Position *q;

        q = ClonePosition(p);
        SearchMode = Puzzling;
        PBAltMove = M_NONE;

        Print(2, "Puzzling over a move to ponder on...\n");
        PBMove = Iterate(q);
        FreePosition(q);

        if(SearchMode == Interrupted) {
            return PB_NO_PB_MOVE;
        }

        if(PBAltMove != M_NONE) {
            DoMove(p, PBAltMove);
            return PB_NO_PB_HIT;
        }

        if(!LegalMove(p, PBMove)) {
            Print(0, "No PB move.\n");
            return PB_NO_PB_MOVE;
        }
    }

    if(LegalMove(p, PBMove)) {
        int move = M_NONE;
	struct Position *q;
        int inbook = FALSE;

	q = ClonePosition(p);

        PBActMove = PBMove;
        PBAltMove = M_NONE;
        PBHit = FALSE;

        Print(0, "%s(%d): %s (in Permanent Brain)\n", 
	      p->turn == White ? "White":"Black", 
	      (p->ply/2)+1,
	      SAN(p, PBActMove));		

        DoMove(q, PBActMove);

	if(q->outOfBookCnt[q->turn] < 3) {
	    move = SelectBook(q);
	    if(move != M_NONE) {
		PBHit = FALSE;
		PBAltMove = M_NONE;
		inbook = TRUE;
	    }
	}

        SearchMode = Pondering;

	if(!inbook) {
	    move = Iterate(q);
	}

        FreePosition(q);

        if(SearchMode == Interrupted) {
            return PB_NO_PB_MOVE;
        }

        if(PBHit) {
            double elapsed = (double)(CurTime-WallTimeStart)/
                             (double)ONE_SECOND;
            Print(2, "PB Hit! (elapsed %g secs)\n", elapsed);
            Print(0, "%s(%d): %s\n", 
                    p->turn == White ? "White":"Black", 
                    (p->ply/2)+1,
                    SAN(p, PBActMove));		

            DoMove(p, PBActMove);
            DoTC(p, (int)(elapsed+0.5));

            Print(0, REVERSE "%s(%d): %s" NORMAL "\n", 
                p->turn == White ? "White":"Black", 
                (p->ply/2)+1,
                SAN(p, move));		

            if(XBoardMode) {
                PrintNoLog(0, "move %s\n", ICS_SAN(move));
            }

            DoMove(p, move);

            return PB_HIT;
        }
        else if(!PBHit && PBAltMove != M_NONE) {
            Print(2, "PB not Hit! Alternate move is %s\n", SAN(p, PBAltMove));

	    DoMove(p, PBAltMove);

            return PB_NO_PB_HIT;
        }
    }

    return PB_NO_PB_MOVE;
}

/**
 * Analysis mode for xboard.
 */
void AnalysisMode(struct Position *p)
{
    struct Position *q;
    int move;

    SearchMode = Analyzing;

    q = ClonePosition(p);
    move = Iterate(q);
    FreePosition(q);
}
