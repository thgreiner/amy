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

#include "bpd.h"
#include "neural_net.h"
#include "tree.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static int count_non_pawn(BinaryPosition *pos) {
    int non_pawn = 0;
    for (int i = 0; i < 4; i++) {
        uint64_t b = pos->pos[i];

        while (b) {
            int nibble = b & 0xf;
            b >>= 4;

            if ((nibble >= PN_W_KNIGHT && nibble <= PN_W_QUEEN) ||
                (nibble >= PN_B_KNIGHT && nibble <= PN_B_QUEEN)) {
                non_pawn++;
            }
        }
    }

    return non_pawn;
}

static size_t walk_tree(tree_node_t *node, size_t *buckets,
                        size_t *phase_buckets) {
    if (node == NULL)
        return 0;

    size_t count = *((size_t *)node->value_data);

    buckets[count] += 1;

    int non_pawn = count_non_pawn((BinaryPosition *)node->key_data);

    int phase = non_pawn / 2;
    phase_buckets[phase] += 1;

    if (phase < 7 && (non_pawn % 2 == 1)) {
        phase_buckets[phase + 1] += 1;
    }

    size_t node_count = 1;
    node_count += walk_tree(node->left_child, buckets, phase_buckets);
    node_count += walk_tree(node->right_child, buckets, phase_buckets);

    return node_count;
}

static void create_histogram(tree_node_t *database, size_t max_count) {
    size_t *buckets = calloc(max_count + 1, sizeof(size_t));
    size_t phase_buckets[NN_PHASE_COUNT] = {0};

    size_t total_nodes = walk_tree(database, buckets, phase_buckets);

    printf("Unique positions: %lu\n", total_nodes);

    printf("\n");
    printf("  #     Count   relative\n");

    for (size_t s = 1; s <= max_count; s++) {
        if (buckets[s] != 0) {
            double percentage =
                (100.0 * (double)buckets[s]) / (double)total_nodes;
            printf("%3lu: %8lu    %5.2f %%\n", s, buckets[s], percentage);
        }
    }

    free(buckets);

    printf("\n");
    printf("Positions per phase:\n");
    printf("  #     Count   relative\n");

    for (int phase = 0; phase < NN_PHASE_COUNT; phase++) {
        double percentage =
            (100.0 * (double)phase_buckets[phase]) / (double)total_nodes;
        printf("%3d: %8lu    %5.2f %%\n", phase, phase_buckets[phase],
               percentage);
    }
}

static void analyze(const char *file_name) {
    FILE *fin = fopen(file_name, "rb");

    if (fin == NULL) {
        fprintf(stderr, "Cannot open %s\n", file_name);
        exit(1);
    }

    size_t total_positions = 0;
    size_t max_count = 0;
    tree_node_t *database = NULL;

    for (;;) {
        BinaryPosition pos;
        size_t items_read = fread(&pos, sizeof(BinaryPosition), 1, fin);

        if (items_read != 1)
            break;

        total_positions++;

        size_t *entry = NULL;

        if (database != NULL) {
            entry =
                (size_t *)tree_lookup_value(database, &pos, sizeof(pos), NULL);
        }

        if (entry == NULL) {
            entry = calloc(1, sizeof(size_t));
        }

        *entry += 1;

        assert(*entry >= 1);

        database = tree_add_node(database, &pos, sizeof(pos), (char *)entry,
                                 sizeof(size_t));

        if (*entry > max_count) {
            max_count = *entry;
        }

        free(entry);
    }

    printf("Total positions: %lu\n", total_positions);

    create_histogram(database, max_count);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        exit(1);
    }

    analyze(argv[1]);
}
