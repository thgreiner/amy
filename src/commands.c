/*

    Amy - a chess playing program

    Copyright (c) 2002-2025, Thorsten Greiner
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
 * commands.c - Command interpreter
 */

#include "amy.h"
#include "heap.h"
#include <signal.h>

static void Quit(char *);
static void Show(char *);
static void ShowEco(char *);
static void Test(char *);
static void SetTime(char *);
static void SetXBoard(char *);
static void Go(char *);
static void Force(char *);
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
static void Perft(char *);
static void Load(char *);
static void Save(char *);
static void Prefs(char *);
static void Flatten(char *);
static void XboardTime(char *);
static void Analyze(char *);
static void StopAnalyze(char *);
static void SelfPlay(char *);
static void TestNext(char *);
static void Conf(char *);
static void SaveConf(char *);
static void ShowScore(char *);
static void TestScore(char *);

static struct CommandEntry Commands[] = {
    {"analyze", &Analyze, false, false, "enter analyze mode (xboard)", NULL},
    {"anno", &Anno, false, false, "annotate a game", NULL},
    {"bench", &Benchmark, false, false, "run a benchmark", NULL},
    {"book", &Book, false, false, "display book moves", NULL},
    {"bk", &Book, false, false, "display book moves (xboard)", NULL},
    {"bookup", &Bookup, false, false, "create a book", NULL},
    {"conf", &Conf, false, false, "load scoring config", NULL},
    {"conf-save", &SaveConf, false, false, "save scoring config", NULL},
    {"d", &Show, true, false, "display current position", NULL},
    {"distribution", &ShowDistribution, true, false,
     "show terms of distribution", NULL},
    {"e", &ShowEco, false, false, "show ECO code", NULL},
    {"eco", &ParseEcoPgn, false, false, "create ECO database", NULL},
    {"easy", &Easy, true, false, "switch off permanent brain", NULL},
    {"epd", &SetEPD, false, false, "set position in EPD", NULL},
    {"edit", &Edit, false, false, "edit position (xboard!)", NULL},
    {"exit", &StopAnalyze, true, true, "exit analyze mode (xboard)", NULL},
    {"flatten", &Flatten, true, false, "flatten book", NULL},
    {"force", &Force, true, false, "switch force mode (xboard)", NULL},
    {"go", &Go, false, false, "start searching", NULL},
    {"hard", &Hard, true, false, "switch on permanent brain", NULL},
    {"help", &Help, true, false, "show help", NULL},
    {"level", &SetTime, false, false, "set time control", NULL},
    {"load", &Load, false, false, "load game from PGN file", NULL},
    {"moves", &MovesCmd, false, false, "show legal moves", NULL},
    {"name", &Name, true, false, "set the opponents name", NULL},
    {"new", &NewGame, true, true, "start new game", NULL},
    {"nopost", &NoPost, true, false, "switch off post mode (xboard)", NULL},
    {"perft", &Perft, false, false, "Run the perft benchmark", NULL},
    {"post", &Post, true, false, "switch on post mode (xboard)", NULL},
    {"prefs", &Prefs, false, false, "read opening book preferences", NULL},
    {"quit", &Quit, true, false, "quit Amy", NULL},
    {"s", &ShowScore, true, false, "display static evaluation", NULL},
    {"save", &Save, false, false, "save game to PGN file", NULL},
    {"self", &SelfPlay, false, false, "start self play", NULL},
    {"show", &Show, true, false, "display current position", NULL},
    {"test", &Test, false, false, "run EPD test suite", NULL},
    {"test-score", &TestScore, false, false,
     "run static evaluatior on EPD test suite", NULL},
    {"time", &XboardTime, true, false, "set time (xboard)", NULL},
    {"undo", &Undo, true, true, "undo last move", NULL},
    {"warranty", &ShowWarranty, true, false, "show terms of warranty", NULL},
    {"xboard", &SetXBoard, false, false, "switch to xboard compatibility",
     NULL},
    {"?", &MoveNow, true, false, "move now", NULL},
    {"tn", &TestNext, false, false, "test the move generators", NULL},
    {NULL, NULL, false, false, NULL, NULL}};

struct Command *ParseInput(char *line) {
    static struct Command theCommand;
    char *token;
    int move;
    struct CommandEntry *entry;

    token = nextToken(&line, " \t\n\r");
    if (token == NULL)
        return NULL;

    /*
     * Try to interpret as move.
     */

    move = ParseSAN(CurrentPosition, token);
    if (move == M_NONE) {
        move = ParseGSAN(CurrentPosition, token);
    }

    if (move != M_NONE) {
        theCommand.move = move;
        theCommand.command_func = NULL;
        theCommand.args = NULL;
        return &theCommand;
    }

    entry = Commands;
    while (entry->name) {
        if (!strcmp(entry->name, token)) {
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

void ExecuteCommand(struct Command *theCommand) {
    if (theCommand->move != M_NONE) {
        DoMove(CurrentPosition, theCommand->move);
    } else {
        COMMAND cfunc = theCommand->command_func;
        cfunc(theCommand->args);
    }
}

static void Quit(char *args) {
    (void)args;
#if MP
    StopHelpers();
#endif
    FreePosition(CurrentPosition);
    Print(0, "\n\nI'll be back.\n");
    exit(0);
}

static void Show(char *args) {
    (void)args;
    ShowPosition(CurrentPosition);
}

static void ShowEco(char *args) {
    (void)args;
    char eco[128] = "";

    FindEcoCode(CurrentPosition, eco);

    Print(0, "Eco code is %s\n", eco);
}

static void Test(char *fname) {
    struct Position *p;
    int solved = 0, total = 0;
    FILE *fin, *fout;
    int i;
    int btav = 0;
    int btval;
    int bsval;
    int lctval = 1900;
    char line[256];

    if (!fname) {
        Print(0, "Usage: test <filename>\n");
        return;
    }

    fin = fopen(fname, "r");
    if (!fin) {
        Print(0, "Couldn't open %s for input.\n", fname);
        return;
    }

    fout = fopen("nsolved.epd", "w");

    for (i = 1;; i++) {
        int move, j;
        bool correct = false;

        if (fgets(line, 256, fin) == NULL)
            break;
        Print(0, "Problem %d:\n", i);
        p = CreatePositionFromEPD(line);
        ShowPosition(p);

        /* TestSwap(); */

        move = Iterate(p);
        for (j = 0; goodmove[j] != M_NONE; j++)
            if (move == goodmove[j])
                correct = true;

        if (!correct && badmove[0] != M_NONE) {
            correct = true;

            for (j = 0; badmove[j] != M_NONE; j++)
                if (move == badmove[j])
                    correct = false;
        }

        total++;
        if (correct) {
            Print(0, "solved!\n");
            solved++;

            btav += (FHTime < 900) ? FHTime : 900;

            if (FHTime < 10)
                lctval += 30;
            else if (FHTime < 30)
                lctval += 25;
            else if (FHTime < 90)
                lctval += 20;
            else if (FHTime < 180)
                lctval += 15;
            else if (FHTime < 390)
                lctval += 10;
            else if (FHTime <= 600)
                lctval += 5;
        } else {
            Print(0, "not solved!\n");
            btav += 900;
            if (fout)
                fprintf(fout, "%s", line);
        }

        btval = 2630 - (btav / total);
        bsval = (btav / (17 * 60));
        bsval = 2830 - bsval * bsval;

        Print(0, "solved %d out of %d  (BT2630 = %d, LCT2 = %d, BS2830 = %d)\n",
              solved, total, btval, lctval, bsval);
        Print(0, "-----------------------------------------------\n\n");

        FreePosition(p);
    }

    if (fin)
        fclose(fin);
    if (fout)
        fclose(fout);
}

static void TestScore(char *fname) {
    struct Position *p;
    FILE *fin, *fout;
    char line[256];

    if (!fname) {
        Print(0, "Usage: test-score <filename>\n");
        return;
    }

    fin = fopen(fname, "r");
    if (!fin) {
        Print(0, "Couldn't open %s for input.\n", fname);
        return;
    }

    fout = fopen("test_score.epd", "w");

    for (;;) {

        if (fgets(line, 256, fin) == NULL)
            break;
        p = CreatePositionFromEPD(line);
        InitEvaluation(p);
        int score = EvaluatePosition(p);

        if (fout) {
            int l = strlen(line);
            l--;
            line[l] = '\0';
            l--;
            if (line[l] == ';') {
                line[l] = '\0';
            }

            fprintf(fout, "%s; score %d;\n", line, score);
        }

        FreePosition(p);
    }

    if (fin)
        fclose(fin);
    if (fout)
        fclose(fout);
}

static void SetTime(char *arg) {
    char *args[3];

    args[0] = strtok(arg, " \t");
    args[1] = strtok(NULL, " \t");
    args[2] = strtok(NULL, " \t");

    SetTimeControl(args, XBoardMode);
}

static void SetXBoard(char *args) {
    (void)args;
    XBoardMode = true;
    Verbosity = 1;

    Print(0, "\n");
    Print(0, "feature myname=\"Amy " VERSION "\"\n");
    Print(0, "feature san=1\n");
    Print(0, "feature name=1\n");
    Print(0, "feature done=1\n");

    /* Set up signal handler fuer Ctrl+C */
    signal(SIGINT, SIG_IGN);
}

static void Go(char *args) {
    (void)args;
    ForceMode = false;
    State = STATE_CALCULATING;
}

static void Force(char *args) {
    (void)args;
    ForceMode = true;
    AbortSearch = true;
}

void NewGame(char *args) {
    (void)args;
    /*
     * Create a new save file.
     */
    GetTmpFileName(AutoSaveFileName, sizeof(AutoSaveFileName));

    ForceMode = false;
    FreePosition(CurrentPosition);
    CurrentPosition = InitialPosition();
    if (State != STATE_ANALYZING) {
        State = STATE_WAITING;
    }
    ResetTimeControl(!XBoardMode);
}

static void MoveNow(char *args) {
    (void)args;
    AbortSearch = true;
}

void Edit(char *args) {
    (void)args;
    bool editing = true;
    int i;
    int side = White;
    char buffer[16];
    struct Position *p = CurrentPosition;

    for (i = 0; i < 64; i++)
        p->piece[i] = Neutral;
    p->mask[White][0] = p->mask[Black][0] = 0;

    while (editing) {
        int sq;

        if (!ReadLine(buffer, 256))
            break;

        sq = (buffer[1] - 'a') + 8 * (buffer[2] - '1');

        switch (buffer[0]) {
        case '.':
            editing = false;
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
    if (p->piece[e1] == King) {
        if (p->piece[h1] == Rook)
            p->castle |= CastleMask[White][0];
        if (p->piece[a1] == Rook)
            p->castle |= CastleMask[White][1];
    }
    if (p->piece[e8] == -King) {
        if (p->piece[h8] == -Rook)
            p->castle |= CastleMask[Black][0];
        if (p->piece[a8] == -Rook)
            p->castle |= CastleMask[Black][1];
    }
    RecalcAttacks(p);
    ShowPosition(p);
}

static void Undo(char *args) {
    (void)args;
    if (CurrentPosition->ply > 0) {
        UndoMove(CurrentPosition, (CurrentPosition->actLog - 1)->gl_Move);
    }
}

static void Book(char *args) {
    (void)args;
    QueryBook(CurrentPosition);
}

static void Post(char *args) {
    (void)args;
    PostMode = true;
}

static void NoPost(char *args) {
    (void)args;
    PostMode = false;
}

static void Easy(char *args) {
    (void)args;
    EasyMode = true;
}

static void Hard(char *args) {
    (void)args;
    EasyMode = false;
}

static void MovesCmd(char *args) {
    (void)args;
    ShowMoves(CurrentPosition);
}

static void SetEPD(char *args) {
    if (!args) {
        Print(0, "Usage: epd <EPD>\n");
        return;
    }
    FreePosition(CurrentPosition);
    CurrentPosition = CreatePositionFromEPD(args);
}

static void RunAnnotate(char *fname, int side) {
    FILE *fin = fopen(fname, "r");
    struct Position *p;

    if (fin) {
        struct PGNHeader header;
        char move[16];

        while (!scanHeader(fin, &header)) {
            p = InitialPosition();
            while (!scanMove(fin, move)) {
                int themove = ParseSAN(p, move);
                if (themove != M_NONE) {
                    char san_buffer[16];
                    ShowPosition(p);
                    Print(0, "%s(%d): ", p->turn == White ? "White" : "Black",
                          (p->ply / 2) + 1);
                    Print(0, "%s\n", SAN(p, themove, san_buffer));
                    if (side == -1 || (side == p->turn)) {
                        Iterate(p);
                    }
                    DoMove(p, themove);
                }
            }
            FreePosition(p);
        }
        fclose(fin);
    } else
        Print(0, "Couldn't open %s\n", fname);
}

static void Anno(char *args) {
    int side = -1;
    char *arg1 = strtok(args, " \n\r");
    char *arg2 = strtok(NULL, " \n\r");

    if (!arg1) {
        Print(0, "Usage: anno <file> [w|b|wb]\n");
        return;
    }

    if (arg2) {
        if (!strcmp(arg2, "w")) {
            side = White;
        } else if (!strcmp(arg2, "b")) {
            side = Black;
        }
    }

    RunAnnotate(arg1, side);
}

static char *distribution =
    "\n    Copyright (c) 2002-2025, Thorsten Greiner\n"
    "    All rights reserved.\n"
    "\n"
    "    Redistribution and use in source and binary forms, with or without\n"
    "    modification, are permitted provided that the following conditions "
    "are met:\n"
    "\n"
    "    * Redistributions of source code must retain the above copyright "
    "notice,\n"
    "      this list of conditions and the following disclaimer.\n"
    "\n"
    "    * Redistributions in binary form must reproduce the above copyright "
    "notice,\n"
    "      this list of conditions and the following disclaimer in the "
    "documentation\n"
    "      and/or other materials provided with the distribution.\n"
    "\n";

static char *warranty =
    "\n THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "
    "\"AS IS\"\n"
    " AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, "
    "THE\n"
    " IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR "
    "PURPOSE ARE\n"
    " DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE "
    "LIABLE\n"
    " FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR "
    "CONSEQUENTIAL\n"
    " DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS "
    "OR\n"
    " SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) "
    "HOWEVER\n"
    " CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT "
    "LIABILITY,\n"
    " OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF "
    "THE USE\n"
    " OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.\n"
    "\n";

static void ShowWarranty(char *args) {
    (void)args;
    Print(0, warranty);
}

static void ShowDistribution(char *args) {
    (void)args;
    Print(0, distribution);
    Print(0, warranty);
}

static void Help(char *args) {
    struct CommandEntry *entry = Commands;

    (void)args;
    Print(2, "\nEnter a legal move (like e4, Nxd5, O-O, d1=Q+) or one of the\n"
             "following commands:\n\n");
    while (entry->name) {
        char template[] = ". . . . . . . . ";
        memcpy(template, entry->name, strlen(entry->name));
        Print(2, template);
        if (entry->short_help) {
            Print(2, "%s", entry->short_help);
        }
        Print(2, "\n");

        entry++;
    }

    Print(2, "\n");
}

static void Benchmark(char *args) {
    (void)args;
    int move = g1 | (f3 << 6);
    int i;
    const int cycles = 10000000;
    int start, end;
    double elapsed;
    struct Position *p;

    p = InitialPosition();

    start = GetTime();

    for (i = cycles; i > 0; i--) {
        DoMove(p, move);
        UndoMove(p, move);
    }

    end = GetTime();

    elapsed = (end - start) / 100.0;

    Print(0, "Nf3: %.2g secs, %g moves/sec\n", elapsed, cycles / elapsed);

    FreePosition(p);
}

static BitBoard SearchFully(struct Position *p, BitBoard cnt, int depth,
                            heap_t heap) {
    unsigned int i;

    if (depth <= 0) {
        return cnt + 1;
    }

    push_section(heap);
    PLegalMoves(p, heap);

    for (i = heap->current_section->start; i < heap->current_section->end;
         i++) {
        int move = heap->data[i];
        if (move & M_CANY && !MayCastle(p, move))
            continue;

        DoMove(p, move);
        if (!InCheck(p, OPP(p->turn))) {
            cnt = SearchFully(p, cnt, depth - 1, heap);
        }
        UndoMove(p, move);
    }
    pop_section(heap);

    return cnt;
}

static void Perft(char *args) {
    if (args == NULL) {
        Print(0, "Usage: perft <depth>\n");
        return;
    }

    int depth;
    sscanf(args, "%d", &depth);

    BitBoard cnt = 0;
    heap_t heap = allocate_heap();

    int start = GetTime();
    cnt = SearchFully(CurrentPosition, cnt, depth, heap);
    int end = GetTime();

    free_heap(heap);

    double elapsed = (end - start) / 100.0;
    double nps = cnt / elapsed;

    Print(0, "Perft(%d): %lld terminal positions in %g secs (%g nps)\n", depth,
          cnt, elapsed, nps);
}

static void Load(char *args) {
    if (args == NULL) {
        Print(0, "Usage: load <filename>\n");
        return;
    }
    CurrentPosition = InitialPosition();
    LoadGame(CurrentPosition, args);
}

static void Save(char *args) {
    if (args == NULL) {
        Print(0, "Usage: save <filename>\n");
        return;
    }
    SaveGame(CurrentPosition, args);
}

static void Prefs(char *args) { CreateLearnDB(args); }

static void Flatten(char *args) {
    int threshold;
    if (args == NULL) {
        Print(0, "Usage: flatten <count>\n");
        return;
    }

    threshold = atoi(args);
    if (threshold < 1) {
        threshold = 1;
    }

    FlattenBook(threshold);
}

static void XboardTime(char *args) {
    if (args != NULL) {
        int seconds = atoi(args) / 100;

        /*
         * xboard sends time for the side not to move.
         */

        Time[ComputerSide] = seconds;
    }
}

static void Analyze(char *args) {
    (void)args;
    State = STATE_ANALYZING;
}

static void StopAnalyze(char *args) {
    (void)args;
    State = STATE_WAITING;
}

static void Name(char *args) {
    if (args) {
        strncpy(OpponentName, args, OPP_NAME_LENGTH - 1);
        Print(2, "Your name is %s\n", OpponentName);
    }
}

static void SelfPlay(char *args) {
    (void)args;
    SelfPlayMode = true;
    State = STATE_CALCULATING;
}

static void TestNext(char *args) {
    (void)args;
    TestNextGenerators(CurrentPosition);
}

static void Conf(char *args) {
    if (args == NULL) {
        Print(0, "Usage: conf <filename>\n");
        return;
    }

    LoadEvaluationConfig(args);
    RecalcAttacks(CurrentPosition);
}

static void SaveConf(char *args) {
    if (args == NULL) {
        Print(0, "Usage: save-conf <filename>\n");
        return;
    }

    SaveEvaluationConfig(args);
}

static void ShowScore(char *args) {
    (void)args;
    InitEvaluation(CurrentPosition);
    int score = EvaluatePosition(CurrentPosition);
    Print(0, "Static evaluation: %d\n", score);
}
