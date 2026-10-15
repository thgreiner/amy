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
 * pgn_write.c - pgn write
 */

#include "config.h"
#include "safe_malloc.h"
#include <stdarg.h>
#include <stddef.h>

#include <stdio.h>
#include <string.h>

#if HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#include <time.h>

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "dbase.h"
#include "eco.h"
#include "pgn_write.h"
#include "state_machine.h"

#define AMY_NAME "Amy " VERSION

char OpponentName[OPP_NAME_LENGTH] = "Opponent";

static void pgn_get_date_time(char *date_buf, char *time_buf) {
    time_t tnow;
    struct tm *now;

    time(&tnow);
    now = localtime(&tnow);
    strftime(date_buf, 15, "%Y.%m.%d", now);
    strftime(time_buf, 15, "%H:%M:%S", now);
}

static void pgn_get_hostname(char *hostname_buf) {
#if HAVE_GETHOSTNAME
    gethostname(hostname_buf, 256);
#else
    strcpy(hostname_buf, "Your computer");
#endif
}

static char *pgn_get_white_name(void) {
    return (ComputerSide == White) ? (AMY_NAME) : (OpponentName);
}

static char *pgn_get_black_name(void) {
    return (ComputerSide == Black) ? (AMY_NAME) : (OpponentName);
}

static void pgn_save_moves(FILE *fout, struct Position *p) {
    int ply = p->ply;
    move_t *history = safe_calloc(ply, sizeof(int));

    for (int i = ply; i > 0; i--) {
        int move = (p->actLog - 1)->gl_Move;
        history[i - 1] = move;
        dbase_undo_move(p, move);
    }

    int width = 0;

    for (int i = 0; i < ply; i++) {
        int move = history[i];
        if ((i & 1) == 0) {
            pgn_emit_line(fout, &width, "%d. ", (i / 2) + 1);
        }

        char san_buffer[16];
        char *san = dbase_san(p, move, san_buffer);

        pgn_emit_line(fout, &width, "%s ", san);

        dbase_do_move(p, move);
    }

    free(history);
}

void pgn_save_game(struct Position *p, char *file_name) {
    /* Do not save if no move made yet. */
    if (p->ply > 0) {
        FILE *fout = fopen(file_name, "w");
        if (fout) {
            const char *gameend;
            char shortgameend[8] = "1/2-1/2";
            char eco[128] = "";
            char date_buf[16];
            char time_buf[16];
            char hostname_buf[256];

            pgn_get_hostname(hostname_buf);
            pgn_get_date_time(date_buf, time_buf);

            gameend = dbase_game_end(CurrentPosition);
            if (gameend == NULL) {
                gameend = "*";
                strcpy(shortgameend, gameend);
            } else {
                strncpy(shortgameend, gameend, 3);
                if (shortgameend[0] == '0' || shortgameend[2] == '0') {
                    shortgameend[3] = '\0';
                }
            }

            struct PGNHeader header = {
                .event = "Amy game",
                .round = "?",
                .is_setup = false,
            };

            header.result = shortgameend;
            header.site = hostname_buf;
            header.date = date_buf;
            header.time = time_buf;

            if (eco_find_code(p, eco)) {
                header.eco = eco;
            }

            header.white = pgn_get_white_name();
            header.black = pgn_get_black_name();

            pgn_print_header(fout, &header);

            pgn_save_moves(fout, p);

            fprintf(fout, "\n%s\n\n", gameend);
            fclose(fout);
        }
    }
}

/*
 * print a formatted message to fout as fprintf would, track the current
 * line width, and start a new line when it exceeds 72 characters.
 * The width is derived from the number of characters actually written,
 * so callers no longer need to compute it by hand.
 */
void pgn_emit_line(FILE *fout, int *width, const char *fmt, ...) {
    va_list ap, copy;

    va_start(ap, fmt);
    va_copy(copy, ap);
    int needed = vsnprintf(NULL, 0, fmt, copy);
    va_end(copy);
    if (needed <= 0) {
        va_end(ap);
        return;
    }

    char *buf = safe_malloc(needed + 1);
    int written = vsnprintf(buf, (size_t)needed + 1, fmt, ap);
    va_end(ap);
    if (written > 0) {
        fwrite(buf, 1, (size_t)written, fout);
        *width += written;
    }
    free(buf);

    if (*width > 72) {
        fputc('\n', fout);
        *width = 0;
    }
}

static inline char *pgn_str_or_empty(char *str) { return str ? str : ""; }

void pgn_print_header(FILE *fout, struct PGNHeader *header) {
    fprintf(fout, "[Event \"%s\"]\n", pgn_str_or_empty(header->event));
    fprintf(fout, "[Site \"%s\"]\n", pgn_str_or_empty(header->site));
    fprintf(fout, "[Date \"%s\"]\n", pgn_str_or_empty(header->date));

    if (header->time) {
        fprintf(fout, "[Time \"%s\"]\n", header->time);
    }

    fprintf(fout, "[Round \"%s\"]\n", pgn_str_or_empty(header->round));

    if (header->eco) {
        fprintf(fout, "[ECO \"%s\"]\n", header->eco);
    }

    fprintf(fout, "[White \"%s\"]\n", pgn_str_or_empty(header->white));
    fprintf(fout, "[Black \"%s\"]\n", pgn_str_or_empty(header->black));
    fprintf(fout, "[Result \"%s\"]\n", pgn_str_or_empty(header->result));

    if (header->fen) {
        fprintf(fout, "[SetUp \"1\"]\n");
        fprintf(fout, "[FEN \"%s\"]\n", header->fen);
    }

    fprintf(fout, "\n");
}
