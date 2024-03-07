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
#include <errno.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include "arplib.h"
#include "../config/config.h"

//-- Functions header --------------------------------------------
void sigusr1Handler(int signum, siginfo_t *info, void *context);

int main(int argc, char *argv[])
{
    // variable used in for cycle
    int i;
    pid_t obstacle_pid = getpid();
    // write into logfile the pid
    writeLog("OBSTACLE create with pid %d ", obstacle_pid);

    // configure the handler for sigusr1
    struct sigaction sa_usr1;
    sa_usr1.sa_sigaction = sigusr1Handler;
    sa_usr1.sa_flags = SA_SIGINFO;

    if (sigaction(SIGUSR1, &sa_usr1, NULL) == -1)
    {
        error("obstacle: sigaction");
    }

    //// -- manage pipe ----------------------------------------------------------
    // Take the fd for communicating with master; its positions are 1,2 in argv[]
    int fd5[2];
    for (i = 1; i < 3; i++)
    {
        fd5[i - 1] = atoi(argv[i]);
    }
    // close the read file descriptor fd1[0]
    closeAndLog(fd5[0], "obstacle: close fd5[0]");
    // write the pid in the pipe
    if (write(fd5[1], &obstacle_pid, sizeof(obstacle_pid)) < 0)
    {
        error("obstacle: write fd5[1]");
    }
    closeAndLog(fd5[1], "obstacle: close fd5[1]");

    int fdo_s[2];
    for (i = 3; i < 5; i++)
    {
        fdo_s[i - 3] = atoi(argv[i]);
    }
    // close the read file descriptor
    closeAndLog(fdo_s[0], "obstacle: close fdo_s[0] ");

    // print all the file descriptor received on logfile
    writeLog("OBSTACLE value of fd5 are: %d %d ", fd5[0], fd5[1]);
    writeLog("OBSTACLE value of fdo_s are: %d %d ", fdo_s[0], fdo_s[1]);

    // initialize the time on random generator
    time_t t = 0;
    srand(time(NULL));
    int random_number = 0;
    double set_of_obstacle[MAX_OBST_ARR_SIZE][2] = {0};

    // define write return value
    int retVal_write;

    while (1)
    {
        // set an random integer from 0 to MAX_OBST_ARR_SIZE
        time_t t = time(NULL);
        for (i = 0; i < MAX_OBST_ARR_SIZE; i++)
        {
            random_number = rand() % 2;
            if(random_number == 0){
                set_of_obstacle[i][0] = (double)rand() / RAND_MAX;
                set_of_obstacle[i][1] = (double)rand() / RAND_MAX;
            }else{
                set_of_obstacle[i][0] = 0;
                set_of_obstacle[i][1] = 0;
            }
        }

        // avoid system call interruption by signal
        do
        {
            retVal_write = write(fdo_s[1], set_of_obstacle, sizeof(double) * MAX_OBST_ARR_SIZE * 2);
        } while (retVal_write == -1 && errno == EINTR);
        // general write error
        if (retVal_write < 0)
        {
            error("obstacle: error write fdo_s[1]");
        }
        // check written byte inside the pipe
        // writeLog("OBSTACLE: write %d bytes in fdo_s[1]", retVal_write);
        // generate obstacle every N seconds; implement a non-blocking timer to avoid problems with signals
        time_t t2 = time(NULL);
        while ((t2 - t) < N)
        {
            t2 = time(NULL);
        }

        // reset the obstacle set to zero
        for(i = 0; i < MAX_OBST_ARR_SIZE; i++){
            set_of_obstacle[i][0] = 0;
            set_of_obstacle[i][1] = 0;
        } 
    }
    // close the write file descriptor fdo_s
    closeAndLog(fdo_s[1], "obstacle: close fdo_s[1]");
    exit(EXIT_SUCCESS); // Not reached, but included for completeness
}

//// --- Function section -------------------------------------------------------------

void sigusr1Handler(int signum, siginfo_t *info, void *context)
{
    if (signum == SIGUSR1)
    {
        /*send a signal SIGUSR2 to watchdog */
        if (kill(info->si_pid, SIGUSR2) == 0)
        {
            writeLog_wd("OBSTACLE: pid %d, received signal from wd pid: %d ", getpid(), info->si_pid);
        }
        else
        {
            error("obstacle: kill SIGUSR2 ");
        }
    }
}
