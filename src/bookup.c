/*

    Amy - a chess playing program

    Copyright (c) 2014, Thorsten Greiner
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

#if HAVE_LIBDB_5
#include <db.h>
#endif

#include <math.h>

#define BOOK_NAME "Book.db"
#define LEARN_NAME "Learn.db"

#define DEFAULT_BOOK_NAME BOOKDIR "/" BOOK_NAME

#define WITH_ELO 1

enum { GoodMove = 1, BadMove = 2 };

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

#if HAVE_LIBDB_5
static DB *BookDB = NULL;
static DB *LearnDB = NULL;
#endif

#if HAVE_LIBDB_5
static void PutBookEntry(DB *database, hash_t hk, int result, int elo) {
    Print(9, "storing position");
    if (database != NULL) {
        DBT key;
        DBT value;
        struct BookEntry *entry;
        int res;

        memset(&key, 0, sizeof(key));
        memset(&value, 0, sizeof(value));
        memset(&entry, 0, sizeof(entry));

        key.data = &hk;
        key.size = sizeof(hk);

        value.flags = DB_DBT_MALLOC;

        res = database->get(database, NULL, &key, &value, 0);

        if (res == 0 || res == DB_NOTFOUND) {

            if (res == 0) {
                entry = value.data;
            } else {
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

            memset(&key, 0, sizeof(key));
            memset(&value, 0, sizeof(value));

            key.data = &hk;
            key.size = sizeof(hk);

            value.data = entry;
            value.size = sizeof(struct BookEntry);

            res = database->put(database, NULL, &key, &value, 0);
            if (res != 0) {
                Print(0, "Problem storing data: %s\n", strerror(res));
            }

            free(entry);
        } else {
            Print(0, "Problem retrieving data: %s\n", strerror(res));
            Print(0, "%d < %d\n", sizeof(struct BookEntry), value.size);
        }
    }
}
#endif

#if HAVE_LIBDB_5
static int OpenBookFile(DB **db) {
    int result;

    result = db_create(db, NULL, 0);
    if (result != 0) {
        return result;
    }
    result = (*db)->open(*db, NULL, BOOK_NAME, NULL, DB_BTREE, DB_RDONLY, 0);

    if (result == 0) {
        Print(0, "Book file " BOOK_NAME " opened successfully.\n");
    }
    if (result != 0) {
        result = (*db)->open(*db, NULL, DEFAULT_BOOK_NAME, NULL, DB_BTREE,
                             DB_RDONLY, 0);
    }

    return result;
}
#endif

static struct BookEntry *GetBookEntry(hash_t hk) {
    struct BookEntry *retval = NULL;
#if HAVE_LIBDB_5
    int result;
    DBT key;
    DBT value;

    if (BookDB == NULL) {
        result = OpenBookFile(&BookDB);
    }
    if (BookDB != NULL) {
        memset(&key, 0, sizeof(key));
        memset(&value, 0, sizeof(value));

        key.data = &hk;
        key.size = sizeof(hk);

        value.flags = DB_DBT_MALLOC;

        result = BookDB->get(BookDB, NULL, &key, &value, 0);
        if (result == 0) {
            retval = value.data;
        }
    }

#endif
    return retval;
}

static struct LearnEntry *GetLearnEntry(hash_t hk) {
    struct LearnEntry *retval = NULL;
#if HAVE_LIBDB_5
    int result;
    DBT key;
    DBT value;

    if (LearnDB == NULL) {
        result = db_create(&LearnDB, NULL, 0);
        if (result != 0) {
            LearnDB = NULL;
            return NULL;
        }
        result = LearnDB->open(LearnDB, NULL, LEARN_NAME, NULL, DB_BTREE,
                               DB_RDONLY, 0);
        if (result != 0) {
            LearnDB = NULL;
            return NULL;
        }
    }
    if (LearnDB != NULL) {
        memset(&key, 0, sizeof(key));
        memset(&value, 0, sizeof(value));

        key.data = &hk;
        key.size = sizeof(hk);

        value.flags = DB_DBT_MALLOC;

        result = LearnDB->get(LearnDB, NULL, &key, &value, 0);
        if (result == 0) {
            retval = value.data;
        }
    }

#endif
    return retval;
}

#if HAVE_LIBDB_5

/**
 * Merge the contents from 'from' (usually an in memory database)
 * with 'to'.
 */

static void DBMerge(DB *from, DB *to) {
    DBC *cursor;
    int result;

    if ((result = from->cursor(from, NULL, &cursor, 0)) == 0) {

        DBT key, value;
        DBT key2, value2;
        u_int32_t flags = DB_FIRST;

        for (;;) {
            memset(&key, 0, sizeof(key));
            memset(&value, 0, sizeof(value));
            result = cursor->c_get(cursor, &key, &value, flags);

            if (result != 0)
                break;

            memset(&key2, 0, sizeof(key2));
            memset(&value2, 0, sizeof(value2));
            key2.data = key.data;
            value2.flags = DB_DBT_MALLOC;

            result = to->get(to, NULL, &key2, &value2, 0);
            if (result == 0) {
                struct BookEntry *b1 = (struct BookEntry *)value.data;
                struct BookEntry *b2 = (struct BookEntry *)value2.data;

                b1->win += b2->win;
                b1->loss += b2->loss;
                b1->draw += b2->draw;
#if WITH_ELO
                b1->sumElo += b2->sumElo;
                b1->nElo += b2->nElo;
#endif
            }
            to->put(to, NULL, &key, &value, 0);
            flags = DB_NEXT;
        }

        cursor->c_close(cursor);
    } else {
        Print(0, "Error creating cursor: %s\n", strerror(result));
    }
}

static DB *OpenTemporaryDB(void) {
    DB *tempdb;
    int result;

    result = db_create(&tempdb, NULL, 00);
    if (result == 0) {
        result =
            tempdb->open(tempdb, NULL, NULL, NULL, DB_BTREE, DB_CREATE, 0644);
    }

    if (result) {
        Print(0, "Cannot create temporary database: %s\n", strerror(result));
        return NULL;
    }

    return tempdb;
}
#endif

void CloseBook() {
#if HAVE_LIBDB_5
    if (BookDB) {
        BookDB->close(BookDB, 0);
        BookDB = NULL;
    }
#endif
}

static void BookupInternal(char *file_name, int verbosity) {
#if HAVE_LIBDB_5
    struct Position *p;
    FILE *fin;
    int result;
    int afterEco = 0;
    DB *tmpbase;
    DB *database;
    int lines = 0;

    struct PGNHeader header;
    char move[12];

    fin = fopen(file_name, "r");
    if (fin == NULL) {
        Print(0, "Can't open bookfile.\n");
        return;
    } else {
        Print(verbosity, "   Parsing PGN file %s. '.'= 100 Games\n", file_name);
    }

    CloseBook();

#if HAVE_LIBDB_5
    result = db_create(&database, NULL, 00);
    if (result == 0) {
        result = database->open(database, NULL, BOOK_NAME, NULL, DB_BTREE,
                                DB_CREATE, 0644);
    }
#endif

    if (result != 0) {
        Print(0, "Can't open database: %s\n", strerror(result));
        fclose(fin);
        return;
    }

    /*
    tmpbase = OpenTemporaryDB();

    if(tmpbase == NULL) {
    database->close(database, 0);
        fclose(fin);
        return;
    }
    */

    while (!scanHeader(fin, &header)) {
        int result;

        p = InitialPosition();

        if (!strcmp(header.result, "1-0"))
            result = 1;
        else if (!strcmp(header.result, "0-1"))
            result = -1;
        else if (!strcmp(header.result, "1/2-1/2"))
            result = 0;
        else
            continue;

        while (!scanMove(fin, move)) {
            if (!(strlen(move) < 12)) {
                printf("\n<%s>\n", move);
                exit(1);
            }

            int themove = ParseSAN(p, move);
            if (themove != M_NONE) {
                DoMove(p, themove);
                if (GetEcoCode(p->hkey) != 0) {
                    afterEco = 0;
                } else {
                    afterEco++;
                }
                if (afterEco <= 20) {
                    if (p->turn == Black) {
                        /* white played the move */
                        PutBookEntry(database, p->hkey, result,
                                     header.white_elo);
                    } else {
                        /* black played the move */
                        PutBookEntry(database, p->hkey, -result,
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
                /*
                        DBMerge(tmpbase, database);
                        tmpbase->close(tmpbase, 0);
                        tmpbase = OpenTemporaryDB();
                        if(tmpbase == NULL) {
                            database->close(database, 0);
                            fclose(fin);
                            return;
                        }
                */
            }
        }
    }

    Print(verbosity, "(%d)\n", lines);
    /*
    DBMerge(tmpbase, database);
    tmpbase->close(tmpbase, 0);
    */
    database->close(database, 0);
    fclose(fin);
#endif
}

void Bookup(char *file_name) { BookupInternal(file_name, 0); }

void BookupQuiet(char *file_name) { BookupInternal(file_name, 9); }

static void GetAllBookMoves(struct Position *p, int *cnt, int *book_moves,
                            struct BookQuery *entries) {
    int mvs[256];
    int mv_cnt = LegalMoves(p, mvs);
    int i;

    for (i = 0; i < mv_cnt; i++) {
        struct BookEntry *be = NULL;
        struct LearnEntry *le = NULL;

        DoMove(p, mvs[i]);
        /* If the move leads to a repetition, do not accept it. */
        if (!Repeated(p, FALSE)) {
            be = GetBookEntry(p->hkey);
            le = GetLearnEntry(p->hkey);
        }
        UndoMove(p, mvs[i]);

        if (be) {
            memset(entries + *cnt, 0, sizeof(struct BookQuery));
            book_moves[*cnt] = mvs[i];
            entries[*cnt].be = *be;
            if (le) {
                entries[*cnt].le = *le;
                free(le);
            }
            (*cnt)++;
            free(be);
        }
    }
}

static void SortBook(int cnt, int *mvs, struct BookQuery *entries) {
    int done = FALSE;

    while (!done) {
        int i;

        done = TRUE;

        for (i = 1; i < cnt; i++) {
            int f1 = entries[i - 1].be.win + entries[i - 1].be.loss +
                     entries[i - 1].be.draw;
            int f2 =
                entries[i].be.win + entries[i].be.loss + entries[i].be.draw;
            if (f1 < f2) {
                struct BookQuery betmp = entries[i];
                int move;
                entries[i] = entries[i - 1];
                entries[i - 1] = betmp;
                move = mvs[i];
                mvs[i] = mvs[i - 1];
                mvs[i - 1] = move;

                done = FALSE;
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
    int moves[32];
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
    int moves[32];
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
        Print(0, "\t%5s%c %6d %3d%% %3d%% %3d%% %5d %3.f\n", SAN(p, moves[i]),
              modifier, freq, (100 * entry->be.win) / freq,
              (100 * entry->be.loss) / freq, (100 * entry->be.draw) / freq,
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
#if HAVE_LIBDB_5
    if (LearnDB == NULL) {
        int result = db_create(&LearnDB, NULL, 0);
        if (result != 0) {
            return;
        }
        LearnDB->open(LearnDB, NULL, LEARN_NAME, NULL, DB_BTREE, DB_CREATE,
                      0644);
    }

    if (LearnDB != NULL) {
        DBT key;
        DBT value;
        struct LearnEntry entry;
        int res;

        entry.learn_value = learn_value;
        entry.flags = flags;

        memset(&key, 0, sizeof(key));
        memset(&value, 0, sizeof(value));

        key.data = &hk;
        key.size = sizeof(hk);

        value.data = &entry;
        value.size = sizeof(entry);

        res = LearnDB->put(LearnDB, NULL, &key, &value, 0);

        if (res != 0) {
            Print(0, "PutLearnEntry failed: %s\n", strerror(res));
        }
    }
#endif /* HAVE_LIBDB || HAVE_LIBDB2 */
}

void CreateLearnDB(char *file_name) {
    FILE *fin = fopen(file_name, "r");
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
            int themove;

            if (*modifier == '!') {
                flags = GoodMove;
                *modifier = '\0';
            } else if (*modifier == '?') {
                flags = BadMove;
                *modifier = '\0';
            }

            themove = ParseSAN(p, move);
            if (themove != M_NONE) {
                Print(0, "%s ", SAN(p, themove));

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

#if HAVE_LIBDB_5
    if (LearnDB != NULL) {
        LearnDB->close(LearnDB, 0);
        LearnDB = NULL;
    }
#endif
}

void FlattenBook(int threshold) {
#if HAVE_LIBDB_5
    int result;
    int read = 0;
    int wrote = 0;

    if (BookDB == NULL) {
        result = OpenBookFile(&BookDB);
    }
    if (BookDB != NULL) {
        DB *tmpdb;
        DBC *cursor;

        int result = db_create(&tmpdb, NULL, 0);
        if (result == 0) {
            result = tmpdb->open(tmpdb, NULL, "Book2.db", NULL, DB_BTREE,
                                 DB_CREATE, 0644);
        }

        if (result != 0) {
            Print(0, "Error creating database: %s\n", strerror(result));
            return;
        }

        if ((result = BookDB->cursor(BookDB, NULL, &cursor, 0)) == 0) {

            DBT key, value;
            u_int32_t flags = DB_FIRST;

            Print(0, "Flattening book with threshold %d\n", threshold);

            for (;;) {
                struct BookEntry *b;
                memset(&key, 0, sizeof(key));
                memset(&value, 0, sizeof(key));
                result = cursor->c_get(cursor, &key, &value, flags);

                if (result != 0)
                    break;
                read++;

                b = (struct BookEntry *)value.data;
                if ((b->win + b->draw + b->loss) > threshold) {
                    tmpdb->put(tmpdb, NULL, &key, &value, 0);
                    wrote++;
                }

                if ((read % 10000) == 0) {
                    PrintNoLog(0, "Read %d entries, wrote %d entries\r", read,
                               wrote);
                }
                flags = DB_NEXT;
            }
            Print(0, "Read %d entries, wrote %d entries\n", read, wrote);

            cursor->c_close(cursor);
            tmpdb->close(tmpdb, 0);
        } else {
            Print(0, "Error creating cursor: %s\n", strerror(result));
        }
    }
#endif
}
