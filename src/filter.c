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
#include "evaluation.h"
#include "inline.h"
#include "pgn_read.h"
#include "pgn_write.h"
#include "search.h"
#include "utils.h"

#include <stdio.h>
#include <string.h>

#define THRESHOLD 200

void filter_quiescent_positions(char *file_name) {
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

    FILE *dout = fopen("evaluation_data.csv", "w");
    fprintf(dout, "eval,prob\n");

    int verbosity_safe = Verbosity;
    Verbosity = 0;

    unsigned int count_games = 0;
    unsigned int count_comments = 0;

    while (!pgn_scan_header(fin, &header)) {
        struct Position *p;

        count_games++;

        if (header.is_setup) {
            p = dbase_create_position_from_epd(header.fen);
        } else {
            p = dbase_initial_position();
        }

        pgn_print_header(fout, &header);

        bool last_position_was_quiet = false;
        int last_position_evaluation = 0;

        int width = 0;

        while (!pgn_scan_move(fin, move)) {
            if (!(strlen(move) < 12)) {
                printf("\n<%s>\n", move);
                exit(1);
            }

            pgn_get_and_reset_comment(comment, sizeof(comment) - 1);

            if (last_position_was_quiet) {
                count_comments++;

                char *stripped_comment = utils_strip(comment);
                pgn_emit_line(fout, &width, "{ %s }", stripped_comment);

                char eval_buf[16];
                memcpy(eval_buf, stripped_comment + 2, 6);
                eval_buf[6] = '\0';

                fprintf(dout, "%d,%s\n", last_position_evaluation, eval_buf);
            }

            move_t themove = dbase_parse_san(p, move);

            if (themove == M_NONE)
                break;

            if ((p->ply % 2) == 0) {
                pgn_emit_line(fout, &width, "%d. ", 1 + p->ply / 2);
            }

            pgn_emit_line(fout, &width, "%s ", dbase_san(p, themove, san_buffer));

            last_position_was_quiet = true;

            if (!dbase_game_end(p)) {
                const int static_evaluation = eval_position(p);
                const int dynamic_evaluation = search_quiescence(p);

                const int diff = ABS(static_evaluation - dynamic_evaluation);

                last_position_evaluation = static_evaluation;

                if (diff > THRESHOLD) {
                    // dbase_show_position(p);
                    // print(0, "Static: %d Dynamic: %d\n", static_evaluation,
                    //      dynamic_evaluation);
                    last_position_was_quiet = false;
                } else {
                    int search_evaluation;
                    search_iterate(p, &search_evaluation, M_NONE, NULL);

                    const int search_diff =
                        ABS(static_evaluation - search_evaluation);

                    if (search_diff > THRESHOLD) {
                        last_position_was_quiet = false;
                    }
                }
            }

            if (themove != M_NONE) {
                dbase_do_move(p, themove);
            } else {
                break;
            }
        }

        fprintf(fout, "%s\n\n", header.result);
        pgn_get_and_reset_comment(comment, sizeof(comment) - 1);

        dbase_free_position(p);
        pgn_free_header(&header);
    }

    fclose(dout);
    fclose(fout);

    fclose(fin);

    Verbosity = verbosity_safe;

    print(0, "Games: %u Comments: %u\n", count_games, count_comments);
}
