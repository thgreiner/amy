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

#include <assert.h>
#include <string.h>

#include "test_utils.h"
#include "utils.h"

static void test_strip_null(void) { assert(utils_strip(NULL) == NULL); }

static void test_strip_empty(void) {
    char buf[] = "";
    char *result = utils_strip(buf);
    assert(result == buf);
    assert(result[0] == 0);
}

static void test_strip_only_whitespace(void) {
    char buf[] = "   \t  ";
    char *result = utils_strip(buf);
    assert(result != NULL);
    assert(result[0] == 0);
}

static void test_strip_leading_and_trailing_spaces(void) {
    char buf[] = "   hello world   ";
    char *result = utils_strip(buf);
    assert(!strcmp(result, "hello world"));
}

static void test_strip_leading_only(void) {
    char buf[] = "    hello";
    char *result = utils_strip(buf);
    assert(!strcmp(result, "hello"));
}

static void test_strip_trailing_only(void) {
    char buf[] = "hello    ";
    char *result = utils_strip(buf);
    assert(!strcmp(result, "hello"));
}

static void test_strip_mixed_whitespace(void) {
    char buf[] = "\t\n\r  value \t\n";
    char *result = utils_strip(buf);
    assert(!strcmp(result, "value"));
}

static void test_strip_preserves_internal_whitespace(void) {
    char buf[] = "  a b\tc  ";
    char *result = utils_strip(buf);
    assert(!strcmp(result, "a b\tc"));
}

static void test_strip_single_char(void) {
    char buf[] = " x ";
    char *result = utils_strip(buf);
    assert(!strcmp(result, "x"));
}

void test_all_utils(void) {
    test_strip_null();
    test_strip_empty();
    test_strip_only_whitespace();
    test_strip_leading_and_trailing_spaces();
    test_strip_leading_only();
    test_strip_trailing_only();
    test_strip_mixed_whitespace();
    test_strip_preserves_internal_whitespace();
    test_strip_single_char();
}
