#ifndef DUSTY_SCANNER_H
#define DUSTY_SCANNER_H

#include <stdbool.h>

struct dytext {
        char *text;
        int length;
};

struct scanner {
        const char *source;
        int cursor;
        int line;
        int column;
};

enum token_t {
        TOKEN_EOF = 0,
        TOKEN_INT,
        TOKEN_FLOAT,
        TOKEN_DOUBLE,
        TOKEN_CHAR,
        TOKEN_STRING,
        TOKEN_IF,
        TOKEN_ELSE,
        TOKEN_FUNCTION,
        TOKEN_FOR,
        TOKEN_WHILE,
        TOKEN_RETURN,
        TOKEN_NONE,
        TOKEN_NUMBER,
        TOKEN_LPAREN,
        TOKEN_RPAREN,
        TOKEN_LBRACE,
        TOKEN_RBRACE,
        TOKEN_LESS,
        TOKEN_LESS_EQUAL,
        TOKEN_GREATER_EQUAL,
        TOKEN_GREATER,
        TOKEN_COMMA,
        TOKEN_MUL,
        TOKEN_MUL_EQUAL,
        TOKEN_DIV,
        TOKEN_DIV_EQUAL,
        TOKEN_PLUS,
        TOKEN_PLUS_EQUAL,
        TOKEN_PLUS_PLUS,
        TOKEN_MINUS,
        TOKEN_MINUS_EQUAL,
        TOKEN_MINUS_MINUS,
        TOKEN_MOD,
        TOKEN_MOD_EQUAL,
        TOKEN_COLON,
        TOKEN_ACCESS,
        TOKEN_SEMICOLON,
        TOKEN_EQUAL,
        TOKEN_EQUAL_EQUAL,
        TOKEN_STRING_LITERAL,
        TOKEN_IDENT,
        TOKEN_SKIP,
        TOKEN_ERROR
};

struct token {
        enum token_t type;
        int start;
        int length;
        char *semantic_value;
};

struct token_list {
    struct token *tokens;
    size_t length;
    size_t capacity;
};

int tokenize(const char *source, struct token_list *tokens);
extern const char *dyfile;
void token_list_print(struct token_list *tokens);

#endif // DUSTY_SCANNER_H
