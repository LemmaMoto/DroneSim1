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
#include <errno.h>
#include "include/constants.h"

#define PIPE_READ 0
#define PIPE_WRITE 1

pid_t watchdog_pid;
pid_t process_id;
char *process_name;
struct timeval prev_t;
char logfile_name[256]= LOG_FILE_NAME;
char command;

int pipefd[2];
fd_set read_fds;
struct timeval tv;
int retval;

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
    // Set up sigaction for receiving signals from processes
    struct sigaction p_action;
    p_action.sa_flags = SA_SIGINFO;
    p_action.sa_sigaction = watchdog_handler;
    if(sigaction(SIGUSR1, &p_action, NULL) < 0)
    {
        perror("sigaction");
    }

    int process_num;
    if(argc == 4){
        sscanf(argv[1],"%d", &process_num);
        sscanf(argv[2],"%d", &pipefd[PIPE_READ]);
        sscanf(argv[3],"%d", &pipefd[PIPE_WRITE]);  
    } else {
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
    int sleep_durations[NUM_PROCESSES] = PROCESS_SLEEPS_US;
    int sleep_duration = sleep_durations[process_num];
    char *process_names[NUM_PROCESSES] = PROCESS_NAMES;
    process_name = process_names[process_num]; // added to logfile for readability
    
    
    int flags = fcntl(pipefd[PIPE_READ], F_GETFL, 0);
    fcntl(pipefd[PIPE_READ], F_SETFL, flags | O_NONBLOCK);



    while (1) {  
        FD_ZERO(&read_fds);
        FD_SET(pipefd[PIPE_READ], &read_fds);

        // Timeout after 1 second
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        clear();
        retval = select(pipefd[PIPE_READ] + 1, &read_fds, NULL, NULL, &tv);
        if (retval == -1) {
            if (errno == EINTR) {
                // The call was interrupted by a signal, just continue with the loop
                continue;
            } else {
                perror("select");
            }
        } else if (retval) {
            char command;
            ssize_t bytesRead = read(pipefd[PIPE_READ], &command, sizeof(char));
            if (bytesRead > 0) {
                mvprintw(0, 0, "Comando inviato: %c\n", command);
            }
        } else {
            mvprintw(0, 0, "No command sent");
        }
        refresh();
    }

    // Close the write end of the pipe when you're done with it
    close(pipefd[PIPE_WRITE]);

    return 0; 
} 