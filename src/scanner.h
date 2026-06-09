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

enum token_type {
        TOKEN_EOF = 0,
        TOKEN_INT,
        TOKEN_STRING,
        TOKEN_FUNCTION,
        TOKEN_FOR,
        TOKEN_WHILE,
        TOKEN_IF,
        TOKEN_ELSE,
        TOKEN_RETURN,
        TOKEN_NUMBER,
        TOKEN_LPAREN,
        TOKEN_RPAREN,
        TOKEN_LBRACE,
        TOKEN_RBRACE,
        TOKEN_LESS_EQUAL,
        TOKEN_GREATER_EQUAL,
        TOKEN_LESS,
        TOKEN_GREATER,
        TOKEN_COMMA,
        TOKEN_MUL_EQUAL,
        TOKEN_MUL,
        TOKEN_DIV_EQUAL,
        TOKEN_DIV,
        TOKEN_PLUS_EQUAL,
        TOKEN_PLUS_PLUS,
        TOKEN_MINUS_EQUAL,
        TOKEN_MINUS_MINUS,
        TOKEN_PLUS,
        TOKEN_MINUS,
        TOKEN_MOD,
        TOKEN_MOD_EQUAL,
        TOKEN_COLON,
        TOKEN_SEMICOLON,
        TOKEN_EQUAL,
        TOKEN_STRING_LITERAL,
        TOKEN_IDENT,
        TOKEN_SKIP,
        TOKEN_ERROR
};

struct token {
        enum token_type type;
        int start;
        int length;
        const char *semantic_value;
};

struct token make_token(enum token_type type, char *dytext);
void tokens_append(struct token dytoken);

int tokenize(const char *source);

extern const char *dyfile;

#endif // DUSTY_SCANNER_H
