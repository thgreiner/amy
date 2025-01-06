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

#ifndef HEAP_H
#define HEAP_H

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "amy.h"

static inline void append_to_heap(heap_t heap, move_t data) {
    if (heap->current_section->end >= heap->capacity) {
        // printf("Reallocating heap.\n");
        move_t *new_data =
            (move_t *)realloc(heap->data, 2 * heap->capacity * sizeof(move_t));
        if (new_data == NULL) {
            perror("Cannot grow heap.");
            exit(1);
        }
        heap->data = new_data;
        heap->capacity = 2 * heap->capacity;
        // printf("Grown heap capacity to %d.\n", heap->capacity);
    }
    heap->data[heap->current_section->end++] = data;
}

static inline void push_section(heap_t heap) {
    heap->current_section++;
    if (heap->current_section == heap->sections_end) {
        // printf("Reallocating sections.\n");
        int nsections = heap->sections_end - heap->sections_start;
        heap_section_t new_sections = (heap_section_t)realloc(
            heap->sections_start, 2 * nsections * sizeof(struct heap_section));
        if (new_sections == NULL) {
            perror("Cannot grow sections.");
            exit(1);
        }
        heap->sections_start = new_sections;
        heap->sections_end = new_sections + 2 * nsections;
        heap->current_section = new_sections + nsections;
    }
    heap->current_section->start = (heap->current_section - 1)->end;
    heap->current_section->end = heap->current_section->start;
}

static inline void pop_section(heap_t heap) {
    assert(heap->current_section > heap->sections_start);
    heap->current_section--;
}

#endif
