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
#include <errno.h>
#include "arplib.h"
#include "../config/config.h"

/*  when signal arrive counter --
    when wd send kill counter ++ */
int counter = 0;
/*signal hadler function*/
void sigusr2Handler(int signum, siginfo_t *info, void *context)
{
    if (signum == SIGUSR2)
    {
        writeLog_wd("WATCHDOG received signal from %d ", info->si_pid);
        counter--;
    }
}

int main(int argc, char *argv[])
{

    // write into logfile
    int i, n;
    int num_ps = PROCESS_NUMBER;
    pid_t wd_pid = getpid();
    pid_t pids_from_master[num_ps];
    pid_t pids_from_process[num_ps];

    // Open the log file for cancel the content
    FILE *logfile = fopen("../log/logfile_wd.txt", "w");
    if (logfile == NULL)
    {
        perror("wd: fopen");
        writeLog_wd("==> ERROR ==> wd: fopen");
    }

    writeLog_wd("WATCHDOG is create with pid %d ", wd_pid);
    writeLog("WATCHDOG is create with pid %d ", wd_pid);
    // In this array I will put all the proces pid converted in int

    // configure the handler for sigusr2
    struct sigaction sa_usr2;
    sa_usr2.sa_sigaction = sigusr2Handler;
    sa_usr2.sa_flags = SA_SIGINFO; // I need also the info foruse the pid of the process for unde
    if (sigaction(SIGUSR2, &sa_usr2, NULL) == -1)
    {
        perror("wd: sigaction");
        writeLog_wd("==> ERROR ==> wd: sigaction");
    }

    // exctract and convert the pid send from master, they are in position 1 => nm_ps +1
    for (i = 1; i < num_ps + 1; i++)
    {
        pids_from_master[i - 1] = atoi(argv[i]);
        if (pids_from_master[i - 1] == 0)
        {
            perror("wd: atoi");
            writeLog_wd("==> ERROR ==> wd: atoi");
        }
        writeLog_wd("WATCHDOG received pid from master: %d", pids_from_master[i - 1]);
    }
    writeLog_wd("\n");
    // extract and convert in integer the pid received from process position num_ps +1 => 2*num_ps +1
    for (i = num_ps + 1; i < (2 * num_ps) + 1; i++)
    {
        pids_from_process[i - (num_ps + 1)] = atoi(argv[i]);
        if (pids_from_process[i - (num_ps + 1)] == 0)
        {
            perror("wd: atoi");
            writeLog_wd("==> ERROR ==> wd: atoi");
        }
        writeLog_wd("WATCHDOG received pid from process: %d", pids_from_process[i - (num_ps + 1)]);
    }

    writeLog_wd("WATCHDOG is ready to work");
    // infinite loop for the operations of wd
    while (1)
    {
        // Inizialize the counter every time enter in the loop
        counter = 0;
        // send a signal to all process
        for (i = 0; i < num_ps; i++)
        {
            /* send signal to all process*/
            n = kill(pids_from_process[i], SIGUSR1);
            if (n == -1)
            {
                if (errno == ESRCH)
                {
                    // No such process
                    writeLog_wd("==> WARNING ==> wd: No such process: %d ", pids_from_process[i]);
                }
                else
                {
                    // Some other error occurred
                    perror("wd: kill");
                    writeLog_wd("==> ERROR ==> wd: kill signal SIGUSR1 %m ");
                }
            }
            /* increment the counter when send the signal SIGUSR1*/
            counter++;
            sleep(1);
            printf("%d", counter);
            fflush(stdout);
            if (counter == 0)
            {
                // case where the proccess is alive
                writeLog_wd("WATCHDOG: Process %d is alive ", pids_from_process[i]);
            }
            else
            {
                // Case where the process do not work
                /*kill all process*/
                for (int j = 0; j < num_ps + 1; j++)
                {

                    if (kill(pids_from_process[j], SIGKILL) == 0)
                    {
                        /*write into logfile that wd close the process*/
                        writeLog_wd(" * WATCHDOG: process %d is closed by wd ", pids_from_process[j]);
                    }
                    else
                    {
                        if (errno == ESRCH)
                        {
                            writeLog_wd("==> WARNING ==> wd: No such process: %d", pids_from_process[j]);
                        }
                        else
                        {
                            writeLog_wd("==> ERROR ==> wd: kill signal SIGKILL %m ");
                        }
                    }

                    if (kill(pids_from_master[j], SIGKILL) == 0)
                    {
                        /*write into logfile that wd close the process*/
                        writeLog_wd("WATCHDOG: process %d is closed by wd ", pids_from_master[j]);
                    }
                    else
                    {
                        if (errno == ESRCH)
                        {
                            writeLog_wd("==> WARNING ==> wd: No such process: %d", pids_from_master[j]);
                        }
                        else
                        {
                            writeLog_wd("==> ERROR ==> wd: kill signal SIGKILL wd %m ");
                        }
                    }
                }
                return 0;
            }
            sleep(1);
        }
    }
}