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
 * search_io.c - output search results
 *
 * $Id: search_io.c 304 2003-12-30 11:15:54Z thorsten $
 *
 */

#include <stdio.h>
#include "amy.h"

static void PrintPV(char *pv)
{
    static char PVBuffer[512];
    char *x;
    int len = 21;

    strcpy(PVBuffer, pv);

    for(x=PVBuffer; *x;) {
        char *y = x;

        while(*y != ' ' && *y != '\0') y++;
        if(*y == '\0') *(y+1) = '\0';
        *y = '\0';

        len += strlen(x);

        if(len >= 79) {
            Print(1, "\n                    ");
            len = 21+strlen(x);
        }
        Print(1, "%s ", x);
        len += 1;
        x = y+1;
    }
    Print(1, "\n");
}

void SearchHeader(void)
{
    Print(1, "It    Time   Score  principal Variation\n");
}

void SearchOutput(int depth, int time, int score, char *line, int nodes)
{
    Print(1, "%2d  %s %7s  ", depth, TimeToText(time), ScoreToText(score));
    PrintPV(line);
    if(PostMode) {
        char *short_line = strdup(line);
        int s = score/10;

        if(s >= 9999) {
            s = 9999;
        } else if(s <= -9999) {
            s = -9999;
        }

        if(short_line) {
            if(strlen(short_line) > 80) {
                int idx;
                for(idx=80; idx>1; idx--) {
                    if(short_line[idx] == ' ' && short_line[idx-1] != '.') {
                        break;
                    }
                }
                short_line[idx] = '\0';
            }
            PrintNoLog(
                0,
                "%d %d %d %d %s\n",
                depth,
                s,
                time,
                nodes,
                short_line);
            free(short_line);
        }
    }
}

void SearchOutputFailHighLow(
    int depth, int time, int isfailhigh, char *move, int nodes)
{
    if(isfailhigh) {
        Print(1, "%2d  %s     +++  %s\n", depth, TimeToText(time), move);
        if(PostMode) {
            PrintNoLog(0, "%d 0 %d %d %s!\n", depth, time, nodes, move);
        }
    }
    else {
        Print(1, "%2d  %s     ---  %s\n", depth, TimeToText(time), move);
        if(PostMode) {
            PrintNoLog(0, "%d 0 %d %d %s?\n", depth, time, nodes, move);
        }
    }
}
