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
#include "include/constants.h"
#include <sys/ipc.h>
#include <sys/shm.h>

#define SHM_DRN 12 // Define a key for the shared memory segment
#define SHM_WRLD 34

#define PIPE_READ 0
#define PIPE_WRITE 1

pid_t watchdog_pid;
pid_t process_id;
char *process_name;
struct timeval prev_t;
char logfile_name[256] = LOG_FILE_NAME;

struct Drone
{
    int x;
    int y;
    char symbol;
    short color_pair;
};

struct Screen
{
    int height;
    int width;
};

struct Obstacle
{
    int x;
    int y;
    char symbol;
};

struct Target
{
    int x;
    int y;
    char symbol;
};

struct World
{
    struct Drone drone;
    struct Obstacle obstacle[676];
    struct Screen screen;
    struct Target target[9];
};
int pipesd[2];
int pipeds[2];
int pipeso[2];
int pipeos[2];
int pipest[2];
int pipets[2];
int pipesw[2];
int pipews[2];

// logs time update to file
void log_receipt(struct timeval tv)
{
    FILE *lf_fp = fopen(logfile_name, "a");
    fprintf(lf_fp, "%d %ld %ld\n", process_id, tv.tv_sec, tv.tv_usec);
    fclose(lf_fp);
}

void watchdog_handler(int sig, siginfo_t *info, void *context)
{
    printf("received signal \n");
    if (info->si_pid == watchdog_pid)
    {
        gettimeofday(&prev_t, NULL);
        log_receipt(prev_t);
    }
}

int main(int argc, char *argv[])
{
    struct Drone drone;
    int process_num;
    if (argc == 18)
    {
        sscanf(argv[1], "%d", &process_num);
        sscanf(argv[2], "%d", &pipesd[PIPE_READ]);
        sscanf(argv[3], "%d", &pipesd[PIPE_WRITE]);
        sscanf(argv[4], "%d", &pipeds[PIPE_READ]);
        sscanf(argv[5], "%d", &pipeds[PIPE_WRITE]);
        sscanf(argv[6], "%d", &pipeso[PIPE_READ]);
        sscanf(argv[7], "%d", &pipeso[PIPE_WRITE]);
        sscanf(argv[8], "%d", &pipeos[PIPE_READ]);
        sscanf(argv[9], "%d", &pipeos[PIPE_WRITE]);
        sscanf(argv[10], "%d", &pipest[PIPE_READ]);
        sscanf(argv[11], "%d", &pipest[PIPE_WRITE]);
        sscanf(argv[12], "%d", &pipets[PIPE_READ]);
        sscanf(argv[13], "%d", &pipets[PIPE_WRITE]);
        sscanf(argv[14], "%d", &pipesw[PIPE_READ]);
        sscanf(argv[15], "%d", &pipesw[PIPE_WRITE]);
        sscanf(argv[16], "%d", &pipews[PIPE_READ]);
        sscanf(argv[17], "%d", &pipews[PIPE_WRITE]);
    }
    else
    {
        printf("wrong args\n");
        return -1;
    }
    printf("process_num: %d\n", process_num);
    printf("pipesd[PIPE_READ]: %d\n", pipesd[PIPE_READ]);
    printf("pipesd[PIPE_WRITE]: %d\n", pipesd[PIPE_WRITE]);
    printf("pipeds[PIPE_READ]: %d\n", pipeds[PIPE_READ]);
    printf("pipeds[PIPE_WRITE]: %d\n", pipeds[PIPE_WRITE]);
    printf("pipeso[PIPE_READ]: %d\n", pipeso[PIPE_READ]);
    printf("pipeso[PIPE_WRITE]: %d\n", pipeso[PIPE_WRITE]);
    printf("pipeos[PIPE_READ]: %d\n", pipeos[PIPE_READ]);
    printf("pipeos[PIPE_WRITE]: %d\n", pipeos[PIPE_WRITE]);
    printf("pipest[PIPE_READ]: %d\n", pipest[PIPE_READ]);
    printf("pipest[PIPE_WRITE]: %d\n", pipest[PIPE_WRITE]);
    printf("pipets[PIPE_READ]: %d\n", pipets[PIPE_READ]);
    printf("pipets[PIPE_WRITE]: %d\n", pipets[PIPE_WRITE]);
    printf("pipesw[PIPE_READ]: %d\n", pipesw[PIPE_READ]);
    printf("pipesw[PIPE_WRITE]: %d\n", pipesw[PIPE_WRITE]);
    printf("pipews[PIPE_READ]: %d\n", pipews[PIPE_READ]);
    printf("pipews[PIPE_WRITE]: %d\n", pipews[PIPE_WRITE]);

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
    if (stat(PID_FILE_PW, &sbuf) == -1)
    {
        perror("error-stat");
        return -1;
    }
    // waits until the file has data
    while (sbuf.st_size <= 0)
    {
        if (stat(PID_FILE_PW, &sbuf) == -1)
        {
            perror("error-stat");
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
    if (sigaction(SIGUSR1, &p_action, NULL) < 0)
    {
        perror("sigaction");
    }

    // Read how long to sleep process for
    int sleep_durations[NUM_PROCESSES] = PROCESS_SLEEPS_US;
    int sleep_duration = sleep_durations[process_num];
    char *process_names[NUM_PROCESSES] = PROCESS_NAMES;
    process_name = process_names[process_num]; // added to logfile for readability

    struct World world;
    while (1)
    {
        read(pipeds[PIPE_READ], &world.drone, sizeof(world.drone));
        read(pipeos[PIPE_READ], &world.obstacle, sizeof(world.obstacle));
        // printf("x: %d, y: %d, symbol: %c, color_pair: %d\n", drone.x, drone.y, drone.symbol, drone.color_pair);
        read(pipets[PIPE_READ], &world.target, sizeof(world.target));

        write(pipesw[PIPE_WRITE], &world.drone, sizeof(world.drone));
        fsync(pipesw[PIPE_WRITE]);

        write(pipesw[PIPE_WRITE], &world.obstacle, sizeof(world.obstacle));
        fsync(pipesw[PIPE_WRITE]);

        write(pipesw[PIPE_WRITE], &world.target, sizeof(world.target));
        fsync(pipesw[PIPE_WRITE]);

        write(pipesd[PIPE_WRITE], &world.obstacle, sizeof(world.obstacle));
        fsync(pipesd[PIPE_WRITE]);

        write(pipesd[PIPE_WRITE], &world.target, sizeof(world.target));
        fsync(pipesd[PIPE_WRITE]);

        read(pipews[PIPE_READ], &world.screen, sizeof(world.screen));

        write(pipeso[PIPE_WRITE], &world.screen, sizeof(world.screen));
        fsync(pipeso[PIPE_WRITE]);
        write(pipest[PIPE_WRITE], &world.screen, sizeof(world.screen));
        fsync(pipest[PIPE_WRITE]);
        printf("x: %d, y: %d\n", world.drone.x, world.drone.y);
       
    }

    return 0;
}