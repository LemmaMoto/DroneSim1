#include <ncurses.h>
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
#include <sys/shm.h>
#include <sys/ipc.h>
#include <math.h>
#include <errno.h>
#include "arplib.h"
#include "../config/config.h"

void sigusr1Handler(int signum, siginfo_t *info, void *context);

// int sign(int x);
WINDOW *create_new_window(int row, int col, int ystart, int xstart); // creazione delle finestre

void print_screen(char *txt_path, int txt_row, int txt_col);

void destroy_win(WINDOW *local_win);

int signum(int x);

int main(int argc, char *argv[])
{
    // variable usefull for the for cycle
    int i;
    // actual pid of the server
    pid_t server_pid = getpid();
    // write into logfile
    writeLog("SERVER create with pid %d ", server_pid);

    // configure the handler for sigusr1
    struct sigaction sa_usr1;
    sa_usr1.sa_sigaction = sigusr1Handler;
    sa_usr1.sa_flags = SA_SIGINFO;

    if (sigaction(SIGUSR1, &sa_usr1, NULL) == -1)
    {
        error("server: sigaction");
    }

    ///////////////////////////////////////////////////////////////////////////////////////
    //                        MANAGE PIPE                                                //
    ///////////////////////////////////////////////////////////////////////////////////////

    // Take the fd for comunicating with master, it's position is 1,2 in argv[]
    int fd1[2];
    for (i = 1; i < 3; i++)
    {
        fd1[i - 1] = atoi(argv[i]);
    }
    // close the read fiel descriptor fd1[0]
    closeAndLog(fd1[0], "server: close fd1[0]");
    // write the pid in the pipe
    if (write(fd1[1], &server_pid, sizeof(server_pid)) < 0)
    {
        error("server: write fd1[1]");
    }
    closeAndLog(fd1[1], "server: close fd1[1]");

    //// Take the fdi_s for comunication input-server, fdi_s are in positions 3, 4
    int fdi_s[2];
    for (i = 3; i < 5; i++)
    {
        fdi_s[i - 3] = atoi(argv[i]);
    }
    // close the write file descriptor fdi_s[1], server only read from input
    closeAndLog(fdi_s[1], "server: close fdi_s[1]");

    //// Take the fdd_s for comunication drone-server, fdd_s are in positions 5, 6
    // This pipe is usefull for send and receive data from server to drone (is used for dynamics)
    int fdd_s[2];
    for (i = 5; i < 7; i++)
    {
        fdd_s[i - 5] = atoi(argv[i]);
    }
    closeAndLog(fdd_s[1], "server: close fdd_s[1]");

    //// Take the fds_t dor the comunication between server -> drone, are in positions 11, 12
    int fds_d[2];
    for (i = 7; i < 9; i++)
    {
        fds_d[i - 7] = atoi(argv[i]);
    }
    closeAndLog(fds_d[0], "server: close fds_d[0]");

    //// Take the fdss_s_t for comunication between socket_server-server-target, position 7, 8
    int fdss_s_t[2];
    for (i = 9; i < 11; i++)
    {
        fdss_s_t[i - 9] = atoi(argv[i]);
    }
    // close the write file descriptor fdss_s_t[1], server only read from target
    closeAndLog(fdss_s_t[1], "server: close fdss_s_t[1]");

    //// Take the fdss_s_o for comunication between obstacle-server, position 9, 10
    int fdss_s_o[2];
    for (i = 11; i < 13; i++)
    {
        fdss_s_o[i - 11] = atoi(argv[i]);
    }
    // close the write file descriptor fdss_s_o[1], server only read from object
    closeAndLog(fdss_s_o[1], "server: close fdss_s_o[1]");

    // Take the fds_ss for comunication between server -> socket_server, are in positions 13, 14
    int fds_ss[2];
    for (i = 13; i < 15; i++)
    {
        fds_ss[i - 13] = atoi(argv[i]);
    }
    // close the read file descriptor fds_ss[0, server only write from socket_server
    closeAndLog(fds_ss[0], "server: close fds_ss[0]");

    writeLog("SERVER value of fd1 are:      %d %d ", fd1[0], fd1[1]);
    writeLog("SERVER value of fdi_s are:    %d %d ", fdi_s[0], fdi_s[1]);
    writeLog("SERVER value of fdd_s are:    %d %d ", fdd_s[0], fdd_s[1]);
    writeLog("SERVER value of fds_d are:    %d %d ", fds_d[0], fds_d[1]);
    writeLog("SERVER value of fdss_s_t are: %d %d ", fdss_s_t[0], fdss_s_t[1]);
    writeLog("SERVER value of fdss_s_o are: %d %d ", fdss_s_o[0], fdss_s_o[1]);
    writeLog("SERVER value of fds_ss are:   %d %d ", fds_ss[0], fds_ss[1]);

    //////////////////////////////////////////////////////////////////////////////////////////////////////
    //                    VARIABLE FOR DYNAMICS                                                         //
    //////////////////////////////////////////////////////////////////////////////////////////////////////
    // In our window all the obstacle and targer are printed like int for semplicity, also the drone position is an integer

    // variable forthe positions of target
    double set_of_target[MAX_TARG_ARR_SIZE][2] = {{0.0}};
    int int_set_of_target[MAX_TARG_ARR_SIZE][2] = {{0}};

    // variable for the position of obstacle and
    double set_of_obstacle[MAX_OBST_ARR_SIZE][2] = {{0.0}};
    int int_set_of_obstacle[MAX_OBST_ARR_SIZE][2] = {{0}};

    // drone position, from pipe fdd_s
    double dronePosition[2] = {0};
    int int_dronePosition[2] = {0};

    // input force, cames from pipe fdi_s
    double inputForce[2] = {0.0};

    // variable for the computation of the forces
    double obstForce[2] = {0.0};
    double totalForce[2] = {0.0};

    // variabili per il calcolo delle forze
    int distance[2] = {0};
    int counter = 0;

    int result;

    // variable for select
    int retVal_sel;
    int retVal_read;
    fd_set read_fd;
    struct timeval time_sel;

    // define the array wit all the read file descritor (fdd_s: drone -> server, fdss_s_o: obstacle -> server, fdi_s: input -> server)
    int fd_array[3] = {fdd_s[0], fdss_s_o[0], fdi_s[0]};
    // find the maximum fd
    int max_fd;
    max_fd = (fdd_s[0] > fdss_s_o[0]) ? fdd_s[0] : fdss_s_o[0];
    max_fd = (max_fd > fdi_s[0]) ? max_fd : fdi_s[0];

    // variabili di controllo per evitare di ristampare la mappa al ritmo del while
    int new_position, new_obstacles;

    //////////////////////////////////////////////////////////////////////////////////////////
    //                     NCURSES INITIALIZATION                                           //
    //////////////////////////////////////////////////////////////////////////////////////////
    // print window for the map
    WINDOW *spawn_window;
    // parte legata ad ncurses per il server
    int Srow, Scol; // righe e colonne massime dello schermo
    int Srow_new, Scol_new;
    int rowSH, colSH;
    int retVal_write;
    char termin_msg[MAX_MSG_LENGHT] = "GE";
    // initialization row
    initscr();
    raw();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    // initialize color in the terminal
    start_color();
    // define the color pair
    init_pair(1, COLOR_RED, COLOR_BLACK);
    init_pair(2, COLOR_GREEN, COLOR_BLACK);
    box(stdscr, 0, 0);
    refresh();
    getmaxyx(stdscr, Srow, Scol);

    // window for printing the drone, obst, targh
    int spawn_Col = Scol - 2;
    int spawn_Row = Srow - 2;
    int window_size[2] = {spawn_Row, spawn_Col};
    writeLog("spawn_Col = %i, spawn_Row = %i", spawn_Col, spawn_Row);

    ///////////////////// write to socket server /////////////////////////////////////////////////////////////
    if (write(fds_ss[1], window_size, sizeof(int) * 2) == -1)
    {
        error("server: write fd1[1]");
    }

    rowSH = spawn_Row / 2; // definisco gli shift per traslare (0,0) al centro dello schermo
    colSH = spawn_Col / 2;
    writeLog("rowSH = %i, rowSH = %i", rowSH, colSH);

    // printing window,avoid reprint fliker
    spawn_window = newwin(spawn_Row, spawn_Col, 1, 1);
    wrefresh(spawn_window);

    // read the set of target (blocking read)
    do
    {
        retVal_read = read(fdss_s_t[0], set_of_target, sizeof(double) * MAX_TARG_ARR_SIZE * 2);
    } while (retVal_read == -1 && errno == EINTR);
    if (retVal_read == -1)
    {
        error("server: read fdss_s_t[0]");
    }
    else
    {
        // checking readed byte in pipe
        writeLog("SERVER: read %d bytes in fdss_s_t[0]", retVal_read);
    }

    // moltiply the target for the reference system. I use the same reference system of the drone
    // The obstacle are between -1 and 1
    int condition_victory = 0; // couter to control if the targhet are all collected
    for (i = 0; i < MAX_TARG_ARR_SIZE; i++)
    {
        int_set_of_target[i][0] = (int)((set_of_target[i][0] - 0.5) * spawn_Col);
        int_set_of_target[i][1] = (int)((set_of_target[i][1] - 0.5) * spawn_Row);

        if (set_of_target[i][0] < 0.5 && set_of_target[i][1] < 0.5)
        {
            condition_victory++;
        }
        writeLog("SERVER: converted target %d, x = %d, y = %d, target %lf, %lf", i, int_set_of_target[i][0], int_set_of_target[i][1], set_of_target[i][0], set_of_target[i][1]);
    }
    writeLog("SERVER: condition_victory = %d", condition_victory);

    //////////////////////////////////////////////////////////////////////////////////////////////////////
    //                  INFINITE LOOP                                                                   //
    //////////////////////////////////////////////////////////////////////////////////////////////////////
    while (1)
    {
        // azzero controlli di stampa
        new_position = 0;
        new_obstacles = 0;

        ////////////////////////////////////////////////////////////////////////////////////////////////
        //              PART RELATED TO SELECT FOR CHOSE THE READ FILE DESCRIPTOR                     //
        ////////////////////////////////////////////////////////////////////////////////////////////////
        // define the set of fd
        FD_ZERO(&read_fd);
        FD_SET(fdd_s[0], &read_fd);
        FD_SET(fdss_s_o[0], &read_fd);
        FD_SET(fdi_s[0], &read_fd);

        // time interval for select
        time_sel.tv_sec = 0;
        time_sel.tv_usec = 3000;
        // do-while statement for avoid problem with signals
        do
        {
            retVal_sel = select(max_fd + 1, &read_fd, NULL, NULL, &time_sel);
        } while (retVal_sel == -1 && errno == EINTR);
        // select for check the value
        if (retVal_sel == -1)
        {
            error("server: error select: ");
        }
        else if (retVal_sel > 0)
        // case there is some data in the observed file descriptor
        {
            // check wich file descriptor have data inside
            for (i = 0; i < (sizeof(fd_array) / sizeof(int)); i++)
            {
                // check if the fd is inside the ready file descriptor set
                if (FD_ISSET(fd_array[i], &read_fd))
                {

                    if (fd_array[i] == fdd_s[0]) // <<<< drone - server >>>>
                    {
                        // read the position from the drone process
                        do
                        {
                            retVal_read = read(fdd_s[0], dronePosition, sizeof(double) * 2);
                        } while (retVal_read == -1 && errno == EINTR);
                        if (retVal_read == -1)
                        {
                            error("server: read fdd_s[0]");
                        }
                        else
                        {
                            // convert the position in integer
                            int_dronePosition[0] = (int)dronePosition[0];
                            int_dronePosition[1] = (int)dronePosition[1];
                            // variable for print
                            new_position = 1;
                        }
                    }
                    else if (fd_array[i] == fdss_s_o[0]) // <<<< obstacle - server >>>>
                    {
                        // read the set of obstacle
                        do
                        {
                            retVal_read = read(fdss_s_o[0], set_of_obstacle, sizeof(double) * MAX_OBST_ARR_SIZE * 2);
                        } while (retVal_read == -1 && errno == EINTR);
                        if (retVal_read == -1)
                        {
                            error("server: read fdss_s_o[0]");
                        }
                        else
                        {
                            // checking readed byte in pipe
                            writeLog("SERVER: read %d bytes in fdss_s_o[0]", retVal_read);
                            // moltiply the obstacle for the windows size --> get int position of obstacle
                            for (i = 0; i < MAX_OBST_ARR_SIZE; i++)
                            {
                                int_set_of_obstacle[i][0] = (int)((set_of_obstacle[i][0] - 0.5) * spawn_Col);
                                int_set_of_obstacle[i][1] = (int)((set_of_obstacle[i][1] - 0.5) * spawn_Row);
                            }
                            new_obstacles = 1; // set the flag for print the map
                        }
                    }
                    else if (fd_array[i] == fdi_s[0]) // <<<< input - server >>>>
                    {
                        // read the input force
                        do
                        {
                            retVal_read = read(fdi_s[0], inputForce, sizeof(double) * 2);
                        } while (retVal_read == -1 && errno == EINTR);
                        if (retVal_read == -1)
                        {
                            error("server: read fdi_s[0]");
                        }
                    }
                    else
                    {
                        break;
                    }
                }
            }
        }
        //////////////////////////////////////////////////////////////////////////////////////////////////////
        //          COMPUTE THE FORCE ON THE DRONE                                                          //
        //////////////////////////////////////////////////////////////////////////////////////////////////////
        // useful variables
        // initilize every loop the distance obj-drone to zero
        distance[0] = 0;
        distance[1] = 0;
        // each cycle i have to reset the obstacle force
        obstForce[0] = 0.0;
        obstForce[1] = 0.0;

        //---------- OBSTACLE REPULSIVE FORCE --------------------------------------------------------------------------
        // for now I am conidering the total force so for the first iteration the drone don't have repulsive force
        for (i = 0; i < MAX_OBST_ARR_SIZE; i++)
        {
            // calculating the distance between drone and obstacle
            distance[0] = int_dronePosition[0] - int_set_of_obstacle[i][0];
            distance[1] = int_dronePosition[1] - int_set_of_obstacle[i][1];

            // For compute the force, for avoid overflow conider as less distance from point = 2 and if the distance is > 10 don't consider force
            // Consider first the x coordinate

            if (abs(distance[0]) > OBST_RADIUS_FAR || abs(distance[1]) > OBST_RADIUS_FAR)
            {
                // case when the drone is too far from obstacel, don't add any repulsive force
                obstForce[0] = obstForce[0] + 0; // don't add any force
                obstForce[1] = obstForce[1] + 0; // don't add any force
            }
            else if (abs(distance[0]) < OBST_RADIUS_CLOSE)
            // case when the drone is in the nearest circle around the obstacle, assign a default force
            {
                // case when the drone is too near the obstacle
                obstForce[0] = obstForce[0] + (sign(distance[0]) * 6); //(K_CLOSE * abs(inputForce[0])));
            }
            else
            {
                // general case when the drone is OBST_RADIUS_CLOSE < drone < OBST_RADIUS_FAR
                obstForce[0] = obstForce[0] + (sign(distance[0]) * ((K_STD * abs(inputForce[0])) / pow(distance[0], 2)));
            }

            // consider now the y coordinate

            if (abs(distance[0]) > OBST_RADIUS_FAR || abs(distance[1]) > OBST_RADIUS_FAR)
            {
                // case when the drone is too far form obstacle, no repulsive force
                obstForce[0] = obstForce[0] + 0; // don't add any force
                obstForce[1] = obstForce[1] + 0; // don't add any force
            }
            else if (abs(distance[1]) < OBST_RADIUS_CLOSE)
            {
                // rone to near to the obstacle,risk an overflow, so limit the distance
                obstForce[1] = obstForce[1] + (sign(distance[1]) * 6); //(K_CLOSE * abs(inputForce[1])));
            }
            else
            {
                // general case when the drone is OBST_RADIUS_CLOSE < drone < OBST_RADIUS_FAR
                obstForce[1] = obstForce[1] + (sign(distance[1]) * ((K_STD * abs(inputForce[1])) / pow((distance[1]), 2)));
            }
        }

        //---- COMPUTE TOTAL FORCE ---------------------------------------------------------------------------------------------
        totalForce[0] = inputForce[0] + obstForce[0];
        totalForce[1] = inputForce[1] + obstForce[1];

        ///////////////////////////////////////////////////////////////////////////////////////////////////////
        //          CONTROL THE EDGE OF THE WIDOW                                                            //
        ///////////////////////////////////////////////////////////////////////////////////////////////////////
        // control that the drone is not outside the upper and lower edge of window
        if (abs(rowSH - int_dronePosition[1]) == 0)
        {
            totalForce[1] = -30;
        }
        else if ((rowSH - int_dronePosition[1]) == spawn_Row)
        {
            totalForce[1] = +30;
        }
        // control thet the drone is inide the right and left edge of the window
        if ((colSH + int_dronePosition[0]) == 0)
        {
            totalForce[0] = +60;
        }
        else if ((colSH + int_dronePosition[0]) == spawn_Col)
        {
            totalForce[0] = -60;
        }

        // write totalForce to drone
        // check for numeric error in total force X
        if (isnan(totalForce[0]) || isinf(totalForce[0]) || isnan(totalForce[1]) || isinf(totalForce[1]))
        {
            writeLog("server: total force error, nan or inf");
        }
        else
        {
            // write the total force to drone
            do
            {
                result = write(fds_d[1], totalForce, sizeof(double) * 2);
            } while (result == -1 && errno == EINTR);
            if (write(fds_d[1], totalForce, sizeof(double) * 2) == -1)
            {
                error("server: write fds_d[1]");
            }
        }
        //////////////////////////////////////////////////////////////////////////
        //               SCREEN UPDATING SECTION                                //
        //////////////////////////////////////////////////////////////////////////
        if (new_obstacles == 1 || new_position == 1)
        {
            // clear the map window
            wclear(spawn_window);

            // print the drone icon
            // activate attribute for printing
            wattr_on(spawn_window, A_STANDOUT, NULL);
            mvwaddch(spawn_window, rowSH - int_dronePosition[1], colSH + int_dronePosition[0], DRONE_ICON);
            // deactivate attribute for printing
            wattr_off(spawn_window, A_STANDOUT, NULL);

            // print the obstacle
            wattr_on(spawn_window, COLOR_PAIR(1), NULL);
            for (i = 0; i < MAX_OBST_ARR_SIZE; i++)
            {
                mvwaddch(spawn_window, rowSH - int_set_of_obstacle[i][1], colSH + int_set_of_obstacle[i][0], 'O');
            }
            wattr_off(spawn_window, COLOR_PAIR(1), NULL);

            // print the target
            wattr_on(spawn_window, COLOR_PAIR(2), NULL);
            for (i = 0; i < MAX_TARG_ARR_SIZE; i++)
            {
                if (int_set_of_target[i][0] != -1000 && int_set_of_target[i][1] != -1000)
                {
                    if (abs(int_dronePosition[0] - int_set_of_target[i][0]) <= 1 && abs(int_dronePosition[1] - int_set_of_target[i][1]) <= 1)
                    {
                        // set the value to -1, aka target reached
                        int_set_of_target[i][0] = -1000;
                        int_set_of_target[i][1] = -1000;
                        counter++;
                    }
                    else
                    {
                        mvwaddch(spawn_window, rowSH - int_set_of_target[i][1], colSH + int_set_of_target[i][0], 'T');
                    }
                }
                writeLog("SERVER: target %d reached, counter = %d", i, counter);
            }
            wattr_off(spawn_window, COLOR_PAIR(2), NULL);

            // refresch of the ncurses window
            wrefresh(spawn_window);
            if ((counter + condition_victory) == MAX_TARG_ARR_SIZE) //PROBLEMA --> se ci mandano meno di 20 targhet siamo fottuti --> non vinciamo mai
            {
                print_screen("../config/winScreen.txt", 6, 87);

                // game end handling --> writing to the socket server that the game has ended///
                if((retVal_write = write(fds_ss[1], termin_msg, sizeof(termin_msg))) < 0){
                    writeLog("SERVER: write error, termination message, byte %d, %m", retVal_write);
                }else{
                    writeLog(" =====> SERVER: write termination message to socket server");
                }

                // close all the open fd
                closeAndLog(fds_d[1], "server: close fds_d[1]");
                closeAndLog(fdi_s[0], "server: close fdi_s[0]");
                closeAndLog(fdss_s_o[0], "server: close fdss_s_o[0]");
                closeAndLog(fdd_s[0], "server: close fdd_s[0]");
                //
                exit(EXIT_SUCCESS);
                // after closing win screen, the server will be closed
            }
        }
        // resize ncurses window
        getmaxyx(stdscr, Srow_new, Scol_new);
        if (Srow_new != Srow || Scol_new != Scol)
        {
            // use the new dimension to create the new environment
            Srow = Srow_new;
            Scol = Scol_new;

            // clear the screen
            clear();

            spawn_Col = Scol - 2;
            spawn_Row = Srow - 2;

            rowSH = spawn_Row / 2; // definisco gli shift per traslare (0,0) al centro dello schermo
            colSH = spawn_Col / 2;

            // recreating the environment
            box(stdscr, 0, 0);

            // destroy the old window
            destroy_win(spawn_window);

            // spawn the new window in the new size
            spawn_window = newwin(spawn_Row, spawn_Col, 1, 1);

            // obtain the new position of the target
            for (i = 0; i < MAX_TARG_ARR_SIZE; i++)
            {
                int_set_of_target[i][0] = (int)(set_of_target[i][0] * spawn_Col);
                int_set_of_target[i][1] = (int)(set_of_target[i][1] * spawn_Row);
            }

            // refresh the screen and the window
            wrefresh(stdscr);
            wrefresh(spawn_window); // ends up on top of the screen
        }

    } // while(1) end --> if all the target are reached, we exit from this cycle

    // close all the open file desciptor
    closeAndLog(fds_d[1], "server: close fds_d[1]");
    closeAndLog(fdi_s[0], "server: close fdi_s[0]");
    closeAndLog(fdss_s_o[0], "server: close fdss_s_o[0]");
    closeAndLog(fdd_s[0], "server: close fdd_s[0]");
    closeAndLog(fds_ss[0], "server: close fds_ss[0]");

    return 0;
}

//////////////////////////////////////////////////////////////////////////
//                    FUNCTION SECTION                                  //
//////////////////////////////////////////////////////////////////////////
void sigusr1Handler(int signum, siginfo_t *info, void *context)
{
    if (signum == SIGUSR1)
    {
        // send a signal SIGUSR2 to watchdog
        if (kill(info->si_pid, SIGUSR2) == -1)
        {
            error("server: kill SIGUSR2");
        }
        writeLog_wd("SERVER, pid %d, received signal from wd pid: %d ", getpid(), info->si_pid);
    }
}

void print_screen(char *txt_path, int txt_row, int txt_col)
{
    FILE *screen_img = fopen(txt_path, "r");
    if (screen_img == NULL)
    {
        printf("null file pointer\n");
        fflush(stdout);
    }

    int Srow, Scol;
    getmaxyx(stdscr, Srow, Scol);
    char start_char = '?';
    char resisize_request[] = "please resize the window";
    char rule_line[100];
    // print rules
    while (Srow < txt_row || Scol < txt_col)
    {
        clear();
        mvaddstr((Srow / 2), ((Scol - strlen(resisize_request)) / 2), resisize_request);
        refresh();

        getmaxyx(stdscr, Srow, Scol);
    }

    clear();
    refresh();

    int indx = 0;
    int indx_offset = (Srow - txt_row) / 2; // da modificare se cambia il rule.txt
    while ((fgets(rule_line, sizeof(rule_line), screen_img)) != NULL)
    {
        mvprintw(indx + indx_offset, (Scol - strlen(rule_line)) / 2, "%s", rule_line);
        indx++;
    }

    refresh();
    fclose(screen_img);

    while ((start_char = getch()) != ' ')
    {
        // don't do anything
    }

    clear();
    refresh();
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
