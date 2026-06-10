#include <stdio.h>
#include <stdlib.h>
#include "fileio.h"
#include "scanner.h"

const char *dyfile = NULL;

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("usage: %s <file>\n", argv[0]);
        return 1;
    }

    dyfile = argv[1];
    const char *source = NULL;

    if (read_file(dyfile, &source) != 0) {
        printf("error: failed to read file\n");
        return 1;
    }

    struct token_list tokens;

    if (tokenize(source, &tokens) != 0) {
        printf("errors encountered during tokenization\n");
        return 1;
    }

    return 0;
}
