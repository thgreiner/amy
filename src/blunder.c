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
#include "pgn.h"
#include "search.h"
#include "types.h"
#include "utils.h"
#include "yaml.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#define THRESHOLD 2000

move_t get_best_move_from_comment(char *comment, struct Position *p,
                                  char *eval_buf) {
    char *ptr = comment;

    move_t best_move = M_NONE;
    int best_count = -1;

    while (*ptr && *ptr != '=') {
        ptr++;
    }
    ptr++;

    char *start_eval = ptr;

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

    char *buffer = malloc(len + 1);
    abort_if_allocation_failed(buffer);

    strncpy(buffer, ptr, len + 1);

    char *brko;

    for (char *elem = strtok_r(buffer, ", ]", &brko); elem;
         elem = strtok_r(NULL, ", ]", &brko)) {
        char *brki;

        char *move = strtok_r(elem, ":", &brki);
        char *count = strtok_r(NULL, ":", &brki);

        move_t parsed_move = ParseSAN(p, move);

        if (parsed_move != M_NONE) {
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
        Print(0, "Cannot open input file %s\n");
        return;
    }

    FILE *fout = fopen("filtered.pgn", "w");

    while (!scanHeader(fin, &header)) {
        struct Position *p;

        if (header.is_setup) {
            p = CreatePositionFromEPD(header.fen);
        } else {
            p = InitialPosition();
        }

        print_header(fout, &header);

        move_t last_move = M_NONE;

        while (!scanMove(fin, move)) {
            if (!(strlen(move) < 12)) {
                printf("\n<%s>\n", move);
                exit(1);
            }

            get_and_reset_comment(comment, sizeof(comment) - 1);

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
                    ShowPosition(p);
                    Print(1, ">>> best move from comment: %s\n",
                          SAN(p, best_move, sanbuf));
                    Print(1, ">>> optimal move: %s\n",
                          SAN(p, optimal_move, sanbuf));
                    Print(1, ">>> score diff: %d\n",
                          search_evaluation - alternate_evaluation);

                    fprintf(fout, "{ q=%s; p=[%s:100] } ", evalbuf,
                            SAN(p, optimal_move, sanbuf));
                }

                DoMove(p, last_move);
            }

            move_t themove = ParseSAN(p, move);

            if (themove == M_NONE)
                break;

            if ((p->ply % 2) == 0) {
                fprintf(fout, "%d. ", 1 + p->ply / 2);
            }
            fprintf(fout, "%s ", SAN(p, themove, san_buffer));

            if (themove != M_NONE) {
                DoMove(p, themove);
                last_move = themove;
            } else {
                break;
            }
        }

        fprintf(fout, "%s\n\n", header.result);
        get_and_reset_comment(comment, sizeof(comment) - 1);

        FreePosition(p);
    }

    Print(0, "\n");
    fclose(fin);
}
