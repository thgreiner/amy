#include "dbase.h"
#include "evaluation.h"
#include "inline.h"
#include "pgn.h"
#include "search.h"
#include "utils.h"

#include <string.h>

#define THRESHOLD 600

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

    while (!scanHeader(fin, &header)) {
        struct Position *p;

        if (header.is_setup) {
            p = CreatePositionFromEPD(header.fen);
        } else {
            p = InitialPosition();
        }

        print_header(fout, &header);

        bool last_position_was_quiet = true;

        while (!scanMove(fin, move)) {
            if (!(strlen(move) < 12)) {
                printf("\n<%s>\n", move);
                exit(1);
            }

            get_and_reset_comment(comment, sizeof(comment) - 1);

            if (last_position_was_quiet) {
                fprintf(fout, "{ %s }\n", strip(comment));
            }

            move_t themove = ParseSAN(p, move);

            if (themove == M_NONE)
                break;

            if ((p->ply % 2) == 0) {
                fprintf(fout, "%d. ", 1 + p->ply / 2);
            }
            fprintf(fout, "%s ", SAN(p, themove, san_buffer));

            last_position_was_quiet = true;

            if (!GameEnd(p)) {
                const int static_evaluation = EvaluatePosition(p);
                const int dynamic_evaluation = QuiescenceSearch(p);

                const int diff = ABS(static_evaluation - dynamic_evaluation);

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
    }

    Print(0, "\n");
    fclose(fin);
}
