#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))
#define MAX_BUF 1024

int main() {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    mousemask(ALL_MOUSE_EVENTS, NULL);

    int yMax, xMax;
    getmaxyx(stdscr, yMax, xMax);

    const char *buttonLabels[] = {"Bottone 1 (UP)", "Bottone 2 (DOWN)", "Bottone 3 (LEFT)", "Bottone 4 (RIGHT)", "Bottone 5 (STOP)"};
    const char *commands[] = {"UP\n", "DOWN\n", "LEFT\n", "RIGHT\n", "STOP\n"};

    bool buttonSelected[ARRAY_SIZE(buttonLabels)] = {false};

    MEVENT event;

    // Creazione della FIFO per la scrittura
    const char *fifoName = "/tmp/myfifo";
    mkfifo(fifoName, 0666);
    int fd = open(fifoName, O_WRONLY);

    if (fd == -1) {
        perror("Errore nell'apertura della FIFO");
        exit(EXIT_FAILURE);
    }

    while (1) {
        clear();

        for (int i = 0; i < ARRAY_SIZE(buttonLabels); i++) {
            if (buttonSelected[i]) {
                attron(A_REVERSE);
            }
            mvprintw(yMax / 2 - 2 + i, xMax / 2 - 15, "%s", buttonLabels[i]);
            attroff(A_REVERSE);
        }

        refresh();

        int choice = getch();

        if (choice == KEY_MOUSE) {
            if (getmouse(&event) == OK) {
                if (event.bstate & BUTTON1_CLICKED) {
                    for (int i = 0; i < ARRAY_SIZE(buttonLabels); i++) {
                        if (event.y == yMax / 2 - 2 + i) {
                            // Invia il comando corrispondente alla FIFO
                            write(fd, commands[i], strlen(commands[i]));
                            buttonSelected[i] = !buttonSelected[i];
                        } else {
                            buttonSelected[i] = false;
                        }
                    }
                }
            }
        } else if (choice == 27) {
            break;
        }
    }

    close(fd);
    endwin();
    return 0;
}

