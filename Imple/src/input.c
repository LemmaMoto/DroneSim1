#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/select.h>
#include <unistd.h>
#include <stdlib.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <signal.h>
#include <time.h>
#include <stdarg.h>
#include <sys/wait.h>
#include <ncurses.h>
#include <ctype.h>
#include "arplib.h"
#include "../config/config.h"

#define WIND_NUMBER 12

// signals handler functions
void sigusr1Handler(int signum, siginfo_t *info, void *context);

// declarations of functions used for windows
void open_control_window(int Srow, int Scol, WINDOW *array_pointer[], char *icon_string, int *active_power);
void print_screen(char *txt_path, int txt_row, int txt_col);

WINDOW *create_new_window(int row, int col, int ystart, int xstart, char icon, int index, int *active_power);
void destroy_win(WINDOW *local_win);

void print_on_button(WINDOW *pointer, char icon, int row, int col, int active_power);
void print_on_window(WINDOW *print_pointer, char input_char);
void color_blink(WINDOW *color_pointer);

void case_execution(char input_char, WINDOW *array_pointer[], char *icon_string, int *active_power);
int *manage_input(char input_char, char *icon_string, int *active_power, double *resulting_power);
int **generate_wind_info(int Srow, int Scol);

// global variables
pid_t wd;

int main(int argc, char *argv[])
{
    int i;
    pid_t input_pid = getpid();
    // write into logfile the pid
    writeLog("INPUT create with pid %d ", input_pid);

    //// configure the handler for SIGUSR1
    struct sigaction sa_usr1;
    sa_usr1.sa_sigaction = sigusr1Handler;
    sa_usr1.sa_flags = SA_SIGINFO;

    if (sigaction(SIGUSR1, &sa_usr1, NULL) == -1)
    {
        error("input: sigaction");
    }

    ////---- Manage pipe ----------------------------------------------------------

    //// pipe from master: fd2 are in position 1 and 2 of argv
    int fd2[2];
    for (i = 1; i < 3; i++)
    {
        fd2[i - 1] = atoi(argv[i]);
    }
    writeLog("INPUT value of fd2 are: %d, %d ", fd2[0], fd2[1]);

    // input need to write the pid inside the pipe
    // close the read file descriptor fd2[0]
    closeAndLog(fd2[0], "input: close fd2[0] ");

    // write the pid inside the pipe
    if (write(fd2[1], &input_pid, sizeof(input_pid)) < 0)
    {
        error("write fd2[1] input");
    }
    // close the write file descriptor fd2[1]
    closeAndLog(fd2[1], "input: close fd2[1] ");

    /// Take pipe fdi_s for comunication between input->server, they are in position 3, 4
    int fdi_s[2];
    for (i = 3; i < 5; i++)
    {
        fdi_s[i - 3] = atoi(argv[i]);
    }
    // close the read file descriptor fdi_s[0], input only write in the pipe
    closeAndLog(fdi_s[0], "input: close fdi_s[0] ");

    ////---- Manage pipe end --------------------------------------------------------

    char input_char = '?'; // definisco la variabile di input

    // definizione delle variabili di ncurses ------
    int Srow, Scol, SrowNew, ScolNew;

    WINDOW *external_window;
    WINDOW *printing_window;

    WINDOW *central_butt;
    WINDOW *quit_butt;

    WINDOW *up_butt;
    WINDOW *down_butt;
    WINDOW *left_butt;
    WINDOW *right_butt;

    WINDOW *up_left_butt;
    WINDOW *up_right_butt;
    WINDOW *down_left_butt;
    WINDOW *down_right_butt;

    // definizione delle variabili di ncurses ------ 3    4    5    6    7    8    9    10   11
    char icon_string[WIND_NUMBER] = {'-', '-', 'Q', 'W', 'E', 'R', 'S', 'D', 'F', 'X', 'C', 'V'};

    int *active_power = (int *)malloc(WIND_NUMBER * sizeof(int));
    for (int i = 0; i < WIND_NUMBER; i++)
    {
        active_power[i] = 0;
    }
    active_power[2] = -1;
    active_power[7] = -1;

    double resulting_power[2] = {0.0, 0.0};
    double resulting_power_old[2] = {0.0, 0.0};

    WINDOW *wind_pointer_array[WIND_NUMBER] = {external_window, printing_window, quit_butt, up_left_butt, up_butt, up_right_butt, left_butt, central_butt, right_butt, down_left_butt, down_butt, down_right_butt};

    FILE *rules_text = fopen("../config/rule.txt", "r");
    if (rules_text == NULL)
    {
        printf("null file pointer\n");
        fflush(stdout);
    }

    // ncurses initialization row, attivo la modalità ncurses
    initscr();
    cbreak();
    raw();
    noecho();
    keypad(stdscr, TRUE);

    // definisco i limiti massimi della finsetra, refresh di stdscr
    getmaxyx(stdscr, Srow, Scol);
    refresh();

    // inizio del gioco

    open_control_window(Srow, Scol, wind_pointer_array, icon_string, active_power);

    while ((input_char = getch()) != 'q')
    {

        active_power = manage_input(input_char, icon_string, active_power, resulting_power);
        if (resulting_power[0] == resulting_power_old[0] && resulting_power[1] == resulting_power_old[1])
        {
            // printf("no print \n");
        }
        else
        {
            // sending force data to the server process, trogh the pipe fdi_s[1]
            if (write(fdi_s[1], resulting_power, sizeof(double) * 2) < 0)
            {
                error("input: write fdi_s[1] ");
            }
            resulting_power_old[0] = resulting_power[0];
            resulting_power_old[1] = resulting_power[1];
        }
        case_execution(input_char, wind_pointer_array, icon_string, active_power);

        getmaxyx(stdscr, SrowNew, ScolNew);
        if (SrowNew != Srow || ScolNew != Scol)
        {
            Scol = ScolNew;
            Srow = SrowNew;

            clear();

            for (int i = 0; i < WIND_NUMBER; i++)
            {
                destroy_win(wind_pointer_array[i]);
            }

            refresh();

            open_control_window(Srow, Scol, wind_pointer_array, icon_string, active_power);
        }
    }
    // free memory
    free(active_power);

    // termination row
    endwin();
    // close the write file descriptor
    closeAndLog(fdi_s[1], "input: close fdi_s[1] ");

    return 0;
}

////--- Function section -------------------------------------------------------
void sigusr1Handler(int signum, siginfo_t *info, void *context)
{
    if (signum == SIGUSR1)
    {
        /*send a signal SIGUSR2 to watchdog */
        kill(info->si_pid, SIGUSR2);
        writeLog_wd("INPUT, pid: %d, received signal from wd pid: %d ", getpid(), info->si_pid);
    }
}

WINDOW *create_new_window(int row, int col, int ystart, int xstart, char icon, int index, int *active_power)
{
    WINDOW *local_window = newwin(row, col, ystart, xstart);
    box(local_window, 0, 0);

    if (icon != '-' && index >= 2)
    {
        print_on_button(local_window, icon, row, col, active_power[index]);
    }

    wrefresh(local_window);
    return local_window;
}

void destroy_win(WINDOW *local_win)
{
    box(local_win, ' ', ' ');
    wborder(local_win, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');
    wrefresh(local_win);
    delwin(local_win);
}

int **generate_wind_info(int Srow, int Scol)
{
    // external_window, printing_window, quit_butt, up_left_butt, up_butt, up_right_butt,
    // left_butt, central_butt, right_butt, down_left_butt, down_butt, down_right_butt
    int **wind_info = (int **)malloc(12 * sizeof(int *));

    for (int i = 0; i < 12; i++)
    {
        wind_info[i] = (int *)malloc(4 * sizeof(int));
    }

    // central button -> definisco le altre dimensioni su questa -----
    wind_info[7][0] = (int)(Srow * 0.1);
    wind_info[7][1] = wind_info[7][0] * 2;
    wind_info[7][2] = ((int)((Srow - wind_info[7][0]) / 2));
    wind_info[7][3] = ((int)((Scol - wind_info[7][1]) / 2));
    // ---------------------------------------------------------------
    int square_row = wind_info[7][0];
    int square_col = wind_info[7][1];

    int center_y = wind_info[7][2];
    int center_x = wind_info[7][3];

    // external window
    wind_info[0][0] = (int)(Srow * 0.9);
    wind_info[0][1] = (int)(Scol * 0.8);
    wind_info[0][2] = 0;
    wind_info[0][3] = (int)((Scol - wind_info[0][1]) / 2);

    // printing window
    wind_info[1][0] = (int)(Srow * 0.1);
    wind_info[1][1] = (int)(Scol * 0.8);
    wind_info[1][2] = Srow - wind_info[1][0];
    wind_info[1][3] = (int)((Scol - wind_info[1][1]) / 2);

    // quit button
    wind_info[2][0] = square_row;
    wind_info[2][1] = square_col * 2;
    wind_info[2][2] = 1;
    wind_info[2][3] = (int)((Scol - wind_info[2][1]) / 2);

    // up left button
    wind_info[3][0] = square_row;
    wind_info[3][1] = square_col;
    wind_info[3][2] = (center_y) - (square_row);
    wind_info[3][3] = (center_x) - (square_col);

    // up button --> bottone su lungo
    wind_info[4][0] = square_row * 2;
    wind_info[4][1] = square_col;
    wind_info[4][2] = center_y - (square_row * 2);
    wind_info[4][3] = center_x;

    // up right button
    wind_info[5][0] = square_row;
    wind_info[5][1] = square_col;
    wind_info[5][2] = center_y - square_row;
    wind_info[5][3] = center_x + square_col;

    // left button --> bottone sinistra lungo
    wind_info[6][0] = square_row;
    wind_info[6][1] = square_col * 2;
    wind_info[6][2] = center_y;
    wind_info[6][3] = (center_x) - (square_col * 2);

    // right button --> bottone destra lungo
    wind_info[8][0] = square_row;
    wind_info[8][1] = square_col * 2;
    wind_info[8][2] = center_y;
    wind_info[8][3] = center_x + square_col;

    // down left button
    wind_info[9][0] = square_row;
    wind_info[9][1] = square_col;
    wind_info[9][2] = center_y + square_row;
    wind_info[9][3] = center_x - square_col;

    // down button --> bottone giu lungo
    wind_info[10][0] = square_row * 2;
    wind_info[10][1] = square_col;
    wind_info[10][2] = center_y + square_row;
    wind_info[10][3] = center_x;

    // down right button
    wind_info[11][0] = square_row;
    wind_info[11][1] = square_col;
    wind_info[11][2] = center_y + square_row;
    wind_info[11][3] = center_x + square_col;

    return wind_info;
}

void open_control_window(int Srow, int Scol, WINDOW *array_pointer[], char *icon_string, int *active_power)
{
    int **local_array;
    local_array = generate_wind_info(Srow, Scol);

    for (int i = 0; i < WIND_NUMBER; i++)
    {
        array_pointer[i] = create_new_window(local_array[i][0], local_array[i][1], local_array[i][2], local_array[i][3], icon_string[i], i, active_power);
    }

    free(local_array);
}

void case_execution(char input_char, WINDOW *array_pointer[], char *icon_string, int *active_power)
{
    int pointer_index = 0;
    int BTy, BTx;

    for (int i = 2; i < WIND_NUMBER; i++)
    {
        if (toupper(input_char) == icon_string[i])
        {
            pointer_index = i;
        }
    }

    if (pointer_index > 2 && pointer_index < WIND_NUMBER)
    {
        if (pointer_index == 7)
        {
            for (int i = 3; i < WIND_NUMBER; i++)
            {
                getmaxyx(array_pointer[i], BTy, BTx);
                print_on_button(array_pointer[i], icon_string[i], BTy, BTx, active_power[i]);
            }
        }
        else
        {
            getmaxyx(array_pointer[pointer_index], BTy, BTx);
            print_on_button(array_pointer[pointer_index], icon_string[pointer_index], BTy, BTx, active_power[pointer_index]);

            print_on_window(array_pointer[1], input_char);

            if (pointer_index >= 2)
            {
                color_blink(array_pointer[pointer_index]);
            }
        }
    }
}

int *manage_input(char input_char, char *icon_string, int *active_power, double *resulting_power)
{
    int pointer_index = 0;
    for (int i = 3; i < WIND_NUMBER; i++)
    {
        if (toupper(input_char) == icon_string[i])
        {
            pointer_index = i;
        }
    }

    if (pointer_index > 2 && pointer_index < WIND_NUMBER)
    {
        if (pointer_index == 7)
        {
            for (int i = 0; i < WIND_NUMBER; i++)
            {
                active_power[i] = 0;
            }
            active_power[2] = -1;
            active_power[7] = -1;

            /*
            // definizione delle variabili di ncurses ------ 3    4    5    6    7    8    9    10   11
            char icon_string[WIND_NUMBER] = {'-', '-', 'Q', 'W', 'E', 'R', 'S', 'D', 'F', 'X', 'C', 'V'};
            */

            // resulting power on the y direction
            resulting_power[1] = active_power[4] - active_power[10] + active_power[3] / 2.0 + active_power[5] / 2.0 - active_power[9] / 2.0 - active_power[11] / 2.0;

            // resulting power on the x direction
            resulting_power[0] = active_power[8] - active_power[6] - active_power[3] / 2.0 - active_power[9] / 2.0 + active_power[5] / 2.0 + active_power[11] / 2.0;
        }
        else
        {
            active_power[pointer_index] += 1;

            if (active_power[pointer_index] > LIMIT)
            {
                active_power[pointer_index] -= 1;
            }
            // resulting power on the y direction
            resulting_power[1] = active_power[4] - active_power[10] + active_power[3] / 2.0 + active_power[5] / 2.0 - active_power[9] / 2.0 - active_power[11] / 2.0;

            // resulting power on the x direction
            resulting_power[0] = active_power[8] - active_power[6] - active_power[3] / 2.0 - active_power[9] / 2.0 + active_power[5] / 2.0 + active_power[11] / 2.0;
        }
    }

    return active_power;
}

void print_on_button(WINDOW *pointer, char icon, int row, int col, int active_power)
{
    if (active_power == -1)
    {
        char string[50];

        sprintf(string, "%c", icon);
        mvwaddstr(pointer, (row / 2), ((col - strlen(string)) / 2), string);
        wrefresh(pointer);
    }
    else
    {
        char string[50];

        wclear(pointer);
        wrefresh(pointer);

        box(pointer, 0, 0);

        sprintf(string, "%c - %d", icon, active_power);
        mvwaddstr(pointer, (row / 2), ((col - strlen(string)) / 2), string);
        wrefresh(pointer);
    }
}

void print_on_window(WINDOW *print_pointer, char input_char)
{
    int PRy, PRx;
    getmaxyx(print_pointer, PRy, PRx);

    char string[30] = "hai premuto il tasto: ";
    strncat(string, &input_char, 1);
    mvwaddstr(print_pointer, PRy / 2, ((PRx - strlen(string)) / 2), string);
    // wprintw(print_pointer, "valore controllo: %d", controllo);
    wrefresh(print_pointer);
}

void color_blink(WINDOW *color_pointer)
{

    start_color();
    init_pair(2, COLOR_RED, COLOR_BLUE);
    init_pair(1, COLOR_WHITE, COLOR_BLACK);

    wbkgd(color_pointer, COLOR_PAIR(2));
    wrefresh(color_pointer);
    napms(50);
    // Return to the default color
    wbkgd(color_pointer, COLOR_PAIR(1));
    wrefresh(color_pointer);
}
