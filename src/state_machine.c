/*

    Amy - a chess playing program

    Copyright (c) 2014, Thorsten Greiner
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
 * state_machine.c - the state machine which handles program states
 */

#include "amy.h"

static char InputBuffer[1024];

int State;
int XBoardMode = FALSE;
int ForceMode = FALSE;
int EasyMode = FALSE;
int PostMode = FALSE;
int AutoSave = FALSE;
int SelfPlayMode = FALSE;

struct Position *CurrentPosition;
int ComputerSide;

/**
 * Implements the state machine.
 */
void StateMachine(void) {
    char *gameend;

    State = STATE_WAITING;

    CurrentPosition = InitialPosition();
    ComputerSide = Black;

    while (State != STATE_END) {

        if (AutoSave) {
            SaveGame(CurrentPosition, AutoSaveFileName);
        }

        switch (State) {
        case STATE_WAITING:
            if (!XBoardMode) {
                Print(0, "%s(%d): ",
                      CurrentPosition->turn == White ? "White" : "Black",
                      (CurrentPosition->ply / 2) + 1);
            }

            if (!ReadLine(InputBuffer, 1023)) {
                State = STATE_END;
            } else {
                struct Command *command = ParseInput(InputBuffer);
                if (command) {
                    ExecuteCommand(command);
                    if (command->move != M_NONE) {
                        if (!ForceMode)
                            State = STATE_CALCULATING;
                    }
                }
            }
            break;
        case STATE_CALCULATING:
            ComputerSide = CurrentPosition->turn;
            SearchRoot(CurrentPosition);
            if (EasyMode || ForceMode) {
                State = STATE_WAITING;
            } else if (!SelfPlayMode) {
                State = STATE_PONDERING;
            }
            break;
        case STATE_PONDERING:
            switch (PermanentBrain(CurrentPosition)) {
            case PB_NO_PB_MOVE:
                State = STATE_WAITING;
                break;
            case PB_NO_PB_HIT:
                State = ForceMode ? STATE_WAITING : STATE_CALCULATING;
                break;
            case PB_HIT:
                if (EasyMode) {
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
        if (gameend != NULL) {
            Print(0, "%s\n", gameend);
            if (State == STATE_ANALYZING) {
                State = STATE_WAITING;
            }
            SelfPlayMode = FALSE;
        }
    }

    FreePosition(CurrentPosition);
}
