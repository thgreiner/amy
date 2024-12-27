/*

    Amy - a chess playing program

    Copyright (c) 2002-2024, Thorsten Greiner
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
 * eco.c - ECO handling routines
 */

#include "amy.h"
#include "tree.h"

#define ECO_NAME "Eco.db"

#ifdef ECODIR
#define DEFAULT_ECO_NAME ECODIR "/" ECO_NAME
#else
#warning "ECODIR is not defined!"
#endif

void ParseEcoPgn(char *fname) {
    FILE *fin = fopen(fname, "r");
    char buffer[1024];
    char name[128];

    if (!fin) {
        Print(0, "Cannot open file %s\n", fname);
        return;
    }

    tree_node_t *node = NULL;

    while (fgets(buffer, sizeof(buffer) - 1, fin) != NULL) {
        char *x;
        strtok(buffer, " \t");
        x = strtok(NULL, "]\n\r");
        strncpy(name, x, sizeof(name) - 1);

        Print(0, ".");

        struct Position *p = InitialPosition();

        if (fgets(buffer, 1024, fin) != NULL) {
            for (x = strtok(buffer, " \n\r\t"); x;
                 x = strtok(NULL, " \n\r\t")) {
                move_t move = ParseSAN(p, x);
                if (move != M_NONE) {
                    DoMove(p, move);
                }
            }

            node = add_node(node, (char *)&(p->hkey), sizeof(p->hkey), name,
                            strlen(name) + 1);
        }

        FreePosition(p);
    }

    fclose(fin);

    FILE *fout = fopen(ECO_NAME, "w");
    if (fout == NULL) {
        Print(0, "\nCannot save ECO database to %s: %s\n", ECO_NAME,
              strerror(errno));
        return;
    }

    save_tree(node, fout);
    fclose(fout);

    free_node(node);

    Print(0, "\nECO database created.\n");
}

static tree_node_t *EcoDB = NULL;

char *GetEcoCode(hash_t hkey) {
    char *retval = NULL;

    if (EcoDB == NULL) {
        FILE *fin = fopen(ECO_NAME, "r");

#ifdef DEFAULT_ECO_NAME
        if (fin == NULL) {
            fin = fopen(DEFAULT_ECO_NAME, "r");
        }
#endif

        if (fin == NULL) {
            Print(0, "Can't open database: %s\n", strerror(errno));
            return NULL;
        }
        EcoDB = load_tree(fin);
        fclose(fin);
    }

    if (EcoDB != NULL) {
        retval = lookup_value(EcoDB, (char *)&hkey, sizeof(hkey), NULL);
    }

    return retval;
}

bool FindEcoCode(const struct Position *p, char *result) {
    int ply = 0;
    char *res;
    bool found = false;

    while (ply <= p->ply) {
        hash_t key = p->gameLog[ply].gl_HashKey;
        if (ply == p->ply) {
            key = p->hkey;
        }
        res = GetEcoCode(key);
        if (res != NULL) {
            strcpy(result, res);
            found = true;
            free(res);
        }
        ply++;
    }

    return found;
}
