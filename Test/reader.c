#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#define MAX_BUF 1024

int main() {
    int fd;
    char *myfifo = "/tmp/myfifo";
    char buf[MAX_BUF];

    // Apri la FIFO per la lettura
    fd = open(myfifo, O_RDONLY);
    if (fd == -1) {
        perror("Errore nell'apertura della FIFO");
        exit(EXIT_FAILURE);
    }

    while (1) {
        // Leggi i dati dalla FIFO
        ssize_t bytesRead = read(fd, buf, MAX_BUF);
        if (bytesRead == -1) {
            perror("Errore nella lettura dalla FIFO");
            close(fd);
            exit(EXIT_FAILURE);
        }

        if (bytesRead > 0) {
            printf("Received:\n%s", buf);
        }

        // Pulizia del buffer
        memset(buf, 0, MAX_BUF);

        // Puoi inserire qui un'eventuale logica aggiuntiva per elaborare i dati

        // Sleep per evitare un loop troppo intenso
        usleep(100000);  // Aspetta 100ms prima di leggere di nuovo
    }

    // Chiudi la FIFO (questa parte potrebbe non essere raggiunta nel caso di un loop infinito)
    close(fd);

    return 0;
}
