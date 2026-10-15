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

/*
 * commands.c - Command interpreter
 */

#include <signal.h>
#include <string.h>

#include "bitboard.h"
#include "blunder.h"
#include "bookup.h"
#include "commands.h"
#include "dbase.h"
#include "eco.h"
#include "evaluation.h"
#include "evaluation_config.h"
#include "filter.h"
#include "heap.h"
#include "inline.h"
#include "next.h"
#include "pgn_read.h"
#include "pgn_write.h"
#include "search.h"
#include "state_machine.h"
#include "time_ctl.h"
#include "utils.h"

static void cmd_quit(char *);
static void cmd_show(char *);
static void cmd_show_eco(char *);
static void cmd_test(char *);
static void cmd_set_time(char *);
static void cmd_set_xboard(char *);
static void cmd_go(char *);
static void cmd_force(char *);
static void cmd_name(char *);
static void cmd_move_now(char *);
static void cmd_edit(char *);
static void cmd_undo(char *);
static void cmd_book(char *);
static void cmd_post(char *);
static void cmd_nopost(char *);
static void cmd_easy(char *);
static void cmd_hard(char *);
static void cmd_moves(char *);
static void cmd_set_epd(char *);
static void cmd_annotate(char *);
static void cmd_show_warranty(char *);
static void cmd_show_distribution(char *);
static void cmd_help(char *);
static void cmd_benchmark(char *);
static void cmd_perft(char *);
static void cmd_load(char *);
static void cmd_save(char *);
static void cmd_prefs(char *);
static void cmd_flatten(char *);
static void cmd_xboard_time(char *);
static void cmd_analyze(char *);
static void cmd_stop_analyze(char *);
static void cmd_selfplay(char *);
static void cmd_test_next(char *);
static void cmd_conf(char *);
static void cmd_save_conf(char *);
static void cmd_show_score(char *);
static void cmd_test_score(char *);
static void cmd_set_search_depth(char *);

static struct CommandEntry Commands[] = {
    {"analyze", &cmd_analyze, false, false, "enter analyze mode (xboard)",
     NULL},
    {"anno", &cmd_annotate, false, false, "annotate a game", NULL},
    {"bench", &cmd_benchmark, false, false, "run a benchmark", NULL},
    {"blunder", &blunder_check, false, false, "check for blunders", NULL},
    {"book", &cmd_book, false, false, "display book moves", NULL},
    {"bk", &cmd_book, false, false, "display book moves (xboard)", NULL},
    {"bookup", &book_up, false, false, "create a book", NULL},
    {"conf", &cmd_conf, false, false, "load scoring config", NULL},
    {"conf-save", &cmd_save_conf, false, false, "save scoring config", NULL},
    {"d", &cmd_show, true, false, "display current position", NULL},
    {"depth", &cmd_set_search_depth, false, false, "set maximum search depth",
     NULL},
    {"distribution", &cmd_show_distribution, true, false,
     "show terms of distribution", NULL},
    {"e", &cmd_show_eco, false, false, "show ECO code", NULL},
    {"eco", &eco_parse_pgn, false, false, "create ECO database", NULL},
    {"easy", &cmd_easy, true, false, "switch off permanent brain", NULL},
    {"epd", &cmd_set_epd, false, false, "set position in EPD", NULL},
    {"edit", &cmd_edit, false, false, "edit position (xboard!)", NULL},
    {"exit", &cmd_stop_analyze, true, true, "exit analyze mode (xboard)", NULL},
    {"filter", &filter_quiescent_positions, false, false,
     "filter quiescent positions", NULL},
    {"flatten", &cmd_flatten, true, false, "flatten book", NULL},
    {"force", &cmd_force, true, false, "switch force mode (xboard)", NULL},
    {"go", &cmd_go, false, false, "start searching", NULL},
    {"hard", &cmd_hard, true, false, "switch on permanent brain", NULL},
    {"help", &cmd_help, true, false, "show help", NULL},
    {"level", &cmd_set_time, false, false, "set time control", NULL},
    {"load", &cmd_load, false, false, "load game from PGN file", NULL},
    {"moves", &cmd_moves, false, false, "show legal moves", NULL},
    {"name", &cmd_name, true, false, "set the opponents name", NULL},
    {"new", &cmd_newgame, true, true, "start new game", NULL},
    {"nopost", &cmd_nopost, true, false, "switch off post mode (xboard)", NULL},
    {"perft", &cmd_perft, false, false, "Run the perft benchmark", NULL},
    {"post", &cmd_post, true, false, "switch on post mode (xboard)", NULL},
    {"prefs", &cmd_prefs, false, false, "read opening book preferences", NULL},
    {"quit", &cmd_quit, true, false, "quit Amy", NULL},
    {"s", &cmd_show_score, true, false, "display static evaluation", NULL},
    {"save", &cmd_save, false, false, "save game to PGN file", NULL},
    {"self", &cmd_selfplay, false, false, "start self play", NULL},
    {"show", &cmd_show, true, false, "display current position", NULL},
    {"test", &cmd_test, false, false, "run EPD test suite", NULL},
    {"test-score", &cmd_test_score, false, false,
     "run static evaluatior on EPD test suite", NULL},
    {"time", &cmd_xboard_time, true, false, "set time (xboard)", NULL},
    {"undo", &cmd_undo, true, true, "undo last move", NULL},
    {"warranty", &cmd_show_warranty, true, false, "show terms of warranty",
     NULL},
    {"xboard", &cmd_set_xboard, false, false, "switch to xboard compatibility",
     NULL},
    {"?", &cmd_move_now, true, false, "move now", NULL},
    {"tn", &cmd_test_next, false, false, "test the move generators", NULL},
    {NULL, NULL, false, false, NULL, NULL}};

char AutoSaveFileName[64];

struct Command *cmd_parse_input(char *line) {
    static struct Command theCommand;
    char *token;
    int move;
    struct CommandEntry *entry;

    token = utils_next_token(&line, " \t\n\r");
    if (token == NULL)
        return NULL;

    /*
     * Try to interpret as move.
     */

    move = dbase_parse_san(CurrentPosition, token);
    if (move == M_NONE) {
        move = dbase_parse_gsan(CurrentPosition, token);
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
            theCommand.args = utils_next_token(&line, "\n\r");
            return &theCommand;
        }
        entry++;
    }

    return NULL;
}

void cmd_execute(struct Command *theCommand) {
    if (theCommand->move != M_NONE) {
        dbase_do_move(CurrentPosition, theCommand->move);
    } else {
        COMMAND cfunc = theCommand->command_func;
        cfunc(theCommand->args);
    }
}

static void cmd_quit(char *args) {
    (void)args;
#if MP
    search_stop_helpers();
#endif
    dbase_free_position(CurrentPosition);
    print(0, "\n\nI'll be back.\n");
    exit(0);
}

static void cmd_show(char *args) {
    (void)args;
    dbase_show_position(CurrentPosition);
}

static void cmd_show_eco(char *args) {
    (void)args;
    char eco[128] = "";

    eco_find_code(CurrentPosition, eco);

    print(0, "Eco code is %s\n", eco);
}

static void cmd_test(char *fname) {
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
        print(0, "Usage: test <filename>\n");
        return;
    }

    fin = fopen(fname, "r");
    if (!fin) {
        print(0, "Couldn't open %s for input.\n", fname);
        return;
    }

    fout = fopen("nsolved.epd", "w");

    for (i = 1;; i++) {
        int move, j;
        bool correct = false;

        if (fgets(line, 256, fin) == NULL)
            break;
        print(0, "Problem %d:\n", i);
        p = dbase_create_position_from_epd(line);
        dbase_show_position(p);

        /* TestSwap(); */

        move = search_iterate(p, NULL, M_NONE, NULL);
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
            print(0, "solved!\n");
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
            print(0, "not solved!\n");
            btav += 900;
            if (fout)
                fprintf(fout, "%s", line);
        }

        btval = 2630 - (btav / total);
        bsval = (btav / (17 * 60));
        bsval = 2830 - bsval * bsval;

        print(0, "solved %d out of %d  (BT2630 = %d, LCT2 = %d, BS2830 = %d)\n",
              solved, total, btval, lctval, bsval);
        print(0, "-----------------------------------------------\n\n");

        dbase_free_position(p);
    }

    if (fin)
        fclose(fin);
    if (fout)
        fclose(fout);
}

static void cmd_test_score(char *fname) {
    struct Position *p;
    FILE *fin, *fout;
    char line[256];

    if (!fname) {
        print(0, "Usage: test-score <filename>\n");
        return;
    }

    fin = fopen(fname, "r");
    if (!fin) {
        print(0, "Couldn't open %s for input.\n", fname);
        return;
    }

    fout = fopen("test_score.epd", "w");

    for (;;) {

        if (fgets(line, 256, fin) == NULL)
            break;
        p = dbase_create_position_from_epd(line);
        eval_init(p);
        int score = eval_position(p);

        if (fout) {
            size_t l = strlen(line);
            l--;
            line[l] = '\0';
            l--;
            if (line[l] == ';') {
                line[l] = '\0';
            }

            fprintf(fout, "%s; score %d;\n", line, score);
        }

        dbase_free_position(p);
    }

    if (fin)
        fclose(fin);
    if (fout)
        fclose(fout);
}

static void cmd_set_time(char *arg) {
    char *args[3];

    args[0] = strtok(arg, " \t");
    args[1] = strtok(NULL, " \t");
    args[2] = strtok(NULL, " \t");

    time_ctl_set(args, XBoardMode);
}

static void cmd_set_xboard(char *args) {
    (void)args;
    XBoardMode = true;
    Verbosity = 1;

    print(0, "\n");
    print(0, "feature myname=\"Amy " VERSION "\"\n");
    print(0, "feature san=1\n");
    print(0, "feature name=1\n");
    print(0, "feature done=1\n");

    /* Set up signal handler fuer Ctrl+C */
    signal(SIGINT, SIG_IGN);
}

static void cmd_go(char *args) {
    (void)args;
    ForceMode = false;
    State = STATE_CALCULATING;
}

static void cmd_force(char *args) {
    (void)args;
    ForceMode = true;
    AbortSearch = true;
}

void cmd_newgame(char *args) {
    (void)args;
    /*
     * Create a new save file.
     */
    utils_get_tmp_file_name(AutoSaveFileName, sizeof(AutoSaveFileName));

    ForceMode = false;
    dbase_free_position(CurrentPosition);
    CurrentPosition = dbase_initial_position();
    if (State != STATE_ANALYZING) {
        State = STATE_WAITING;
    }
    time_ctl_reset(!XBoardMode);
}

static void cmd_move_now(char *args) {
    (void)args;
    AbortSearch = true;
}

void cmd_edit(char *args) {
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

        if (!utils_read_line(buffer, 256))
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
            bitboard_set_bit(p->mask[side][0], sq);
            break;
        case 'N':
            p->piece[sq] = PIECEID(Knight, side);
            bitboard_set_bit(p->mask[side][0], sq);
            break;
        case 'B':
            p->piece[sq] = PIECEID(Bishop, side);
            bitboard_set_bit(p->mask[side][0], sq);
            break;
        case 'R':
            p->piece[sq] = PIECEID(Rook, side);
            bitboard_set_bit(p->mask[side][0], sq);
            break;
        case 'Q':
            p->piece[sq] = PIECEID(Queen, side);
            bitboard_set_bit(p->mask[side][0], sq);
            break;
        case 'K':
            p->piece[sq] = PIECEID(King, side);
            bitboard_set_bit(p->mask[side][0], sq);
            break;
        }
    }

    p->castle = p->enPassant = 0;

    dbase_recalc_attacks(p);
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
    dbase_recalc_attacks(p);
    dbase_show_position(p);
}

static void cmd_undo(char *args) {
    (void)args;
    if (CurrentPosition->ply > 0) {
        dbase_undo_move(CurrentPosition,
                        (CurrentPosition->actLog - 1)->gl_Move);
    }
}

static void cmd_book(char *args) {
    (void)args;
    book_query(CurrentPosition);
}

static void cmd_post(char *args) {
    (void)args;
    PostMode = true;
}

static void cmd_nopost(char *args) {
    (void)args;
    PostMode = false;
}

static void cmd_easy(char *args) {
    (void)args;
    EasyMode = true;
}

static void cmd_hard(char *args) {
    (void)args;
    EasyMode = false;
}

static void cmd_moves(char *args) {
    (void)args;
    dbase_show_moves(CurrentPosition);
}

static void cmd_set_epd(char *args) {
    if (!args) {
        print(0, "Usage: epd <EPD>\n");
        return;
    }
    dbase_free_position(CurrentPosition);
    CurrentPosition = dbase_create_position_from_epd(args);
}

static void cmd_run_annotate(char *fname, int side) {
    FILE *fin = fopen(fname, "r");
    struct Position *p;

    if (fin) {
        struct PGNHeader header;
        char move[16];

        while (!pgn_scan_header(fin, &header)) {
            p = dbase_initial_position();
            while (!pgn_scan_move(fin, move)) {
                int themove = dbase_parse_san(p, move);
                if (themove != M_NONE) {
                    char san_buffer[16];
                    dbase_show_position(p);
                    print(0, "%s(%d): ", p->turn == White ? "White" : "Black",
                          (p->ply / 2) + 1);
                    print(0, "%s\n", dbase_san(p, themove, san_buffer));
                    if (side == -1 || (side == p->turn)) {
                        search_iterate(p, NULL, M_NONE, NULL);
                    }
                    dbase_do_move(p, themove);
                }
            }
            dbase_free_position(p);
            pgn_free_header(&header);
        }
        fclose(fin);
    } else
        print(0, "Couldn't open %s\n", fname);
}

static void cmd_annotate(char *args) {
    int side = -1;
    char *arg1 = strtok(args, " \n\r");
    char *arg2 = strtok(NULL, " \n\r");

    if (!arg1) {
        print(0, "Usage: anno <file> [w|b|wb]\n");
        return;
    }

    if (arg2) {
        if (!strcmp(arg2, "w")) {
            side = White;
        } else if (!strcmp(arg2, "b")) {
            side = Black;
        }
    }

    cmd_run_annotate(arg1, side);
}

static char *distribution =
    "\n    Copyright (c) 2002-2026, Thorsten Greiner\n"
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

static void cmd_show_warranty(char *args) {
    (void)args;
    print(0, warranty);
}

static void cmd_show_distribution(char *args) {
    (void)args;
    print(0, distribution);
    print(0, warranty);
}

static void cmd_help(char *args) {
    struct CommandEntry *entry = Commands;

    (void)args;
    print(2, "\nEnter a legal move (like e4, Nxd5, O-O, d1=Q+) or one of the\n"
             "following commands:\n\n");
    while (entry->name) {
        char template[] = ". . . . . . . . ";
        memcpy(template, entry->name, strlen(entry->name));
        print(2, template);
        if (entry->short_help) {
            print(2, "%s", entry->short_help);
        }
        print(2, "\n");

        entry++;
    }

    print(2, "\n");
}

static void cmd_benchmark(char *args) {
    (void)args;
    int move = g1 | (f3 << 6);
    int i;
    const int cycles = 10000000;
    unsigned long start, end;
    double elapsed;
    struct Position *p;

    p = dbase_initial_position();

    start = utils_get_time();

    for (i = cycles; i > 0; i--) {
        dbase_do_move(p, move);
        dbase_undo_move(p, move);
    }

    end = utils_get_time();

    elapsed = (double)(end - start) / 100.0;

    print(0, "Nf3: %.2g secs, %g moves/sec\n", elapsed, cycles / elapsed);

    dbase_free_position(p);
}

static BitBoard cmd_search_fully(struct Position *p, BitBoard cnt, int depth,
                                 heap_t heap) {
    unsigned int i;

    if (depth <= 0) {
        return cnt + 1;
    }

    heap_push_section(heap);
    dbase_plegal_moves(p, heap);

    for (i = heap->current_section->start; i < heap->current_section->end;
         i++) {
        int move = heap->data[i];
        if (move & M_CANY && !dbase_may_castle(p, move))
            continue;

        dbase_do_move(p, move);
        if (!dbase_in_check(p, OPP(p->turn))) {
            cnt = cmd_search_fully(p, cnt, depth - 1, heap);
        }
        dbase_undo_move(p, move);
    }
    heap_pop_section(heap);

    return cnt;
}

static void cmd_perft(char *args) {
    if (args == NULL) {
        print(0, "Usage: perft <depth>\n");
        return;
    }

    int depth;
    sscanf(args, "%d", &depth);

    BitBoard cnt = 0;
    heap_t heap = heap_allocate();

    unsigned long start = utils_get_time();
    cnt = cmd_search_fully(CurrentPosition, cnt, depth, heap);
    unsigned long end = utils_get_time();

    heap_free(heap);

    double elapsed = (double)(end - start) / 100.0;
    double nps = (double)cnt / elapsed;

    print(0, "cmd_perft(%d): %lld terminal positions in %g secs (%g nps)\n",
          depth, cnt, elapsed, nps);
}

static void cmd_load(char *args) {
    if (args == NULL) {
        print(0, "Usage: load <filename>\n");
        return;
    }
    CurrentPosition = dbase_initial_position();
    pgn_load_game(CurrentPosition, args);
}

static void cmd_save(char *args) {
    if (args == NULL) {
        print(0, "Usage: save <filename>\n");
        return;
    }
    pgn_save_game(CurrentPosition, args);
}

static void cmd_prefs(char *args) { book_create_learn_db(args); }

static void cmd_flatten(char *args) {
    int threshold;
    if (args == NULL) {
        print(0, "Usage: flatten <count>\n");
        return;
    }

    threshold = atoi(args);
    if (threshold < 1) {
        threshold = 1;
    }

    book_flatten(threshold);
}

static void cmd_xboard_time(char *args) {
    if (args != NULL) {
        int seconds = atoi(args) / 100;

        /*
         * xboard sends time for the side not to move.
         */

        Time[ComputerSide] = seconds;
    }
}

static void cmd_analyze(char *args) {
    (void)args;
    State = STATE_ANALYZING;
}

static void cmd_stop_analyze(char *args) {
    (void)args;
    State = STATE_WAITING;
}

static void cmd_name(char *args) {
    if (args) {
        strncpy(OpponentName, args, OPP_NAME_LENGTH - 1);
        print(2, "Your name is %s\n", OpponentName);
    }
}

static void cmd_selfplay(char *args) {
    (void)args;
    SelfPlayMode = true;
    State = STATE_CALCULATING;
}

static void cmd_test_next(char *args) {
    (void)args;
    next_test_generators(CurrentPosition);
}

static void cmd_conf(char *args) {
    if (args == NULL) {
        print(0, "Usage: conf <filename>\n");
        return;
    }

    eval_config_load(args);
    dbase_recalc_attacks(CurrentPosition);
}

static void cmd_save_conf(char *args) {
    if (args == NULL) {
        print(0, "Usage: save-conf <filename>\n");
        return;
    }

    eval_config_save(args);
}

static void cmd_show_score(char *args) {
    (void)args;
    eval_init(CurrentPosition);
    int score = eval_position(CurrentPosition);
    print(0, "Static evaluation: %d\n", score);
}

static void cmd_set_search_depth(char *args) {
    if (args == NULL) {
        print(0, "Usage: depth <depth>");
        return;
    }

    search_set_max_depth(atoi(args));
}
