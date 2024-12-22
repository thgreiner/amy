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
struct IntArrayLookupResult get_as_int_array(struct Node *, char *, int (*)[],
                                             int);

#endif
