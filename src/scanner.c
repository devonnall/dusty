#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "scanner.h"

void dy_print_error(struct scanner *dyscanner, const char *msg) {
        if (!errors) {
                errors = true;
        }

        printf("%s:%d:%d: error: %s\n",
                dyfile,
                dyscanner->line,
                dyscanner->column,
                msg);
}

int iswhitespace(char ch) {
        if (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\0') 
                return 1;
        
        return 0;
}

int iswordchar(char ch) {
        if (isalnum(ch) || ch == '_') {
                return 1;
        }
        return 0;
}

int match(const char *s1, const char *s2, size_t n) {
        char after = *(s2 + n);
        if (strncmp(s1, s2, n) == 0 && iswhitespace(after))
                return 1;

        return 0;
}

void advance(struct scanner *dyscanner, size_t n) {
        for (size_t i = 0; i < n; i++) {
                dyscanner->cursor++;
                if (dyscanner->source[dyscanner->cursor] == '\n') {
                        dyscanner->line++;
                        dyscanner->column = 1;
                } else if (dyscanner->source[dyscanner->cursor] == '\0') {
                        break;
                } else {
                        dyscanner->column++;
                }
        }
}

int get_identifier_length(struct scanner *dyscanner) {
        int start_pos = dyscanner->cursor;
        const char *start = &dyscanner->source[start_pos];
        while (iswordchar(*start++)) {
                advance(dyscanner, 1);
        }
        return dyscanner->cursor - start_pos;
}

void recover(struct scanner *dyscanner) {
        const char *ch = &dyscanner->source[dyscanner->cursor];
        while (*ch != ';' && *ch != '\n' && *ch != ' ' 
                && *ch != '\t' && *ch != ':' && *ch != '\0') {
                advance(dyscanner, 1);
                ch = &dyscanner->source[dyscanner->cursor];
        }
}

int get_number_length(struct scanner *dyscanner, bool *syntax_error) {
        int start_pos = dyscanner->cursor;

        while (isdigit(dyscanner->source[dyscanner->cursor])) {
                advance(dyscanner, 1);
        }

        if (isalpha(dyscanner->source[dyscanner->cursor])) {
                recover(dyscanner);
                *syntax_error = true;
                dy_print_error(dyscanner, "invalid number");
        }

        return dyscanner->cursor - start_pos;
}

void set_dytext(struct dytext *text, const char *literal, size_t n) {
        text->text = realloc(text->text, n+1);
        text->length = n;
        strlcpy(text->text, literal, n+1);
}

void handle_keyword(struct scanner *dyscanner, struct dytext *text,
                    const char *keyword, size_t n) {
        set_dytext(text, keyword, n);
        advance(dyscanner, n);
}

void handle_identifier(struct scanner *dyscanner, struct dytext *text,
                       const char *start) {
        int len = get_identifier_length(dyscanner);
        set_dytext(text, start, len);
}

void handle_number(struct scanner *dyscanner, struct dytext *text,
                   const char *start, enum token_type *type, bool *serror) {
        int len = get_number_length(dyscanner, serror);
        text->text = realloc(text->text, len+1);
        text->length = len;
        strncpy(text->text, start, len);

        if (*serror)
                *type = TOKEN_ERROR;
        else
                *type = TOKEN_NUMBER;
}

int isstring(struct scanner *dyscanner, int *len) {
        advance(dyscanner, 1);
        const char *current = &dyscanner->source[dyscanner->cursor];
        *len = 1;

        while (*current != '\n' && *current != '\0') {
                if (*current == '"' && *(current - 1) != '\\') {
                        (*len)++;
                        advance(dyscanner, 1);
                        return 1;
                }

                current++;
                advance(dyscanner, 1);
                (*len)++;
        }

        return 0;
}

void handle_string(struct dytext *text, const char *start, 
                  enum token_type *type, size_t len) {
        set_dytext(text, start, len);
        *type = TOKEN_STRING_LITERAL;
}

enum token_type get_token(struct scanner *dyscanner, struct dytext *text) {
        const char *start = &dyscanner->source[dyscanner->cursor];
        enum token_type type;
        int len = 0;
        bool syntax_error = false;

        memset(text->text, 0, text->length);

        switch(*start) {
        case ' ': case '\t': case '\r': case '\n':
                advance(dyscanner, 1); 
                type = TOKEN_SKIP;
                break;
        case 'i':
                if (match(start, "int", 3)) {
                        handle_keyword(dyscanner, text, "int", 3);
                        type = TOKEN_INT;
                        break;
                }
                goto identifier;
        case 's':
                if (match(start, "string", 6)) {
                        handle_keyword(dyscanner, text, "string", 6);
                        type = TOKEN_STRING;
                        break;
                }
                goto identifier;
        case '"':
                if (isstring(dyscanner, &len)) {
                        handle_string(text, start, &type, len);
                        break;
                } else {
                        dy_print_error(dyscanner, "unterminated string");
                        type = TOKEN_ERROR;
                }
        case ':':
                set_dytext(text, ":", 1);
                advance(dyscanner, 1);
                type = TOKEN_COLON;
                break;
        case ';':
                set_dytext(text, ";", 1);
                advance(dyscanner, 1);
                type = TOKEN_SEMICOLON;
                break;
        case  '=':
                set_dytext(text, "=", 1);
                advance(dyscanner, 1);
                type = TOKEN_EQUAL;
                break;
        case '\0':
                type = TOKEN_EOF;
                break;
        default:
                if (isalpha(*start)) {
                        goto identifier;
                } else if (isdigit(*start)) {
                        handle_number(dyscanner, text, start,
                                      &type, &syntax_error);
                } else {
                        dy_print_error(dyscanner, "invalid character");
                        type = TOKEN_ERROR;
                }
                break;
        }

        goto done;

identifier:
        handle_identifier(dyscanner, text, start);
        type = TOKEN_IDENT;

done:
        return type;
}
