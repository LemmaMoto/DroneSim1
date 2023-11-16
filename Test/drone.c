#include <ncurses.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define MAX_BUF 1024

struct Drone {
    int x;
    int y;
    char symbol;
    int dx;
    int dy;
};

void moveDrone(struct Drone *drone) {
    drone->x += drone->dx;
    drone->y += drone->dy;
}

int main() {
    initscr();
    curs_set(0);
    timeout(100);

    struct Drone drone = {COLS / 2, LINES / 2, '+', 0, 0};

    // Apri la FIFO per la lettura
    const char *fifoName = "/tmp/myfifo";
    int fd = open(fifoName, O_RDONLY);
    if (fd == -1) {
        perror("Errore nell'apertura della FIFO");
        exit(EXIT_FAILURE);
    }

    int ch;

    // Inizializza il drone con movimento fermo
    drone.dx = 0;
    drone.dy = 0;

    int bytesRead; 

while (1) {
    clear();

    mvaddch(drone.y, drone.x, drone.symbol);

    refresh();

    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(fd, &readfds);

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;

    int ready = select(fd + 1, &readfds, NULL, NULL, &timeout);

    if (ready > 0 && FD_ISSET(fd, &readfds)) {
        char buffer[MAX_BUF];

        // Leggi i comandi dalla FIFO
        bytesRead = read(fd, buffer, MAX_BUF);

        if (bytesRead > 0) {
            buffer[bytesRead] = '\0';

            // Processa i comandi ricevuti e modifica il movimento del drone
            if (strcmp(buffer, "UP\n") == 0) {
                drone.dx = 0;
                drone.dy = -1;
            } else if (strcmp(buffer, "DOWN\n") == 0) {
                drone.dx = 0;
                drone.dy = 1;
            } else if (strcmp(buffer, "LEFT\n") == 0) {
                drone.dx = -1;
                drone.dy = 0;
            } else if (strcmp(buffer, "RIGHT\n") == 0) {
                drone.dx = 1;
                drone.dy = 0;
            } else if (strcmp(buffer, "STOP\n") == 0) {
                drone.dx = 0;
                drone.dy = 0;
            }
        }
    }

        // Muovi il drone
        moveDrone(&drone);

        ch = getch();

        if (ch == 'q') {
            break;
        }
    }

    close(fd);
    endwin();
    return 0;
}

