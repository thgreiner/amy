/*

    Amy - a chess playing program
    Copyright (C) 2002-2003 Thorsten Greiner

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
 * version.c - version information
 *
 * $Id: version.c 434 2004-02-26 21:45:12Z thorsten $
 *
 */

#include "amy.h"

extern char RcsId_score_c[];
extern char RcsId_search_c[];

static char CopyrightNotice[] = 
    "    Amy version " VERSION ", Copyright (C) 2002-2004 Thorsten Greiner\n" 
    "    Amy comes with ABSOLUTELY NO WARRANTY; for details type 'warranty'.\n"
    "    This is free software, and you are welcome to redistribute it\n"
    "    under certain conditions; type 'distribution' for details.\n\n"
    "    Amy contains table base access code which is copyrighted\n"
    "    by Eugene Nalimov and not free software.\n\n";

/**
 * Show the version of Amy.
 */
void ShowVersion(void)
{
	Print(0, "\n");
	Print(0, CopyrightNotice);
#if MP
	Print(0, "    Multiprocessor support (%d CPUs).\n\n", NumberOfCPUs);
#else
	Print(0, "    No multiprocessor support.\n\n");
#endif
	Print(0, "    %s\n", RcsId_score_c);
	Print(0, "    %s\n\n", RcsId_search_c);
}

