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
 * pgn.c - pgn handling routines
 */

#include "amy.h"

#define AMY_NAME "Amy " VERSION

char OpponentName[OPP_NAME_LENGTH] = "Opponent";

static int PGNMoveHistory[1024];
static char DateBuf[16];
static char TimeBuf[16];
static char HostNameBuf[256];

static void MakeDateTime(void) {
    time_t tnow;
    struct tm *now;

    time(&tnow);
    now = localtime(&tnow);
    strftime(DateBuf, 15, "%Y.%m.%d", now);
    strftime(TimeBuf, 15, "%H:%M:%S", now);
}

static void MakeHostName(void) {
#if HAVE_GETHOSTNAME
    gethostname(HostNameBuf, 256);
#else
    strcpy(HostNameBuf, "Your computer");
#endif
}

static char *getWhiteName(void) {
    return (ComputerSide == White) ? (AMY_NAME) : (OpponentName);
}

static char *getBlackName(void) {
    return (ComputerSide == Black) ? (AMY_NAME) : (OpponentName);
}

void SaveGame(struct Position *p, char *file_name) {
    /* Do not save if no move made yet. */
    if (p->ply > 0) {
        FILE *fout = fopen(file_name, "w");
        if (fout) {
            int i;
            int ply = p->ply;
            int width = 0;
            const char *gameend;
            char shortgameend[8] = "1/2-1/2";
            char eco[128] = "";

            gameend = GameEnd(CurrentPosition);
            if (gameend == NULL) {
                gameend = "*";
                strcpy(shortgameend, gameend);
            } else {
                strncpy(shortgameend, gameend, 3);
                if (shortgameend[0] == '0' || shortgameend[2] == '0') {
                    shortgameend[3] = '\0';
                }
            }

            fprintf(fout, "[Event \"Amy game\"]\n");
            MakeHostName();
            fprintf(fout, "[Site \"%s\"]\n", HostNameBuf);
            MakeDateTime();
            fprintf(fout, "[Date \"%s\"]\n", DateBuf);
            fprintf(fout, "[Time \"%s\"]\n", TimeBuf);
            fprintf(fout, "[Round \"?\"]\n");
            if (FindEcoCode(p, eco)) {
                fprintf(fout, "[ECO \"%c%c%c\"]\n", eco[0], eco[1], eco[2]);
            }
            fprintf(fout, "[White \"%s\"]\n", getWhiteName());
            fprintf(fout, "[Black \"%s\"]\n", getBlackName());
            fprintf(fout, "[Result \"%s\"]\n\n", shortgameend);

            for (i = ply; i > 0; i--) {
                int move = (p->actLog - 1)->gl_Move;
                PGNMoveHistory[i - 1] = move;
                UndoMove(p, move);
            }

            for (i = 0; i < ply; i++) {
                int move = PGNMoveHistory[i];
                if ((i & 1) == 0) {
                    fprintf(fout, "%d. ", (i / 2) + 1);
                    width += 3;
                    if (i > 18)
                        width++;
                    if (i > 98)
                        width++;
                }

                char san_buffer[16];
                char *san = SAN(p, move, san_buffer);
                fprintf(fout, "%s ", san);
                width += strlen(san) + 1;
                if (width > 67) {
                    width = 0;
                    fprintf(fout, "\n");
                }
                DoMove(p, move);
            }
            fprintf(fout, "\n%s\n\n", gameend);
            fclose(fout);
        }
    }
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
    }
}

int scanHeader(FILE *fin, struct PGNHeader *header) {
    static char buffer[1024];
    int state = 0;

    /* Clear it */
    memset(header, 0, sizeof(struct PGNHeader));

    while (fgets(buffer, 1024, fin)) {
        if (buffer[0] == '[') {
            char *x = buffer + 1;
            char *key = nextToken(&x, " \"");
            char *value = nextToken(&x, "\"");

            if (!strcmp("Event", key)) {
                strncpy(header->event, value, sizeof(header->event) - 1);
            }
            if (!strcmp("Site", key)) {
                strncpy(header->site, value, sizeof(header->site) - 1);
            }
            if (!strcmp("Date", key)) {
                strncpy(header->date, value, sizeof(header->date) - 1);
            }
            if (!strcmp("Round", key)) {
                strncpy(header->round, value, sizeof(header->round) - 1);
            }
            if (!strcmp("White", key)) {
                strncpy(header->white, value, sizeof(header->white) - 1);
            }
            if (!strcmp("Black", key)) {
                strncpy(header->black, value, sizeof(header->black) - 1);
            }
            if (!strcmp("Result", key)) {
                strncpy(header->result, value, sizeof(header->result) - 1);
            }
            if (!strcmp("WhiteElo", key)) {
                header->white_elo = atoi(value);
            }
            if (!strcmp("BlackElo", key)) {
                header->black_elo = atoi(value);
            }

            state = 1;

        } else {
            if (state != 0)
                return 0;
        }
    }

    return 1;
}

int scanMove(FILE *fin, char *nextMove) {
    static char buffer[1024];
    static int haveLine = 0;
    static char *x;
    char *token;

    int braces = 0;
    int parens = 0;

    do {
        if (!haveLine) {
            if (!fgets(buffer, 1024, fin))
                return 1;
            haveLine = 1;
            x = buffer;
        }

        if (braces) {
            if (*x == '\0') {
                haveLine = 0;
                continue;
            } else if (*(x++) == '}') {
                braces = 0;
            }

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
