#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "fileio.h"

int get_file_length(const char *filename) {
        struct stat st;

        if (stat(filename, &st) == 0)
                return (int)st.st_size;
        else
                return -1;
}

int read_file(const char *filename, char **buffer) {
        FILE *file = NULL;
        char *content = NULL;
        int len;

        file = fopen(filename, "r");
        if (!file)
                return -1;

        len = get_file_length(filename);
        if (len < 0) {
                fclose(file);
                return -1;
        }

        content = (char*)malloc(len * sizeof(char));
        if (!content) {
                fclose(file);
                return -1;
        }

        for (int i = 0; i < len; i++) {
                content[i] = fgetc(file);
        }

        content[len - 1] = '\0';
        *buffer = content;

        return 0;
}
