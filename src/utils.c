/*

    Amy - a chess playing program

    Copyright (c) 2014, Thorsten Greiner
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this
      list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
    FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
    DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
    OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

/*
 * utils.c - utility routines
 */

#include "amy.h"

FILE *LogFile = NULL;
int Verbosity = 9;

/**
 * Open a log file, remember fp in global variable LogFile
 */

void OpenLogFile(char *name)
{
    if(LogFile) {
        fclose(LogFile);
    }
    LogFile = fopen(name, "w");
}

/**
 * Print something to stdout and to the logfile.
 */

void CDECL Print(int vb, char *fmt, ...)
{
    if (vb < Verbosity) {
        va_list va;
        va_start(va,fmt);
        vprintf(fmt, va);
        fflush(stdout);
        va_end(va);
    }
    if (LogFile) {
        va_list va;
        va_start(va,fmt);
        vfprintf(LogFile, fmt, va);
        fflush(LogFile);
        va_end(va);
    }
}

/**
 * Print to stdout only.
 */

void CDECL PrintNoLog(int vb, char *fmt, ...)
{
    va_list va;
    va_start(va,fmt);
    if (vb < Verbosity) {
        vprintf(fmt, va);
        fflush(stdout);
    }
    va_end(va);
}

/**
 * Read a line from stdin.
 */

int ReadLine(char *buffer, int cnt)
{
    return fgets(buffer, cnt, stdin) != NULL;
}

/**
 * Convert an int representing a time in seconds to a string.
 */

char *TimeToText(unsigned int secs)
{
    static char buffer[15];

    if(secs >= 60*ONE_SECOND) {
	int mins;
	secs = secs/ONE_SECOND;
	mins  = secs / 60; secs -= mins*60;

	if(mins >= 100) sprintf(buffer, "%d:%02d", mins, secs);
	else if(mins >= 10) sprintf(buffer, " %d:%02d", mins, secs);
	else sprintf(buffer, "  %d:%02d", mins, secs);
    }
    else {
	int tsecs = (secs % ONE_SECOND)/10;
	secs = secs / ONE_SECOND;

	sprintf(buffer, "  %2d.%d", secs, tsecs);
    }
    return buffer;
}

/**
 * Convert a score to a string.
 */

char *ScoreToText(int score)
{
    static char buffer[10];

    if(score > CMLIMIT) {
        sprintf(buffer, "+M%d",  (INF-score)/2 + 1);
    } else if(score < -CMLIMIT) {
        sprintf(buffer, "-M%d", (score+INF)/2);
    } else if(score == CMLIMIT) {
        sprintf(buffer, "+Mate");
    } else if(score == -CMLIMIT) {
        sprintf(buffer, "-Mate");
    } else if(score >= 0) {
        sprintf(buffer, "+%d.%03d", score/1000, score%1000);
    } else {
        sprintf(buffer, "-%d.%03d", (-score)/1000, (-score)%1000);
    }

    return buffer;
}

/**
 * Get the current time.
 */

unsigned int GetTime(void)
{
#if HAVE_GETTIMEOFDAY
    static struct timeval timeval;
    unsigned int now;

    gettimeofday(&timeval, NULL);
    now = timeval.tv_sec*100+(timeval.tv_usec / 10000L);
    return now;
#else
#ifdef _WIN32
    return ((unsigned int) GetTickCount () / 10);
#else
#error TIME COUNTING MUST BE IMPLEMENTED
#endif
#endif
}

/**
 * Create a filename for a temporary file
 */

char *GetTmpFileName(void)
{
    int cnt = 0;
    static char buf[128];

    for(cnt=0; ;cnt++) {
	    int result;
	    struct stat dummy;

	    sprintf(buf, "save_%03d.pgn", cnt);
	    result = stat(buf, &dummy);

	    if(result < 0) break;
    }

    return buf;
}

/**
 * Calculate the 'king distance' between two squares.
 * This the number of king moves to go from sq1 to sq2.
 */

int KingDist(int sq1, int sq2) {
    int file_dist = ABS((sq1 & 7) - (sq2 & 7));
    int rank_dist = ABS((sq1 >> 3) - (sq2 >> 3));

    return MAX(file_dist, rank_dist);
}

/**
 * Calculate the 'minimum distance' between two squares.
 * This the minimum of the file and rank distances.
 */

int MinDist(int sq1, int sq2) {
    int file_dist = ABS((sq1 & 7) - (sq2 & 7));
    int rank_dist = ABS((sq1 >> 3) - (sq2 >> 3));

    return MIN(file_dist, rank_dist);
}

/**
 * Calculate the 'Manhattan distance' between two squares.
 */

int ManhattanDist(int sq1, int sq2)
{
    int file_dist = ABS((sq1 & 7) - (sq2 & 7));
    int rank_dist = ABS((sq1 >> 3) - (sq2 >> 3));

    return file_dist + rank_dist;
}

int FileDist(int sq1, int sq2) {
    return ABS((sq1 & 7) - (sq2 & 7));
}

/**
 * Calculate the distance of 'sq' to any edge on the chessboard
 */

int EdgeDist(int sq)
{
    int filedist = MIN(sq & 7, 7 - (sq & 7));
    int rankdist = MIN(sq >> 3, 7 - (sq >> 3));

    return MAX(filedist, rankdist);
}

/**
 * Check if we can read from stdin without blocking.
 */

int InputReady(void)
{
#if HAVE_SELECT
    fd_set rfd;
    struct timeval timeout;
    timeout.tv_sec = timeout.tv_usec = 0;
    FD_ZERO(&rfd);
    FD_SET(0, &rfd);

    return select(1, &rfd, NULL, NULL, &timeout) > 0;
#else
#ifdef _WIN32
    int             i;
    static int      init = 0,
                    pipe;
    static HANDLE   inh;
    DWORD           dw;

    if (!XBoardMode && !isatty(fileno(stdin)))
        return (0);
    if (XBoardMode) {
#if defined(FILE_CNT)
        if (stdin->_cnt > 0)
            return stdin->_cnt;
#endif
        if (!init) {
            init = 1;
            inh = GetStdHandle(STD_INPUT_HANDLE);
            pipe = !GetConsoleMode(inh, &dw);
            if (!pipe) {
                SetConsoleMode(inh, dw & ~(ENABLE_MOUSE_INPUT | ENABLE_WINDOW_INPUT));
                FlushConsoleInputBuffer(inh);
            }
        }
        if (pipe) {
            if (!PeekNamedPipe(inh, NULL, 0, NULL, &dw, NULL)) {
                return 1;
            }
            return dw;
        } else {
            GetNumberOfConsoleInputEvents(inh, &dw);
            return dw <= 1 ? 0 : dw;
        }
    } else {
        i = _kbhit();
    }
    return (i);
#else
    return 1;
#endif
#endif /* HAVE_SELECT */
}

/**
 * Tokenize a string.
 */

char *nextToken(char **string, const char *delim)
{
    char *start = *string;
    char *end;
    const char *t;
    int flag = TRUE;

    if(start == NULL) return NULL;
    
    while(flag) {
	flag = FALSE;
	if(*start == '\0') return NULL;
	for(t = delim; *t; t++) {
	    if(*t == *start) {
		flag = TRUE;
		start++;
		break;
	    }
	}
    }
    
    end = start+1;
    
    for(;;) {
      	if(*end == '\0') {
	    *string = end;
	    return start;
	}
	for(t = delim; *t; t++) {
	    if(*t == *end) {
		*end = 0;
		*string = end+1;
		return start;
	    }
	}

	end++;
    }

    /* NEVER REACHED */
}

