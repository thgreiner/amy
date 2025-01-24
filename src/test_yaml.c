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

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "tree.h"
#include "yaml.h"

static void test_simple_dict(void) {
    char *payload = "key: value\n";

    struct Node *result = parse_yaml(payload);

    assert(result != NULL);

    assert(result->type == DICT);

    tree_node_t *tree = result->payload;

    size_t value_len;
    struct Node *value = lookup_value(tree, "key", 4, &value_len);

    assert(value != NULL);
    assert(value_len == sizeof(struct Node));

    assert(value->type == SCALAR);

    assert(!strcmp("value", value->payload));

    free(value);

    free_yaml_node(result);
}

static void test_nested_dict(void) {
    char *payload = "level1:\n"
                    "  level2: value";

    struct Node *result = parse_yaml(payload);

    assert(result != NULL);
    assert(result->type == DICT);

    tree_node_t *tree = result->payload;

    size_t value_len;
    struct Node *value = lookup_value(tree, "level1", 7, &value_len);

    assert(value != NULL);
    assert(value_len == sizeof(struct Node));

    assert(value->type == DICT);

    tree_node_t *tree2 = value->payload;

    size_t value2_len;
    struct Node *value2 = lookup_value(tree2, "level2", 7, &value2_len);

    assert(value2 != NULL);

    assert(value2->type == SCALAR);
    assert(!strcmp("value", value2->payload));

    free(value);
    free(value2);
    free_yaml_node(result);
}

static void test_multi_line_array(void) {
    char *payload = "array:\n"
                    "- a\n"
                    "- b\n";

    struct Node *result = parse_yaml(payload);

    assert(result != NULL);
    assert(result->type == DICT);

    tree_node_t *tree = result->payload;

    size_t value_len;
    struct Node *value = lookup_value(tree, "array", 6, &value_len);

    assert(value != NULL);
    assert(value_len == sizeof(struct Node));

    assert(value->type == LIST);
}

static void test_nested_multi_line_array(void) {
    char *payload = "level1:\n"
                    "  array:\n"
                    "  - a\n"
                    "  - b\n"
                    "  scalar: x\n";

    struct Node *result = parse_yaml(payload);

    assert(result != NULL);
    assert(result->type == DICT);

    tree_node_t *tree = result->payload;

    size_t value_len;
    struct Node *value = lookup_value(tree, "level1", 7, &value_len);

    assert(value != NULL);
    assert(value_len == sizeof(struct Node));

    assert(value->type == DICT);

    tree_node_t *nested = value->payload;

    value = lookup_value(nested, "array", 6, &value_len);

    assert(value != NULL);
    assert(value_len == sizeof(struct Node));

    assert(value->type == LIST);

    value = lookup_value(nested, "scalar", 7, &value_len);

    assert(value != NULL);
    assert(value_len == sizeof(struct Node));

    assert(value->type == SCALAR);
}

static void test_nested_multi_line_array_2(void) {
    char *payload = "level1:\n"
                    "  array:\n"
                    "  - a\n"
                    "  - b\n"
                    "scalar: x\n";

    struct Node *result = parse_yaml(payload);

    assert(result != NULL);
    assert(result->type == DICT);

    tree_node_t *tree = result->payload;

    size_t value_len;
    struct Node *value = lookup_value(tree, "level1", 7, &value_len);

    assert(value != NULL);
    assert(value_len == sizeof(struct Node));

    assert(value->type == DICT);

    tree_node_t *nested = value->payload;

    value = lookup_value(nested, "array", 6, &value_len);

    assert(value != NULL);
    assert(value_len == sizeof(struct Node));

    assert(value->type == LIST);

    value = lookup_value(nested, "scalar", 7, &value_len);

    assert(value == NULL);
}

static void test_get_as_string(void) {
    char *payload = "key: value\n";

    struct Node *result = parse_yaml(payload);
    assert(result != NULL);

    struct StringLookupResult lookup_result = get_as_string(result, "key");
    assert(lookup_result.result_code == OK);
    assert(lookup_result.result != NULL);
    assert(!strcmp(lookup_result.result, "value"));

    free_yaml_node(result);
}

static void test_get_as_string_not_found(void) {
    char *payload = "key: value\n";

    struct Node *result = parse_yaml(payload);
    assert(result != NULL);

    struct StringLookupResult lookupResult = get_as_string(result, "other_key");
    assert(lookupResult.result_code == NOT_FOUND);

    free_yaml_node(result);
}

static void test_get_as_string_nested(void) {
    char *payload = "level1:\n"
                    "  level2: value";

    struct Node *result = parse_yaml(payload);
    assert(result != NULL);

    struct StringLookupResult lookup_result =
        get_as_string(result, "level1.level2");
    assert(lookup_result.result_code == OK);
    assert(lookup_result.result != NULL);
    assert(!strcmp(lookup_result.result, "value"));

    free_yaml_node(result);
}

static void test_get_as_int(void) {
    char *payload = "key: 500\n";

    struct Node *result = parse_yaml(payload);
    assert(result != NULL);

    struct IntLookupResult lookup_result = get_as_int(result, "key");
    assert(lookup_result.result_code == OK);
    assert(lookup_result.result == 500);

    free_yaml_node(result);
}

static void test_get_as_int_format_error(void) {
    char *payload = "key: x\n";

    struct Node *result = parse_yaml(payload);
    assert(result != NULL);

    struct IntLookupResult lookup_result = get_as_int(result, "key");
    assert(lookup_result.result_code == FORMAT_ERROR);

    free_yaml_node(result);
}

static void test_get_as_list(void) {
    char *payload = "key: [1, 0, 2]\n";

    struct Node *result = parse_yaml(payload);
    assert(result != NULL);

    struct ListLookupResult lookup_result = get_as_list(result, "key");
    assert(lookup_result.result_code == OK);

    struct ListNode *list = lookup_result.result;
    assert(list != NULL);

    struct ListNode *next = list->next;
    assert(next != NULL);

    next = next->next;
    assert(next != NULL);

    next = next->next;
    assert(next == NULL);

    free_yaml_node(result);
}

static void test_get_as_int_array(void) {
    char *payload = "key: [1, 0, 2]\n";

    struct Node *node = parse_yaml(payload);
    assert(node != NULL);

    int buf[3];

    struct IntArrayLookupResult lookup_result =
        get_as_int_array(node, "key", buf, 3);

    assert(lookup_result.result_code == OK);
    assert(lookup_result.elements_read == 3);
    assert(buf[0] == 1);
    assert(buf[1] == 0);
    assert(buf[2] == 2);

    free_yaml_node(node);
}

static void test_get_as_int_array_short(void) {
    char *payload = "key: [1, -1, 2]\n";

    struct Node *node = parse_yaml(payload);
    assert(node != NULL);

    int buf[2];

    struct IntArrayLookupResult lookup_result =
        get_as_int_array(node, "key", buf, 2);

    assert(lookup_result.result_code == OK);
    assert(lookup_result.elements_read == 2);
    assert(buf[0] == 1);
    assert(buf[1] == -1);

    free_yaml_node(node);
}

static void test_get_as_list_flow_style(void) {
    char *payload = "key: [1,\n"
                    "  0, 2,\n"
                    "  3]\n";

    struct Node *result = parse_yaml(payload);
    assert(result != NULL);

    struct ListLookupResult lookupResult = get_as_list(result, "key");
    assert(lookupResult.result_code == OK);

    struct ListNode *list = lookupResult.result;
    assert(list != NULL);

    struct ListNode *next = list->next;
    assert(next != NULL);

    next = next->next;
    assert(next != NULL);

    next = next->next;
    assert(next != NULL);

    next = next->next;
    assert(next == NULL);

    free_yaml_node(result);
}

static void test_get_as_int_array_illegal_input(void) {
    char *payload = "key: [1, a, 2]\n";

    struct Node *node = parse_yaml(payload);
    assert(node != NULL);

    int buf[3];

    struct IntArrayLookupResult lookup_result =
        get_as_int_array(node, "key", buf, 3);

    assert(lookup_result.result_code == FORMAT_ERROR);

    free_yaml_node(node);
}

static void test_malformed_input(void) {
    char *payload = "item1:\n  key: [1, a, 2\nitem2: scalar\n";
    struct Node *node = parse_yaml(payload);
    assert(node == NULL);
}

static void test_list_trailing_comma(void) {
    char *payload = "key: [1, 0,]\n";

    struct Node *result = parse_yaml(payload);
    assert(result != NULL);

    struct ListLookupResult lookup_result = get_as_list(result, "key");
    assert(lookup_result.result_code == OK);

    struct ListNode *list = lookup_result.result;
    assert(list != NULL);

    struct ListNode *next = list->next;
    assert(next != NULL);

    next = next->next;
    assert(next == NULL);

    free_yaml_node(result);
}

static void test_comments_and_empty_lines(void) {
    char *payload = "key1: value1 # comment\n"
                    "\n\n"
                    "# top level comment\n"
                    "key2: value2\n"
                    "nested:\n"
                    "  key3: value3\n"
                    "\n\n"
                    "  key4: value4\n";

    struct Node *result = parse_yaml(payload);
    assert(result != NULL);
    assert(result->type == DICT);

    struct StringLookupResult lookup_result = get_as_string(result, "key1");
    assert(lookup_result.result_code == OK);
    assert(lookup_result.result != NULL);
    assert(!strcmp(lookup_result.result, "value1"));

    lookup_result = get_as_string(result, "key2");
    assert(lookup_result.result_code == OK);
    assert(lookup_result.result != NULL);
    assert(!strcmp(lookup_result.result, "value2"));

    lookup_result = get_as_string(result, "nested.key3");
    assert(lookup_result.result_code == OK);
    assert(lookup_result.result != NULL);
    assert(!strcmp(lookup_result.result, "value3"));

    lookup_result = get_as_string(result, "nested.key4");
    assert(lookup_result.result_code == OK);
    assert(lookup_result.result != NULL);
    assert(!strcmp(lookup_result.result, "value4"));

    free_yaml_node(result);
}

void test_all_yaml(void) {
    test_simple_dict();
    test_nested_dict();
    test_multi_line_array();
    test_nested_multi_line_array();
    test_nested_multi_line_array_2();
    test_get_as_string();
    test_get_as_string_not_found();
    test_get_as_string_nested();
    test_get_as_int();
    test_get_as_int_format_error();
    test_get_as_list();
    test_get_as_list_flow_style();
    test_get_as_int_array();
    test_get_as_int_array_short();
    test_get_as_int_array_illegal_input();
    test_malformed_input();
    test_comments_and_empty_lines();
    test_list_trailing_comma();
}
