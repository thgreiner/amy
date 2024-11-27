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

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tree.h"

/** Magic constant to identify trees written to disk. */
static const char *MAGIC = "ATRE";

/**
 * Allocate a tree node.
 */
static tree_node_t *allocate_node(char *key_data, size_t key_len,
                                  char *value_data, size_t value_len) {
    tree_node_t *node = malloc(sizeof(tree_node_t));
    if (node == NULL) {
        perror("Failed to allocate node");
        exit(1);
    }

    node->key_data = malloc(key_len);
    if (node->key_data == NULL) {
        perror("Failed to allocate key_data");
        exit(1);
    }
    node->key_len = key_len;

    node->value_data = malloc(value_len);
    if (node->value_data == NULL) {
        perror("Failed to allocate value_data");
        exit(1);
    }
    node->value_len = value_len;

    memcpy(node->key_data, key_data, key_len);
    memcpy(node->value_data, value_data, value_len);

    node->left_child = NULL;
    node->right_child = NULL;
    node->depth = 0;

    return node;
}

/**
 * Free a tree node recursively.
 */
void free_node(tree_node_t *node) {
    if (node == NULL)
        return;

    free_node(node->left_child);
    free_node(node->right_child);

    free(node->value_data);
    free(node->key_data);
    free(node);
}

/**
 * Compare two keys.
 */
static int cmp_keys(char *key1, size_t len1, char *key2, size_t len2) {
    size_t min_len = (len1 < len2) ? len1 : len2;
    int result = memcmp(key1, key2, min_len);

    if (result != 0) {
        return result;
    }

    return len1 - len2;
}

/**
 * Get the depth of a node - returns 0 if node is NULL.
 */
static inline unsigned int get_depth(tree_node_t *node) {
    return (node == NULL) ? 0 : node->depth;
}

/**
 * Checks whether node is a leaf node (has no children).
 */
static inline bool is_leaf(tree_node_t *node) {
    return node->left_child == NULL && node->right_child == NULL;
}

/**
 * Update 'depth' of the supplied node using the depth information of
 * its children.
 */
static void update_depth(tree_node_t *node) {
    if (is_leaf(node)) {
        node->depth = 0;
        return;
    }
    unsigned int left_depth = get_depth(node->left_child);
    unsigned int right_depth = get_depth(node->right_child);
    unsigned int max_depth =
        (left_depth > right_depth) ? left_depth : right_depth;
    node->depth = max_depth + 1;
}

/**
 * Perform a right rotation of node.
 */
static tree_node_t *rotate_right(tree_node_t *node) {
    tree_node_t *child = node->left_child;
    tree_node_t *tmp = child->right_child;

    child->right_child = node;
    node->left_child = tmp;

    update_depth(node);
    update_depth(child);

    return child;
}

/**
 * Perform a left rotation of a node.
 */
static tree_node_t *rotate_left(tree_node_t *node) {
    tree_node_t *child = node->right_child;
    tree_node_t *tmp = child->left_child;

    child->left_child = node;
    node->right_child = tmp;

    update_depth(node);
    update_depth(child);

    return child;
}

/**
 * Performs a right rotation or a left right rotation of node.
 */
static tree_node_t *rotate_right_full(tree_node_t *node) {
    tree_node_t *child = node->left_child;
    if (get_depth(child->right_child) > get_depth(child->left_child)) {
        child = rotate_left(child);
        node->left_child = child;
    }
    return rotate_right(node);
}

/**
 * Performs a left rotation or a right left rotation of node.
 */
static tree_node_t *rotate_left_full(tree_node_t *node) {
    tree_node_t *child = node->right_child;
    if (get_depth(child->left_child) > get_depth(child->right_child)) {
        child = rotate_right(child);
        node->right_child = child;
    }
    return rotate_left(node);
}

/**
 * Balance a node.
 */
tree_node_t *balance(tree_node_t *node) {
    unsigned int left_depth = get_depth(node->left_child);
    unsigned int right_depth = get_depth(node->right_child);

    int imbalance = left_depth - right_depth;

    if (abs(imbalance) < 2) {
        return node;
    }

    if (imbalance > 0) {
        return rotate_right_full(node);
    } else {
        return rotate_left_full(node);
    }
}

/**
 * Add a node to the tree.
 */
tree_node_t *add_node(tree_node_t *node, char *key_data, size_t key_len,
                      char *value_data, size_t value_len) {
    if (node == NULL) {
        return allocate_node(key_data, key_len, value_data, value_len);
    }

    int comparison = cmp_keys(key_data, key_len, node->key_data, node->key_len);

    if (comparison == 0) {
        node->value_data = realloc(node->value_data, value_len);
        if (node->value_data == NULL) {
            perror("Failed to allocate value_data");
            exit(1);
        }
        memcpy(node->value_data, value_data, value_len);
        return node;
    } else if (comparison < 0) {
        node->left_child = add_node(node->left_child, key_data, key_len,
                                    value_data, value_len);
    } else {
        node->right_child = add_node(node->right_child, key_data, key_len,
                                     value_data, value_len);
    }
    update_depth(node);

    return balance(node);
}

/**
 * Lookup a value in the tree.
 */
static char *lookup_value_internal(tree_node_t *node, char *key_data,
                                   size_t key_len, size_t *value_len,
                                   int depth) {
    if (node == NULL) {
        return NULL;
    }

    int comparison = cmp_keys(key_data, key_len, node->key_data, node->key_len);

    if (comparison == 0) {
        if (value_len != NULL) {
            *value_len = node->value_len;
        }
        char *buffer = malloc(node->value_len);
        if (buffer == NULL) {
            return NULL;
        }
        memcpy(buffer, node->value_data, node->value_len);
        return buffer;
    } else if (comparison < 0) {
        return lookup_value_internal(node->left_child, key_data, key_len,
                                     value_len, depth + 1);
    } else {
        return lookup_value_internal(node->right_child, key_data, key_len,
                                     value_len, depth + 1);
    }
}

/**
 * Lookup a value in the tree. Returns NULL if the tree does not
 * contain the key. Otherwise, a copy of the value is returned.
 * Use free() on the return value to free the memory of the copy.
 */
char *lookup_value(tree_node_t *node, char *key_data, size_t key_len,
                   size_t *value_len) {
    return lookup_value_internal(node, key_data, key_len, value_len, 0);
}

/**
 * Write a size_t value to fout. This does a little bit of compression
 * by saving only 7 bits of the value and setting a continuation
 * bit if the value exceeds seven bits.
 */
static inline void write_size(size_t value, FILE *fout) {
    for (;;) {
        int output_value = value & 0x7f;
        value >>= 7;
        if (value) {
            output_value |= 0x80;
        }
        fputc(output_value, fout);

        if (value == 0)
            break;
    }
}

/**
 * Traverse the tree recursively and write to file.
 */
static void save_tree_recursive(tree_node_t *node, FILE *fout) {
    if (node == NULL) {
        return;
    }

    write_size(node->key_len, fout);
    fwrite(node->key_data, node->key_len, 1, fout);
    write_size(node->value_len, fout);
    fwrite(node->value_data, node->value_len, 1, fout);

    save_tree_recursive(node->left_child, fout);
    save_tree_recursive(node->right_child, fout);
}

/**
 * Save the tree to a file.
 */
void save_tree(tree_node_t *node, FILE *fout) {
    int records_written = fwrite(MAGIC, 4, 1, fout);
    if (records_written != 1)
        return;
    save_tree_recursive(node, fout);
}

/**
 * Read a size_t value from fin. This uncompresses the
 * value written by write_size.
 */
static inline size_t read_size(FILE *fin) {
    size_t value = 0;
    for (;;) {
        int input_value = fgetc(fin);
        if (input_value == EOF)
            return 0;

        value = (value << 7) | (input_value & 0x7f);
        if ((input_value & 0x80) == 0)
            break;
    }
    return value;
}

/**
 * Load the tree from a file.
 */
static tree_node_t *load_tree_internal(FILE *fin) {
    tree_node_t *node = NULL;
    size_t key_len;
    size_t value_len;
    char *key_data = malloc(0);
    char *value_data = malloc(9);

    for (;;) {
        key_len = read_size(fin);
        if (key_len == 0)
            break;

        key_data = realloc(key_data, key_len);
        int amount_read = fread(key_data, key_len, 1, fin);
        if (amount_read != 1)
            break;

        value_len = read_size(fin);
        if (value_len == 0)
            break;

        value_data = realloc(value_data, value_len);
        amount_read = fread(value_data, value_len, 1, fin);
        if (amount_read != 1)
            break;

        node = add_node(node, key_data, key_len, value_data, value_len);
    }

    free(key_data);
    free(value_data);

    return node;
}

/**
 * Load the tree from a file.
 */
tree_node_t *load_tree(FILE *fin) {
    char buffer[4];
    int records_read = fread(buffer, 4, 1, fin);
    if (records_read != 1)
        return NULL;

    if (memcmp(buffer, MAGIC, 4) != 0)
        return NULL;

    return load_tree_internal(fin);
}
