#include <stdio.h>
#include <stdlib.h>
#include "fileio.h"
#include "dscanner.h"

const char *dyfile = NULL;

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("usage: %s <file>\n", argv[0]);
        return 1;
    }

    dyfile = argv[1];
    char *source = NULL;

    if (read_file(dyfile, &source) != 0) {
        printf("error: failed to read file\n");
        return 1;
    }

    struct TokenList tokens;

    if (tokenize(source, &tokens) != 0) {
        printf("errors encountered during tokenization\n");
        return 1;
    }

    free(source);
    token_list_free(&tokens);

    return 0;
}
