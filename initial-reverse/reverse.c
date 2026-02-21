#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

int main(int argc, char **argv)
{
    FILE *in = stdin;
    FILE *out = stdout;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    char **lines = NULL;
    size_t line_count = 0;

    if (argc > 3) {
        fprintf(stderr, "usage: reverse <input> <output>\n");
        exit(1);
    }

    if (argc >= 2) {
        in = fopen(argv[1], "r");
        if (in == NULL) {
            fprintf(stderr, "reverse: cannot open file '%s'\n", argv[1]);
            exit(1);
        }
    }

    if (argc == 3) {
        out = fopen(argv[2], "w");
        if (out == NULL) {
            fprintf(stderr, "reverse: cannot open file '%s'\n", argv[2]);
            exit(1);
        }
    }

    struct stat stat1, stat2;
    stat(argv[1], &stat1);
    stat(argv[2], &stat2);
    if (stat1.st_ino == stat2.st_ino) {
        fprintf(stderr, "reverse: input and output file must differ\n");
        exit(1);
    }

    while ((read = getline(&line, &len, in)) != -1) {
        lines = realloc(lines, (line_count + 1) * sizeof(char*));
        lines[line_count++] = strdup(line);
    }

    for (int i = line_count - 1; i >= 0; i--) {
        fprintf(out, "%s", lines[i]);
        free(lines[i]);
    }

    free(line);
    if (in != stdin) fclose(in);
    if (out != stdout) fclose(out);

    return 0;
}
