/*

    Amy - a chess playing program

    Copyright (c) 2014, Thorsten Greiner
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this
      list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
    FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
    DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
    OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

/*
 * eco.c - ECO handling routines
 */

#include "amy.h"

#if HAVE_LIBDB || HAVE_LIBDB2
#include <db.h>
#endif

#if HAVE_LIBDB3
#include <db3/db.h>
#endif

#if HAVE_LIBDB_5
#include <db.h>
#endif

#define ECO_NAME "Eco.db"

#define DEFAULT_ECO_NAME ECODIR "/" ECO_NAME

void ParseEcoPgn(char *fname)
{
#if HAVE_LIBDB || HAVE_LIBDB2 || HAVE_LIBDB3 || HAVE_LIBDB_5
    FILE *fin = fopen(fname, "r");
    char buffer[1024];
    char name[128];
    DB *database;
    int result;
    DBT key;
    DBT value;
    struct Position *p;

    if(!fin) {
        Print(0, "Can´t open file %s\n", fname);
        return;
    }

#if HAVE_LIBDB || HAVE_LIBDB2
    result = db_open(ECO_NAME, DB_BTREE, DB_CREATE | DB_TRUNCATE,
                      0644, NULL, NULL, &database);
#endif
#if HAVE_LIBDB3
    result = db_create(&database, NULL, 0);
    if (result == 0) {
	result = database->open(
	    database, ECO_NAME, NULL, DB_BTREE, DB_CREATE | DB_TRUNCATE, 0644);
    }
#endif
#if HAVE_LIBDB_5
    result = db_create(&database, NULL, 0);
    if (result == 0) {
	result = database->open(
	    database, NULL, ECO_NAME, NULL, DB_BTREE, DB_CREATE | DB_TRUNCATE, 0644);
    }
#endif

    if(result != 0) {
        Print(0, "Can't open database: %s\n", strerror(result));
        fclose(fin);
        return;
    }

    while(fgets(buffer, 1023, fin) != NULL) {
        char *x;
        int len;
        strtok(buffer, " \t");
        x = strtok(NULL, "]\n\r");
        strncpy(name, x, 128);

        Print(0, ".");
        p = InitialPosition();

        len = strlen(name);

        if(fgets(buffer, 1024, fin) != NULL) {
            for(x = strtok(buffer, " \n\r\t"); x; x=strtok(NULL, " \n\r\t")) {
                int move = ParseSAN(p, x);
                if(move != M_NONE) {
                    DoMove(p, move);
                }
            }

            memset(&key, 0, sizeof(key));
            memset(&value, 0, sizeof(value));

            key.data = &(p->hkey);
            key.size = sizeof(hash_t);

            value.data = malloc(len+1);
            value.size = len+1; /* store trailing null */
            strncpy(value.data, name, len+1);

            database->put(database, NULL, &key, &value, 0);

            free(value.data);
        }

        FreePosition(p);
    }

    Print(0, "\nECO database created.\n");

    database->close(database, 0);
    fclose(fin);
#endif
}

#if HAVE_LIBDB || HAVE_LIBDB2 || HAVE_LIBDB3 || HAVE_LIBDB_5
static DB *EcoDB = NULL;
#endif

char *GetEcoCode(hash_t hkey)
{
    char *retval = NULL;
#if HAVE_LIBDB || HAVE_LIBDB2 || HAVE_LIBDB3 || HAVE_LIBDB_5
    int result;
    DBT key;
    DBT value;

    if(EcoDB == NULL) {
#if HAVE_LIBDB || HAVE_LIBDB2
    	result = db_open(ECO_NAME, DB_BTREE, DB_RDONLY,
      			 0, NULL, NULL, &EcoDB);
	if(result != 0) {
	    result = db_open(DEFAULT_ECO_NAME, DB_BTREE, DB_RDONLY,
	 		     0, NULL, NULL, &EcoDB);
	}
#endif
#if HAVE_LIBDB3
	result = db_create(&EcoDB, NULL, 0);
	if (result == 0) {
	    result = EcoDB->open(
		EcoDB, ECO_NAME, NULL, DB_BTREE, DB_RDONLY, 0);
	    if(result != 0) {
		result = EcoDB->open(
	    	    EcoDB, DEFAULT_ECO_NAME, NULL, DB_BTREE, DB_RDONLY, 0);
	    }
	}
#endif
#if HAVE_LIBDB_5
	result = db_create(&EcoDB, NULL, 0);
	if (result == 0) {
	    result = EcoDB->open(
		EcoDB, NULL, ECO_NAME, NULL, DB_BTREE, DB_RDONLY, 0);
	    if(result != 0) {
		result = EcoDB->open(
	    	    EcoDB, NULL, DEFAULT_ECO_NAME, NULL, DB_BTREE, DB_RDONLY, 0);
	    }
	}
#endif
    }
    if(EcoDB != 0) {
        memset(&key, 0, sizeof(key));
        memset(&value, 0, sizeof(value));

        key.data = &hkey;
        key.size = sizeof(hkey);

        value.flags = DB_DBT_MALLOC;

        result = EcoDB->get(EcoDB, NULL, &key, &value, 0);
        if(result == 0) {
            static char code[128];
            strncpy(code, value.data, value.size);
            free(value.data);
            retval = code;
        }

        /* database->close(database, 0); */
    }
#endif

    return retval;
}

int FindEcoCode(struct Position *p, char *result) {
    int ply = 0;
    char *res;
    int found = FALSE;

    while(ply <= CurrentPosition->ply) {
        hash_t key = CurrentPosition->gameLog[ply].gl_HashKey;
        if(ply == CurrentPosition->ply) {
            key = CurrentPosition->hkey;
        }
        res = GetEcoCode(key);
        if(res != 0) {
            strcpy(result, res);
            found = TRUE;
        }
        ply++;
    }

    return found;
}
