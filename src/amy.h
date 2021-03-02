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
 * amy.h - Amy headerfile
 *
 * $Id: amy.h 90 2003-06-22 08:55:34Z thorsten $
 *
 */

#ifndef AMY_H
#define AMY_H

#include "config.h"

#ifdef _WIN32

   /*
    * Windows stuff by Dann Corbit.
    */

#  define CDECL __cdecl
#  define STDC_HEADERS 1
#  define HAVE_FCNTL_H 1

#else
#  define CDECL
#endif

#if STDC_HEADERS
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#endif

#ifdef _WIN32
#include <windows.h>
#include <conio.h>
#include <io.h>
#endif

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#if HAVE_FCNTL_H
#include <fcntl.h>
#endif

#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#if HAVE_LIBPTHREAD
#include <pthread.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>

#define SQUARE(x) 'a'+((x) & 7), '1'+((x) >> 3)

#define SetBit(b, i) ((b) |= SetMask[i])
#define ClrBit(b, i) ((b) &= ClrMask[i])
#define TstBit(b, i) ((b) & SetMask[i])

#define OPP(x) (1 ^ (x))

#define TYPE(x) (((x) >= 0) ? (x) : -(x))
#define COLOR(x) (((x) == 0) ? Neutral : (((x) > 0) ? White : Black))
#define SAME_COLOR(p, c) \
	(((c) == White && (p) > 0) || ((c) == Black && (p) < 0))
#define PIECEID(p, c) \
	(((c) == White) ? (p) : -(p))

#define M_FROM(m) ((m) & 63)
#define M_TO(m)   (((m) >> 6) & 63)

#define M_CAPTURE    (1 << 13)
#define M_SCASTLE    (1 << 14)
#define M_LCASTLE    (1 << 15)
#define M_PAWND      (1 << 16)
#define M_PQUEEN     (1 << 17)
#define M_PROOK      (1 << 18)
#define M_PBISHOP    (1 << 19)
#define M_PKNIGHT    (1 << 20)
#define M_ENPASSANT  (1 << 21)
#define M_NULL       (1 << 22)
#define M_HASHED     (1 << 23)

#define M_CANY       (M_SCASTLE|M_LCASTLE)
#define M_PANY       (M_PQUEEN|M_PROOK|M_PBISHOP|M_PKNIGHT)

#define M_TACTICAL   (M_CAPTURE|M_ENPASSANT|M_PANY)

#define M_NONE       0

#define PAWN_Value     1000
#define KNIGHT_Value   3500
#define BISHOP_Value   3500
#define ROOK_Value     5500
#define QUEEN_Value    11000

#define STRUCTURE_FIANCHETTO_WK		(1<<0)
#define STRUCTURE_FIANCHETTO_WQ		(1<<1)
#define STRUCTURE_FIANCHETTO_BK		(1<<2)
#define STRUCTURE_FIANCHETTO_BQ		(1<<3)

#define INF           200000      /* max. score */
#define CMLIMIT       100000      /* scores above this (or below -CMLIMIT)
                                   * indicate checkmate */
#define ON_EVALUATION (INF+1)

#define CUT_ABORTED (INF+1)

#define MAX_TREE_SIZE     64    /* maximum depth we will search to */
#define MAX_SEARCH_HEAP 2000

#define GAME_LOG_SIZE 1000  /* maximum do's we support */

#define PB_NO_PB_MOVE     0
#define PB_NO_PB_HIT      1
#define PB_HIT            2
#define PB_ALT_COMMAND    3

#define STATE_WAITING     0
#define STATE_CALCULATING 1
#define STATE_PONDERING   2
#define STATE_ANALYZING   3
#define STATE_END         4

#ifndef TRUE
#define TRUE  1
#define FALSE 0
#endif

#ifndef NULL
#define NULL ((void *)0)
#endif

#define ONE_SECOND 100u
#define ABS(x) ((x) < 0 ? -(x) : (x))
#define MAX(x,y) ((x) > (y) ? (x) : (y))
#define MIN(x,y) ((x) < (y) ? (x) : (y))

#define SIGNATURE_BIT(x) (1 << ((x)-1))
#define CALCULATE_INDEX(a,b) \
    (((a) | (b)) + 32 * ((a) != 0 && (b) != 0))
#define RECOGNIZER_INDEX(p) \
    CALCULATE_INDEX((p)->material_signature[White], \
                    (p)->material_signature[Black])

#define BOOK_MOVE -1

#define OPP_NAME_LENGTH 1024

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Constants for piece types and colors.
 */

enum PTypes { Neutral = 0, Pawn, Knight, Bishop, Rook, Queen, King, BPawn };
enum CTypes { White = 0, Black = 1 };

/*
 * Constants for chess board squares.
 */

enum {
    a1 = 0, b1, c1, d1, e1, f1, g1, h1,
        a2, b2, c2, d2, e2, f2, g2, h2,
        a3, b3, c3, d3, e3, f3, g3, h3,
        a4, b4, c4, d4, e4, f4, g4, h4,
        a5, b5, c5, d5, e5, f5, g5, h5,
        a6, b6, c6, d6, e6, f6, g6, h6,
        a7, b7, c7, d7, e7, f7, g7, h7,
        a8, b8, c8, d8, e8, f8, g8, h8
};

/*
 * Constants for value status.
 */

enum {
    ExactScore, LowerBound, UpperBound, Useful, Useless, OnEvaluation
};

typedef void (*COMMAND)(char *args);

#if SIZEOF_LONG == 8
typedef unsigned long BitBoard;
typedef unsigned long ran_t;
typedef long hash_t;
#else
#  if SIZEOF_LONG_LONG == 8
typedef unsigned long long BitBoard;
typedef unsigned long long ran_t;
typedef long long hash_t;
#  else
#    if defined(NT_i386) || defined(NT_AXP) || defined(_WIN32)
typedef unsigned __int64 BitBoard;
typedef unsigned __int64 ran_t;
#    else
#      error "Need 64 bit datatype!"
#    endif
#  endif
#endif

struct Position
{
    BitBoard atkTo[64];
    BitBoard atkFr[64];
    BitBoard mask[2][7];
    BitBoard slidingPieces;
    signed char piece[64];
    int ply;
    int castle;
    int enPassant;
    int turn;           /* 0 == white, 1 == black */
    hash_t hkey;
    hash_t pkey;
    int material[2], nonPawn[2];
    int outOfBookCnt[2];
    struct GameLog *gameLog;
    struct GameLog *actLog;
    int kingSq[2];
    int material_signature[2];
};

struct Command
{
    int move;
    COMMAND command_func;
    int allowed_during_search;
    int interrupts_search;
    char *args;
};

struct CommandEntry
{
    char *name;
    COMMAND command_func;
    int allowed_during_search;
    int interrupts_search;
    char *short_help;
    char *long_help;
};

struct SearchStatus
{
    int st_phase;
    int st_first;
    int st_nc_first;
    int st_last;
    int st_hashmove;
    int st_k1, st_k2, st_kl, st_cm, st_k3;
};

struct KillerEntry {
    int killer1, killer2;   /* killer moves */
    int kcount1, kcount2;   /* killer count */
};

struct SearchData
{
    struct Position     *position;

    struct SearchStatus *current;
    struct SearchStatus *statusTable;
    struct KillerEntry  *killer;
    struct KillerEntry  *killerTable;
#if MP
    struct HTEntry      *localHashTable;
#endif

    int                 *moveHeap;
    int                 *dataHeap;
    unsigned int        counterTab[2][4096];  /* counter moves per side */
    unsigned int        historyTab[2][4096];  /* history moves per side */

    int                 pv_save[64];

    int                 ply;

    int                 master;               /* true if a master process */

    int                 best_move;
    int                 depth;

    int                 nrootmoves;
    int                 movenum;
};

struct GameLog
{
    int    gl_Move;          /* the move that has been made in the position */
    int    gl_Piece;         /* the piece that was captured (if any) */
    int    gl_Castle;        /* the castling rights */
    int    gl_EnPassant;     /* the enpassant target square (if any) */
    int    gl_IrrevCount;    /* number of moves since last irreversible move */
    hash_t gl_HashKey;       /* used to detect repetitions */
    hash_t gl_PawnKey;
};

struct PawnFacts {
    BitBoard pf_WhitePassers;
    BitBoard pf_BlackPassers;
    int      pf_Flags;
    char     pf_WhiteKingSide;
    char     pf_BlackKingSide;
    char     pf_WhiteQueenSide;
    char     pf_BlackQueenSide;
};

struct HTEntry {
    int  ht_Signature;
    int   ht_Move;
    int   ht_Score;
    short ht_Flags;
    short ht_Depth;
};

struct PTEntry {
    unsigned int     pt_Signature;
    int              pt_Score;
    struct PawnFacts pt_PawnFacts;
};

struct STEntry {
    unsigned int st_Signature;
    int          st_Score;
};

struct MoveData {
    signed char nextPos;
    signed char nextDir;
};

struct PGNHeader
{
    char event[64];
    char site[64];
    char date[64];
    char round[64];
    char white[64];
    char black[64];
    char result[8];
    int  white_elo;
    int  black_elo;
};

extern int AutoSave;
extern char AutoSaveFileName[64];
extern struct Position *CurrentPosition;

extern BitBoard SetMask[64], ClrMask[64];
extern int goodmove[256];
extern int badmove[256];
extern const int EPTranslate[];
extern const int Sliding[];
extern char PieceName[];

extern const int CastleMask[2][2];
extern hash_t HashKeys[2][8][64];
extern hash_t HashKeysEP[64];
extern hash_t HashKeysCastle[16];
extern hash_t STMKey;

extern int HT_Bits;

extern BitBoard SetMask[64], ClrMask[64];
extern BitBoard FileMask[8], IsoMask[8];
extern BitBoard RankMask[8];
extern BitBoard ForwardRayW[64], ForwardRayB[64];
extern BitBoard PassedMaskW[64], PassedMaskB[64];
extern BitBoard ArtIsoMaskW[64], ArtIsoMaskB[64];
extern BitBoard OutpostMaskW[64], OutpostMaskB[64];
extern BitBoard InterPath[64][64];
extern BitBoard Ray[64][64];
extern BitBoard WPawnEPM[64], BPawnEPM[64];
extern BitBoard KnightEPM[64], BishopEPM[64], RookEPM[64], QueenEPM[64];
extern BitBoard KingEPM[64];
extern BitBoard SeventhRank[2], ThirdRank[2], EighthRank[2];
extern BitBoard *ForwardRay[2];
extern BitBoard *BackwardRay[2];
extern BitBoard LeftOf[8], RightOf[8], FarLeftOf[8], FarRightOf[8];
extern BitBoard CenterMask, ExtCenterMask, NoCenterMask;
extern BitBoard EdgeMask;
extern BitBoard BlackSquaresMask, WhiteSquaresMask;
extern BitBoard KingSquareW[64], KingSquareB[64];
extern BitBoard NotAFileMask, NotHFileMask;
extern BitBoard CornerMaskA1, CornerMaskA8, CornerMaskH1, CornerMaskH8;
extern BitBoard WKingTrapsRook1, WKingTrapsRook2;
extern BitBoard WRookTrapped1, WRookTrapped2;
extern BitBoard BKingTrapsRook1, BKingTrapsRook2;
extern BitBoard BRookTrapped1, BRookTrapped2;
extern BitBoard StrongSquareW[64], StrongSquareB[64];
extern BitBoard KingSafetyMask[64];
extern BitBoard WKingOpeningMask, BKingOpeningMask;
extern BitBoard WPawnOpeningMask, BPawnOpeningMask;
extern BitBoard WPawnBackwardMask[64], BPawnBackwardMask[64];
extern BitBoard WPawnKingAttacks[4];
extern BitBoard BPawnKingAttacks[4];
extern BitBoard PawnCenterMask;
extern BitBoard Rook7thKingMask[2];
extern BitBoard KingSideMask, QueenSideMask;
extern BitBoard WhitesHalf, BlacksHalf;
extern BitBoard FianchettoMaskWhiteKingSide, FianchettoMaskBlackKingSide;
extern BitBoard FianchettoMaskWhiteQueenSide, FianchettoMaskBlackQueenSide;
extern BitBoard ConnectedMask[64];

extern unsigned char FirstBit8[256];
extern unsigned char FirstBit16[65536];


extern int w_kingside_score, w_queenside_score;
extern int b_kingside_score, b_queenside_score;

extern int XBoard;
extern int XPost;
extern int PBCommand;

extern int MVerbose;

extern signed char NextPos[8][64][64];
extern signed char NextDir[8][64][64];
extern signed char NextSQ[64][64];
extern struct MoveData NextSquare[8][64][64];

extern struct SearchStatus SStatus[64];

extern int EGTBProbe, EGTBProbeSucc;

extern int Value[];
extern int MaxPos;
extern long PHit, PTry;
extern int PawnStructure;
extern long SHit, STry;
extern long HHit, HTry;

extern int FHTime, GUIMode;
extern int ComputerSide;
extern int VerboseMode;
extern int MoveHist[64];
extern int AbortSearch;

extern char AnalysisLine[];

extern int MaxSearchDepth;
extern int WatchMode;

extern int XBoardMode;
extern int State;
extern int ForceMode;
extern int EasyMode;
extern int PostMode;
extern int SelfPlayMode;

extern int TMoves,   TTime;
extern int Moves[3], Time[3];
extern int Increment;
extern int TMoves2, TTime2;
extern int TwoTimeControls;

extern int Verbosity;

extern int NumberOfCPUs;

extern char OpponentName[OPP_NAME_LENGTH];

extern int L_HT_Bits, L_HT_Size, L_HT_Mask;

#if HAVE___BUILTIN_POPCOUNTLL
#define CountBits(x) __builtin_popcountll(x)
#else
int CountBits(BitBoard);
#endif

#if HAVE___BUILTIN_CLZLL
#define FindSetBit(x) __builtin_clzll(x)
#else
int FindSetBit(BitBoard);
#endif

void Bookup(char *);
void BookupQuiet(char *);
void FlattenBook(int );
void CreateLearnDB(char *);
void QueryBook(struct Position *);
int SelectBook(struct Position *);
struct Command *ParseInput(char *line);
void ExecuteCommand(struct Command *theCommand);
void DoMove(struct Position *, int move);
void UndoMove(struct Position *, int move);
void DoNull(struct Position *);
void UndoNull(struct Position *);
int GenTo(struct Position *, int square, int *moves);
int GenEnpas(struct Position *, int *moves);
int GenFrom(struct Position *, int square, int *moves);
void GenRest(int *moves);
int GenCaps(int *moves, int good);
int GenChecks(struct Position *, int *moves);
int GenContactChecks(int *moves);
int MayCastle(struct Position *, int move);
int LegalMove(struct Position *, int move);
int IsCheckingMove(struct Position *, int move);
int LegalMoves(struct Position *, int *mvs);
int PLegalMoves(struct Position *, int *mvs);
int Repeated(struct Position *, int mode);
char *SAN(struct Position *, int move);
int ParseSAN(struct Position *, char *);
int ParseSANList(char *, int, int *, int, int *);
char *MakeEPD(struct Position *);
void ShowPosition(struct Position *);
int PromoType(int move);

struct Position *CreatePositionFromEPD(char *);
struct Position *InitialPosition(void);
struct Position *ClonePosition(struct Position *src);
void FreePosition(struct Position *);

void ShowMoves(struct Position *);
int ParseGSAN(struct Position *, char *san);
int ParseGSANList(char *san, int side, int *mvs, int cnt);
char *ICS_SAN(int move);
int InCheck(struct Position *, int);
void RecalcAttacks(struct Position *);
char *GameEnd(struct Position *);
void ParseEcoPgn(char *);
char *GetEcoCode(hash_t);
int FindEcoCode(struct Position *, char *);

void HashInit(void);

void ClearHashTable(void);
void AgeHashTable(void);
void ClearPawnHashTable(void);
void AllocateHT(void);
#if MP
void StoreHT(hash_t, int, int, int, int, int, int, int, struct HTEntry *);
#else
void StoreHT(hash_t, int, int, int, int, int, int, int);
#endif
void StorePT(hash_t, int, struct PawnFacts *);
void StoreST(hash_t, int);
#if MP
int ProbeHT(hash_t, int *, int, int *, int *, int, int, struct HTEntry *);
#else
int ProbeHT(hash_t, int *, int, int *, int *, int);
#endif
int ProbePT(hash_t, int *, struct PawnFacts *);
int ProbeST(hash_t, int *);
void ShowHashStatistics(void);
void GuessHTSizes(char *);

extern BitBoard ShiftUp(BitBoard);
extern BitBoard ShiftDown(BitBoard);
extern BitBoard ShiftLeft(BitBoard);
extern BitBoard ShiftRight(BitBoard);

void InitAll(void);

void PrintBitBoard(BitBoard);
void Print2BitBoards(BitBoard, BitBoard);

void KingPawnStructure(void);

int InterpretCommandPB(char *string);

int MateThreat(struct Position *, int);
int GenMates(struct Position *, int *mvs);

void InitMoves(void);

struct SearchData *CreateSearchData(struct Position *);
void FreeSearchData(struct SearchData *);
void EnterNode(struct SearchData *);
void LeaveNode(struct SearchData *);
int NextMove(struct SearchData *);
int NextEvasion(struct SearchData *);
int NextMoveQ(struct SearchData *, int);
void PutKiller(struct SearchData *, int);

void SaveGame(struct Position *, char *);
void LoadGame(struct Position *, char *);
int scanHeader(FILE *, struct PGNHeader *);
int scanMove(FILE *fin, char *nextMove);

void InitEGTB(char *);
int ProbeEGTB(struct Position *, int *, int);

int MaterialBalance(struct Position *);
int ScorePosition(struct Position *, int, int);
int CheckDraw(struct Position *);
int IsPassed(struct Position *, int, int);
void InitScore(struct Position *);
int OptimisticBound(void);

int Iterate(struct Position *);
void SearchRoot(struct Position *);
void AnalysisMode(struct Position *);
int PermanentBrain(struct Position *);
#if MP
void StopHelpers(void);
#endif

void SearchHeader(void);
void SearchOutput(int depth, int time, int score, char *line, int nodes);
void SearchOutputFailHighLow(int, int, int, char *, int);
void StateMachine(void);

int SwapOff(struct Position *, int);

void DoTC(struct Position *, int);
void CalcTime(struct Position *, float *, float *);

void OpenLogFile(char *name);
void Print(int, char *, ...);
void PrintNoLog(int, char *, ...);
int InputReady(void);
int ReadLine(char *buffer, int cnt);
char *TimeToText(unsigned int);
char *ScoreToText(int);
unsigned int GetTime(void);
char *GetTmpFileName(void);
int KingDist(int, int);
int MinDist(int, int);
int FileDist(int, int);
int EdgeDist(int);
int ManhattanDist(int, int);
char *nextToken(char **, const char *);

void ShowVersion(void);

ran_t Random64();
double Random();
void InitRandom(ran_t seed);

void RecogInit(void);
int ProbeRecognizer(struct Position *p, int *score);

void DoBookLearning(void);

#ifdef __cplusplus
}
#endif

#endif /* AMY_H */
