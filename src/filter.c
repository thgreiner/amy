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

void FilterQuiescentPositions(char *file_name) {
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

    FILE *dout = fopen("evaluation_data.csv", "w");
    fprintf(dout, "eval,prob\n");

    int verbosity_safe = Verbosity;
    Verbosity = 0;

    unsigned int count_games = 0;
    unsigned int count_comments = 0;

    while (!scanHeader(fin, &header)) {
        struct Position *p;

        count_games++;

        if (header.is_setup) {
            p = CreatePositionFromEPD(header.fen);
        } else {
            p = InitialPosition();
        }

        print_header(fout, &header);

        bool last_position_was_quiet = false;
        int last_position_evaluation = 0;

        int width = 0;

        while (!scanMove(fin, move)) {
            if (!(strlen(move) < 12)) {
                printf("\n<%s>\n", move);
                exit(1);
            }

            get_and_reset_comment(comment, sizeof(comment) - 1);

            if (last_position_was_quiet) {
                count_comments++;

                char *stripped_comment = strip(comment);
                EmitPgnLine(fout, &width, "{ %s }", stripped_comment);

                char eval_buf[16];
                memcpy(eval_buf, stripped_comment + 2, 6);
                eval_buf[6] = '\0';

                fprintf(dout, "%d,%s\n", last_position_evaluation, eval_buf);
            }

            move_t themove = ParseSAN(p, move);

            if (themove == M_NONE)
                break;

            if ((p->ply % 2) == 0) {
                EmitPgnLine(fout, &width, "%d. ", 1 + p->ply / 2);
            }

            EmitPgnLine(fout, &width, "%s ", SAN(p, themove, san_buffer));

            last_position_was_quiet = true;

            if (!GameEnd(p)) {
                const int static_evaluation = EvaluatePosition(p);
                const int dynamic_evaluation = QuiescenceSearch(p);

                const int diff = ABS(static_evaluation - dynamic_evaluation);

                last_position_evaluation = static_evaluation;

                if (diff > THRESHOLD) {
                    // ShowPosition(p);
                    // Print(0, "Static: %d Dynamic: %d\n", static_evaluation,
                    //      dynamic_evaluation);
                    last_position_was_quiet = false;
                } else {
                    int search_evaluation;
                    Iterate(p, &search_evaluation, M_NONE, NULL);

                    const int search_diff =
                        ABS(static_evaluation - search_evaluation);

                    if (search_diff > THRESHOLD) {
                        last_position_was_quiet = false;
                    }
                }
            }

            if (themove != M_NONE) {
                DoMove(p, themove);
            } else {
                break;
            }
        }

        fprintf(fout, "%s\n\n", header.result);
        get_and_reset_comment(comment, sizeof(comment) - 1);

        FreePosition(p);
        free_header(&header);
    }

    fclose(dout);
    fclose(fout);

    fclose(fin);

    Verbosity = verbosity_safe;

    Print(0, "Games: %u Comments: %u\n", count_games, count_comments);
}
