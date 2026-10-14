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

#include "pgn_write.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Fresh in-memory FILE with a zeroed buffer, so each test is independent. */
typedef struct {
    char data[2048];
    FILE *f;
} emit_buf_t;

/* Heap-allocated so the fmemopen FILE keeps a valid buffer past the call. */
static emit_buf_t *new_emit_buf(void) {
    emit_buf_t *b = calloc(1, sizeof(emit_buf_t));
    assert(b != NULL);
    b->f = fmemopen(b->data, sizeof(b->data), "w");
    assert(b->f != NULL);
    return b;
}

static void close_emit_buf(emit_buf_t *buf) {
    fclose(buf->f);
    free(buf);
}

/* pgn_emit_line writes the message and tracks the characters written */
static void test_emit_pgn_line_writes_and_tracks_width(void) {
    emit_buf_t *buf = new_emit_buf();
    int width = 0;

    pgn_emit_line(buf->f, &width, "%s ", "hello");
    fflush(buf->f);

    assert(width == 6);
    assert(strcmp(buf->data, "hello ") == 0);
    close_emit_buf(buf);
}

/* reaching exactly 72 characters must not wrap */
static void test_emit_pgn_line_no_wrap_at_72(void) {
    emit_buf_t *buf = new_emit_buf();
    int width = 0;

    char token[41], token2[33];
    memset(token, 'a', 40);
    token[40] = '\0';
    memset(token2, 'b', 32);
    token2[32] = '\0';

    pgn_emit_line(buf->f, &width, "%s", token);  /* 40 */
    pgn_emit_line(buf->f, &width, "%s", token2); /* 32 */
    fflush(buf->f);

    assert(width == 72);
    assert(strlen(buf->data) == 72); /* no trailing newline */
    close_emit_buf(buf);
}

/* crossing 72 characters emits a newline and resets the width */
static void test_emit_pgn_line_wraps_over_72(void) {
    emit_buf_t *buf = new_emit_buf();
    int width = 0;

    char token[71], token2[6];
    memset(token, 'a', 70);
    token[70] = '\0';
    memset(token2, 'b', 5);
    token2[5] = '\0';

    pgn_emit_line(buf->f, &width, "%s", token);  /* 70 */
    pgn_emit_line(buf->f, &width, "%s", token2); /*  5 -> 75, wraps */
    fflush(buf->f);

    assert(width == 0);
    assert(strlen(buf->data) == 76); /* 75 chars + newline */
    assert(buf->data[75] == '\n');
    close_emit_buf(buf);
}

/* a single over-long message is still fully written before wrapping */
static void test_emit_pgn_line_long_message_wraps(void) {
    emit_buf_t *buf = new_emit_buf();
    int width = 0;

    char token[101];
    memset(token, 'b', 100);
    token[100] = '\0';

    pgn_emit_line(buf->f, &width, "%s", token);
    fflush(buf->f);

    assert(width == 0);
    assert(strlen(buf->data) == 101);
    assert(buf->data[100] == '\n');
    close_emit_buf(buf);
}

/* an empty message writes nothing and leaves the width untouched */
static void test_emit_pgn_line_empty_is_noop(void) {
    emit_buf_t *buf = new_emit_buf();
    int width = 5;

    pgn_emit_line(buf->f, &width, "%s", "");
    fflush(buf->f);

    assert(width == 5);
    assert(buf->data[0] == '\0');
    close_emit_buf(buf);
}

void test_all_pgn_write(void) {
    test_emit_pgn_line_writes_and_tracks_width();
    test_emit_pgn_line_no_wrap_at_72();
    test_emit_pgn_line_wraps_over_72();
    test_emit_pgn_line_long_message_wraps();
    test_emit_pgn_line_empty_is_noop();
}
