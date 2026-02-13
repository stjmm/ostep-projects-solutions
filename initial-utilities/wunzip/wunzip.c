#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    if (argc < 2) {
        printf("wunzip: file1 [file2 ...]\n");
        exit(1);
    }
    FILE *stream = NULL;

    int run_len;
    char run_char;

    for (int i = 1; i < argc; i++) {
        stream = fopen(argv[i], "r");
        if (stream == NULL ){
            printf("wunzip: cant open file\n");
            exit(1);
        }

        while (fread(&run_len, 4, 1, stream) == 1) {
            if (fread(&run_char, 1, 1, stream) != 1) {
                exit(1);
            }

            for (int i = 0; i < run_len; i++) {
                fputc(run_char, stdout);
            }
        }

        fclose(stream);
    }

    return 0;
}
