#include "headers/wrapfunc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
int main(int argc, char *argv[]) {
    int readp, writep;
    sscanf(argv[2], "%d %d", &readp, &writep);
    close(readp);
    printf("Number %d\n", writep);

    while (1) {
        m_write(writep, argv[1], strlen(argv[1]) + 1);
        printf("Sent number %s\n", argv[1]);
        sleep(argv[1][0] - '0');
    }
    return EXIT_SUCCESS;
}
