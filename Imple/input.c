#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <time.h>
#include "include/constants.h"
#include <ncurses.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>

#define PIPE_READ 0
#define PIPE_WRITE 1

pid_t watchdog_pid;
pid_t process_id;
char *process_name;
struct timeval prev_t;
char logfile_name[256] = LOG_FILE_NAME;

// Dichiarazioni delle variabili globali
int pipefd[2];

pid_t drone_pid;

void drawRoundedSquare(WINDOW *win, int y, int x, int height, int width)
{

    // Disegna gli angoli arrotondati
    mvwaddch(win, y, x, ACS_ULCORNER);                          // Angolo in alto a sinistra
    mvwaddch(win, y, x + width - 1, ACS_URCORNER);              // Angolo in alto a destra
    mvwaddch(win, y + height - 1, x, ACS_LLCORNER);             // Angolo in basso a sinistra
    mvwaddch(win, y + height - 1, x + width - 1, ACS_LRCORNER); // Angolo in basso a destra

    // Disegna i lati orizzontali
    for (int i = x + 1; i < x + width - 1; ++i)
    {
        mvwaddch(win, y, i, ACS_HLINE);              // Linea orizzontale superiore
        mvwaddch(win, y + height - 1, i, ACS_HLINE); // Linea orizzontale inferiore
    }

    // Disegna i lati verticali
    for (int i = y + 1; i < y + height - 1; ++i)
    {
        mvwaddch(win, i, x, ACS_VLINE);             // Linea verticale sinistra
        mvwaddch(win, i, x + width - 1, ACS_VLINE); // Linea verticale destra
    }
    
}

void highlightSquare(WINDOW *win, int y, int x, int height, int width)
{
    // Imposta attributi per evidenziare il quadrato
    wattron(win, COLOR_PAIR(2)); // Utilizza il colore invertito per evidenziare
    drawRoundedSquare(win, y, x, height, width);
    wattroff(win, COLOR_PAIR(1)); // Ripristina gli attributi precedenti
    wrefresh(win); // Aggiorna la finestra
    usleep(20000);
    drawRoundedSquare(win, y, x, height, width);
    wrefresh(win); // Aggiorna la finestra di nuovo
}

void ui_process()
{

    // Inizializza ncurses
    initscr();
    start_color(); // Abilita il colore
    init_pair(1, COLOR_WHITE, COLOR_BLACK);
    cbreak();
    noecho();
    keypad(stdscr, TRUE);

    int gridHeight = 3;
    int gridWidth = 3;
    int squareHeight = 5;
    int squareWidth = 10;
    int maxRows, maxCols;
    getmaxyx(stdscr, maxRows, maxCols);

    // Calcola la posizione iniziale della finestra
    int startY = (maxRows - gridHeight * squareHeight) / 2;
    int startX = (maxCols - gridWidth * squareWidth) / 2;

    WINDOW *mywin = newwin(gridHeight * squareHeight, gridWidth * squareWidth, startY, startX);

    for (int i = 0; i < gridHeight; ++i)
    {
        for (int j = 0; j < gridWidth; ++j)
        {
            int y = i * squareHeight;
            int x = j * squareWidth;

            // Stampa il quadrato con angoli arrotondati
            wattron(mywin, COLOR_PAIR(1)); // Utilizza il colore blu
            drawRoundedSquare(mywin, y, x, squareHeight, squareWidth);
             // Ripristina il colore precedente
        }
    }

    // Loop principale dell'interfaccia utente
    while (1)
    {
        int ch = getch();
        char command = '\0';

        // Mappa i tasti per comandare il drone
        switch (ch)
        {
        case 'w':
            command = 'w'; // forza verso USx
            highlightSquare(mywin, 0, 0, squareHeight, squareWidth);
            break;
        case 'e':
            command = 'e'; // forza verso U
            highlightSquare(mywin, 0, squareWidth, squareHeight, squareWidth);
            break;
        case 'r':
            command = 'r'; // forza verso UDx
            highlightSquare(mywin, 0, 2 * squareWidth, squareHeight, squareWidth);
            break;
        case 's':
            command = 's'; // forza verso Sx
            highlightSquare(mywin, squareHeight, 0, squareHeight, squareWidth);
            break;
        case 'd':
            command = 'd'; // annulla forza
            highlightSquare(mywin, squareHeight, squareWidth, squareHeight, squareWidth);
            break;
        case 'f':
            command = 'f'; // forza verso Dx
            highlightSquare(mywin, squareHeight, 2 * squareWidth, squareHeight, squareWidth);
            break;
        case 'x':
            command = 'x'; // forza verso DSx
            highlightSquare(mywin, 2 * squareHeight, 0, squareHeight, squareWidth);
            break;
        case 'c':
            command = 'c'; // forza verso D
            highlightSquare(mywin, 2 * squareHeight, squareWidth, squareHeight, squareWidth);
            break;
        case 'v':
            command = 'v'; // forza verso DDx
            highlightSquare(mywin, 2 * squareHeight, 2 * squareWidth, squareHeight, squareWidth);
            break;
        case 'q':
            command = 'Q'; // Termina il programma
            mvprintw(0, 0, "Terminazione in corso...\n");
            sleep(3);
            break;
        default:
            command = '\0'; // Comando non valido
            break;
        }

        wrefresh(mywin); // Aggiungi questa linea

        // Invia il comando al drone attraverso il pipe
        if (command != '\0')
        {

            int retwrite = write(pipefd[PIPE_WRITE], &command, sizeof(char));
            fsync(pipefd[PIPE_WRITE]); // flush the pipe

            if (retwrite > 0)
            {
                if (command == 'Q')
                {
                    clear();
                    mvprintw(0, 0, "Terminazione in corso...\n");
                    refresh();
                    sleep(3);
                    break;
                }
                else
                {
                    mvprintw(0, 0, "Comando inviato: %c\n", command);
                }
            }
            else if (retwrite < 0)
            {
                perror("write");
                mvprintw(0, 0, "Error writing to pipe\n");

                continue;
            }
            else if (retwrite == 0)
            {
                mvprintw(0, 0, "No bytes written to pipe\n");

                continue;
            }
        }
    }

    // Chiudi ncurses
    endwin();
}

// logs time update to file
void log_receipt(struct timeval tv)
{
    FILE *lf_fp = fopen(logfile_name, "a");
    fprintf(lf_fp, "%d %ld %ld\n", process_id, tv.tv_sec, tv.tv_usec);
    fclose(lf_fp);
}

void watchdog_handler(int sig, siginfo_t *info, void *context)
{
    if (info->si_pid == watchdog_pid)
    {
        gettimeofday(&prev_t, NULL);
        log_receipt(prev_t);
    }
}

int main(int argc, char *argv[])
{
    // Set up sigaction for receiving signals from processes
    struct sigaction p_action;
    p_action.sa_flags = SA_SIGINFO;
    p_action.sa_sigaction = watchdog_handler;
    if (sigaction(SIGUSR1, &p_action, NULL) < 0)
    {
        perror("sigaction");
    }

    int process_num;
    if (argc == 4)
    {
        sscanf(argv[1], "%d", &process_num);
        sscanf(argv[2], "%d", &pipefd[PIPE_READ]);
        sscanf(argv[3], "%d", &pipefd[PIPE_WRITE]);
    }
    else
    {
        printf("wrong args\n");
        return -1;
    }

    printf("pipefd[0] = %d, pipefd[1] = %d\n", pipefd[PIPE_READ], pipefd[PIPE_WRITE]);

    // Publish your pid
    process_id = getpid();

    char *fnames[NUM_PROCESSES] = PID_FILE_SP;

    FILE *pid_fp = fopen(fnames[process_num], "w");
    fprintf(pid_fp, "%d", process_id);
    fclose(pid_fp);

    printf("Published pid %d \n", process_id);

    // Read watchdog pid
    FILE *watchdog_fp = NULL;
    struct stat sbuf;

    /* call stat, fill stat buffer, validate success */
    if (stat(PID_FILE_PW, &sbuf) == -1)
    {
        perror("error-stat");
        return -1;
    }
    // waits until the file has data
    while (sbuf.st_size <= 0)
    {
        if (stat(PID_FILE_PW, &sbuf) == -1)
        {
            perror("error-stat");
            return -1;
        }
        usleep(50000);
    }

    watchdog_fp = fopen(PID_FILE_PW, "r");

    fscanf(watchdog_fp, "%d", &watchdog_pid);
    printf("watchdog pid %d \n", watchdog_pid);
    fclose(watchdog_fp);

    // Read how long to sleep process for
    int sleep_durations[NUM_PROCESSES] = PROCESS_SLEEPS_US;
    int sleep_duration = sleep_durations[process_num];
    char *process_names[NUM_PROCESSES] = PROCESS_NAMES;
    process_name = process_names[process_num]; // added to logfile for readability

    ui_process();

    // Termina il processo del drone
    kill(drone_pid, SIGKILL);

    return 0;
}