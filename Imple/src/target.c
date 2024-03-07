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
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <stdarg.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include "arplib.h"
#include "../config/config.h"

// Functions header
void sigusr1Handler(int signum, siginfo_t *info, void *context);

int main(int argc, char *argv[])
{
    // variable used in for cycle
    int i;
    pid_t target_pid = getpid();
    // write into logfile the pid
    writeLog("TARGET created with pid %d ", target_pid);

    // configure the handler for sigusr1
    struct sigaction sa_usr1;
    sa_usr1.sa_sigaction = sigusr1Handler;
    sa_usr1.sa_flags = SA_SIGINFO;

    if (sigaction(SIGUSR1, &sa_usr1, NULL) == -1)
    {
        error("target: sigaction");
    }

    // Take the fd for comunicating with master, it's position is 1,2 in argv[]
    int fd4[2];
    for (i = 1; i < 3; i++)
    {
        fd4[i - 1] = atoi(argv[i]);
    }
    // close the read fiel descriptor fd4[0]
    closeAndLog(fd4[0], "target: close fd4[0]");

    // write the pid in the pipe, sended to master
    if (write(fd4[1], &target_pid, sizeof(target_pid)) < 0)
    {
        error("target: write fd4[1]");
    }
    closeAndLog(fd4[1], "target: close fd4[1]");

    //// pipe for communication between target->server, are in position 3, 4, only write target
    int fdt_s[2];
    for (i = 3; i < 5; i++)
    {
        fdt_s[i - 3] = atoi(argv[i]);
    }
    // close the read file descriptor, target only write data in the pipe
    closeAndLog(fdt_s[0], "target: close fdt_s[0] ");

    // print all the file descriptor received
    writeLog("TARGET value of fd4 are: %d %d ", fd4[0], fd4[1]);
    writeLog("TARGET value of fdt_s are: %d %d ", fdt_s[0], fdt_s[1]);

    double set_of_target[TARGET_NUMBER][2];
    for (int i = 0; i < TARGET_NUMBER; i++)
    {
        set_of_target[i][0] = ((double)rand() / RAND_MAX);
        set_of_target[i][1] = ((double)rand() / RAND_MAX);
    }

    // define write retVal
    int retVal_write;

    // avoid syscall beign blocked by a signal
    do
    {
        retVal_write = write(fdt_s[1], set_of_target, sizeof(double) * TARGET_NUMBER * 2);
    } while (retVal_write == -1 && errno == EINTR);
    // check for general errors
    if (retVal_write < 0)
    {
        error("obstacle: error write fdt_s[1]");
    }
    // checking written byte in pipe 
    // writeLog("TARGET: write %d bytes in fdt_s[1]", retVal_write);
    while (1)
    {
        sleep(1);
    }

    // close the write file descriptor, target only write data in the pipe
    closeAndLog(fdt_s[1], "target: close fdt_s[1] ");

    return 0; 
}

////---- Functions section -----------------------------------------------------------
/* function for write in logfile*/

void sigusr1Handler(int signum, siginfo_t *info, void *context)
{
    if (signum == SIGUSR1)
    {
        /*send a signal SIGUSR2 to watchdog */
        // printf("SERVER sig handler");
        if (kill(info->si_pid, SIGUSR2) == 0)
        {
            writeLog_wd("TARGET: pid %d, received signal from wd pid: %d ", getpid(), info->si_pid);
        }
        else
        {
            error("target: kill SIGUSR2 ");
        }
    }
}
