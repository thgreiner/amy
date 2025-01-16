/*

    Amy - a chess playing program

    Copyright (c) 2002-2025, Thorsten Greiner
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

#ifndef YAML_H
#define YAML_H

#include <stdbool.h>

typedef enum { SCALAR = 0, LIST, DICT } NodeType;

typedef enum {
    END = 0,
    UNKNOWN,
    COLON,
    WORD,
    OPENING_BRACKET,
    CLOSING_BRACKET,
    COMMA,
    OPENING_BRACE,
    CLOSING_BRACE,
    BAD_INDENT,
    CONTINUE,
} TokenType;

typedef enum { OK = 0, NOT_FOUND, TYPE_ERROR, FORMAT_ERROR } LookupResultCode;

struct Token {
    TokenType type;
    char *text;
};

struct Node {
    NodeType type;
    void *payload;
};

struct ListNode {
    struct ListNode *next;
    struct Node *value;
};

struct StringLookupResult {
    LookupResultCode result_code;
    char *result;
};

struct IntLookupResult {
    LookupResultCode result_code;
    int result;
};

struct ListLookupResult {
    LookupResultCode result_code;
    struct ListNode *result;
};

struct IntArrayLookupResult {
    LookupResultCode result_code;
    unsigned int elements_read;
};

struct Node *parse_yaml(char *);
struct StringLookupResult get_as_string(struct Node *, char *);
struct IntLookupResult get_as_int(struct Node *, char *);
struct ListLookupResult get_as_list(struct Node *, char *);
struct IntArrayLookupResult get_as_int_array(struct Node *, char *, int *, int);
void abort_if_allocation_failed(void *x);

void free_yaml_node(struct Node *);

#endif
