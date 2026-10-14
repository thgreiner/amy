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

#include "dbase.h"
#include "pgn_read.h"
#include "pgn_write.h"
#include "safe_malloc.h"
#include "search.h"
#include "types.h"
#include "utils.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#define THRESHOLD 2000

move_t get_best_move_from_comment(const char *comment, struct Position *p,
                                  char *eval_buf) {
    const char *ptr = comment;

    move_t best_move = M_NONE;
    int best_count = -1;

    while (*ptr && *ptr != '=') {
        ptr++;
    }
    ptr++;

    const char *start_eval = ptr;

    while (*ptr && *ptr != ';') {
        ptr++;
    }
    ptr++;

    size_t eval_len = ptr - start_eval - 1;
    strncpy(eval_buf, start_eval, eval_len);
    eval_buf[eval_len] = '\0';

    while (*ptr && *ptr != '[') {
        ptr++;
    }
    ptr++;

    size_t len = strlen(ptr);

    char *buffer = safe_malloc(len + 1);

    strncpy(buffer, ptr, len + 1);

    char *brko;

    for (char *elem = strtok_r(buffer, ", ]", &brko); elem;
         elem = strtok_r(NULL, ", ]", &brko)) {
        char *brki;

        char *move = strtok_r(elem, ":", &brki);
        char *count = strtok_r(NULL, ":", &brki);

        move_t parsed_move = ParseSAN(p, move);

        if (parsed_move != M_NONE && count != NULL) {
            int parsed_count = atoi(count);
            if (parsed_count > best_count) {
                best_move = parsed_move;
                best_count = parsed_count;
            }
        }
    }

    free(buffer);

    return best_move;
}

void BlunderCheck(char *file_name) {
    struct PGNHeader header;
    char move[12];
    char comment[2048];
    char san_buffer[16];

    FILE *fin = fopen(file_name, "r");
    if (fin == NULL) {
        print(0, "Cannot open input file %s\n");
        return;
    }

    FILE *fout = fopen("filtered.pgn", "w");

    int verbosity_safe = Verbosity;
    Verbosity = 0;

    unsigned int count_games = 0;
    unsigned int count_blunders = 0;

    while (!scanHeader(fin, &header)) {
        struct Position *p;

        count_games++;

        if (header.fen != NULL) {
            p = CreatePositionFromEPD(header.fen);
        } else {
            p = InitialPosition();
        }

        print_header(fout, &header);

        move_t last_move = M_NONE;
        int width = 0;

        while (!scanMove(fin, move)) {
            if (!(strlen(move) < 12)) {
                print(0, "\nMalformed move: <%s>\n", move);
                exit(1);
            }

            get_and_reset_comment(comment, sizeof(comment) - 1);
            // print(1, "**** %s\n", comment);

            if (last_move != M_NONE && strlen(comment) != 0) {
                char sanbuf[16], evalbuf[16];

                UndoMove(p, last_move);

                move_t best_move =
                    get_best_move_from_comment(comment, p, evalbuf);

                int search_evaluation;
                int alternate_evaluation;

                move_t optimal_move = Iterate(p, &search_evaluation, best_move,
                                              &alternate_evaluation);
                int score_diff = search_evaluation - alternate_evaluation;

                if (optimal_move != best_move && score_diff >= 1500) {
                    count_blunders++;

                    ShowPosition(p);

                    // Logging
                    print(1, ">>> best move from comment: %s\n",
                          SAN(p, best_move, sanbuf));
                    print(1, ">>> optimal move: %s\n",
                          SAN(p, optimal_move, sanbuf));
                    print(1, ">>> score diff: %d\n",
                          search_evaluation - alternate_evaluation);

                    EmitPgnLine(fout, &width, "{ q=%s; p=[%s:100] } ", evalbuf,
                                SAN(p, optimal_move, sanbuf));
                } else {
                    EmitPgnLine(fout, &width, "{%s} ", comment);
                }

                DoMove(p, last_move);
            }

            move_t themove = ParseSAN(p, move);

            if (themove == M_NONE) {
                get_and_reset_comment(comment, sizeof(comment) - 1);
                if (strlen(comment) != 0) {
                    EmitPgnLine(fout, &width, "{%s} ", comment);
                }
                break;
            }

            if ((p->ply % 2) == 0) {
                EmitPgnLine(fout, &width, "%d. ", 1 + p->ply / 2);
            }

            EmitPgnLine(fout, &width, "%s ", SAN(p, themove, san_buffer));

            DoMove(p, themove);
            last_move = themove;
        }

        get_and_reset_comment(comment, sizeof(comment) - 1);
        if (strlen(comment) != 0) {
            fprintf(fout, "{%s} ", comment);
        }

        fprintf(fout, "%s\n\n", header.result);

        FreePosition(p);
        free_header(&header);
    }

    fclose(fin);

    Verbosity = verbosity_safe;

    print(0, "Games: %u Blunders: %u\n", count_games, count_blunders);
}
