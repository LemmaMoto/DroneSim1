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
    pid_t child_process4;
    pid_t child_process5;

    char logfile_name[256];
    sprintf(logfile_name, LOG_FILE_NAME);
    remove(logfile_name);

    FILE *logfile = fopen(logfile_name, "w");
    if (logfile == NULL)
    {
        perror("fopen");
        return -1;
    }

    // PIPE PER COLLEGARE SERVER E WORLD
    int pipesw[2];
    if (pipe(pipesw) == -1)
    {
        perror("pipe");
        return -1;
    }
    char pipe_read_sw[10];
    char pipe_write_sw[10];
    sprintf(pipe_read_sw, "%d", pipesw[PIPE_READ]);
    sprintf(pipe_write_sw, "%d", pipesw[PIPE_WRITE]);

    // PIPE PER COLLEGARE WORLD E SERVER
    int pipews[2];
    if (pipe(pipews) == -1)
    {
        perror("pipe");
        return -1;
    }
    char pipe_read_ws[10];
    char pipe_write_ws[10];
    sprintf(pipe_read_ws, "%d", pipews[PIPE_READ]);
    sprintf(pipe_write_ws, "%d", pipews[PIPE_WRITE]);

    // PIPE PER COLLEGARE SERVER E DRONE
    int pipesd[2];
    if (pipe(pipesd) == -1)
    {
        perror("pipe");
        return -1;
    }

    char pipe_read_sd[10];
    char pipe_write_sd[10];
    sprintf(pipe_read_sd, "%d", pipesd[PIPE_READ]);
    sprintf(pipe_write_sd, "%d", pipesd[PIPE_WRITE]);

    // PIPE PER COLLEGARE DRONE E SERVER
    int pipeds[2];
    if (pipe(pipeds) == -1)
    {
        perror("pipe");
        return -1;
    }
    char pipe_read_ds[10];
    char pipe_write_ds[10];
    sprintf(pipe_read_ds, "%d", pipeds[PIPE_READ]);
    sprintf(pipe_write_ds, "%d", pipeds[PIPE_WRITE]);

    // PIPE PER COLLEGARE SERVER E OBSTACLE
    int pipeso[2];
    if (pipe(pipeso) == -1)
    {
        perror("pipe");
        return -1;
    }

    char pipe_read_so[10];
    char pipe_write_so[10];
    sprintf(pipe_read_so, "%d", pipeso[PIPE_READ]);
    sprintf(pipe_write_so, "%d", pipeso[PIPE_WRITE]);

    // PIPE PER COLLEGARE OBSTACLE E SERVER
    int pipeos[2];
    if (pipe(pipeos) == -1)
    {
        perror("pipe");
        return -1;
    }

    char pipe_read_os[10];
    char pipe_write_os[10];
    sprintf(pipe_read_os, "%d", pipeos[PIPE_READ]);
    sprintf(pipe_write_os, "%d", pipeos[PIPE_WRITE]);

    // PIPE PER COLLEGARE SERVER E TARGET
    int pipest[2];
    if (pipe(pipest) == -1)
    {
        perror("pipe");
        return -1;
    }

    char pipe_read_st[10];
    char pipe_write_st[10];
    sprintf(pipe_read_st, "%d", pipest[PIPE_READ]);
    sprintf(pipe_write_st, "%d", pipest[PIPE_WRITE]);

    // PIPE PER COLLEGARE TARGET E SERVER
    int pipets[2];
    if (pipe(pipets) == -1)
    {
        perror("pipe");
        return -1;
    }

    char pipe_read_ts[10];
    char pipe_write_ts[10];
    sprintf(pipe_read_ts, "%d", pipets[PIPE_READ]);
    sprintf(pipe_write_ts, "%d", pipets[PIPE_WRITE]);

    // PIPE PER COLLEGARE DRONE E INPUT (GIÀ FATTO)
    int pipedi[2];
    if (pipe(pipedi) == -1)
    {
        perror("pipe");
        return -1;
    }

    char pipe_read_di[10];
    char pipe_write_di[10];
    sprintf(pipe_read_di, "%d", pipedi[PIPE_READ]);
    sprintf(pipe_write_di, "%d", pipedi[PIPE_WRITE]);

    // There should be a check that the log folder exists but I haven't done that
    // sprintf(logfile_name, "./log/watchdog/watchdog%i-%i-%i_%i:%i:%i.txt", timenow->tm_year + 1900, timenow->tm_mon, timenow->tm_mday, timenow->tm_hour, timenow->tm_min, timenow->tm_sec);

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
        perror("Fork");
        return -1;
    }

    if (child_watchdog == 0)
    {
        char *arg_list[] = {"konsole", "-e", "./watchdog", NULL};
        execvp("konsole", arg_list);
        return 0;
    }
    num_children += 1;

    // Make child processes
    child_process0 = fork();
    if (child_process0 < 0)
    {
        perror("Fork");
        return -1;
    }

    if (child_process0 == 0)
    {
        char *arg_list[] = {"konsole", "-e", "./server", "0", pipe_read_sd, pipe_write_sd, pipe_read_ds, pipe_write_ds, pipe_read_so, pipe_write_so, pipe_read_os, pipe_write_os, pipe_read_st, pipe_write_st, pipe_read_ts, pipe_write_ts, pipe_read_sw, pipe_write_sw, pipe_read_ws, pipe_write_ws, NULL};
        execvp("konsole", arg_list);
        return 0;
    }
    num_children += 1;

    child_process1 = fork();
    if (child_process1 < 0)
    {
        perror("Fork");
        return -1;
    }

    if (child_process1 == 0)
    {
        char *arg_list[] = {"konsole", "-e", "./drone", "1", pipe_read_di, pipe_write_di, pipe_read_sd, pipe_write_sd, pipe_read_ds, pipe_write_ds, NULL};
        execvp("konsole", arg_list);
        return 0;
    }
    num_children += 1;

    child_process2 = fork();
    if (child_process2 < 0)
    {
        perror("Fork");
        return -1;
    }

    if (child_process2 == 0)
    {
        char *arg_list[] = {"konsole", "-e", "./input", "2", pipe_read_di, pipe_write_di, NULL};
        execvp("konsole", arg_list);
        return 0;
    }
    num_children += 1;

    child_process3 = fork();
    if (child_process3 < 0)
    {
        perror("Fork");
        return -1;
    }

    if (child_process3 == 0)
    {
        char *arg_list[] = {"konsole", "-e", "./world", "3", pipe_read_sw, pipe_write_sw, pipe_read_ws, pipe_write_ws, NULL};
        execvp("konsole", arg_list);
        return 0;
    }
    num_children += 1;

    child_process4 = fork();
    if (child_process4 < 0)
    {
        perror("Fork");
        return -1;
    }

    if (child_process4 == 0)
    {
        char *arg_list[] = {"konsole", "-e", "./obstacles", "4", pipe_read_so, pipe_write_so, pipe_read_os, pipe_write_os, NULL};
        execvp("konsole", arg_list);
        return 0;
    }
    num_children += 1;

    child_process5 = fork();
    if (child_process5 < 0)
    {
        perror("Fork");
        return -1;
    }

    if (child_process5 == 0)
    {
        char *arg_list[] = {"konsole", "-e", "./targets", "5", pipe_read_st, pipe_write_st, pipe_read_ts, pipe_write_ts, NULL};
        execvp("konsole", arg_list);
        return 0;
    }
    num_children += 1;

    // Wait for all children to die
    for (int i = 0; i < num_children; i++)
    {
        wait(&res);
    }

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

    printf("pipedi[0] = %d, pipedi[1] = %d\n", pipedi[PIPE_READ], pipedi[PIPE_WRITE]);
    printf("pipesd[0] = %d, pipesd[1] = %d\n", pipesd[PIPE_READ], pipesd[PIPE_WRITE]);
    printf("pipeso[0] = %d, pipeso[1] = %d\n", pipeso[PIPE_READ], pipeso[PIPE_WRITE]);
    printf("pipest[0] = %d, pipest[1] = %d\n", pipest[PIPE_READ], pipest[PIPE_WRITE]);
    printf("pipeos[0] = %d, pipeos[1] = %d\n", pipeos[PIPE_READ], pipeos[PIPE_WRITE]);
    printf("pipets[0] = %d, pipets[1] = %d\n", pipets[PIPE_READ], pipets[PIPE_WRITE]);
    printf("pipeds[0] = %d, pipeds[1] = %d\n", pipeds[PIPE_READ], pipeds[PIPE_WRITE]);
    printf("pipesw[0] = %d, pipesw[1] = %d\n", pipesw[PIPE_READ], pipesw[PIPE_WRITE]);
    printf("pipews[0] = %d, pipews[1] = %d\n", pipews[PIPE_READ], pipews[PIPE_WRITE]);
    

    return 0;
}
