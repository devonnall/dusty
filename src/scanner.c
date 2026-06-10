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
handle_keyword(const char *keyword) {
    int len = strlen(keyword);
    set_dytext(keyword, len);
    advance(len);
}

static void
handle_identifier(const char *start, enum token_t *out_type) {
    const char *keywords[] = {
        "int", "float", "double", "char", "string", "if", 
        "else", "fn",  "for", "while", "return", "none"
    };

    size_t num_keywords = sizeof(keywords) / sizeof(keywords[0]);

    for (size_t i = 0; i < num_keywords; i++) {
        if (match(start, keywords[i], strlen(keywords[i]))) {
            handle_keyword(keywords[i]);
            *out_type = (enum token_t)(i+1);
            return;
        }
    }

    size_t start_pos = dyscanner.cursor;
    const char *ch = start;
    while (iswordchar(*ch)) {
        advance(1);
        ch++;
    }

    *out_type = (enum token_t)TOKEN_IDENT;

    size_t len = dyscanner.cursor - start_pos;
    set_dytext(start, len);
}

static void 
handle_number(const char *start, enum token_t *type, bool *serror) {
    int len = get_number_length(serror);
    dytext.text = realloc(dytext.text, len+1);
    dytext.length = len;
    strncpy(dytext.text, start, len);
    dytext.text[len] = '\0';

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
handle_string(const char *start, enum token_t *type) {
    int len = 1;
    if (isstring(&len)) {
        set_dytext(start, len);
        *type = TOKEN_STRING_LITERAL;
    } else {
        dy_print_error("unterminated string");
        recover();
        *type = TOKEN_ERROR;
    }
}

static void 
skip(enum token_t *type) {
    advance(1);
    *type = TOKEN_SKIP;
}

static void 
skip_comment() {
    const char *start = &dyscanner.source[dyscanner.cursor];

    while (*start != '\n' && *start != '\0') {
        start++;
        advance(1);
    }
}

static void
handle_token(const char *str, enum token_t *out_type, enum token_t type) {
    size_t n = strlen(str);
    set_dytext(str, n);
    advance(n);
    *out_type = type;
}

static void
handle_op(const char *op, enum token_t *type, 
        enum token_t op_t, int repeat, int equals) {
    char tmp_op[3];
    tmp_op[0] = *op;
    tmp_op[2] = '\0';

    int repeat_offset = equals + repeat;

    if (repeat && peek() == *op) {
        tmp_op[1] = *op;
        handle_token(tmp_op, type, op_t + repeat_offset); 
    } else if (equals && peek() == '=') {
        tmp_op[1] = '=';
        handle_token(tmp_op, type, op_t + 1); 
    } else {
        tmp_op[1] = '\0';
        handle_token(tmp_op, type, op_t); 
    }
}


static void
handle_alphanum(const char *start, enum token_t *out_type, bool *serror) {
    if (isalpha(*start) || *start == '_') {
        handle_identifier(start, out_type);
    } else if (isdigit(*start)) {
        handle_number(start, out_type, serror);
    } else {
        dy_print_error("illegal character");
    }
}

static enum token_t
get_token(struct token *token) {
    int start_idx = dyscanner.cursor;
    const char *start = &dyscanner.source[start_idx];
    enum token_t type;
    bool syntax_error = false;
    
    memset(dytext.text, 0, dytext.length);

    switch (*start) {
    case ' ': case '\t': case '\r': case '\n': skip(&type); break;
    case '*': 
        handle_op("*", &type, (enum token_t)TOKEN_MUL, false, true); break;
    case '/': 
        if (peek() != '/') {
            handle_op("/", &type, (enum token_t)TOKEN_DIV, false, true);
        } else {
            skip_comment();
            type = TOKEN_SKIP;
        }
        break;
    case '+': handle_op("+", &type, (enum token_t)TOKEN_PLUS, 1, 1); break;
    case '-': handle_op("-", &type, (enum token_t)TOKEN_MINUS, 1, 1); break;
    case '=': handle_op("=", &type, (enum token_t)TOKEN_EQUAL, 1, 0); break;
    case '<': handle_op("<", &type, (enum token_t)TOKEN_LESS, 0, 1); break;
    case '>': handle_op(">", &type, (enum token_t)TOKEN_GREATER, 0, 1); break;
    case '%': handle_op("%", &type, (enum token_t)TOKEN_MOD, 0, 1); break;
    case ':': handle_op(":", &type, (enum token_t)TOKEN_COLON, 1, 0); break;
    case '(': handle_token("(", &type, (enum token_t)TOKEN_LPAREN); break;
    case ')': handle_token(")", &type, (enum token_t)TOKEN_RPAREN); break;
    case '{': handle_token("{", &type, (enum token_t)TOKEN_LBRACE); break;
    case '}': handle_token("}", &type, (enum token_t)TOKEN_RBRACE); break;
    case ',': handle_token(",", &type, (enum token_t)TOKEN_COMMA); break;
    case ';': handle_token(";", &type, (enum token_t)TOKEN_SEMICOLON); break;
    case '"': handle_string(start, &type); break;
    case '\0': type = TOKEN_EOF; break;
    default: handle_alphanum(start, &type, &syntax_error); break;
    }

    if (type != TOKEN_SKIP && type != TOKEN_EOF) {
        token->type = type;
        token->start = start_idx;
        token->length = strlen(dytext.text);
        token->semantic_value = strdup(dytext.text);
    }

    return type;
}

static int
token_list_init(struct token_list *tokens) {
    tokens->capacity = 32;
    tokens->length = 0;
    tokens->tokens = malloc(tokens->capacity * sizeof(struct token));

    if (!tokens->tokens)
        return -1;

    return 0;
}

static int
token_list_append(struct token_list *tokens, struct token *token) {
    if (tokens->length >= tokens->capacity) {
        tokens->capacity *= 2;
        tokens->tokens = realloc(tokens->tokens, tokens->capacity * sizeof(struct token));
        if (!tokens->tokens)
            return -1;
    }

    tokens->tokens[tokens->length++] = *token;
    return 0;
}

void
token_list_print(struct token_list *tokens) {
    for (size_t i = 0; i < tokens->length; i++) {
        printf("%s ", tokens->tokens[i].semantic_value);
    }
    printf("\n");
}

int
tokenize(const char *source, struct token_list *tokens) {
    init_scanner(source);
    token_list_init(tokens);

    for (;;) {
        struct token token = {0};
        enum token_t type = get_token(&token);
        if (type == TOKEN_EOF) break;
        if (type == TOKEN_SKIP) continue;
        token_list_append(tokens, &token);
    }

    token_list_print(tokens);

    // if (errors)
    //     return -1;

    return 0;

}
