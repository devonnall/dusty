#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "scanner.h"

static struct scanner dyscanner;
static struct dytext dytext = {0};
static bool errors = false;

static void
init_scanner(const char *source) {
        dyscanner.source = source;
        dyscanner.cursor = 0;
        dyscanner.line = 1;
        dyscanner.column = 1;
}

static void
dy_print_error(const char *msg) {
        if (!errors) {
                errors = true;
        }

        printf("%s:%d:%d: error: %s\n",
                dyfile,
                dyscanner.line,
                dyscanner.column,
                msg);
}

static int
iswhitespace(char ch) {
        if (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\0') 
                return 1;
        
        return 0;
}

static int
iswordchar(char ch) {
        if (isalnum(ch) || ch == '_') {
                return 1;
        }
        return 0;
}

static int 
match(const char *s1, const char *s2, size_t n) {
        char after = *(s2 + n);
        if (strncmp(s1, s2, n) == 0 && iswhitespace(after))
                return 1;

        return 0;
}

static char 
peek() {
        return dyscanner.source[dyscanner.cursor+1];
}

static void 
advance(size_t n) {
        for (size_t i = 0; i < n; i++) {
                dyscanner.cursor++;
                if (dyscanner.source[dyscanner.cursor] == '\n') {
                        dyscanner.line++;
                        dyscanner.column = 1;
                } else if (dyscanner.source[dyscanner.cursor] == '\0') {
                        break;
                } else {
                        dyscanner.column++;
                }
        }
}

static int 
get_identifier_length() {
        int start_pos = dyscanner.cursor;
        const char *start = &dyscanner.source[start_pos];
        while (iswordchar(*start++)) {
                advance(1);
        }
        return dyscanner.cursor - start_pos;
}

static void 
recover() {
        const char *ch = &dyscanner.source[dyscanner.cursor];
        while (*ch != ';' && *ch != '\n' && *ch != ' ' 
                && *ch != '\t' && *ch != ':' && *ch != '\0') {
                advance(1);
                ch = &dyscanner.source[dyscanner.cursor];
        }
}

static int 
get_number_length(bool *syntax_error) {
        int start_pos = dyscanner.cursor;

        while (isdigit(dyscanner.source[dyscanner.cursor])) {
                advance(1);
        }

        if (isalpha(dyscanner.source[dyscanner.cursor])) {
                recover();
                *syntax_error = true;
                dy_print_error("invalid number");
        }

        return dyscanner.cursor - start_pos;
}

static void 
set_dytext(const char *literal, size_t n) {
        dytext.text = realloc(dytext.text, n+1);
        dytext.length = n;
        strlcpy(dytext.text, literal, n+1);
}

static void 
handle_keyword(const char *keyword, size_t n) {
        set_dytext(keyword, n);
        advance(n);
}

static void 
handle_identifier(const char *start) {
        int len = get_identifier_length();
        set_dytext(start, len);
}

static void 
handle_number(const char *start, enum token_type *type, bool *serror) {
        int len = get_number_length(serror);
        dytext.text = realloc(dytext.text, len+1);
        dytext.length = len;
        strncpy(dytext.text, start, len);

        if (*serror)
                *type = TOKEN_ERROR;
        else
                *type = TOKEN_NUMBER;
}

static int 
isstring(int *len) {
        advance(1);
        const char *current = &dyscanner.source[dyscanner.cursor];

        while (*current != '\n' && *current != '\0') {
                if (*current == '"' && *(current - 1) != '\\') {
                        (*len)++;
                        advance(1);
                        return 1;
                }

                current++;
                advance(1);
                (*len)++;
        }

        return 0;
}

static void 
handle_string(const char *start, enum token_type *type, size_t len) {
        set_dytext(start, len);
        *type = TOKEN_STRING_LITERAL;
}

static void 
skip_comment() {
        const char *start = &dyscanner.source[dyscanner.cursor];

        while (*start != '\n' && *start != '\0') {
                start++;
                advance(1);
        }
}

static enum token_type 
get_token() {
        const char *start = &dyscanner.source[dyscanner.cursor];
        enum token_type type;
        int len = 1;
        bool syntax_error = false;

        memset(dytext.text, 0, dytext.length);

        switch(*start) {
        case ' ': case '\t': case '\r': case '\n':
                advance(1); 
                type = TOKEN_SKIP;
                break;
        case 'i':
                if (match(start, "int", 3)) {
                        handle_keyword("int", 3);
                        type = TOKEN_INT;
                        break;
                } else if (match(start, "if", 2)) {
                        handle_keyword("if", 2);
                        type = TOKEN_IF;
                        break;
                }
                goto identifier;
        case 'e':
                if (match(start, "else", 4)) {
                        handle_keyword("else", 4);
                        type = TOKEN_ELSE;
                        break;
                }
                goto identifier;
        case 's':
                if (match(start, "string", 6)) {
                        handle_keyword("string", 6);
                        type = TOKEN_STRING;
                        break;
                }
                goto identifier;
        case 'f':
                if (match(start, "fn", 2)) {
                        handle_keyword("fn", 2);
                        type = TOKEN_FUNCTION;
                        break;
                } else if (match(start, "for", 3)) {
                        handle_keyword("for", 3);
                        type = TOKEN_FOR;
                        break;
                }
                goto identifier;
        case 'r':
                if (match(start, "return", 6)) {
                        handle_keyword("return", 6);
                        type = TOKEN_RETURN;
                        break;
                }
        case 'w':
                if (match(start, "while", 5)) {
                        handle_keyword("while", 5);
                        type = TOKEN_WHILE;
                        break;
                }
                goto identifier;
        case '*':
                if (peek() == '=') {
                        set_dytext("*=", 2);
                        advance(2);
                        type = TOKEN_DIV_EQUAL;
                        break;
                } else {
                        set_dytext("*", 1);
                        advance(1);
                        type = TOKEN_DIV;
                        break;
                }
        case '/':
                if (peek() == '/') {
                        skip_comment();
                        type = TOKEN_SKIP;
                        break;
                } else if (peek() == '=') {
                        set_dytext("/=", 2);
                        advance(2);
                        type = TOKEN_DIV_EQUAL;
                        break;
                } else {
                        set_dytext("/", 1);
                        advance(1);
                        type = TOKEN_DIV;
                        break;
                }
        case '+':
                if (peek() == '+') {
                        set_dytext("++", 2);
                        advance(2);
                        type = TOKEN_PLUS_PLUS;
                        break;
                } else if (peek() == '=') {
                        set_dytext("+=", 2);
                        advance(2);
                        type = TOKEN_PLUS_EQUAL;
                        break;
                } else {
                        set_dytext("+", 1);
                        advance(1);
                        type = TOKEN_PLUS;
                        break;
                }
        case '-':
                if (peek() == '-') {
                        set_dytext("--", 2);
                        advance(2);
                        type = TOKEN_MINUS_MINUS;
                        break;
                } else if (peek() == '=') {
                        set_dytext("-=", 2);
                        advance(2);
                        type = TOKEN_MINUS_EQUAL;
                        break;
                } else {
                        set_dytext("-", 1);
                        advance(1);
                        type = TOKEN_MINUS;
                        break;
                }
        case '%':
                if (peek() == '=') {
                        set_dytext("%=", 2);
                        advance(2);
                        type = TOKEN_MOD_EQUAL;
                        break;
                } else {
                        set_dytext("%", 1);
                        advance(1);
                        type = TOKEN_MOD;
                        break;
                }
        case '(':
                set_dytext("(", 1);
                advance(1);
                type = TOKEN_LPAREN;
                break;
        case ')':
                set_dytext(")", 1);
                advance(1);
                type = TOKEN_RPAREN;
                break;
        case '{':
                set_dytext("{", 1);
                advance(1);
                type = TOKEN_LBRACE;
                break;
        case '}':
                set_dytext("}", 1);
                advance(1);
                type = TOKEN_RBRACE;
                break;
        case '<':
                if (peek() == '=') {
                        set_dytext("<=", 2);
                        advance(2);
                        type = TOKEN_LESS_EQUAL;
                        break;
                } else {
                        set_dytext("<", 1);
                        advance(1);
                        type = TOKEN_LESS;
                        break;
                }
        case '>':
                if (peek() == '=') {
                        set_dytext(">=", 2);
                        advance(2);
                        type = TOKEN_GREATER_EQUAL;
                        break;
                } else {
                        set_dytext(">", 1);
                        advance(1);
                        type = TOKEN_GREATER;
                        break;
                }
        case ',':
                set_dytext(",", 1);
                advance(1);
                type = TOKEN_COMMA;
                break;
        case '"':
                if (isstring(&len)) {
                        handle_string(start, &type, len);
                        break;
                } else {
                        dy_print_error("unterminated string");
                        type = TOKEN_ERROR;
                }
        case ':':
                set_dytext(":", 1);
                advance(1);
                type = TOKEN_COLON;
                break;
        case ';':
                set_dytext(";", 1);
                advance(1);
                type = TOKEN_SEMICOLON;
                break;
        case  '=':
                set_dytext("=", 1);
                advance(1);
                type = TOKEN_EQUAL;
                break;
        case '\0':
                type = TOKEN_EOF;
                break;
        default:
                if (isalpha(*start)) {
                        goto identifier;
                } else if (isdigit(*start)) {
                        handle_number(start, &type, &syntax_error);
                } else {
                        dy_print_error("invalid character");
                        type = TOKEN_ERROR;
                }
                break;
        }

        goto done;

identifier:
        handle_identifier(start);
        type = TOKEN_IDENT;

done:
        return type;
}

int
tokenize(const char *source) {
        init_scanner(source);

        for (;;) {
                enum token_type type = get_token();
                if (type == TOKEN_EOF) break;
                if (type == TOKEN_SKIP) continue;
                printf("%u: %s\n", type, dytext.text);
        }
        
        if (errors)
                return -1;

        return 0;
        
}
