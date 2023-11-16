#include <stdio.h> 
#include <string.h> 
#include <fcntl.h> 
#include <sys/stat.h> 
#include <sys/types.h> 
#include <unistd.h> 
#include <signal.h>
#include <stdlib.h>
#include <time.h>
#include "include/constants.h"

pid_t watchdog_pid;

int main(int argc, char *argv[]) 
{
    int process_num;
    if(argc == 2){
        sscanf(argv[1],"%d", &process_num);  
    } else {
        printf("wrong args\n"); 
        return -1;
    }
    

    // Publish your pid
    pid_t my_pid = getpid();

    char *fnames[NUM_PROCESSES] = PID_FILE_SP;

    FILE *pid_fp = fopen(fnames[process_num], "w");
    fprintf(pid_fp, "%d", my_pid);
    fclose(pid_fp);

    printf("Published pid %d \n", my_pid);


    // Read watchdog pid
    FILE *watchdog_fp = NULL;
    struct stat sbuf;

    /* call stat, fill stat buffer, validate success */
    if (stat (PID_FILE_PW, &sbuf) == -1) {
        perror ("error-stat");
        return -1;
    }
    // waits until the file has data
    while (sbuf.st_size <= 0) {
        if (stat (PID_FILE_PW, &sbuf) == -1) {
            perror ("error-stat");
            return -1;
        }
        usleep(50000);
    }

    watchdog_fp = fopen(PID_FILE_PW, "r");

    fscanf(watchdog_fp, "%d", &watchdog_pid);
    printf("watchdog pid %d \n", watchdog_pid);
    fclose(watchdog_fp);


    // Read how long to sleep process for
    int sleep_durations[3] = PROCESS_SLEEPS_US;
    int sleep_duration = sleep_durations[process_num];

    int counter;
    
    while (1) 
    {   
        printf("Sleepy process\n");
        if(counter == PROCESS_SIGNAL_INTERVAL)
        {
            // send signal to watchdog every process_signal_interval
            if(kill(watchdog_pid, SIGUSR1) < 0)
            {
                perror("kill");
            }
            printf("Sent signal to watchdog\n");
            counter = 0;
        }

        counter++;
        usleep(sleep_duration);
    } 
    return 0; 
} 