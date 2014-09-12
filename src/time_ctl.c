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
 * time_ctl.c - time management routines
 *
 * $Id: time_ctl.c 27 2003-02-11 22:39:17Z thorsten $
 *
 */

#include <stdio.h>
#include <string.h>
#include "amy.h"

int TMoves = 60,   TTime = 5*60;
int Moves[3] = { 60, 60 }, 
    Time[3]  = { 5*60, 5*60 }
;
int TMoves2, TTime2;
int TwoTimeControls = FALSE;

int Increment = 0;

void DoTC(struct Position *p, int mtime)
{
    Time[p->turn] += -mtime + Increment;
    
    if(Moves[p->turn] > 0) {
	Moves[p->turn] -= 1;
	if(Moves[p->turn] <= 0) {
            if(TwoTimeControls) {
                Print(0, "Switching to second time control.\n");
                TMoves = TMoves2;
                TTime  = TTime2;
                TwoTimeControls = FALSE;
            }
	    Moves[p->turn] = TMoves;
	    Time[p->turn] += TTime;
	}
    }
}

void CalcTime(struct Position *p, float *soft, float *hard)
{
    if(TMoves >= 0) {
	if(Moves[p->turn] > 0) {
	    /*  int limit = (13*TTime/TMoves)/8 + (3*Increment)/4;  */
            float limit = (1.625*TTime/TMoves) + (0.75*Increment);
	    
	    Print(1, "TC: %d moves in %s\n", 
                Moves[p->turn], 
                TimeToText((unsigned int) (Time[p->turn]) * ONE_SECOND ));

	    /*  *soft = (7*Time[p->turn]/Moves[p->turn])/8 + (3*Increment)/4;  */
            *soft = (0.875*Time[p->turn]/Moves[p->turn]) + (0.75*Increment);
	    if(*soft > limit) *soft = limit;

            if(TwoTimeControls && Moves[p->turn] <= 5) {
                int moves = TMoves2;
                int soft2;
                if(moves == 0) moves = 60;
                soft2 = TTime2/moves;
                /*    *soft = ((*soft)+(float)soft2)/2;    */
                *soft = 0.5 *((*soft)+(float)soft2);
                Print(1, "Adjusted timing to %.4f secs\n", *soft);
            }
	    *hard = 4.0*(*soft);
	}
	else {
            /*  expect additional game length of 60 moves beyond current move */
	    /*  use equations from section above with fixed Moves[p->turn] of 60  */
	    /*  rearrange equation to eliminate floating point division  */
	    /*  1.625 / 60 = 0.0271  */	
            float limit =  0.0271 * (float)Time[p->turn];  
	    
	    Print(1, "TC: all moves in %s\n", 
                TimeToText((unsigned int) (Time[p->turn]) * ONE_SECOND ));

	    /*  0.875 / 60.0 = 0.0146  */
	    *soft = 0.0146 * (float)Time[p->turn] + (0.75 * Increment);     
	    if(*soft > limit) *soft = limit;
	    *hard = 4.0*(*soft);
	}	
	if(*hard > Time[p->turn]) *hard = 0.5 * (float)Time[p->turn];
	if(*soft > *hard) *soft = 0.67*(*hard);
	
	Print(1, "TL: %.2f/%.2f\n", *soft, *hard);
    }
    else {
	*soft = *hard = (float)TTime;
    }
}
