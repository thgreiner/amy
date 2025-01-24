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
 * main.c - main program for Amy
 */

#include <string.h>

#include "evaluation_config.h"
#include "hashtable.h"
#include "init.h"
#include "learn.h"
#include "movedata.h"
#include "probe.h"
#include "random.h"
#include "recog.h"
#include "search.h"
#include "state_machine.h"
#include "test_dbase.h"
#include "test_yaml.h"
#include "utils.h"

static char CopyrightNotice[] =
    "    Amy version " VERSION ", Copyright (c) 2002-2025, Thorsten Greiner\n"
    "    Amy comes with ABSOLUTELY NO WARRANTY; for details type 'warranty'.\n"
    "    This is free software, and you are welcome to redistribute it\n"
    "    under certain conditions; type 'distribution' for details.\n\n"
    "    Amy contains table base access code which is copyrighted\n"
    "    by Eugene Nalimov and not free software.\n\n";

static char EGTBPath[1024] = "TB";

static char *ConfigFileName = NULL;

static void RunAllTests(void) {
    test_all_yaml();
    test_all_dbase();
}

static void ProcessOptions(int argc, char *argv[]) {
    int i;

    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-ht")) {
            i++;
            if (i < argc) {
                GuessHTSizes(argv[i]);
            }
        }

        if (!strcmp(argv[i], "-conf")) {
            i++;
            if (i < argc) {
                ConfigFileName = argv[i];
            }
        }

        if (!strcmp(argv[i], "--test")) {
            RunAllTests();
            exit(0);
        }
#if MP
        if (!strcmp(argv[i], "-cpu")) {
            i++;
            if (i < argc) {
                NumberOfCPUs = atoi(argv[i]);
            }
        }
#endif
    }
}

static void ProcessRCFile(void) {
    FILE *rcFile = fopen(".amyrc", "r");
    char buf[1024];

    if (!rcFile) {

        /*
         * Windows people have problems naming files .amyrc
         * So lets look for 'Amy.ini' too.
         */

        rcFile = fopen("Amy.ini", "r");
    }

    if (!rcFile)
        return;

    while (fgets(buf, 1023, rcFile)) {
        char *x = buf;
        char *key, *value;

        if (buf[0] == '#')
            continue;

        key = nextToken(&x, "=\t\n\r");
        if (key == NULL)
            continue;

        value = nextToken(&x, "\n\n\r");
        if (value == NULL)
            continue;

        if (!strcmp(key, "ht")) {
            GuessHTSizes(value);
        } else if (!strcmp(key, "tbpath")) {
            strncpy(EGTBPath, value, sizeof(EGTBPath) - 1);
        } else if (!strcmp(key, "cpu")) {
#if MP
            NumberOfCPUs = atoi(value);
#endif /* MP */
        } else if (!strcmp(key, "autosave")) {
            AutoSave = !strcmp(value, "true");
        }
    }

    fclose(rcFile);
}

/**
 * Show the version of Amy.
 */
static void ShowVersion(void) {
    Print(0, "\n");
    Print(0, CopyrightNotice);
#if MP
    Print(0, "    Multiprocessor support (%d CPUs).\n\n", NumberOfCPUs);
#else
    Print(0, "    No multiprocessor support.\n\n");
#endif
}

int main(int argc, char *argv[]) {
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

    if (ConfigFileName) {
        LoadEvaluationConfig(ConfigFileName);
    }

    AllocateHT();
    InitEGTB(EGTBPath);
    RecogInit();

    DoBookLearning();

    Print(0, "\n");

    /* Ensure true random behavior. */
    InitRandom(GetTime());

    StateMachine();

    return 0;
}
