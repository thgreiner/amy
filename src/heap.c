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

#include "heap.h"

static const int DATA_SIZE = 1024;
static const int SECTION_SIZE = 32;

heap_t allocate_heap(void) {
    heap_t heap = (heap_t)malloc(sizeof(struct heap));
    if (heap == NULL) {
        perror("Cannot allocate heap:");
        exit(1);
    }

    move_t *data = (move_t *)malloc(DATA_SIZE * sizeof(move_t));
    if (data == NULL) {
        perror("Cannot allocate heap:");
        exit(1);
    }

    heap->data = data;
    heap->capacity = DATA_SIZE;

    heap_section_t sections =
        (heap_section_t)malloc(SECTION_SIZE * sizeof(struct heap_section));
    if (sections == NULL) {
        perror("Cannot allocate heap:");
        exit(1);
    }

    heap->sections_start = sections;
    heap->sections_end = sections + SECTION_SIZE;
    heap->current_section = sections;

    heap->current_section->start = 0;
    heap->current_section->end = 0;

    return heap;
}

void free_heap(heap_t heap) {
    free(heap->data);
    free(heap->sections_start);
    free(heap);
}
