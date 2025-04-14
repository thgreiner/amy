#include "dbase.h"
#include "evaluation.h"
#include "heap.h"
#include "inline.h"
#include "pgn.h"
#include "search.h"
#include "utils.h"

#include <string.h>

#define THRESHOLD 600

void print_header(FILE *fout, struct PGNHeader *header) {
    fprintf(fout, "[Event \"%s\"]\n", header->event);
    fprintf(fout, "[Site \"%s\"]\n", header->site);
    fprintf(fout, "[Date \"%s\"]\n", header->date);
    fprintf(fout, "[Round \"%s\"]\n", header->round);
    fprintf(fout, "[White \"%s\"]\n", header->white);
    fprintf(fout, "[Black \"%s\"]\n", header->black);
    fprintf(fout, "[Result \"%s\"]\n", header->result);
    if (header->is_setup) {
        fprintf(fout, "[SetUp \"1\"]\n");
        fprintf(fout, "[FEN \"%s\"]\n", header->fen);
    }
    fprintf(fout, "\n");
}

char *strip(char *buffer) {
    char *start = buffer;
    while (*start == ' ') {
        start++;
    }

    int l = strlen(start) - 1;
    char *end = start + l;
    ;

    while (end > start && *end == ' ') {
        *end = 0;
        end--;
    }

    return start;
}
void FilterQuiescentPositions(char *file_name) {
    struct PGNHeader header;
    char move[12];
    char comment[2048];
    char san_buffer[16];
    bool last_position_was_not_quiet = false;

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

        while (!scanMove(fin, move)) {
            if (!(strlen(move) < 12)) {
                printf("\n<%s>\n", move);
                exit(1);
            }

            get_and_reset_comment(comment, sizeof(comment) - 1);
            char *comment_ptr = strip(comment);

            if (strlen(comment)) {
                if (last_position_was_not_quiet) {
                    strncat(comment_ptr, "; quiet=0", sizeof(comment) - 1);
                }
                fprintf(fout, "{ %s }\n", comment_ptr);
            }

            move_t themove = ParseSAN(p, move);

            if ((p->ply % 2) == 0) {
                fprintf(fout, "%d. ", 1 + p->ply / 2);
            }
            fprintf(fout, "%s ", SAN(p, themove, san_buffer));

            last_position_was_not_quiet = false;

            if (!GameEnd(p)) {
                int static_evaluation = EvaluatePosition(p);
                int dynamic_evaluation = QuiescenceSearch(p);

                int diff = ABS(static_evaluation - dynamic_evaluation);
                if (diff > THRESHOLD) {
                    // ShowPosition(p);
                    // Print(0, "Static: %d Dynamic: %d\n", static_evaluation,
                    //      dynamic_evaluation);
                    last_position_was_not_quiet = true;
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
