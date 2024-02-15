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
#include <errno.h>
#include "include/constants.h"

#define PIPE_READ 0
#define PIPE_WRITE 1

void error(char *msg)
{
    FILE *logFile = fopen("log/master/error_log_master.txt", "a");
    if (logFile != NULL)
    {
        time_t now = time(NULL);
        char timeStr[20]; // Buffer to hold the time string
        strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", localtime(&now));
        fprintf(logFile, "%s - %s: %s\n", timeStr, msg, strerror(errno));
        fclose(logFile);
    }
    else
    {
        perror("ERROR opening log file");
    }
}

int main(int argc, char *argv[])
{
    // pids for all children
    pid_t child_watchdog;
    pid_t child_process0;
    pid_t child_process1;
    pid_t child_process2;
    pid_t child_process3;
    pid_t child_process4;
    pid_t child_process5;

    char logfile_name[256];
    sprintf(logfile_name, LOG_FILE_NAME);
    remove(logfile_name);

    FILE *logfile = fopen(logfile_name, "w");
    if (logfile == NULL)
    {
        error("Error opening log file");
        return -1;
    }

    // PIPE PER COLLEGARE SERVER E WORLD
    int pipesw[2];
    if (pipe(pipesw) == -1)
    {
        error("Error creating pipesw");
        return -1;
    }
    printf("Pipesw created successfully\n");

    char pipe_read_sw[10];
    char pipe_write_sw[10];
    sprintf(pipe_read_sw, "%d", pipesw[PIPE_READ]);
    sprintf(pipe_write_sw, "%d", pipesw[PIPE_WRITE]);

    // PIPE PER COLLEGARE WORLD E SERVER
    int pipews[2];
    if (pipe(pipews) == -1)
    {
        error("Error creating pipews");
        return -1;
    }
    printf("Pipews created successfully\n");

    char pipe_read_ws[10];
    char pipe_write_ws[10];
    sprintf(pipe_read_ws, "%d", pipews[PIPE_READ]);
    sprintf(pipe_write_ws, "%d", pipews[PIPE_WRITE]);

    // PIPE PER COLLEGARE SERVER E DRONE
    int pipesd[2];
    if (pipe(pipesd) == -1)
    {
        error("Error creating pipesd");
        return -1;
    }
    printf("Pipesd created successfully\n");

    char pipe_read_sd[10];
    char pipe_write_sd[10];
    sprintf(pipe_read_sd, "%d", pipesd[PIPE_READ]);
    sprintf(pipe_write_sd, "%d", pipesd[PIPE_WRITE]);

    // PIPE PER COLLEGARE DRONE E SERVER
    int pipeds[2];
    if (pipe(pipeds) == -1)
    {
        error("Error creating pipeds");
        return -1;
    }
    printf("Pipeds created successfully\n");

    char pipe_read_ds[10];
    char pipe_write_ds[10];
    sprintf(pipe_read_ds, "%d", pipeds[PIPE_READ]);
    sprintf(pipe_write_ds, "%d", pipeds[PIPE_WRITE]);

    // PIPE PER COLLEGARE SERVER E OBSTACLE
    int pipeso[2];
    if (pipe(pipeso) == -1)
    {
        error("Error creating pipeso");
        return -1;
    }
    printf("Pipeso created successfully\n");

    char pipe_read_so[10];
    char pipe_write_so[10];
    sprintf(pipe_read_so, "%d", pipeso[PIPE_READ]);
    sprintf(pipe_write_so, "%d", pipeso[PIPE_WRITE]);

    // PIPE PER COLLEGARE OBSTACLE E SERVER
    int pipeos[2];
    if (pipe(pipeos) == -1)
    {
        error("Error creating pipeos");
        return -1;
    }
    printf("Pipeos created successfully\n");

    char pipe_read_os[10];
    char pipe_write_os[10];
    sprintf(pipe_read_os, "%d", pipeos[PIPE_READ]);
    sprintf(pipe_write_os, "%d", pipeos[PIPE_WRITE]);

    // PIPE PER COLLEGARE SERVER E TARGET
    int pipest[2];
    if (pipe(pipest) == -1)
    {
        error("Error creating pipest");
        return -1;
    }
    printf("Pipest created successfully\n");

    char pipe_read_st[10];
    char pipe_write_st[10];
    sprintf(pipe_read_st, "%d", pipest[PIPE_READ]);
    sprintf(pipe_write_st, "%d", pipest[PIPE_WRITE]);

    // PIPE PER COLLEGARE TARGET E SERVER
    int pipets[2];
    if (pipe(pipets) == -1)
    {
        error("Error creating pipets");
        return -1;
    }
    printf("Pipets created successfully\n");

    char pipe_read_ts[10];
    char pipe_write_ts[10];
    sprintf(pipe_read_ts, "%d", pipets[PIPE_READ]);
    sprintf(pipe_write_ts, "%d", pipets[PIPE_WRITE]);

    // PIPE PER COLLEGARE DRONE E INPUT (GIÀ FATTO)
    int pipedi[2];
    if (pipe(pipedi) == -1)
    {
        error("Error creating pipedi");
        return -1;
    }
    printf("Pipedi created successfully\n");

    char pipe_read_di[10];
    char pipe_write_di[10];
    sprintf(pipe_read_di, "%d", pipedi[PIPE_READ]);
    sprintf(pipe_write_di, "%d", pipedi[PIPE_WRITE]);

    // PIPE PER COLLEGARE server E DRONE (targets)
    int pipesd_t[2];
    if (pipe(pipesd_t) == -1)
    {
        error("Error creating pipesd_t");
        return -1;
    }
    printf("Pipesd_t created successfully\n");

    char pipe_read_sd_t[10];
    char pipe_write_sd_t[10];
    sprintf(pipe_read_sd_t, "%d", pipesd_t[PIPE_READ]);
    sprintf(pipe_write_sd_t, "%d", pipesd_t[PIPE_WRITE]);

    //  PIPE PER COLLEGARE server E DRONE (screen)
    int pipesd_s[2];
    if (pipe(pipesd_s) == -1)
    {
        error("Error creating pipesd_s");
        return -1;
    }
    printf("Pipesd_s created successfully\n");

    char pipe_read_sd_s[10];
    char pipe_write_sd_s[10];
    sprintf(pipe_read_sd_s, "%d", pipesd_s[PIPE_READ]);
    sprintf(pipe_write_sd_s, "%d", pipesd_s[PIPE_WRITE]);

    // PIPE PER COLLEGARE INPUT E SERVER
    int pipeis[2];
    if (pipe(pipeis) == -1)
    {
        error("Error creating pipeis");
        return -1;
    }
    printf("Pipeis created successfully\n");

    char pipe_read_is[10];
    char pipe_write_is[10];
    sprintf(pipe_read_is, "%d", pipeis[PIPE_READ]);
    sprintf(pipe_write_is, "%d", pipeis[PIPE_WRITE]);

    fopen(PID_FILE_PW, "w");

    char *fnames[NUM_PROCESSES] = PID_FILE_SP;

    for (int i = 0; i < NUM_PROCESSES; i++)
    {
        fopen(fnames[i], "w");
    }

    int res;
    int num_children = 0;

    // Create watchdog
    child_watchdog = fork();
    if (child_watchdog < 0)
    {
        error("Fork");
        return -1;
    }
    printf("Watchdog process created successfully with PID %d\n", child_watchdog);

    if (child_watchdog == 0)
    {
        char *arg_list[] = {"konsole", "-e", "./watchdog", NULL};
        execvp("konsole", arg_list);
        error("execvp failed for watchdog");
        return 0;
    }
    num_children += 1;

    // Make child processes
    child_process0 = fork();
    if (child_process0 < 0)
    {
        error("Fork");
        return -1;
    }
    printf("Server process created successfully with PID %d\n", child_process0);

    if (child_process0 == 0)
    {
        char *arg_list[] = {"konsole", "-e", "./server", "0", pipe_read_sd, pipe_write_sd, pipe_read_ds, pipe_write_ds, pipe_read_so, pipe_write_so, pipe_read_os, pipe_write_os, pipe_read_st, pipe_write_st, pipe_read_ts, pipe_write_ts, pipe_read_sw, pipe_write_sw, pipe_read_ws, pipe_write_ws, pipe_read_sd_t, pipe_write_sd_t, pipe_read_is, pipe_write_di, NULL};
        execvp("konsole", arg_list);
        error("execvp failed for server");
        return 0;
    }
    num_children += 1;
    sleep(3);

    child_process1 = fork();
    if (child_process1 < 0)
    {
        error("Fork");
        return -1;
    }
    printf("Drone process created successfully with PID %d\n", child_process1);

    if (child_process1 == 0)
    {
        char *arg_list[] = {"konsole", "-e", "./drone", "1", pipe_read_di, pipe_write_di, pipe_read_sd, pipe_write_sd, pipe_read_ds, pipe_write_ds, pipe_read_sd_t, pipe_write_sd_t, pipe_read_sd_s, pipe_write_sd_s, NULL};
        execvp("konsole", arg_list);
        error("execvp failed for drone");
        return 0;
    }
    num_children += 1;

    child_process2 = fork();
    if (child_process2 < 0)
    {
        error("Fork");
        return -1;
    }
    printf("Input process created successfully with PID %d\n", child_process2);

    if (child_process2 == 0)
    {
        char *arg_list[] = {"konsole", "-e", "./input", "2", pipe_read_is, pipe_write_is, NULL};
        execvp("konsole", arg_list);
        error("execvp failed for input");
        return 0;
    }
    num_children += 1;

    child_process3 = fork();
    if (child_process3 < 0)
    {
        error("Fork");
        return -1;
    }
    printf("World process created successfully with PID %d\n", child_process3);

    if (child_process3 == 0)
    {
        char *arg_list[] = {"konsole", "-e", "./world", "3", pipe_read_sw, pipe_write_sw, pipe_read_ws, pipe_write_ws, pipe_read_sd_s, pipe_write_sd_s, NULL};
        execvp("konsole", arg_list);
        error("execvp failed for world");
        return 0;
    }
    num_children += 1;

    child_process4 = fork();
    if (child_process4 < 0)
    {
        error("Fork");
        return -1;
    }
    printf("Obstacles process created successfully with PID %d\n", child_process4);

    if (child_process4 == 0)
    {
        char *arg_list[] = {"konsole", "-e", "./obstacles", "4", pipe_read_so, pipe_write_so, pipe_read_os, pipe_write_os, NULL};
        execvp("konsole", arg_list);
        error("execvp failed for obstacles");
        return 0;
    }
    num_children += 1;

    child_process5 = fork();
    if (child_process5 < 0)
    {
        error("Fork");
        return -1;
    }
    printf("Targets process created successfully with PID %d\n", child_process5);

    if (child_process5 == 0)
    {
        char *arg_list[] = {"konsole", "-e", "./targets", "5", pipe_read_st, pipe_write_st, pipe_read_ts, pipe_write_ts, NULL};
        execvp("konsole", arg_list);
        error("execvp failed for targets");
        return 0;
    }
    num_children += 1;

    // Wait for all child processes to terminate
    int status;
    int last_non_zero_status = 0;
    for (int i = 0; i < num_children; i++)
    {
        pid_t pid = wait(&status); // Wait for a child process to terminate
        if (WIFEXITED(status))     // If the child process terminated normally
        {
            printf("Child process %d exited with status %d\n", pid, WEXITSTATUS(status)); // Print the PID and exit status of the terminated child process
            if (WEXITSTATUS(status) != 0)                                                 // If the exit status is not 0 (indicating an error)
            {
                last_non_zero_status = WEXITSTATUS(status); // Store the non-zero exit status
            }
        }
        else // If the child process did not terminate normally
        {
            printf("Child process %d did not exit successfully\n", pid); // Print an error message
        }
    }
    printf("All child processes terminated\n"); // Print a message indicating that all child processes have terminated

    close(pipedi[0]);
    close(pipedi[1]);
    close(pipesw[0]);
    close(pipesw[1]);
    close(pipesd[0]);
    close(pipesd[1]);
    close(pipeso[0]);
    close(pipeso[1]);
    close(pipest[0]);
    close(pipest[1]);
    close(pipeos[0]);
    close(pipeos[1]);
    close(pipets[0]);
    close(pipets[1]);
    close(pipeds[0]);
    close(pipeds[1]);
    close(pipews[0]);
    close(pipews[1]);
    close(pipesd_t[0]);
    close(pipesd_t[1]);
    close(pipesd_s[0]);
    close(pipesd_s[1]);
    close(pipeis[0]);
    close(pipeis[1]);

    printf("pipedi[0] = %d, pipedi[1] = %d\n", pipedi[PIPE_READ], pipedi[PIPE_WRITE]);
    printf("pipesd[0] = %d, pipesd[1] = %d\n", pipesd[PIPE_READ], pipesd[PIPE_WRITE]);
    printf("pipeso[0] = %d, pipeso[1] = %d\n", pipeso[PIPE_READ], pipeso[PIPE_WRITE]);
    printf("pipest[0] = %d, pipest[1] = %d\n", pipest[PIPE_READ], pipest[PIPE_WRITE]);
    printf("pipeos[0] = %d, pipeos[1] = %d\n", pipeos[PIPE_READ], pipeos[PIPE_WRITE]);
    printf("pipets[0] = %d, pipets[1] = %d\n", pipets[PIPE_READ], pipets[PIPE_WRITE]);
    printf("pipeds[0] = %d, pipeds[1] = %d\n", pipeds[PIPE_READ], pipeds[PIPE_WRITE]);
    printf("pipesw[0] = %d, pipesw[1] = %d\n", pipesw[PIPE_READ], pipesw[PIPE_WRITE]);
    printf("pipews[0] = %d, pipews[1] = %d\n", pipews[PIPE_READ], pipews[PIPE_WRITE]);
    printf("pipesd_t[0] = %d, pipesd_t[1] = %d\n", pipesd_t[PIPE_READ], pipesd_t[PIPE_WRITE]);
    printf("pipesd_s[0] = %d, pipesd_s[1] = %d\n", pipesd_s[PIPE_READ], pipesd_s[PIPE_WRITE]);
    printf("pipeis[0] = %d, pipeis[1] = %d\n", pipeis[PIPE_READ], pipeis[PIPE_WRITE]);

    printf("Pipe closed successfully\n"); // Print a message indicating that the pipe has been closed successfully

    return last_non_zero_status;
}
