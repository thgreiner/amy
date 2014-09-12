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
 * state_machine.c - the state machine which handles program states
 *
 * $Id: state_machine.c 59 2003-03-13 19:49:00Z thorsten $
 *
 */

#include "amy.h"

static char InputBuffer[1024];

int State;
int XBoardMode = FALSE;
int ForceMode = FALSE;
int EasyMode  = FALSE;
int PostMode = FALSE;
int AutoSave = FALSE;

struct Position *CurrentPosition;
int ComputerSide;

/**
 * Implements the state machine.
 */
void StateMachine(void)
{
    char *gameend;

    State = STATE_WAITING;

    CurrentPosition = InitialPosition();
    ComputerSide = Black;

    while(State != STATE_END) {

	if(AutoSave) {
	    SaveGame(CurrentPosition, AutoSaveFileName);
	}

        switch(State) {
            case STATE_WAITING:
                if(!XBoardMode) {
                    Print(0, "%s(%d): ", 
                        CurrentPosition->turn == White ? "White":"Black", 
                        (CurrentPosition->ply/2)+1);
                }

                if(!ReadLine(InputBuffer, 1023)) {
                    State = STATE_END;
                } else {
                    struct Command *command = ParseInput(InputBuffer);
                    if(command) {
                        ExecuteCommand(command);
                        if(command->move != M_NONE) {
                            if(!ForceMode) State = STATE_CALCULATING;
                        }
                    }
                }
                break;
            case STATE_CALCULATING:
                ComputerSide = CurrentPosition->turn;
                SearchRoot(CurrentPosition);
                if(EasyMode || ForceMode) {
                    State = STATE_WAITING;
                }
                else {
                    State = STATE_PONDERING;
                }
                break;
            case STATE_PONDERING:
                switch(PermanentBrain(CurrentPosition)) {
                    case PB_NO_PB_MOVE:
                        State = STATE_WAITING;
                        break;
                    case PB_NO_PB_HIT:
                        State = ForceMode ? STATE_WAITING : STATE_CALCULATING;
                        break;
                    case PB_HIT:
                        if(EasyMode) {
                            State = STATE_WAITING;
                        }
                        break;
                }
                break;
            case STATE_ANALYZING:
		AnalysisMode(CurrentPosition);
                break;
        }

        /*
         * Check for game termination
         */

        gameend = GameEnd(CurrentPosition);
        if(gameend != NULL) {
            Print(0, "%s\n", gameend);
            if(State == STATE_ANALYZING) {
                State = STATE_WAITING;
            }
        }
    }

    FreePosition(CurrentPosition);
}
