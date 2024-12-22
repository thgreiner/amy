/*

    Amy - a chess playing program

    Copyright (c) 2002-2024, Thorsten Greiner
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

#include "amy.h"
#include "yaml.h"

static char *read_file(char *);

void ReadScoringConfig(char *file_name) {
    char *buffer = read_file(file_name);

    if (buffer == NULL)
        return;

    struct Node *node = parse_yaml(buffer);

    struct IntLookupResult result;

    result = get_as_int(node, "knight.value");
    if (result.result_code == OK) {
        Print(0, "Knight value: %d\n", result.result);
        Value[Knight] = result.result;
    }

    result = get_as_int(node, "bishop.value");
    if (result.result_code == OK) {
        Print(0, "Bishop value: %d\n", result.result);
        Value[Bishop] = result.result;
    }

    result = get_as_int(node, "rook.value");
    if (result.result_code == OK) {
        Print(0, "Rook value: %d\n", result.result);
        Value[Rook] = result.result;
    }

    result = get_as_int(node, "queen.value");
    if (result.result_code == OK) {
        Print(0, "Queen value: %d\n", result.result);
        Value[Queen] = result.result;
    }

    free(buffer);
}

static char *read_file(char *file_name) {
    FILE *fin = fopen(file_name, "r");
    if (!fin) {
        perror("Cannot open file");
        return NULL;
    }

    const size_t page_size = 1024;
    size_t buf_size = page_size;
    size_t total_bytes_read = 0;

    char *buffer = malloc(buf_size);
    abort_if_allocation_failed(buffer);

    char *ptr = buffer;

    for (;;) {
        size_t bytes_read = fread(ptr, 1, page_size, fin);

        if (bytes_read == 0)
            break;

        ptr += bytes_read;
        total_bytes_read += bytes_read;

        if ((total_bytes_read + page_size) >= buf_size) {
            buf_size *= 2;
            buffer = realloc(buffer, buf_size);
            abort_if_allocation_failed(buffer);
        }
    }

    fclose(fin);

    if ((total_bytes_read + 1) >= buf_size) {
        buf_size += 1;
        buffer = realloc(buffer, buf_size);
        abort_if_allocation_failed(buffer);
    }
    *ptr = '\0';

    return buffer;
}
