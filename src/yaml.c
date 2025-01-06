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
#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tree.h"
#include "yaml.h"

struct TokenizerState {
    const char *ptr;
    unsigned int indent_level;
    unsigned int flow_style_indent_level;
    bool check_indent;
    bool flow_style;
};

void free_yaml_node(struct Node *);
void free_list_node(struct ListNode *);
void free_tree_node(tree_node_t *tree);

void abort_if_allocation_failed(void *x) {
    if (!x) {
        perror("Cannot allocate buffer");
        exit(1);
    }
}

static bool is_word_char(char c) { return isalnum(c) || c == '_' || c == '-'; }

static struct Token token_from_type(TokenType type) {
    struct Token result = {.type = type, .text = NULL};
    return result;
}

struct Token parse_word(struct TokenizerState *state) {
    unsigned int length = 0;
    unsigned int trailing_blanks = 0;

    const char *begin = state->ptr;
    while (is_word_char(*state->ptr) || *state->ptr == ' ') {
        if (*state->ptr == ' ') {
            trailing_blanks++;
        } else {
            trailing_blanks = 0;
        }

        state->ptr++;
        length++;
    }

    length -= trailing_blanks;

    char *buffer = malloc(length + 1);
    abort_if_allocation_failed(buffer);

    memcpy(buffer, begin, length);
    buffer[length] = '\0';

    struct Token result = {.type = WORD, .text = buffer};
    return result;
}

static struct Token handle_indent(struct TokenizerState *state,
                                  unsigned int indent) {
    state->check_indent = false;

    if (state->flow_style) {
        if (state->flow_style_indent_level == 0) {
            if (indent > state->indent_level) {
                state->flow_style_indent_level = indent;
            } else {
                return token_from_type(BAD_INDENT);
            }
        } else {
            if (indent != state->flow_style_indent_level) {
                return token_from_type(BAD_INDENT);
            }
        }
    } else {
        if (indent > state->indent_level) {
            state->indent_level = indent;
            return token_from_type(OPENING_BRACE);
        }

        if (indent < state->indent_level) {
            state->indent_level = indent;
            return token_from_type(CLOSING_BRACE);
        }
    }

    return token_from_type(CONTINUE);
}

/**
 * The lexer function - parses the input and returns the next token.
 */
static struct Token next_token(struct TokenizerState *state) {
    for (;;) {
        unsigned int indent = 0;

        while (isblank(*state->ptr)) {
            state->ptr++;
            indent++;
        }

        if (*state->ptr == '\n') {
            state->ptr++;
            state->check_indent = true;
            continue;
        }

        if (*state->ptr == '#') {
            do {
                state->ptr++;
            } while (*state->ptr != '\0' && *state->ptr != '\n');
            if (*state->ptr == '\n') {
                state->check_indent = true;
                continue;
            }
        }

        if (*state->ptr == '\0') {
            return token_from_type(END);
        }

        if (state->check_indent) {
            struct Token result = handle_indent(state, indent);
            if (result.type != CONTINUE) {
                return result;
            }
        }

        if (*state->ptr == ':') {
            state->ptr++;
            return token_from_type(COLON);
        }

        if (*state->ptr == '[') {
            state->ptr++;
            state->flow_style = true;
            state->flow_style_indent_level = 0;
            return token_from_type(OPENING_BRACKET);
        }

        if (*state->ptr == ']') {
            state->ptr++;
            state->flow_style = false;
            return token_from_type(CLOSING_BRACKET);
        }

        if (*state->ptr == ',') {
            state->ptr++;
            return token_from_type(COMMA);
        }

        if (is_word_char(*state->ptr)) {
            return parse_word(state);
        }

        return token_from_type(UNKNOWN);
    }
}

struct ListNode *parse_list(struct TokenizerState *state) {
    struct ListNode *result = NULL;
    struct ListNode *last_node = NULL;

    struct Token token = next_token(state);

    if (token.type == CLOSING_BRACKET) {
        return result;
    }

    for (;;) {
        if (token.type == WORD) {
            struct Node *value = malloc(sizeof(struct Node));
            abort_if_allocation_failed(value);

            value->type = SCALAR;
            value->payload = token.text;

            struct ListNode *next_node = malloc(sizeof(struct ListNode));
            abort_if_allocation_failed(next_node);

            next_node->value = value;
            next_node->next = NULL;

            if (result == NULL) {
                result = next_node;
                last_node = next_node;
            } else {
                last_node->next = next_node;
                last_node = next_node;
            }

            token = next_token(state);
            if (token.type == CLOSING_BRACKET) {
                return result;
            } else if (token.type == COMMA) {
                token = next_token(state);
                continue;
            }
        }
        printf("parse_list: Unexpected token %d!\n", token.type);
        free_list_node(result);
        return NULL;
    }
}

struct Node *parse_dict(struct TokenizerState *state) {
    tree_node_t *result_dict = NULL;

    for (;;) {
        struct Token token = next_token(state);
        if (token.type == WORD) {
            struct Token expected_colon = next_token(state);
            if (expected_colon.type != COLON) {
                free(token.text);
                printf("Unexpected token %d!\n", expected_colon.type);
                return NULL;
            }
            struct Token expected_value = next_token(state);
            if (expected_value.type == WORD) {
                struct Node node = {.type = SCALAR,
                                    .payload = expected_value.text};
                result_dict =
                    add_node(result_dict, token.text, strlen(token.text) + 1,
                             &node, sizeof(struct Node));
            } else if (expected_value.type == OPENING_BRACKET) {
                struct ListNode *list_node = parse_list(state);
                if (list_node == NULL) {
                    free(token.text);
                    free_tree_node(result_dict);
                    return NULL;
                }
                struct Node node = {.type = LIST, .payload = list_node};
                result_dict =
                    add_node(result_dict, token.text, strlen(token.text) + 1,
                             &node, sizeof(struct Node));
            } else if (expected_value.type == OPENING_BRACE) {
                struct Node *dict_node = parse_dict(state);
                if (dict_node == NULL) {
                    free(token.text);
                    free_tree_node(result_dict);
                    return NULL;
                }
                result_dict =
                    add_node(result_dict, token.text, strlen(token.text) + 1,
                             dict_node, sizeof(struct Node));
                free(dict_node);
            } else {
                printf("Unexpected token %d!\n", expected_value.type);
                free(token.text);
                free_tree_node(result_dict);
                return NULL;
            }

            free(token.text);
        } else {
            break;
        }
    }

    struct Node *result = malloc(sizeof(struct Node));
    abort_if_allocation_failed(result);

    result->type = DICT;
    result->payload = result_dict;

    return result;
}

struct Node *parse_yaml(char *text) {
    struct TokenizerState state = {.ptr = text,
                                   .indent_level = 0,
                                   .check_indent = true,
                                   .flow_style = false};
    return parse_dict(&state);
}

struct Node *get_node(struct Node *node, char *path) {
    // Make a copy of path because strtok will clobber it
    char *path_buffer = malloc(strlen(path) + 1);
    abort_if_allocation_failed(path_buffer);
    memcpy(path_buffer, path, strlen(path) + 1);

    char *x = path_buffer;
    struct Node current_node = *node;

    for (;;) {
        char *path_element = strtok(x, ".");
        x = NULL;

        if (path_element == NULL)
            break;

        if (current_node.type != DICT) {
            free(path_buffer);
            return NULL;
        }

        tree_node_t *tree = current_node.payload;
        size_t value_len;
        struct Node *value = lookup_value(tree, path_element,
                                          strlen(path_element) + 1, &value_len);

        if (value == NULL) {
            free(path_buffer);
            return NULL;
        }

        current_node = *value;
        free(value);
    }
    free(path_buffer);

    struct Node *result = malloc(sizeof(struct Node));
    abort_if_allocation_failed(result);
    memcpy(result, &current_node, sizeof(struct Node));

    return result;
}

struct StringLookupResult get_as_string(struct Node *node, char *path) {
    struct Node *target = get_node(node, path);

    if (target == NULL) {
        struct StringLookupResult lookup_result = {.result_code = NOT_FOUND,
                                                   .result = NULL};
        return lookup_result;
    }

    char *result = target->payload;
    int type = target->type;
    free(target);

    if (type != SCALAR) {
        struct StringLookupResult lookup_result = {.result_code = TYPE_ERROR,
                                                   .result = NULL};
        return lookup_result;
    }

    struct StringLookupResult lookup_result = {.result_code = OK,
                                               .result = result};
    return lookup_result;
}

struct IntLookupResult get_as_int(struct Node *node, char *path) {
    struct Node *target = get_node(node, path);

    if (target == NULL) {
        struct IntLookupResult lookup_result = {.result_code = NOT_FOUND,
                                                .result = 0};
        return lookup_result;
    }

    char *result = target->payload;
    int type = target->type;
    free(target);

    if (type != SCALAR) {
        struct IntLookupResult lookup_result = {.result_code = TYPE_ERROR,
                                                .result = 0};
        return lookup_result;
    }

    char *end;
    long value = strtol(result, &end, 10);
    if (*end != '\0') { // Illegal character in the string
        struct IntLookupResult lookup_result = {.result_code = FORMAT_ERROR,
                                                .result = 0};
        return lookup_result;
    }
    if (value > INT_MAX || value < INT_MIN) { // overflow
        struct IntLookupResult lookup_result = {.result_code = FORMAT_ERROR,
                                                .result = 0};
        return lookup_result;
    }

    struct IntLookupResult lookup_result = {.result_code = OK,
                                            .result = (int)value};
    return lookup_result;
}

struct ListLookupResult get_as_list(struct Node *node, char *path) {
    struct Node *target = get_node(node, path);

    if (target == NULL) {
        struct ListLookupResult lookup_result = {.result_code = NOT_FOUND,
                                                 .result = NULL};
        return lookup_result;
    }

    struct ListNode *result = target->payload;
    int type = target->type;
    free(target);

    if (type != LIST) {
        struct ListLookupResult lookup_result = {.result_code = TYPE_ERROR,
                                                 .result = NULL};
        return lookup_result;
    }

    struct ListLookupResult lookup_result = {.result_code = OK,
                                             .result = result};
    return lookup_result;
}

struct IntArrayLookupResult get_as_int_array(struct Node *node, char *path,
                                             int (*buffer)[], int count) {
    struct Node *target = get_node(node, path);

    if (target == NULL) {
        struct IntArrayLookupResult lookup_result = {.result_code = NOT_FOUND,
                                                     .elements_read = 0};
        return lookup_result;
    }

    struct ListNode *list_node = target->payload;
    int type = target->type;
    free(target);

    if (type != LIST) {
        struct IntArrayLookupResult lookup_result = {.result_code = TYPE_ERROR,
                                                     .elements_read = 0};
        return lookup_result;
    }

    int index = 0;
    for (; index < count; index++) {
        if (list_node == NULL)
            break;
        struct Node *elem = list_node->value;
        if (elem->type != SCALAR) {
            struct IntArrayLookupResult lookup_result = {
                .result_code = TYPE_ERROR, .elements_read = 0};
            return lookup_result;
        }
        char *end;
        long value = strtol(elem->payload, &end, 10);
        if (*end != '\0') { // Illegal character in the string
            struct IntArrayLookupResult lookup_result = {
                .result_code = FORMAT_ERROR, .elements_read = 0};
            return lookup_result;
        }
        if (value > INT_MAX || value < INT_MIN) { // overflow
            struct IntArrayLookupResult lookup_result = {
                .result_code = FORMAT_ERROR, .elements_read = 0};
            return lookup_result;
        }
        (*buffer)[index] = value;
        list_node = list_node->next;
    }

    struct IntArrayLookupResult lookup_result = {.result_code = OK,
                                                 .elements_read = index};
    return lookup_result;
}

void test_next_token(char *buffer) {
    printf("buffer:\n%s\n---\n", buffer);

    struct TokenizerState state = {.ptr = buffer};

    for (;;) {
        struct Token token = next_token(&state);
        printf("token type: %d\n", token.type);

        if (token.type == END || token.type == UNKNOWN)
            break;

        printf("token text: >%s<\n", token.text);
        free(token.text);
    }
}

void dump_node(struct Node *);

void dump_tree(tree_node_t *tree) {
    if (tree == NULL) {
        return;
    }
    dump_tree(tree->left_child);

    printf("%s: ", (char *)tree->key_data);
    struct Node *node = tree->value_data;

    dump_node(node);

    dump_tree(tree->right_child);
}

void dump_list(struct ListNode *list_node) {
    if (list_node == NULL) {
        return;
    }
    dump_node(list_node->value);
    if (list_node->next) {
        printf(",");
        dump_list(list_node->next);
    }
}

void dump_node(struct Node *node) {
    if (node->type == DICT) {
        tree_node_t *tree = node->payload;
        printf("{\n");
        dump_tree(tree);
        printf("}\n");
    } else if (node->type == SCALAR) {
        printf("%s\n", (char *)node->payload);
    } else if (node->type == LIST) {
        struct ListNode *list = node->payload;
        printf("[\n");
        dump_list(list);
        printf("]\n");
    } else {
        printf("Unknown node type: %d\n", node->type);
    }
}

void free_yaml_node(struct Node *);

void free_list_node(struct ListNode *list_node) {
    if (list_node == NULL)
        return;
    free_yaml_node(list_node->value);
    struct ListNode *next = list_node->next;
    free(list_node);
    free_list_node(next);
}

void free_tree_node(tree_node_t *tree) {
    if (tree == NULL) {
        return;
    }
    free_tree_node(tree->left_child);
    free_tree_node(tree->right_child);

    free_yaml_node(tree->value_data);
    free(tree->key_data);

    free(tree);
}

void free_yaml_node(struct Node *node) {
    if (node->type == DICT) {
        tree_node_t *tree = node->payload;
        free_tree_node(tree);
    } else if (node->type == SCALAR) {
        free(node->payload);
    } else if (node->type == LIST) {
        struct ListNode *list = node->payload;
        free_list_node(list);
    } else {
        printf("Unknown node type: %d\n", node->type);
    }
    free(node);
}
