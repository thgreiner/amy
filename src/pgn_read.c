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
 * pgn_read.c - PGN reading
 */

#include "config.h"

#include <_stdio.h>
#include <string.h>

#include "dbase.h"
#include "pgn_read.h"
#include "safe_malloc.h"
#include "utils.h"

static char *read_line(FILE *fin) {
#if HAVE_GETLINE
    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    read = getline(&line, &len, fin);
    if (read == -1) {
        free(line);
        return NULL;
    }

    return line;
#else
#warning "getline() not available, using fgets()"
    char *line = safe_malloc(1024);
    if (!line) {
        return NULL;
    }

    if (!fgets(line, 1024, fin)) {
        free(line);
        return NULL;
    }

    return line;
#endif
}

int scanHeader(FILE *fin, struct PGNHeader *header) {
    char *current_line;
    int state = 0;

    /* Clear it */
    memset(header, 0, sizeof(struct PGNHeader));

    while ((current_line = read_line(fin)) != NULL) {
        if (current_line[0] == '[') {
            char *x = current_line + 1;
            char *key = nextToken(&x, " \"");
            char *value = nextToken(&x, "\"");
            size_t len = strlen(value);

            if (!strcmp("Event", key)) {
                header->event = safe_malloc(len + 1);
                strncpy(header->event, value, len + 1);
            }
            if (!strcmp("Site", key)) {
                header->site = safe_malloc(len + 1);
                strncpy(header->site, value, len + 1);
            }
            if (!strcmp("Date", key)) {
                header->date = safe_malloc(len + 1);
                strncpy(header->date, value, len + 1);
            }
            if (!strcmp("Time", key)) {
                header->time = safe_malloc(len + 1);
                strncpy(header->time, value, len + 1);
            }
            if (!strcmp("Round", key)) {
                header->round = safe_malloc(len + 1);
                strncpy(header->round, value, len + 1);
            }
            if (!strcmp("ECO", key)) {
                header->eco = safe_malloc(len + 1);
                strncpy(header->eco, value, len + 1);
            }
            if (!strcmp("White", key)) {
                header->white = safe_malloc(len + 1);
                strncpy(header->white, value, len + 1);
            }
            if (!strcmp("Black", key)) {
                header->black = safe_malloc(len + 1);
                strncpy(header->black, value, len + 1);
            }
            if (!strcmp("Result", key)) {
                header->result = safe_malloc(len + 1);
                strncpy(header->result, value, len + 1);
            }
            if (!strcmp("WhiteElo", key)) {
                header->white_elo = atoi(value);
            }
            if (!strcmp("BlackElo", key)) {
                header->black_elo = atoi(value);
            }
            if (!strcmp("FEN", key)) {
                header->fen = safe_malloc(len + 1);
                strncpy(header->fen, value, len + 1);
            }
            if (!strcmp("SetUp", key)) {
                header->is_setup = atoi(value);
            }

            state = 1;

            free(current_line);
        } else {
            if (state != 0)
                return 0;
        }
    }

    return 1;
}

void free_header(struct PGNHeader *header) {
    free(header->event);
    free(header->site);
    free(header->date);
    free(header->time);
    free(header->round);
    free(header->eco);
    free(header->white);
    free(header->black);
    free(header->result);
    free(header->fen);
}

static char comment_buffer[2048] = "";
static char *comment_ptr = comment_buffer;

int scanMove(FILE *fin, char *nextMove) {
    static char *current_line = NULL;
    static int haveLine = 0;
    static char *x;

    char *token;

    int braces = 0;
    int parens = 0;

    do {
        if (!haveLine) {
            free(current_line);
            current_line = read_line(fin);

            if (current_line == NULL)
                return 1;

            haveLine = 1;
            x = current_line;
        }

        if (braces) {
            if (*x == '\0') {
                haveLine = 0;
                continue;
            }

            *(comment_ptr) = *x;

            if (*(x++) == '}') {
                *comment_ptr = 0;
                braces = 0;
            }

            comment_ptr++;

            continue;
        } else if (parens) {
            if (*x == '\0') {
                haveLine = 0;
                continue;
            }

            if (*x == ')') {
                parens--;
            }
            if (*x == '(') {
                parens++;
            }
            x++;

            continue;
        } else {
            if (x && *x == '{') {
                braces = 1;
                x++;
                comment_ptr = comment_buffer;
                continue;
            }
            if (x && *x == '(') {
                parens = 1;
                x++;
                continue;
            }
            token = nextToken(&x, " .\n\r\t");

            if (token == NULL) {
                haveLine = 0;
                continue;
            }

            if (*token == '\0')
                continue;

            if (!strcmp("*", token) || !strcmp("1-0", token) ||
                !strcmp("0-1", token) || !strcmp("1/2-1/2", token))
                break;
            if (*token >= '0' && *token <= '9')
                continue;
            if (*token == '$')
                continue;
        }

        strcpy(nextMove, token);
        return 0;
    } while (1);

    return 1;
}

void get_and_reset_comment(char *destination, unsigned int length) {
    strncpy(destination, comment_buffer, length);

    comment_ptr = comment_buffer;
    *comment_buffer = 0;
}

void LoadGame(struct Position *p, char *file_name) {
    FILE *fin = fopen(file_name, "r");

    if (fin) {
        struct PGNHeader header;
        char move[16];
        if (!scanHeader(fin, &header)) {
            while (!scanMove(fin, move)) {
                int themove = ParseSAN(p, move);
                if (themove != M_NONE) {
                    DoMove(p, themove);
                }
            }
        }
        fclose(fin);
        free_header(&header);
    }
}
