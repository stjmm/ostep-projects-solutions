#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void process_stream(FILE *stream, char *searchterm)
{
    char *line = NULL;
    size_t len = 0;

    while (getline(&line, &len, stream) != -1) {
        if (strstr(line, searchterm) != NULL) {
            printf("%s", line);
        }
    }
    free(line);
}

int main(int argc, char **argv)
{
    if (argc <= 1) {
        printf("wgrep: searchterm [file ...]\n");
        exit(1);
    }

    FILE *stream;
    char *searchterm = argv[1];

    if (argc <= 2) {
        stream = stdin;
        process_stream(stream, searchterm);
    } else {
        for (int i = 2; i < argc; i++) {
            stream = fopen(argv[i], "r");
            if (stream == NULL) {
                printf("wgrep: cannot open file\n");
                exit(1);
            }

            process_stream(stream, searchterm);

            fclose(stream);
        }
    }

    return 0;
}
