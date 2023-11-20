#include <stdio.h>
#include <string.h> 
#include <fcntl.h> 
#include <sys/stat.h> 
#include <sys/types.h> 
#include <unistd.h> 
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/wait.h>
#include "include/constants.h"

#define PIPE_READ 0
#define PIPE_WRITE 1


int main(int argc, char *argv[])
{
    // pids for all children
    pid_t child_watchdog;
    pid_t child_process0;
    pid_t child_process1;
    pid_t child_process2;
    pid_t child_process3;

    char read_fd[10];
    char write_fd[10];

    char logfile_name[256];
    sprintf(logfile_name, LOG_FILE_NAME);
    remove(logfile_name);

    FILE *logfile = fopen(logfile_name, "w");
    if (logfile == NULL) {
        perror("fopen");
        return -1;
    }

    // Create a pipe
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe");
        return -1;
    }

    sprintf(read_fd, "%d", pipefd[PIPE_READ]);
    sprintf(write_fd, "%d", pipefd[PIPE_WRITE]);


    //There should be a check that the log folder exists but I haven't done that
    //sprintf(logfile_name, "./log/watchdog/watchdog%i-%i-%i_%i:%i:%i.txt", timenow->tm_year + 1900, timenow->tm_mon, timenow->tm_mday, timenow->tm_hour, timenow->tm_min, timenow->tm_sec);

    fopen(PID_FILE_PW, "w");
    char *fnames[NUM_PROCESSES] = PID_FILE_SP;

    for(int i = 0; i < NUM_PROCESSES; i++)
    {
        fopen(fnames[i], "w");
    }


    int res;
    int num_children=0;

    // Create watchdog
    child_watchdog = fork();
    if (child_watchdog < 0) {
        perror("Fork");
        return -1;
    }

    if (child_watchdog == 0) {
        char * arg_list[] = { "konsole", "-e", "./watchdog", NULL};
        execvp("konsole", arg_list);
	return 0;
    }
    num_children += 1;


    // Make child processes
    child_process0 = fork();
    if (child_process0 < 0) {
        perror("Fork");
        return -1;
    }

    if (child_process0 == 0) {
        char * arg_list[] = { "konsole", "-e", "./server", "0", NULL };
        execvp("konsole", arg_list); 
	return 0;
    }
    num_children += 1;

    child_process1 = fork();
    if (child_process1 < 0) {
        perror("Fork");
        return -1;
    }

    if (child_process1 == 0) {
        close(pipefd[PIPE_WRITE]);
        char * arg_list[] = { "konsole", "-e", "./drone", "1", NULL };
        execvp("konsole", arg_list);
        return 0;
    }
    num_children += 1;

    child_process2 = fork();
    if (child_process2 < 0) {
        perror("Fork");
        return -1;
    }

    if (child_process2 == 0) {
        close(pipefd[PIPE_READ]);
        char * arg_list[] = { "konsole", "-e", "./input", "2", NULL };
        execvp("konsole", arg_list);
        return 0;
    }
    num_children += 1;

    child_process3 = fork();
    if (child_process3 < 0) {
        perror("Fork");
        return -1;
    }

    if (child_process3 == 0) {
        char * arg_list[] = { "konsole", "-e", "./world", "3", NULL };
        execvp("konsole", arg_list);
        return 0;
    }
    num_children += 1;

    

    // Wait for all children to die
    for(int i = 0; i < num_children; i ++){
        wait(&res);
    }

    close(pipefd[0]);
    close(pipefd[1]);
    printf("pipefd[0] = %d, pipefd[1] = %d\n", pipefd[PIPE_READ], pipefd[PIPE_WRITE]);

    return 0;
}
