/*

    Amy - a chess playing program

    Copyright (c) 2014, Thorsten Greiner
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this
      list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
    FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
    DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
    OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

/*
 * commands.c - Command interpreter
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "amy.h"

static void Quit(char *);
static void Show(char *);
static void ShowEco(char *);
static void Test(char *);
static void SetTime(char *);
static void SetXBoard(char *);
static void Go(char *);
static void Force(char *);
static void New(char *);
static void Name(char *);
static void MoveNow(char *);
static void Edit(char *);
static void Undo(char *);
static void Book(char *);
static void Post(char *);
static void NoPost(char *);
static void Easy(char *);
static void Hard(char *);
static void MovesCmd(char *);
static void SetEPD(char *);
static void Anno(char *);
static void ShowWarranty(char *);
static void ShowDistribution(char *);
static void Help(char *);
static void Benchmark(char *);
static void Perft(char *args);
static void Load(char *);
static void Save(char *);
static void Prefs(char *);
static void Flatten(char *);
static void XboardTime(char *);
static void Analyze(char *args);
static void StopAnalyze(char *args);

static struct CommandEntry Commands[] =
{
    { "analyze",      &Analyze,          FALSE, FALSE, 
      "enter analyze mode (xboard)", NULL },
    { "anno",         &Anno,             FALSE, FALSE, 
      "annotate a game", NULL },
    { "bench",        &Benchmark,        FALSE, FALSE,
      "run a benchmark", NULL },
    { "book",         &Book,             FALSE, FALSE,
      "display book moves", NULL },
    { "bk",         &Book,               FALSE, FALSE,
      "display book moves (xboard)", NULL },
    { "bookup",       &Bookup,           FALSE, FALSE,
      "create a book", NULL },
    { "d",            &Show,             TRUE,  FALSE,
      "display current position", NULL },
    { "distribution", &ShowDistribution, TRUE,  FALSE,
      "show terms of distribution", NULL },
    { "e",            &ShowEco,          FALSE, FALSE,
      "show ECO code", NULL },
    { "eco",          &ParseEcoPgn,      FALSE, FALSE,
      "create ECO database", NULL },
    { "easy",         &Easy,             TRUE,  FALSE,
      "switch off permanent brain", NULL },
    { "epd",          &SetEPD,           FALSE, FALSE,
      "set position in EPD", NULL },
    { "edit",         &Edit,             FALSE, FALSE,
      "edit position (xboard!)", NULL },
    { "exit",         &StopAnalyze,      TRUE,  TRUE,
      "exit analyze mode (xboard)", NULL },
    { "flatten",      &Flatten,          TRUE,  FALSE,
      "flatten book", NULL },
    { "force",        &Force,            TRUE,  FALSE,
      "switch force mode (xboard)", NULL },
    { "go",           &Go,               FALSE, FALSE,
      "start searching", NULL },
    { "hard",         &Hard,             TRUE,  FALSE,
      "switch on permanent brain", NULL },
    { "help",         &Help,             TRUE,  FALSE,
      "show help", NULL },
    { "level",        &SetTime,          FALSE, FALSE,
      "set time control", NULL },
    { "load",         &Load,             FALSE, FALSE,
      "load game from PGN file", NULL },
    { "moves",        &MovesCmd,         FALSE, FALSE,
      "show legal moves", NULL },
    { "name",         &Name,             TRUE,  FALSE,
      "set the opponents name", NULL },
    { "new",          &New,              TRUE,  TRUE,
      "start new game", NULL },
    { "nopost",       &NoPost,           TRUE,  FALSE,
      "switch off post mode (xboard)", NULL },
    { "perft",        &Perft,            FALSE, FALSE,
      "Run the perft benchmark", NULL },
    { "post",         &Post,             TRUE,  FALSE,
      "switch on post mode (xboard)", NULL },
    { "prefs",        &Prefs,            FALSE, FALSE,
      "read opening book preferences", NULL },
    { "quit",         &Quit,             TRUE,  FALSE,
      "quit Amy", NULL },
    { "save",         &Save,             FALSE, FALSE,
      "save game to PGN file", NULL },
    { "show",         &Show,             TRUE,  FALSE,
      "display current position", NULL },
    { "test",         &Test,             FALSE, FALSE,
      "run EPD test suite", NULL },
    { "time",         &XboardTime,       TRUE, FALSE,
      "set time (xboard)", NULL },
    { "undo",         &Undo,             TRUE, TRUE, 
      "undo last move", NULL },
    { "warranty",     &ShowWarranty,     TRUE, FALSE,
      "show terms of warranty", NULL },
    { "xboard",       &SetXBoard,        FALSE, FALSE,
      "switch to xboard compatibility", NULL },
    { "?",            &MoveNow,          TRUE,  FALSE,
      "move now", NULL },
    { NULL, NULL, 0 }
};

struct Command *ParseInput(char *line)
{
    static struct Command theCommand;
    char *token;
    int move;
    struct CommandEntry *entry;

    token = nextToken(&line, " \t\n\r");
    if(token == NULL) return NULL;

    /*
     * Try to interpret as move.
     */

    move = ParseSAN(CurrentPosition, token);
    if(move == M_NONE) {
        move = ParseGSAN(CurrentPosition, token);
    }

    if(move != M_NONE) {
        theCommand.move = move;
        theCommand.command_func = NULL;
        theCommand.args = NULL;
        return &theCommand;
    }

    entry = Commands;
    while(entry->name) {
        if(!strcmp(entry->name, token)) {
            theCommand.move = M_NONE;
            theCommand.command_func = entry->command_func;
            theCommand.allowed_during_search = entry->allowed_during_search;
            theCommand.interrupts_search = entry->interrupts_search;
            theCommand.args = nextToken(&line, "\n\r");
            return &theCommand;
        }
        entry++;
    }

    return NULL;
}

void ExecuteCommand(struct Command *theCommand)
{
    if(theCommand->move != M_NONE) {
        DoMove(CurrentPosition, theCommand->move);
    }
    else {
        COMMAND cfunc = theCommand->command_func;
        cfunc(theCommand->args);
    }
}

static void Quit(char *args)
{
#if MP
    StopHelpers();
#endif
    Print(0, "\n\nI'll be back.\n");
    exit(0);
}

static void Show(char *args)
{
    ShowPosition(CurrentPosition);
}

static void ShowEco(char *args)
{
    char eco[128] = "";

    FindEcoCode(CurrentPosition, eco);

    Print(0, "Eco code is %s\n", eco);
}

static void Test(char *fname)
{
    struct Position *p;
    int solved = 0, total = 0;
    FILE *fin, *fout;
    int i;
    int btav = 0;
    int btval;
    int bsval;
    int lctval = 1900;
    char line[256];

    if(!fname) {
        Print(0, "Usage: test <filename>\n");
        return;
    }

    fin = fopen(fname, "r");
    if(!fin) {
        Print(0, "Couldn't open %s for input.\n", fname);
        return;
    }

    fout = fopen("nsolved.epd", "w");

    for(i=1; ; i++) {
        int move, j;
        int correct = FALSE;

        if(fgets(line, 256, fin) == NULL) break;
        Print(0, "Problem %d:\n", i);
        p = CreatePositionFromEPD(line);
        ShowPosition(p);

        /* TestSwap(); */

        move = Iterate(p);
        for(j=0; goodmove[j] != M_NONE; j++) 
            if(move == goodmove[j]) correct = TRUE;

        if(!correct && badmove[0] != M_NONE) {
            correct = TRUE;

            for(j=0; badmove[j] != M_NONE; j++) 
                if(move == badmove[j]) correct = FALSE;
        }

        total++;
        if(correct) {
            Print(0, "solved!\n");
            solved++;

            btav += (FHTime < 900) ? FHTime : 900;

            if(FHTime <10)         lctval += 30;
            else if(FHTime < 30)   lctval += 25;
            else if(FHTime < 90)   lctval += 20;
            else if(FHTime < 180)  lctval += 15;
            else if(FHTime < 390)  lctval += 10;
            else if(FHTime <= 600) lctval += 5;
        }
        else {
            Print(0, "not solved!\n");
            btav += 900;
            if(fout) fprintf(fout, "%s", line);
        }

        btval = 2630 - (btav/total);
        bsval = (btav/(17*60));
        bsval = 2830 - bsval*bsval;

        Print(0, "solved %d out of %d  (BT2630 = %d, LCT2 = %d, BS2830 = %d)\n", 
            solved, total, btval, lctval, bsval);
        Print(0, "-----------------------------------------------\n\n");

        FreePosition(p);
    }

    if(fin) fclose(fin);
    if(fout) fclose(fout);
}

static void SetTime(char *arg)
{
    int ttmoves, ttime, tminutes, tseconds, inc = 0;
    char *x, *colon;
    char *args[3];

    args[0] = strtok(arg, " \t");
    args[1] = strtok(NULL, " \t");
    args[2] = strtok(NULL, " \t");

    if(XBoardMode) {
        sscanf(args[0], "%d", &ttmoves);
        colon = strchr(args[1], ':');           /* check for time in xx:yy format */
        if(colon) {
            sscanf(args[1], "%d:%d", &tminutes, &tseconds);
            ttime = (tminutes * 60) + tseconds;
        } else {
            sscanf(args[1], "%d", &tminutes);
            ttime = tminutes * 60;
        }
        sscanf(args[2], "%d", &inc);

        TwoTimeControls = FALSE;
        TMoves = ttmoves;
        TTime  = ttime;
        Increment = inc;

        Moves[White] = Moves[Black] = TMoves;
        Time[White]  = Time[Black]  = TTime;
    }
    else {
        x = strtok(args[0], "/+ \t\n\r");
        if(x) {
            if(!strcmp(x, "sd")) ttmoves = 0;
            else if(!strcmp(x, "fixed")) ttmoves = -1;
            else sscanf(x, "%d", &ttmoves);
            x = strtok(NULL, "/ \t\n\r");
            if(x) {
                sscanf(x, "%d", &ttime);
                for(x++; *x; x++) {
                    if(*x == '+') {
                        sscanf(x+1, "%d", &inc);
                        break;
                    }
                }
                if(args[1] != NULL) {
                    x = strtok(args[1], " /\n\t\r");
                    TMoves2 = -1;
                    if(!strcmp(x, "sd")) TMoves2 = 0;
                    else sscanf(x, "%d", &TMoves2);
                    x = strtok(NULL, " /\n\t\r");
                    if(x) {
                        TTime2 = -1;
                        sscanf(x, "%d", &TTime2);
                        if(TMoves2 >= 0 && TTime2 > 0) 
                        TwoTimeControls = TRUE;
                    }
                }
                Print(0, "Timecontrol is ");
                if(ttmoves >= 0) {
                    if(ttmoves == 0) Print(0, "all ");
                    else             Print(0, "%d ", ttmoves);
                    if(inc) {
                        Print(0, "moves in %d mins + %d secs Increment\n", 
                              ttime, inc);
                    } else {
                        Print(0, "moves in %d mins\n", ttime);
                    }
                    TMoves = ttmoves;
                    TTime  = ttime*60;
                    Increment = inc;

                    Moves[White] = Moves[Black] = TMoves;
                    Time[White]  = Time[Black]  = TTime;
                }
                else {
                    Print(0, "%d seconds/move fixed time\n", ttime);
                    TMoves = -1;
                    TTime = ttime;
                }
                if(TwoTimeControls) {
                        Print(0, "Second Timecontrol is ");
                        if(TMoves2 == 0) Print(0, "all ");
                        else             Print(0, "%d ", TMoves2);
                        Print(0, "moves in %d mins\n", TTime2);
                        TTime2 *= 60;
                }
            }
        }
    }
}

static void SetXBoard(char *args)
{
    XBoardMode = TRUE;
    Verbosity = 1;

    Print(0, "\n");
    Print(0, "feature myname=\"Amy " VERSION "\"\n");
    Print(0, "feature san=1\n");
    Print(0, "feature name=1\n");
    Print(0, "feature done=1\n");

    /* Set up signal handler fuer Ctrl+C */
    signal(SIGINT, SIG_IGN);
}

static void Go(char *args)
{
    ForceMode = FALSE;
    State = STATE_CALCULATING;
}

static void Force(char *args)
{
    ForceMode = TRUE;
    AbortSearch = TRUE;
}

static void New(char *args)
{
    /*
     * Create a new save file.
     */

    strcpy(AutoSaveFileName, GetTmpFileName());

    ForceMode = FALSE;
    FreePosition(CurrentPosition);
    CurrentPosition = InitialPosition();
    if(State != STATE_ANALYZING) {
        State = STATE_WAITING;
    }
}

static void MoveNow(char *args)
{
    AbortSearch = TRUE;
}

void Edit(char *args)
{
    int editing = TRUE;
    int i;
    int side = White;
    char buffer[16];
    struct Position *p = CurrentPosition;

    for(i=0; i<64; i++) p->piece[i] = Neutral;
    p->mask[White][0] = p->mask[Black][0] = 0;

    while(editing) {
        int sq;

        if(!ReadLine(buffer, 256)) break;

        sq = (buffer[1]-'a') + 8*(buffer[2]-'1');

        switch(buffer[0]) {
        case '.':
            editing = FALSE;
            break;
        case 'c':
            side = OPP(side);
            break;
        case 'P':
            p->piece[sq] = PIECEID(Pawn, side);
            SetBit(p->mask[side][0], sq);
            break;
        case 'N':
            p->piece[sq] = PIECEID(Knight, side);
            SetBit(p->mask[side][0], sq);
            break;
        case 'B':
            p->piece[sq] = PIECEID(Bishop, side);
            SetBit(p->mask[side][0], sq);
            break;
        case 'R':
            p->piece[sq] = PIECEID(Rook, side);
            SetBit(p->mask[side][0], sq);
            break;
        case 'Q':
            p->piece[sq] = PIECEID(Queen, side);
            SetBit(p->mask[side][0], sq);
            break;
        case 'K':
            p->piece[sq] = PIECEID(King, side);
            SetBit(p->mask[side][0], sq);
            break;
        }
    }

    p->castle = p->enPassant = 0;

    RecalcAttacks(p);
    if(p->piece[e1] == King) {
                if(p->piece[h1] == Rook) p->castle |= CastleMask[White][0];
                if(p->piece[a1] == Rook) p->castle |= CastleMask[White][1];
    }
    if(p->piece[e8] == -King) {
                if(p->piece[h8] == -Rook) p->castle |= CastleMask[Black][0];
                if(p->piece[a8] == -Rook) p->castle |= CastleMask[Black][1];
    }
    RecalcAttacks(p);
    ShowPosition(p);
}

static void Undo(char *args)
{
    if(CurrentPosition->ply > 0) {
        UndoMove(CurrentPosition, (CurrentPosition->actLog-1)->gl_Move);
    }
}

static void Book(char *args)
{
    QueryBook(CurrentPosition);
}

static void Post(char *args)
{
    PostMode = TRUE;
}

static void NoPost(char *args)
{
    PostMode = FALSE;
}

static void Easy(char *args)
{
    EasyMode = TRUE;
}

static void Hard(char *args)
{
    EasyMode = FALSE;
}

static void MovesCmd(char *args)
{
    ShowMoves(CurrentPosition);
}

static void SetEPD(char *args)
{
    if(!args) {
        Print(0, "Usage: epd <EPD>\n");
        return;
    }
    FreePosition(CurrentPosition);
    CurrentPosition = CreatePositionFromEPD(args);
}

static void RunAnnotate(char *fname, int side)
{
    FILE *fin = fopen(fname, "r");
    struct Position *p;

    if(fin) {
        struct PGNHeader header;
        char move[16];


        while(!scanHeader(fin, &header)) {
            p = InitialPosition();
            while(!scanMove(fin, move)) {
                int themove = ParseSAN(p, move);
                if(themove != M_NONE) {
                    ShowPosition(p);
                    Print(0, "%s(%d): ", 
                            p->turn == White ? "White":"Black", (p->ply/2)+1);
                    Print(0, "%s\n", SAN(p, themove));
                    if(side == -1 || (side == p->turn)) {
                        Iterate(p);
                    }
                    DoMove(p, themove);
                }
            }
            FreePosition(p);
        }
    }
    else Print(0, "Couldn't open %s\n", fname);
}

static void Anno(char *args)
{
    int side = -1;
    char *arg1 = strtok(args, " \n\r");
    char *arg2 = strtok(NULL, " \n\r");

    if(!arg1) {
        Print(0, "Usage: anno <file> [w|b|wb]\n");
        return;
    }

    if(arg2) {
        if(!strcmp(arg2, "w")) {
            side = White;
        } else if(!strcmp(arg2, "b")) {
            side = Black;
        }
    }

    RunAnnotate(arg1, side);
}

static void ShowWarranty(char *args)
{
    static char *warranty1 = 
"\n    11. BECAUSE THE PROGRAM IS LICENSED FREE OF CHARGE, THERE IS NO WARRANTY\n"
"  FOR THE PROGRAM, TO THE EXTENT PERMITTED BY APPLICABLE LAW.  EXCEPT WHEN\n"
"  OTHERWISE STATED IN WRITING THE COPYRIGHT HOLDERS AND/OR OTHER PARTIES\n"
"  PROVIDE THE PROGRAM \"AS IS\" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED\n"
"  OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF\n"
"  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  THE ENTIRE RISK AS\n"
"  TO THE QUALITY AND PERFORMANCE OF THE PROGRAM IS WITH YOU.  SHOULD THE\n"
"  PROGRAM PROVE DEFECTIVE, YOU ASSUME THE COST OF ALL NECESSARY SERVICING,\n"
"  REPAIR OR CORRECTION.\n"
"\n";
    static char *warranty2 =
"    12. IN NO EVENT UNLESS REQUIRED BY APPLICABLE LAW OR AGREED TO IN WRITING\n"
"  WILL ANY COPYRIGHT HOLDER, OR ANY OTHER PARTY WHO MAY MODIFY AND/OR\n"
"  REDISTRIBUTE THE PROGRAM AS PERMITTED ABOVE, BE LIABLE TO YOU FOR DAMAGES,\n"
"  INCLUDING ANY GENERAL, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES ARISING\n"
"  OUT OF THE USE OR INABILITY TO USE THE PROGRAM (INCLUDING BUT NOT LIMITED\n"
"  TO LOSS OF DATA OR DATA BEING RENDERED INACCURATE OR LOSSES SUSTAINED BY\n"
"  YOU OR THIRD PARTIES OR A FAILURE OF THE PROGRAM TO OPERATE WITH ANY OTHER\n"
"  PROGRAMS), EVEN IF SUCH HOLDER OR OTHER PARTY HAS BEEN ADVISED OF THE\n"
"  POSSIBILITY OF SUCH DAMAGES.\n\n";

    Print(0, warranty1);
    Print(0, warranty2);
}

static void ShowDistribution(char *args)
{
    static char distribution[] = 
"\n    1. You may copy and distribute verbatim copies of the Program's\n"
"  source code as you receive it, in any medium, provided that you\n"
"  conspicuously and appropriately publish on each copy an appropriate\n"
"  copyright notice and disclaimer of warranty; keep intact all the\n"
"  notices that refer to this License and to the absence of any warranty;\n"
"  and give any other recipients of the Program a copy of this License\n"
"  along with the Program.\n"
"\n"
"  You may charge a fee for the physical act of transferring a copy, and\n"
"  you may at your option offer warranty protection in exchange for a fee.\n\n";

    Print(0, distribution);
}

static void Help(char *args)
{
    struct CommandEntry *entry = Commands;

    Print(2, "\nEnter a legal move (like e4, Nxd5, O-O, d1=Q+) or one of the\n"
             "following commands:\n\n");
    while(entry->name) {
        char template[] = ". . . . . . . . ";
        strncpy(template, entry->name, strlen(entry->name));
        Print(2, template);
        if(entry->short_help) {
            Print(2, "%s", entry->short_help);
        }
        Print(2, "\n");

        entry++;
    }

    Print(2, "\n");
}

static void Benchmark(char *args)
{
    int move = g1 | (f3 << 6);
    int i;
    const int cycles = 1000000;
    int start, end;
    double elapsed;
    struct Position *p;

    p = InitialPosition();

    start = GetTime();

    for(i=cycles; i>0; i--) {
        DoMove(p, move);
        UndoMove(p, move);
    }

    end = GetTime();

    elapsed = (end-start) / 100.0;

    Print(0, "Nf3: %.2g secs, %g moves/sec\n", elapsed, cycles / elapsed);

    FreePosition(p);
}

static BitBoard SearchFully(struct Position *p, BitBoard cnt, int depth) {
    int moves[128];
    int mcnt;
    int i;

    if(depth <= 0) {
        return cnt+1;
    }

    mcnt = PLegalMoves(p, moves);

    for(i=0; i<mcnt; i++) {
        int move = moves[i];
        if(move & M_CANY && !MayCastle(p, move)) continue;

        DoMove(p, move);
        if(!InCheck(p, OPP(p->turn))) {
            cnt = SearchFully(p, cnt, depth-1);
        }
        UndoMove(p, move);
    }

    return cnt;
}

static void Perft(char *args)
{
    BitBoard cnt = 0;
    int depth;
    int start, end;
    double elapsed;

    if(args == NULL) {
        Print(0, "Usage: perft <depth>\n");
        return;
    }

    sscanf(args, "%d", &depth);

    start = GetTime();
    cnt = SearchFully(CurrentPosition, cnt, depth);
    end = GetTime();

    elapsed = (end-start) / 100.0;

    Print(0, 
          "Perft(%d): %lld terminal positions in %g secs\n", 
          depth, cnt, elapsed);
}

static void Load(char *args)
{
    if(args == NULL) {
        Print(0, "Usage: load <filename>\n");
        return;
    }
    CurrentPosition = InitialPosition();
    LoadGame(CurrentPosition, args);
}

static void Save(char *args)
{
    if(args == NULL) {
        Print(0, "Usage: save <filename>\n");
        return;
    }
    SaveGame(CurrentPosition, args);
}

static void Prefs(char *args)
{
    CreateLearnDB(args);
}

static void Flatten(char *args)
{
    int threshold;
    if(args == NULL) {
        Print(0, "Usage: flatten <count>\n");
        return;
    }

    threshold = atoi(args);
    if(threshold < 1) {
        threshold = 1;
    }

    FlattenBook(threshold);
}

static void XboardTime(char *args)
{
    if(args != NULL) {
        int seconds = atoi(args) / 100;

        /*
         * xboard sends time for the side not to move.
         */

        Time[ComputerSide] = seconds;
    }
}

static void Analyze(char *args)
{
    State = STATE_ANALYZING;
}

static void StopAnalyze(char *args)
{
    State = STATE_WAITING;
}

static void Name(char *args)
{
    if(args) {
        strncpy(OpponentName, args, OPP_NAME_LENGTH);
        Print(2, "Your name is %s\n", OpponentName);
    }
}
