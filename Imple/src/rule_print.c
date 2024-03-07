#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/select.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <signal.h>
#include <time.h>
#include <sys/wait.h>
#include <stdarg.h>
#include <ncurses.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "arplib.h"
#include "../config/config.h"

void print_screen(char *txt_path, int txt_row, int txt_col, char *buffer);
WINDOW *create_new_window(int row, int col, int ystart, int xstart);
void destroy_win(WINDOW *local_win);

int string_parser(char *string, char *first_arg, char *second_arg);
int input_validation(int ret_val, char *first_arg, char *second_arg);


int main(int argc, char *argv[]){

    pid_t input_pid = getpid();
    // write into logfile the pid
    writeLog("RULE_PRINT create with pid %d ", input_pid);

    int pid_pipe[2];
    for(int i = 0; i<2; i++){
        pid_pipe[i] = atoi(argv[i+1]);
    }
    writeLog("RULE PRINT value of pid pipe are: %d, %d ", pid_pipe[0], pid_pipe[1]);

    if(close(pid_pipe[0]) < 0){
        perror("close pid pipe in rule_print");
    }

    if(write(pid_pipe[1], &input_pid, sizeof(input_pid)) < 0){
        perror("write pid in rule_print");
    }

    int info_pipe[2];
    for(int i = 0; i<2; i++){
        info_pipe[i] = atoi(argv[i+5]);
    }
    if(close(info_pipe[0]) < 0){
        perror("close info pipe in rule_print");
    }
    writeLog("RULE PRINT value of socket info pipe are: %d, %d ", info_pipe[0], info_pipe[1]);

    char socket_info[100];
    char first_arg[100], second_arg[100];
    int ret_val;
    int inputVal;

    int Srow, Scol;

    initscr();
    cbreak();
    raw();
    keypad(stdscr, TRUE);

    // definisco i limiti massimi della finsetra, refresh di stdscr
    getmaxyx(stdscr, Srow, Scol);
    refresh();

    // print the rules and wait to start the game
    do{
        print_screen("../config/rule.txt", 21, 78, socket_info);

        ret_val = string_parser(socket_info, first_arg, second_arg);

        clear();
        refresh();
    }while((inputVal = input_validation(ret_val, first_arg, second_arg) == 0));



    endwin();

    // send the socket info to the master process
    socket_info[strcspn(socket_info, "\n")] = 0;
    int Wret;
    if((Wret = write(info_pipe[1], socket_info, strlen(socket_info))) < 0){
        error("write of soket info in rule_print");
    }

    writeLog("controllo per valori inviati: %d, %s", Wret, socket_info);

    // close pipe fd
    close(info_pipe[1]);

    return 0;
}

int string_parser(char *string, char *first_arg, char *second_arg){
    // define the char that separate the arguments in the string
    char *separator = " ";
    char *arg;
    int ret_val;
    char temp[256];

    strcpy(temp, string);

    arg = strtok(temp, separator);
    strcpy(first_arg, arg);

    arg = strtok(NULL, separator);
    if (arg == NULL)
    {
        ret_val = 0;
    }
    else
    {
        ret_val = 1;
        strcpy(second_arg, arg);
    }

    return ret_val;
}

int input_validation(int ret_val, char *first_arg, char *second_arg){
    int temp, ctrl;
    if(ret_val == 0){
        if(strcmp(first_arg, "singleplayer") != 0){
            return 0;
        }else{
            return 1;   
        }
    }else{
        ctrl = 1;
        temp = atoi(second_arg);
        if(temp < 49152 || temp > 65535){
            ctrl = 0;
        }

        struct sockaddr_in sa;
        if((temp = inet_pton(AF_INET, first_arg, &(sa.sin_addr))) < 0){
            ctrl = 0;
        }

        if(ctrl == 1){
            return 1;
        }else{
            return 0;
        }
    }
}

void print_screen(char *txt_path, int txt_row, int txt_col, char *buffer)
{
    // limiti della finestra
    int Srow, Scol;
    getmaxyx(stdscr, Srow, Scol);

    // control variable for the start of the game
    char start_char = '?';
    char resisize_request[] = "please resize the window";
    char rule_line[100];
    char socket_info[100];

    // window pointer
    WINDOW *rule_window;
    WINDOW *socket_info_window;


    // open the graphics file
    FILE *screen_img = fopen(txt_path, "r");
    if (screen_img == NULL)
    {
        printf("null file pointer\n");
        fflush(stdout);
    }

    // check if the window is big enough
    while (Srow < txt_row+5 || Scol < txt_col)
    {
        clear();
        mvaddstr((Srow / 2), ((Scol - strlen(resisize_request)) / 2), resisize_request);
        refresh();

        getmaxyx(stdscr, Srow, Scol);
    }

    //screen reset
    clear();
    refresh();

    //create the window
    socket_info_window = create_new_window(3, Scol, Srow-3, 0);
    rule_window = create_new_window(Srow-3, Scol, 0, 0);
    

    // print the rules
    int indx = (Srow - txt_row - 3)/2;
    while ((fgets(rule_line, sizeof(rule_line), screen_img)) != NULL)
    {
        mvwprintw(rule_window, indx, (Scol - strlen(rule_line)) / 2, "%s", rule_line);
        indx++;
    }

    // print the socket message info
    sprintf(socket_info, "Insert arguments for socket connection: ");
    mvwprintw(socket_info_window, getmaxy(socket_info_window)/2, 1, "%s", socket_info);
    wmove(socket_info_window, getmaxy(socket_info_window)/2, strlen(socket_info) + 1);
   
    wrefresh(rule_window);
    wrefresh(socket_info_window);

    do{
        wgetnstr(socket_info_window, buffer, sizeof(char)*100);
        
        if(strlen(buffer) == 0){
            wmove(socket_info_window, getmaxy(socket_info_window)/2, strlen(socket_info) + 1);
        }

    }while(strlen(buffer) < 5);

    // remove the new line character
    buffer[strcspn(buffer, "\n")] = 0;

    // close the file
    fclose(screen_img);
}

WINDOW *create_new_window(int row, int col, int ystart, int xstart)
{
    WINDOW *local_window = newwin(row, col, ystart, xstart);
    box(local_window, 0, 0);

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