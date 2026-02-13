#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

// aaaaaaaaaabbbb -> 10a4b

int main(int argc, char **argv)
{
    FILE *stream = NULL;
    if (argc < 2) {
        printf("wzip: file1 [file2 ...]\n");
        exit(1);
    }

    int run_len;
    int cur_char;
    int prev_run_char = EOF;

    for (int i = 1; i < argc; i++) {
        stream = fopen(argv[i], "r");
        if (stream == NULL) {
            printf("wzip: cant open file\n");
            exit(1);
        }

        while ((cur_char = fgetc(stream)) != EOF) {
            if (prev_run_char == EOF) {
                prev_run_char = cur_char;
                run_len = 1;
            }
            else if (prev_run_char == cur_char) {
                run_len++;
            }
            else {
                fwrite(&run_len, 4, 1, stdout);
                fwrite(&prev_run_char, 1, 1, stdout);

                prev_run_char = cur_char;
                run_len = 1;
            }
        }

        fclose(stream);
    }

    if (run_len > 0) {
        fwrite(&run_len, 4, 1, stdout);
        fwrite(&prev_run_char, 1, 1, stdout);
    }

    return 0;
}
