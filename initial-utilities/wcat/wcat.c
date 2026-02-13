#include <stdio.h>
#include <stdlib.h>

#define MAX_LENGTH 64

int main(int argc, char **argv)
{
    FILE *stream = NULL;
    char buffer[MAX_LENGTH];

    for (int i = 1; i < argc; i++) {
        stream = fopen(argv[i], "r");
        if (stream == NULL) {
            printf("wcat: cannot open file\n");
            exit(1);
        }

        while (fgets(buffer, MAX_LENGTH, stream) != NULL) {
            printf("%s", buffer);
        }
    }

    return 0;
}
