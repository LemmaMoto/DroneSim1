#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))
#define MAX_BUF 1024

void drawButton(int y, int x, int width, int height, const char *label, bool selected) {
    attron(A_REVERSE);
    mvprintw(y, x, " ");
    mvhline(y, x + 1, ACS_HLINE, width - 2);
    mvaddch(y, x + width - 1, ACS_RTEE);
    mvvline(y + 1, x, ACS_VLINE, height - 2);
    mvaddch(y + height - 1, x, ACS_LTEE);
    mvhline(y + height - 1, x + 1, ACS_HLINE, width - 2);
    mvaddch(y + height - 1, x + width - 1, ACS_BTEE);
    mvprintw(y + height / 2, x + (width - strlen(label)) / 2, "%s", label);
    attroff(A_REVERSE);
    if (selected) {
        attron(A_BOLD);
    }
}

int main() {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    mousemask(ALL_MOUSE_EVENTS, NULL);

    int yMax, xMax;
    getmaxyx(stdscr, yMax, xMax);

    // Colori personalizzati per i bottoni
    start_color();
    init_pair(1, COLOR_WHITE, COLOR_BLACK);
    init_pair(2, COLOR_WHITE, COLOR_BLACK);
    init_pair(3, COLOR_WHITE, COLOR_BLACK);
    init_pair(4, COLOR_WHITE, COLOR_BLACK);
    init_pair(5, COLOR_RED, COLOR_BLACK);

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

    int buttonWidth = 20;
    int buttonHeight = 3;
    int spacing = 2;
    int startY = (yMax - (buttonHeight + spacing) * ARRAY_SIZE(buttonLabels)) / 2;

    while (1) {
        clear();

        for (int i = 0; i < ARRAY_SIZE(buttonLabels); i++) {
            int x = (xMax - buttonWidth) / 2;
            bool selected = buttonSelected[i];

            // Imposta il colore in base all'indice del bottone
            attron(COLOR_PAIR(i + 1));

            // Disegna il bottone
            drawButton(startY + i * (buttonHeight + spacing), x, buttonWidth, buttonHeight, buttonLabels[i], selected);
        }

        refresh();

        int choice = getch();

        if (choice == KEY_MOUSE) {
            if (getmouse(&event) == OK) {
                if (event.bstate & BUTTON1_CLICKED) {
                    for (int i = 0; i < ARRAY_SIZE(buttonLabels); i++) {
                        int x = (xMax - buttonWidth) / 2;
                        bool selected = buttonSelected[i];
                        int buttonY = startY + i * (buttonHeight + spacing);
                        int buttonX = x;
                        if (event.x >= buttonX && event.x < buttonX + buttonWidth && event.y >= buttonY && event.y < buttonY + buttonHeight) {
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
    //
    close(fd);
    endwin();
    return 0;
}

