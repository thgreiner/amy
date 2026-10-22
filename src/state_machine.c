/*

    Amy - a chess playing program

    Copyright (c) 2002-2026, Thorsten Greiner
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

#include "state_machine.h"
#include "commands.h"
#include "dbase.h"
#include "pgn_write.h"
#include "search.h"
#include "utils.h"

static char InputBuffer[1024];

ui_state_t State = STATE_WAITING;
bool XBoardMode = false;
bool ForceMode = false;
bool EasyMode = false;
bool PostMode = false;
bool AutoSave = false;
bool SelfPlayMode = false;

struct Position *CurrentPosition = NULL;
int ComputerSide;

/**
 * Implements the state machine.
 */
void state_machine_run(void) {
    const char *gameend;

    cmd_newgame(NULL);

    ComputerSide = Black;

    while (State != STATE_END) {

        if (AutoSave) {
            pgn_save_game(CurrentPosition, AutoSaveFileName);
        }

        switch (State) {
        case STATE_WAITING: {
            /*
             * The prompt is passed to the line editing code (linenoise)
             * instead of being printed separately.
             */
            char prompt[64];

            if (XBoardMode)
                prompt[0] = 0;
            else
                snprintf(prompt, sizeof(prompt),
                         "%s(%d): ", dbase_color_name(CurrentPosition->turn),
                         (CurrentPosition->ply / 2) + 1);

            if (!utils_read_line_prompt(InputBuffer, 1023, prompt)) {
                State = STATE_END;
            } else {
                struct Command *command = cmd_parse_input(InputBuffer);
                if (command) {
                    cmd_execute(command);
                    if (command->move != M_NONE) {
                        if (!ForceMode)
                            State = STATE_CALCULATING;
                    }
                }
            }
        } break;
        case STATE_CALCULATING:
            ComputerSide = CurrentPosition->turn;
            search_root(CurrentPosition);
            if (EasyMode || ForceMode) {
                State = STATE_WAITING;
            } else if (!SelfPlayMode) {
                State = STATE_PONDERING;
            }
            break;
        case STATE_PONDERING:
            switch (search_permanent_brain(CurrentPosition)) {
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
            default:
                break;
            }
            break;
        case STATE_ANALYZING:
            search_analysis(CurrentPosition);
            break;
        case STATE_END:
            break;
        }

        /*
         * Check for game termination
         */

        gameend = dbase_game_end(CurrentPosition);
        if (gameend != NULL) {
            print(0, "%s\n", gameend);
            if (State == STATE_ANALYZING) {
                State = STATE_WAITING;
            }
            SelfPlayMode = false;
        }
    }

    dbase_free_position(CurrentPosition);
}
