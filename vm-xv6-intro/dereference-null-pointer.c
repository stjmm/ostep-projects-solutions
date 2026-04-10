#include <stdio.h>

int main(void)
{
    char *n = NULL;
    *n = 'a';
    printf("our null pointer dereference: %s\n", n);
    return 0;
}
