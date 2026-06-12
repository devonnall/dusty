#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include "dscanner.h"

static const char *token_types[] = {
    "eof", "int", "float", "double", "char", "string", "if", "else", "function", "for", "while",
    "return", "break", "continue", "l_paren", "r_paren", "l_brace", "r_brace", "not", "not_equal", 
    "less", "less_equal", "less_less", "greater", "greater_equal", "greater_greater", "comma", 
    "mul", "mul_equal", "div", "div_equal", "plus", "plus_equal", "plus_plus", "minus", "minus_equal", 
    "minus_minus", "mod", "mod_equal", "and", "and_equal", "and_and", "or", "or_equal", "or_or", 
    "colon", "colon_colon", "semi", "equal", "equal_equal", 
    "l_bracket", "r_bracket", "caret", "caret_equal", "numeric_constant", "string_constant", "identifier"
};

static const char *keywords[] = {
    "int", "float", "double", "char", "string", "if", "else", "fn", "for", "while", "return", "break", "continue
};

static void
token_list_init(struct TokenList *list) {
    list->capacity = 64;
    list->count = 0;
    list->tokens = malloc(list->capacity * sizeof(struct Token));
    
    if (!list->tokens) {
        fprintf(stderr, "Fatal: Initial malloc failed for TokenList\n");
        exit(1);
    }
}

static void 
token_list_grow(struct TokenList *list) {
    int new_capacity = list->capacity == 0 ? 64 : list->capacity * 2;
    
    struct Token *tmp = realloc(list->tokens, new_capacity * sizeof(struct Token));
    if (!tmp) {
        fprintf(stderr, "Fatal: realloc failed for TokenList\n");
        exit(1);
    }
    
    list->tokens = tmp;
    list->capacity = new_capacity;
}

static void 
token_list_push(struct TokenList *list, struct Token token) {
    if (list->count >= list->capacity) {
        token_list_grow(list);
    }
    
    list->tokens[list->count] = token;
    list->count++;
}

static bool
iswhitespace(char ch) {
    if (ch == ' ' || ch == '\t' || ch == 'n' || ch == '\0')
        return true;

    return false;
}

static bool
iswordchar(char ch) {
    if (isalnum(ch) || ch == '_')
        return true;

    return false;
}

static bool
match(const char *str, const char *to_match, int n) {
    char after = str[n];
    if (strncmp(str, to_match, n) == 0 && iswhitespace(after))
        return true;

    return false;
}

static char
peek(struct Scanner *scanner) {
    return scanner->source[scanner->cursor + 1];
}

static void
advance(struct Scanner *scanner, int n) {
    for (int i = 0; i < n; i++) {
        char ch = scanner->source[scanner->cursor++];
        if (ch == '\n') {
            scanner->line++;
            scanner->column = 1;
        } else if (ch == '\0') {
            break;
        } else {
            scanner->column++;
        }
    }
}

static void
add_token(const char *str, struct Scanner *scanner, struct TokenList *tokens, enum TokenType type) {
    int n = strlen(str);
    struct Token token = {
        .type = type,
        .start = scanner->cursor,
        .length = n,
        .line = scanner->line;
        .col = scanner->column,
        .lexeme = strdup(str)
    };

    token_list_push(tokens, token);
    advance(scanner, n);
}

static void
add_custom(struct Scanner *scanner, struct TokenList *tokens, enum TokenType type) {
    int len = 0;
    const char *ch = &scanner->source[scanner->cursor];

    while (iswordchar(*ch)) {
        len++;
        ch++;
    }

    struct Token token = {
        .type = type,
        .start = scanner->cursor,
        .length = len,
        .line = scanner->line;
        .col = scanner->column,
        .lexeme = strndup(&scanner->source[scanner->cursor], len)
    };

    token_list_push(tokens, token);

    advance(scanner, len);
}

static void
add_alphanum(struct Scanner *scanner, struct TokenList *tokens) {
    const char *str = &scanner->source[scanner->cursor];

    const int n = sizeof(keywords) / sizeof(keywords[0]);
    if (isdigit(*str)) {
        add_custom(scanner, tokens, (enum TokenType)TOKEN_NUMERIC_CONSTANT);
        return;
    }

    for (int i = 0; i < n; i++) {
        if (match(str, keywords[i], strlen(keywords[i]))) {
            const char *keyword = strndup(keywords[i], strlen(keywords[i]));
            add_token(keyword, scanner, tokens, (enum TokenType)(i + 1));
            return;
        }
    }

    add_custom(scanner, tokens, (enum TokenType)TOKEN_IDENT);
}

static void
add_op(const char *op, struct Scanner *scanner, struct TokenList *tokens, enum TokenType op_t, char *follows) {
    char tmp_op[3];
    tmp_op[0] = *op;
    tmp_op[2] = '\0';

    int n = strlen(follows);

    for (int i = 0; i < n; i++) {
        if (peek(scanner) == follows[i]) {
            tmp_op[1] = follows[i];
            add_token(tmp_op, scanner, tokens, op_t + i + 1);
            return;
        }
    }

    tmp_op[1] = '\0';
    add_token(tmp_op, scanner, tokens, op_t); 
}

static void
skip_comment(struct Scanner *scanner) {
    const char *ch = &scanner->source[scanner->cursor];
    while (*ch != '\n' && *ch != '\0') {
        advance(scanner, 1);
        ch++;
    }
}

static void
add_string_constant(struct Scanner *scanner, struct TokenList *tokens) {
    int len = 1;
    const char *ch = &scanner->source[scanner->cursor+1];

    while (*ch != '"' && *ch != '\n' && *ch != '\0') {
        len++;
        ch++;
    }

    if (*ch != '"') {
        printf("error: unterminated string\n");
        exit(1);
    }

    len++;

    struct Token token = {
        .type = TOKEN_STRING_CONSTANT,
        .start = scanner->cursor,
        .length = len,
        .start_col = scanner->column,
        .end_col = scanner->column + len - 1,
        .lexeme = strndup(&scanner->source[scanner->cursor], len)
    };

    token_list_push(tokens, token);

    advance(scanner, len);
}

static void
get_token(struct Scanner *scanner, struct TokenList *tokens) {
    char ch = scanner->source[scanner->cursor];

    switch(ch) {
        case ' ':
        case '\t':
        case '\r':
            advance(scanner, 1);
            break;
        case '\n': advance(scanner, 1); break;
        case '*': add_op("*", scanner, tokens, (enum TokenType)TOKEN_MUL, "*"); break;
        case '/':
            if (peek(scanner) == '/')
                skip_comment(scanner);
            else
                add_op("/", scanner, tokens, (enum TokenType)TOKEN_DIV, "="); break;
            break;
        case '+': add_op("+", scanner, tokens, (enum TokenType)TOKEN_PLUS, "=+"); break;
        case '-': add_op("-", scanner, tokens, (enum TokenType)TOKEN_MINUS, "=-"); break;
        case '%': add_op("%", scanner, tokens, (enum TokenType)TOKEN_MOD, "="); break;
        case '<': add_op("<", scanner, tokens, (enum TokenType)TOKEN_LESS, "=>"); break;
        case '>': add_op(">", scanner, tokens, (enum TokenType)TOKEN_GREATER, "=<"); break;
        case '&': add_op("&", scanner, tokens, (enum TokenType)TOKEN_AND, "=&"); break;
        case '|': add_op("|", scanner, tokens, (enum TokenType)TOKEN_OR, "=|"); break;
        case '^': add_op("^", scanner, tokens, (enum TokenType)TOKEN_CARET, "="); break;
        case ':': add_op(":", scanner, tokens, (enum TokenType)TOKEN_COLON, ":"); break;
        case '=': add_op("=", scanner, tokens, (enum TokenType)TOKEN_EQUAL, "="); break;
        case '!': add_op("!", scanner, tokens, (enum TokenType)TOKEN_NOT, "="); break;
        case '{': add_token("{", scanner, tokens, (enum TokenType)TOKEN_LBRACE); break;
        case '}': add_token("}", scanner, tokens, (enum TokenType)TOKEN_RBRACE); break;
        case '(': add_token("(", scanner, tokens, (enum TokenType)TOKEN_LPAREN); break;
        case ')': add_token(")", scanner, tokens, (enum TokenType)TOKEN_RPAREN); break;
        case ',': add_token(",", scanner, tokens, (enum TokenType)TOKEN_COMMA); break;
        case ';': add_token(";", scanner, tokens, (enum TokenType)TOKEN_SEMICOLON); break;
        case '[': add_token("[", scanner, tokens, (enum TokenType)TOKEN_LBRACKET); break;
        case ']': add_token("]", scanner, tokens, (enum TokenType)TOKEN_RBRACKET); break;
        case '\0': add_token("", scanner, tokens, (enum TokenType)TOKEN_EOF); break;
        case '"': add_string_constant(scanner, tokens); break;
        default: 
            if (iswordchar(ch)) {
                add_alphanum(scanner, tokens);
            } else {
                printf("error: illegal character: %c\n", ch);
                exit(1);
            }
            break;
    }
}

static void
print_tokens(struct TokenList *tokens) {
    for (int i = 0; i < tokens->count; i++) {
        printf("%s '%s'\n", token_types[tokens->tokens[i].type], tokens->tokens[i].lexeme);
    }
}

int
tokenize(const char *source, struct TokenList *tokens) {
    struct Scanner scanner = {
        .source = source,
        .cursor = 0,
        .line = 1,
        .column = 1
    };

    token_list_init(tokens);

    for (;;) {
        get_token(&scanner, tokens);
        if (tokens->count > 0 && tokens->tokens[tokens->count - 1].type == TOKEN_EOF)
            break;
    }

    print_tokens(tokens);

    return 0;
}
