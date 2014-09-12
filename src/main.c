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
 * main.c - main program for Amy
 *
 * $Id: main.c 63 2003-04-06 14:58:27Z thorsten $
 *
 */

#include "amy.h"

char AutoSaveFileName[64];

#if MP
int NumberOfCPUs;
#endif

static char EGTBPath[1024] = "TB";

static void ProcessOptions(int argc, char *argv[])
{
    int i;

    for(i=1; i<argc; i++) {
        if(!strcmp(argv[i], "-ht")) {
            i++;
            if(i < argc) {
                GuessHTSizes(argv[i]);
            }
        }

#if MP
        if(!strcmp(argv[i], "-cpu")) {
            i++;
            if(i < argc) {
                NumberOfCPUs = atoi(argv[i]);
            }
        }
#endif
    }
}

static void ProcessRCFile(void)
{
    FILE *rcFile = fopen(".amyrc", "r");
    char buf[1024];

    if(!rcFile) {

	/*
	 * Windows people have problems naming files .amyrc
	 * So lets look for 'Amy.ini' too.
	 */

	rcFile = fopen("Amy.ini", "r");
    }
    
    if(!rcFile) return;

    while(fgets(buf, 1023, rcFile)) {
	char *x = buf;
	char *key, *value;

	if(buf[0] == '#') continue;

	key = nextToken(&x, "=\t\n\r");
	if(key == NULL) continue;

	value = nextToken(&x, "\n\n\r");
	if(value == NULL) continue;

	if(!strcmp(key, "ht")) {
	    GuessHTSizes(value);
	} else if(!strcmp(key, "tbpath")) {
	    strncpy(EGTBPath, value, 1023);
	} else if(!strcmp(key, "cpu")) {
#if MP
	    NumberOfCPUs = atoi(value);
#endif /* MP */
	} else if(!strcmp(key, "autosave")) {
	    AutoSave = !strcmp(value, "true");
	}
    }

    fclose(rcFile);
}

int main(int argc, char *argv[])
{
#if HAVE_SETBUF
    setbuf(stdin, NULL);
#endif
       
    OpenLogFile("Amy.log");

    InitMoves();

    InitAll();
    HashInit();

    /*
     * Process rc file first, then command line options. This way command
     * line options can override rc file settings.
     */

    ProcessRCFile();
    ProcessOptions(argc, argv);

    ShowVersion();

    AllocateHT();
    InitEGTB(EGTBPath);
    RecogInit();

    DoBookLearning();

    Print(0, "\n");

    strcpy(AutoSaveFileName, GetTmpFileName());

    /* Ensure true random behavior. */
    InitRandom(GetTime());

    StateMachine();

    return 0;
}
