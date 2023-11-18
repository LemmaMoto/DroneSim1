#include <stdio.h> 
#include <string.h> 
#include <fcntl.h> 
#include <sys/stat.h> 
#include <sys/time.h>
#include <sys/types.h> 
#include <unistd.h> 
#include <signal.h>
#include <stdlib.h>
#include <time.h>
#include <ncurses.h>
#include "include/constants.h"

#define PIPE_READ 0
#define PIPE_WRITE 1

pid_t watchdog_pid;
pid_t process_id;
char *process_name;
struct timeval prev_t;
char logfile_name[80];

int pipefd[2];

// logs time update to file
void log_receipt(struct timeval tv)
{
    FILE *lf_fp = fopen(logfile_name, "a");
    fprintf(lf_fp, "%d %ld %ld\n", process_id, tv.tv_sec, tv.tv_usec);
    fclose(lf_fp);
}

void watchdog_handler(int sig, siginfo_t *info, void *context)
{
    if(info->si_pid == watchdog_pid){
        gettimeofday(&prev_t, NULL);
        log_receipt(prev_t);
    }  
}

int main(int argc, char *argv[]) 
{
    printf("pipefd[0] = %d, pipefd[1] = %d\n", pipefd[PIPE_READ], pipefd[PIPE_WRITE]);
    int process_num;
    if(argc == 3){
        sscanf(argv[1],"%d", &process_num);  
        snprintf(logfile_name, 80, "%s", argv[2]);
    } else {
        printf("wrong args\n"); 
        return -1;
    }
    
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

    // Set up sigaction for receiving signals from processes
    struct sigaction p_action;
    p_action.sa_flags = SA_SIGINFO;
    p_action.sa_sigaction = watchdog_handler;
    if(sigaction(SIGUSR1, &p_action, NULL) < 0)
    {
        perror("sigaction");
    }

    // Read how long to sleep process for
    int sleep_durations[NUM_PROCESSES] = PROCESS_SLEEPS_US;
    int sleep_duration = sleep_durations[process_num];
    char *process_names[NUM_PROCESSES] = PROCESS_NAMES;
    process_name = process_names[process_num]; // added to logfile for readability
    
    char command;

while (1) 
{   
    clear();
    // Non chiudere il pipe dopo ogni lettura
    if(read(pipefd[PIPE_READ], &command, sizeof(char)) > 0) {
        mvprintw(0, 0, "Comando inviato: %c\n", command);
        refresh();
    }
}  
    close(pipefd[PIPE_READ]);
    return 0; 
} 