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
 * bookup.c - opening book management routines
 */

#include "amy.h"
#include "tree.h"

#define BOOK_NAME "Book.db"
#define LEARN_NAME "Learn.db"

#ifdef BOOKDIR
#define DEFAULT_BOOK_NAME BOOKDIR PATH_SEPARATOR BOOK_NAME
#else
#warning "BOOKDIR is not defined!"
#endif

#define WITH_ELO 1

enum MoveCategory { GoodMove = 1, BadMove = 2 };

struct BookEntry {
    unsigned int win;  /* number of wins */
    unsigned int loss; /* number of losses */
    unsigned int draw; /* number of draws */
#if WITH_ELO
    unsigned int sumElo; /* sum of elo played */
    unsigned int nElo;   /* number of elo players */
#endif
};

struct LearnEntry {
    int flags;
    int learn_value;
};

struct BookQuery {
    struct BookEntry be;
    struct LearnEntry le;
};

static tree_node_t *BookDB = NULL;
static tree_node_t *LearnDB = NULL;

static tree_node_t *PutBookEntry(tree_node_t *database, hash_t hk, int result,
                                 int elo) {
    Print(9, "storing position\n");
    struct BookEntry *entry = NULL;

    if (database != NULL) {
        entry = (struct BookEntry *)lookup_value(database, (char *)&hk,
                                                 sizeof(hk), NULL);
    }

    if (entry == NULL) {
        entry = calloc(1, sizeof(struct BookEntry));
    }

    if (result == 1) {
        entry->win += 1;
    } else if (result == 0) {
        entry->draw += 1;
    } else if (result == -1) {
        entry->loss += 1;
    }
#if WITH_ELO
    if (elo != 0) {
        entry->sumElo += elo;
        entry->nElo += 1;
    }
#endif

    database = add_node(database, (char *)&hk, sizeof(hk), (char *)entry,
                        sizeof(struct BookEntry));
    free(entry);

    return database;
}

static void OpenBookFile(tree_node_t **db) {
    static bool error_printed = false;

    FILE *fin = fopen(BOOK_NAME, "rb");

#ifdef DEFAULT_BOOK_NAME
    if (fin == NULL) {
        fin = fopen(DEFAULT_BOOK_NAME, "rb");
    }
#endif

    if (fin == NULL) {
        if (!error_printed) {
            Print(0, "Can't open database: %s\n", strerror(errno));
            error_printed = true;
        }
        return;
    }

    *db = load_tree(fin);
    fclose(fin);
}

static struct BookEntry *GetBookEntry(hash_t hk) {
    struct BookEntry *retval = NULL;
    if (BookDB == NULL) {
        OpenBookFile(&BookDB);
    }
    if (BookDB != NULL) {
        retval = (struct BookEntry *)lookup_value(BookDB, (char *)&hk,
                                                  sizeof(hk), NULL);
    }

    return retval;
}

static struct LearnEntry *GetLearnEntry(hash_t hk) {
    struct LearnEntry *retval = NULL;

    if (LearnDB == NULL) {
        FILE *fin = fopen(LEARN_NAME, "rb");
        if (fin != NULL) {
            LearnDB = load_tree(fin);
            fclose(fin);
        }
    }
    if (LearnDB != NULL) {
        retval = (struct LearnEntry *)lookup_value(LearnDB, (char *)&hk,
                                                   sizeof(hk), NULL);
    }

    return retval;
}

void CloseBook(void) {
    free_node(BookDB);
    BookDB = NULL;
}

static void BookupInternal(char *file_name, int verbosity) {
    struct Position *p;
    FILE *fin;
    int afterEco = 0;
    tree_node_t *database = NULL;
    int lines = 0;

    struct PGNHeader header;
    char move[12];

    fin = fopen(file_name, "rb");
    if (fin == NULL) {
        Print(0, "Can't open bookfile.\n");
        return;
    } else {
        Print(verbosity, "   Parsing PGN file %s. '.'= 100 Games\n", file_name);
    }

    CloseBook();

    while (!scanHeader(fin, &header)) {
        int result;

        if (!strcmp(header.result, "1-0"))
            result = 1;
        else if (!strcmp(header.result, "0-1"))
            result = -1;
        else if (!strcmp(header.result, "1/2-1/2"))
            result = 0;
        else
            continue;

        p = InitialPosition();

        while (!scanMove(fin, move)) {
            if (!(strlen(move) < 12)) {
                printf("\n<%s>\n", move);
                exit(1);
            }

            move_t themove = ParseSAN(p, move);
            if (themove != M_NONE) {
                DoMove(p, themove);
                char *eco_code = GetEcoCode(p->hkey);
                if (eco_code) {
                    afterEco = 0;
                    free(eco_code);
                } else {
                    afterEco++;
                }
                if (afterEco <= 20) {
                    if (p->turn == Black) {
                        /* white played the move */
                        database = PutBookEntry(database, p->hkey, result,
                                                header.white_elo);
                    } else {
                        /* black played the move */
                        database = PutBookEntry(database, p->hkey, -result,
                                                header.black_elo);
                    }
                }
            }
        }

        FreePosition(p);

        lines++;
        if ((lines % 100) == 0) {
            Print(0, ".");

            if ((lines % 7000) == 0) {
                Print(0, "(%d)\n", lines);
            }
        }
    }

    Print(verbosity, "(%d)\n", lines);
    fclose(fin);

    FILE *fout = fopen(BOOK_NAME, "wb");
    if (fout == NULL) {
        Print(0, "Can't write database: %s\n", strerror(errno));
        return;
    }

    save_tree(database, fout);
    fclose(fout);

    free_node(database);
}

void Bookup(char *file_name) { BookupInternal(file_name, 0); }

void BookupQuiet(char *file_name) { BookupInternal(file_name, 9); }

static void GetAllBookMoves(struct Position *p, int *cnt, move_t *book_moves,
                            struct BookQuery *entries) {
    unsigned int i;

    heap_t heap = allocate_heap();
    LegalMoves(p, heap);

    for (i = heap->current_section->start; i < heap->current_section->end;
         i++) {
        move_t move = heap->data[i];
        struct BookEntry *be = NULL;
        struct LearnEntry *le = NULL;

        DoMove(p, move);
        /* If the move leads to a repetition, do not accept it. */
        if (!Repeated(p, false)) {
            be = GetBookEntry(p->hkey);
            le = GetLearnEntry(p->hkey);
        }
        UndoMove(p, move);

        if (be) {
            memset(entries + *cnt, 0, sizeof(struct BookQuery));
            book_moves[*cnt] = move;
            entries[*cnt].be = *be;
            if (le) {
                entries[*cnt].le = *le;
                free(le);
            }
            (*cnt)++;
            free(be);
        }
    }

    free_heap(heap);
}

static void SortBook(int cnt, move_t *mvs, struct BookQuery *entries) {
    bool done = false;

    while (!done) {
        int i;

        done = true;

        for (i = 1; i < cnt; i++) {
            int f1 = entries[i - 1].be.win + entries[i - 1].be.loss +
                     entries[i - 1].be.draw;
            int f2 =
                entries[i].be.win + entries[i].be.loss + entries[i].be.draw;
            if (f1 < f2) {
                struct BookQuery betmp = entries[i];
                move_t move;
                entries[i] = entries[i - 1];
                entries[i - 1] = betmp;
                move = mvs[i];
                mvs[i] = mvs[i - 1];
                mvs[i - 1] = move;

                done = false;
            }
        }
    }
}

static void CalculatePropabilities(int cnt, struct BookQuery *entries,
                                   double *props) {
    int total = 0;
    double totalprops;
    int limit;
    int i;

    for (i = 0; i < cnt; i++) {
        total += entries[i].be.win + entries[i].be.loss + entries[i].be.draw;
    }

    limit = total / 16;
    totalprops = 0.0;

    for (i = 0; i < cnt; i++) {
        struct BookQuery *entry = entries + i;
        int freq = entry->be.win + entry->be.loss + entry->be.draw;

        props[i] = 0.0;
        if (freq > limit) {
            int avelo = 2000;

#if WITH_ELO
            if (entry->be.nElo != 0) {
                avelo = entry->be.sumElo / entry->be.nElo;
            }
#endif

            props[i] = avelo * freq *
                       (double)(2 * entry->be.win + entry->be.draw) /
                       (double)(freq);

            if (entry->le.flags & GoodMove) {
                props[i] *= 2;
            }

            if (entry->le.flags & BadMove) {
                props[i] = 0.0;
            }

            /*
             * Never choose a variation that doesn't have a single win.
             */

            if (entry->be.win == 0) {
                props[i] = 0.0;
            }

            totalprops += props[i];
        }
    }

    if (totalprops != 0.0) {
        totalprops = 1.0 / totalprops;
    } else {
        totalprops = 0.0;
    }

    for (i = 0; i < cnt; i++) {
        props[i] *= totalprops;
    }
}

int SelectBook(struct Position *p) {
    int i, cnt = 0;
    struct BookQuery be[32];
    move_t moves[32];
    double props[32];
    double random_value = Random();

    GetAllBookMoves(p, &cnt, moves, be);

    if (cnt != 0) {
        SortBook(cnt, moves, be);
        CalculatePropabilities(cnt, be, props);

        for (i = 0; i < cnt; i++) {
            if (props[i] > 0.0) {
                random_value -= props[i];
                if (random_value <= 0.0) {
                    return moves[i];
                }
            }
        }
    }

    return M_NONE;
}

void QueryBook(struct Position *p) {
    int i, cnt = 0;
    struct BookQuery be[32];
    move_t moves[32];
    double props[32];

    GetAllBookMoves(p, &cnt, moves, be);
    SortBook(cnt, moves, be);
    CalculatePropabilities(cnt, be, props);

    Print(0, "\tmove    count  win loss draw av. elo prop\n");
    for (i = 0; i < cnt; i++) {
        struct BookQuery *entry = be + i;
        int freq = entry->be.win + entry->be.loss + entry->be.draw;
        char modifier = ' ';

        if (entry->le.flags & GoodMove) {
            modifier = '!';
        }
        if (entry->le.flags & BadMove) {
            modifier = '?';
        }

        char san_buffer[16];
        Print(0, "\t%5s%c %6d %3d%% %3d%% %3d%% %5d %3.f\n",
              SAN(p, moves[i], san_buffer), modifier, freq,
              (100 * entry->be.win) / freq, (100 * entry->be.loss) / freq,
              (100 * entry->be.draw) / freq,
#if WITH_ELO
              entry->be.nElo ? entry->be.sumElo / entry->be.nElo : 0,
#else
              0,
#endif
              props[i] * 100.0);
    }

    Print(0, "\n");
}

static void PutLearnEntry(hash_t hk, int learn_value, int flags) {
    if (LearnDB == NULL) {
        FILE *fin = fopen(LEARN_NAME, "rb");
        if (fin != NULL) {
            LearnDB = load_tree(fin);
            fclose(fin);
        }
    }

    struct LearnEntry entry;

    entry.learn_value = learn_value;
    entry.flags = flags;

    LearnDB = add_node(LearnDB, (char *)&hk, sizeof(hk), (char *)&entry,
                       sizeof(entry));
}

void CreateLearnDB(char *file_name) {
    FILE *fin = fopen(file_name, "rb");
    char buffer[1024];
    struct Position *p;

    if (fin == NULL) {
        Print(0, "Can't open learn file %s\n", file_name);
        return;
    }

    while (fgets(buffer, 1023, fin)) {
        char *x = buffer;
        char *move;

        if (buffer[0] == '#')
            continue;

        p = InitialPosition();

        while ((move = nextToken(&x, " \n\r\t")) != NULL) {
            int flags = 0;
            char *modifier = move + strlen(move) - 1;
            move_t themove;

            if (*modifier == '!') {
                flags = GoodMove;
                *modifier = '\0';
            } else if (*modifier == '?') {
                flags = BadMove;
                *modifier = '\0';
            }

            themove = ParseSAN(p, move);
            if (themove != M_NONE) {
                char san_buffer[16];
                Print(0, "%s ", SAN(p, themove, san_buffer));

                DoMove(p, themove);

                if (flags != 0) {
                    PutLearnEntry(p->hkey, 0, flags);
                }
            } else {
                Print(0, "can't parse >%s<\n", move);
                break;
            }
        }

        Print(0, "\n");
        FreePosition(p);
    }

    fclose(fin);

    if (LearnDB != NULL) {
        FILE *fout = fopen(LEARN_NAME, "wb");
        if (fout != NULL) {
            save_tree(LearnDB, fout);
            fclose(fout);
        } else {
            Print(0, "Failed to save learn file: %s\n", strerror(errno));
        }
    }
}

static tree_node_t *flatten_internal(tree_node_t *source, tree_node_t *target,
                                     unsigned int threshold, int *read,
                                     int *written) {
    if (source == NULL) {
        return target;
    }
    struct BookEntry *entry = (struct BookEntry *)source->value_data;
    (*read)++;
    if ((entry->win + entry->draw + entry->loss) > threshold) {
        target = add_node(target, source->key_data, source->key_len,
                          source->value_data, source->value_len);
        (*written)++;
    }

    target =
        flatten_internal(source->left_child, target, threshold, read, written);
    target =
        flatten_internal(source->right_child, target, threshold, read, written);

    return target;
}

void FlattenBook(unsigned int threshold) {
    int read = 0;
    int written = 0;

    if (BookDB == NULL) {
        OpenBookFile(&BookDB);
    }
    if (BookDB != NULL) {
        tree_node_t *flattened =
            flatten_internal(BookDB, NULL, threshold, &read, &written);

        PrintNoLog(0, "Read %d entries, wrote %d entries\n", read, written);

        FILE *fout = fopen("Book2.db", "wb");
        if (fout != NULL) {
            save_tree(flattened, fout);
            fclose(fout);
        } else {
            Print(0, "Can't write database: %s\n", strerror(errno));
        }

        free_node(flattened);
    }
}
