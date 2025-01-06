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
 * version.c - version information
 */

#include "amy.h"

static char CopyrightNotice[] =
    "    Amy version " VERSION ", Copyright (c) 2002-2025, Thorsten Greiner\n"
    "    Amy comes with ABSOLUTELY NO WARRANTY; for details type 'warranty'.\n"
    "    This is free software, and you are welcome to redistribute it\n"
    "    under certain conditions; type 'distribution' for details.\n\n"
    "    Amy contains table base access code which is copyrighted\n"
    "    by Eugene Nalimov and not free software.\n\n";

/**
 * Show the version of Amy.
 */
void ShowVersion(void) {
    Print(0, "\n");
    Print(0, CopyrightNotice);
#if MP
    Print(0, "    Multiprocessor support (%d CPUs).\n\n", NumberOfCPUs);
#else
    Print(0, "    No multiprocessor support.\n\n");
#endif
}
