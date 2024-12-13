#include <assert.h>
#include <stdio.h>

#include "amy.h"
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
