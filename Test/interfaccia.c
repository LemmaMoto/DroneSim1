#include <ncurses.h>
#include <menu.h>
#include <stdlib.h>

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

int main() {
    initscr(); // Inizializza ncurses
    cbreak(); // Abilita l'input carattere
    noecho();
    keypad(stdscr, TRUE);
    mousemask(ALL_MOUSE_EVENTS, NULL); // Abilita la gestione degli eventi del mouse

    int yMax, xMax;
    getmaxyx(stdscr, yMax, xMax);

    const char *buttonLabels[] = {"Bottone 1", "Bottone 2", "Bottone 3", "Bottone 4", "Bottone 5"};
    int clickCount[ARRAY_SIZE(buttonLabels)] = {0}; // Array per il conteggio dei clic

    MEVENT event;
    int selectedButton = -1;

    while (1) {
        clear();

        // Stampa i bottoni e i conteggi
        for (int i = 0; i < ARRAY_SIZE(buttonLabels); i++) {
            if (i == selectedButton) {
                attron(A_REVERSE);
            }
            mvprintw(yMax / 2 - 2 + i, xMax / 2 - 10, "%s (%d clicchi)", buttonLabels[i], clickCount[i]);
            attroff(A_REVERSE);
        }

        refresh();

        int choice = getch();

        if (choice == KEY_MOUSE) {
            if (getmouse(&event) == OK) {
                if (event.bstate & BUTTON1_CLICKED) {
                    // Il mouse ha cliccato
                    selectedButton = event.y - (yMax / 2 - 2);
                    if (selectedButton >= 0 && selectedButton < ARRAY_SIZE(buttonLabels)) {
                        // Incrementa il conteggio del bottone selezionato
                        clickCount[selectedButton]++;
                    }
                }
            }
        } else if (choice == 27) {
            break;
        }
    }

    endwin(); // Termina ncurses

    return 0;
}
