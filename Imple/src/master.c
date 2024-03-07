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
#include "arplib.h"
#include "../config/config.h"

int string_parser_master(char *string, char *first_arg, char *second_arg);

int main()
{
    /* The master spawn all the process  in oreder server, input, drone, target, obstacle, with watchdog at the end
    also create the necessary pipe for the communication between process*/

    // inizialize the variabiles needed
    // all proces without wd
    int num_ps = PROCESS_NUMBER; // cambiato process number da 5 a 6 (aggiunto il server socket)
    // all process plus watchdog
    int tot_ps = num_ps + 1;
    // array with all the pid of the process execute by master throw execvp
    pid_t child_pids[tot_ps];
    // array with all the pid sended back by process, they are different respect child_pids if the procss was execute throw konsole
    pid_t child_pids_received[num_ps];
    // array with the pid converted in string, the wd is not converted in string
    char str_child_pids[num_ps][20];
    char str_child_pids_received[num_ps][20];
    // inizialize variable for all the forcycle
    int i;

    // Inizialize the log file with mode w, all the data inside will be delete
    FILE *logfile = fopen("../log/logfile.txt", "w");
    if (logfile < 0)
    { // if problem opening file, send error
        error("master: fopen logfile");
    }
    writeLog("MASTER is create with pid %d ", getpid());

    // manage pipe------------------------------------------------------------------------
    // Pipe for comommunication between server -> master for send back the pid
    int fd1[2];
    char str_fd1[2][20];

    create_pipe(fd1, str_fd1, "master: pipe fd1");

    // Pipe for comommunication between input -> master
    int fd2[2];
    char str_fd2[2][20];

    create_pipe(fd2, str_fd2, "master: pipe fd2");

    // Pipe for comommunication between drone -> master
    int fd3[2];
    char str_fd3[2][20];

    create_pipe(fd3, str_fd3, "master: pipe fd3");

    // Pipe for comommunication between target -> master
    int fd4[2];
    char str_fd4[2][20];

    create_pipe(fd4, str_fd4, "master: pipe fd4");

    // Pipe for communication between obstacle -> master
    int fd5[2];
    char str_fd5[2][20];

    create_pipe(fd5, str_fd5, "master: pipe fd5");

    // Pipe for communication between master -> rule_print
    int fd6[2];
    char str_fd6[2][20];
    create_pipe(fd6, str_fd6, "master: pipe fd6");

    // Pipe for communication between master -> soket server
    int fd7[2];
    char str_fd7[2][20];
    create_pipe(fd7, str_fd7, "master: pipe fd7");

    // write in log for debug
    writeLog("MASTER send to server -------- fd1 file desc: %d, %d ", fd1[0], fd1[1]);
    writeLog("MASTER send to input --------- fd2 file desc: %d, %d ", fd2[0], fd2[1]);
    writeLog("MASTER send to drone --------- fd3 file desc: %d, %d ", fd3[0], fd3[1]);
    writeLog("MASTER send to target -------- fd4 file desc: %d, %d ", fd4[0], fd4[1]);
    writeLog("MASTER send to obstacle ------ fd5 file desc: %d, %d ", fd5[0], fd5[1]);
    writeLog("MASTER send to rule print ---- fd6 file desc: %d, %d ", fd6[0], fd6[1]);
    writeLog("MASTER send to socket server - fd7 file desc: %d, %d ", fd7[0], fd7[1]);

    //// Pipe for communication between INPUT and SERVER
    int fdi_s[2];
    char str_fdi_s[2][20];

    create_pipe(fdi_s, str_fdi_s, "master: pipe fdi_s");

    //// Pipe for communication between DRONE and SERVER
    int fdd_s[2];
    char str_fdd_s[2][20];

    create_pipe(fdd_s, str_fdd_s, "master: pipe fdd_s");

    //// Pipe for communication between SERVER and DRONE
    int fds_d[2];
    char str_fds_d[2][20];

    create_pipe(fds_d, str_fds_d, "master: pipe fds_d");

    //// Pipe for communication between TARGET and SERVER ---->> diventa TARGET e SOCKET SERVER
    int fdt_s[2];
    char str_fdt_s[2][20];

    create_pipe(fdt_s, str_fdt_s, "master: pipe fdt_s");

    //// Pipe for comunication between OBSTACLE and SERVER ---->> diventa OBSTACLE e SOCKET SERVER
    int fdo_s[2];
    char str_fdo_s[2][20];

    create_pipe(fdo_s, str_fdo_s, "master: pipe fdo_s");

    //// Pipe for comunication between SOCKET SERVER and SERVER send Target
    int fdss_s_t[2];
    char str_fdss_s_t[2][20];

    create_pipe(fdss_s_t, str_fdss_s_t, "master: pipe fdss_s_t");

    //// Pipe for comunication between SERVER and SOCKET SERVER send Obstacle
    int fdss_s_o[2];
    char str_fdss_s_o[2][20];

    create_pipe(fdss_s_o, str_fdss_s_o, "master: pipe fdss_s_o");

    //// Pipe for comunication between SERVER and SOCKET SERVER send Window_Size
    int fds_ss[2];
    char str_fds_ss[2][20];

    create_pipe(fds_ss, str_fds_ss, "master: pipe fds_ss");

    //// Pipe for comunication between MASTER and SOCKET SERVER
    int rule_pipe[2];
    char str_rule_pipe[2][20];

    create_pipe(rule_pipe, str_rule_pipe, "master: pipe rule_pipe");

    int fdrp_ss[2];
    char str_fdrp_ss[2][20];

    create_pipe(fdrp_ss, str_fdrp_ss, "master: pipe str_fdrp_ss");

    // write log for debug
    writeLog("MASTER to server           fdi_s:     %d,%d   fdd_s: %d,%d fds_d: %d,%d    fdss_s_t: %d,%d fdss_s_o: %d,%d  fds_ss: %d,%d", fdi_s[0], fdi_s[1], fdd_s[0], fdd_s[1], fds_d[0], fds_d[1], fdss_s_t[0], fdss_s_t[1], fdss_s_o[0], fdss_s_o[1], fds_ss[0], fds_ss[1]);
    writeLog("MASTER to socket server    fdt_s:     %d,%d   fdo_s: %d,%d fdrp_ss: %d,%d  fdss_s_t: %d,%d fdss_s_o: %d,%d  fds_ss: %d,%d", fdt_s[0], fdt_s[1], fdo_s[0], fdo_s[1], fdrp_ss[0], fdrp_ss[1], fdss_s_t[0], fdss_s_t[1], fdss_s_o[0], fdss_s_o[1], fds_ss[0], fds_ss[1]);
    writeLog("MASTER to input            fdi_s:     %d,%d", fdi_s[0], fdi_s[1]);
    writeLog("MASTER to drone            fdd_s:     %d,%d   fds_d: %d,%d  ", fdd_s[0], fdd_s[1], fds_d[0], fds_d[1]);
    writeLog("MASTER to target           fdt_s:     %d,%d", fdt_s[0], fdt_s[1]);
    writeLog("MASTER to obstacle         fdo_s:     %d,%d", fdo_s[0], fdo_s[1]);
    writeLog("MASTER to rule             rule_pipe: %d,%d,  fdrp_ss: %d,%d", rule_pipe[0], rule_pipe[1], fdrp_ss[0], fdrp_ss[1]);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //              SPAWN PROCESS                                                                                                             //
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    //--- SOCKET SERVER process -------------------------------------------------------------------------------------------------
    // char *arg_list_socket_server_1[] = {"konsole", "-e", "./socket_server", first_arg, str_fd7[0], str_fd7[1], str_fdt_s[0], str_fdt_s[1], str_fdo_s[0], str_fdo_s[1], str_fdss_s_t[0], str_fdss_s_t[1], str_fdss_s_o[0], str_fdss_s_o[1], str_fds_ss[0], str_fds_ss[1], NULL};
    char *arg_list_socket[] = {"./socket", str_fd7[0], str_fd7[1], str_fdt_s[0], str_fdt_s[1], str_fdo_s[0], str_fdo_s[1], str_fdss_s_t[0], str_fdss_s_t[1], str_fdss_s_o[0], str_fdss_s_o[1], str_fds_ss[0], str_fds_ss[1], str_fdrp_ss[0], str_fdrp_ss[1], NULL};

    child_pids[5] = spawn("./socket", arg_list_socket);
    // printf("first_arg: %s\n\n\n", first_arg);

    writeLog("MASTER spawn socket server with pid: %d ", child_pids[5]);

    // recive the correct pid from socket server
    recive_correct_pid(fd7, &child_pids_received[5]);
    writeLog("MASTER RECIVED socket server real pid: %d ", child_pids_received[5]);

    // --- Rule printing -------------------------------------------------------------------------------------------------
    // variabili per recupero informazioni
    int retVal_read;
    char read_buffer[256];

    int rule_pid, real_rule_pid;
    char *arg_list_rule_print[] = {"konsole", "-e", "./rule_print", str_fd6[0], str_fd6[1], str_rule_pipe[0], str_rule_pipe[1], str_fdrp_ss[0], str_fdrp_ss[1], NULL};

    // create process and launch rule_print
    rule_pid = spawn("konsole", arg_list_rule_print);
    writeLog("MASTER spawn rule process with pid: %d ", rule_pid);

    // recive the correct pid from rule_print
    recive_correct_pid(fd6, &real_rule_pid);
    writeLog("MASTER RECIVED rule process real pid: %d ", real_rule_pid);

    // --- GAME SERVER process ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    // Server process is execute with konsole so, the child_pid(correspond to the pid of the kosole) and the child_pid_received( correspod to the pid of process)
    char *arg_list_server[] = {"konsole", "-e", "./server", str_fd1[0], str_fd1[1], str_fdi_s[0], str_fdi_s[1], str_fdd_s[0], str_fdd_s[1], str_fds_d[0], str_fds_d[1], str_fdss_s_t[0], str_fdss_s_t[1], str_fdss_s_o[0], str_fdss_s_o[1], str_fds_ss[0], str_fds_ss[1], NULL};
    child_pids[0] = spawn("konsole", arg_list_server);
    writeLog("MASTER spawn server with pid: %d ", child_pids[0]);

    recive_correct_pid(fd1, &child_pids_received[0]);
    writeLog("MASTER RECIVED server real pid: %d ", child_pids_received[0]);

    // --- INPUT process ----------------------------------------------------------------------------------------
    char *arg_list_i[] = {"konsole", "-e", "./input", str_fd2[0], str_fd2[1], str_fdi_s[0], str_fdi_s[1], NULL};
    child_pids[1] = spawn("konsole", arg_list_i);
    writeLog("MASTER spawn input with pid: %d ", child_pids[1]);

    recive_correct_pid(fd2, &child_pids_received[1]);
    writeLog("MASTER RECIVED input real pid: %d ", child_pids_received[1]);

    // --- DRONE process -------------------------------------------------------------------------------------------------
    char *arg_list_drone[] = {"./drone", str_fd3[0], str_fd3[1], str_fdd_s[0], str_fdd_s[1], str_fds_d[0], str_fds_d[1], NULL};
    child_pids[2] = spawn("./drone", arg_list_drone);
    writeLog("MASTER spawn drone with pid: %d ", child_pids[2]);

    recive_correct_pid(fd3, &child_pids_received[2]);
    writeLog("MASTER RECIVED drone real pid: %d ", child_pids_received[2]);

    //---- TARGET process -----------------------------------------------------------------------------------------------------
    char *arg_list_target[] = {"./target", str_fd4[0], str_fd4[1], str_fdt_s[0], str_fdt_s[1], NULL};
    child_pids[3] = spawn("./target", arg_list_target);
    writeLog("MASTER spawn target with pid: %d ", child_pids[3]);

    recive_correct_pid(fd4, &child_pids_received[3]);
    writeLog("MASTER RECIVED target real pid: %d ", child_pids_received[3]);

    //---- OBSTACLE process -------------------------------------------------------------------------------------------------------
    char *arg_list_obstacle[] = {"./obstacle", str_fd5[0], str_fd5[1], str_fdo_s[0], str_fdo_s[1], NULL};
    child_pids[4] = spawn("./obstacle", arg_list_obstacle);
    writeLog("MASTER spawn obstacle with pid: %d ", child_pids[4]);

    recive_correct_pid(fd5, &child_pids_received[4]);
    writeLog("MASTER RECIVED obstacle real pid: %d ", child_pids_received[4]);

    // Convert the array child_pids in string
    for (i = 0; i < num_ps; i++)
    {
        sprintf(str_child_pids[i], "%d", child_pids[i]);
    }
    // Convert the array child_pids_received in string
    for (i = 0; i < num_ps; i++)
    {
        sprintf(str_child_pids_received[i], "%d", child_pids_received[i]);
    }

    writeLog("MASTER: child_pids are: %s, %s, %s, %s, %s, %s ", str_child_pids[0], str_child_pids[1], str_child_pids[2], str_child_pids[3], str_child_pids[4], str_child_pids[5]);
    writeLog("MASTER child_pids_received are: %s, %s, %s, %s, %s, %s", str_child_pids_received[0], str_child_pids_received[1], str_child_pids_received[2], str_child_pids_received[3], str_child_pids_received[4], str_child_pids_received[5]);
    
    //------- WATCHDOG process --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    // spawn watchdog, and pass as argument all the pids of processes
    char *arg_list_wd[] = {"./wd", str_child_pids[0], str_child_pids[1], str_child_pids[2], str_child_pids[3], str_child_pids[4], str_child_pids[5], str_child_pids_received[0],str_child_pids_received[1], str_child_pids_received[2], str_child_pids_received[3], str_child_pids_received[4], str_child_pids_received[5], NULL};
    child_pids[num_ps] = spawn("./wd", arg_list_wd);
    writeLog("MASTER spawn WATCHDOG with pid: %d ", child_pids[num_ps]);
    // The master will wait until all the process will terminate

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //                  WAIT FOR THE PID OF THE SON                                                                         //
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    pid_t waitResult;
    int status;
    for (i = 0; i <= num_ps; i++)
    {
        waitResult = waitpid(child_pids[i], &status, 0);
        if (waitResult == -1)
        {
            error("master: waitpid ");

        }
        if (WIFEXITED(status))
        {
            writeLog("Process %d is termined with status %d\n", i, WEXITSTATUS(status));
            writeLog_wd("Process %d is termined with status %d\n", i, WEXITSTATUS(status));
            printf("Process %d is termined with status %d\n", i, WEXITSTATUS(status));
            fflush(stdout);
        }
        else
        {
            writeLog("Process %d is termined with anomaly\n", i);
            writeLog_wd("Process %d is termined with anomaly\n", i);
            printf("Process %d is termined with anomaly\n", i);
            fflush(stdout);
        }
    }
    

    return 0;
}

int string_parser_master(char *string, char *first_arg, char *second_arg)
{
    // define the char that separate the arguments in the string
    char *separator = " ";
    char *arg;
    int ret_val;

    arg = strtok(string, separator);
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