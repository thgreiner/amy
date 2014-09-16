/*

    Amy - a chess playing program
    Copyright (C) 2002 Thorsten Greiner

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/

/*
 * eco.c - ECO handling routines
 *
 * $Id: eco.c 62 2003-03-18 21:02:34Z thorsten $
 *
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
