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
 * learn.c - book learning routines
 *
 * $Id: bookup.c 60 2003-03-13 20:19:18Z thorsten $
 *
 */

#include <amy.h>

void DoBookLearning(void) {
    int cnt = 0;
    char buf[128];

    for(cnt=0; ;cnt++) {
	int result;
	struct stat dummy;

	sprintf(buf, "save_%03d.pgn", cnt);
	result = stat(buf, &dummy);

        if(result < 0) {
            break;
        }
        Print(2, "Book learning with save file %s\n", buf);
        BookupQuiet(buf);
        remove(buf);
    }
}
