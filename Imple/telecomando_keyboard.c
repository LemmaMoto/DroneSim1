#include <ncurses.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>

#define PIPE_READ 0
#define PIPE_WRITE 1

// Dichiarazioni delle variabili globali
int pipe_to_drone[2];
int pipe_to_ui[2];
pid_t drone_pid;

void drone_process() {
    // Chiudi l'estremità di scrittura del pipe verso il drone
    close(pipe_to_drone[PIPE_WRITE]);
    
    // Loop principale del processo del drone
    while (1) {
        // Leggi il comando dal pipe
        char command;
        read(pipe_to_drone[PIPE_READ], &command, sizeof(char));

        // Esegui il comando
        if (command == 'U') {
            // Il drone si muove verso l'alto
            // Aggiungi qui la tua logica di movimento del drone
        } else if (command == 'D') {
            // Il drone si muove verso il basso
            // Aggiungi qui la tua logica di movimento del drone
        } else if (command == 'L') {
            // Il drone si muove a sinistra
            // Aggiungi qui la tua logica di movimento del drone
        } else if (command == 'R') {
            // Il drone si muove a destra
            // Aggiungi qui la tua logica di movimento del drone
        }
    }
}

void ui_process() {
    // Chiudi l'estremità di lettura del pipe verso l'interfaccia utente
    close(pipe_to_ui[PIPE_READ]);
    
    // Inizializza ncurses
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);

    // Loop principale dell'interfaccia utente
    while (1) {
        int ch = getch();
        char command = '\0';

        // Mappa i tasti per comandare il drone
        switch (ch) {
            case 'w':
                command = 'w'; // forza verso USx
                break;
            case 'e':
                command = 'e'; // forza verso U
                break;
            case 'r':
                command = 'r'; // forza verso UDx
                break;
            case 's':
                command = 's'; // forza verso Sx
                break;
            case 'd':
                command = 'd'; // annulla forza 
                break;
            case 'f':
                command = 'f';  // forza verso Dx
                break;
            case 'x':
                command = 'x';  // forza verso DSx
                break;
            case 'c':
                command = 'c'; // forza verso D
                break;
            case 'v':
                command = 'v'; // forza verso DDx
                break;
            case 'q':
                command = 'Q'; // Termina il programma
                break;
            default:
                command = '\0'; // Comando non valido
                break;
        }

        // Invia il comando al drone attraverso il pipe
        clear();
        write(pipe_to_drone[PIPE_WRITE], &command, sizeof(char));
        if (command == 'Q') {
            mvprintw(0, 0, "Terminazione in corso...\n");
            refresh();
            sleep(3);
            break;
        }
        if (command == '\0') {
            mvprintw(0, 0, "Comando non valido\n");
        } else {
            mvprintw(0, 0, "Comando inviato: %c\n", command);
        }
        refresh();
    }

    // Chiudi ncurses
    endwin();
}

int main() {
    // Crea i pipe per la comunicazione tra i processi
    pipe(pipe_to_drone);
    pipe(pipe_to_ui);

    // Crea un processo figlio per il drone
    if ((drone_pid = fork()) == 0) {
        drone_process();
        exit(0);
    } else {
        // Processo genitore, esegui l'interfaccia utente
        ui_process();
    }

    // Termina il processo del drone
    kill(drone_pid, SIGKILL);

    return 0;
}
