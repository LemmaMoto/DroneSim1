#include <stdio.h>
#include <string.h>
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
#include <sys/ipc.h>
#include <sys/shm.h>

#define OBSTACLE_REPULSION_CONSTANT 1.0

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

struct Obstacle
{
    int x;
    int y;
};

struct Screen
{
    int height;
    int width;
};

struct World
{
    struct Drone drone;
    struct Obstacle obstacle[100];
    struct Screen screen;
};

int pipeso[2];
int pipeos[2];

// logs time update to file
void log_receipt(struct timeval tv)
{
    FILE *lf_fp = fopen(logfile_name, "a");
    fprintf(lf_fp, "%d %ld %ld\n", process_id, tv.tv_sec, tv.tv_usec);
    fclose(lf_fp);
}

void watchdog_handler(int sig, siginfo_t *info, void *context)
{
    // printf("received signal \n");
    if (info->si_pid == watchdog_pid)
    {
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
    if (sigaction(SIGUSR1, &p_action, NULL) < 0)
    {
        perror("sigaction");
    }

    int process_num;
    if (argc == 6)
    {
        sscanf(argv[1], "%d", &process_num);
        sscanf(argv[2], "%d", &pipeso[PIPE_READ]);
        sscanf(argv[3], "%d", &pipeso[PIPE_WRITE]);
        sscanf(argv[4], "%d", &pipeos[PIPE_READ]);
        sscanf(argv[5], "%d", &pipeos[PIPE_WRITE]);
    }
    else
    {
        printf("Error: wrong number of arguments\n");
        exit(EXIT_FAILURE);
    }

    printf("process num %d \n", process_num);

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

    // Read how long to sleep process for
    int sleep_durations[NUM_PROCESSES] = PROCESS_SLEEPS_US;
    int sleep_duration = sleep_durations[process_num];
    char *process_names[NUM_PROCESSES] = PROCESS_NAMES;
    process_name = process_names[process_num]; // added to logfile for readability

    // leggere da file file_para numero di ostacoli
    FILE *file = fopen("file_para.txt", "r");
    if (file == NULL)
    {
        perror("Unable to open file");
        exit(EXIT_FAILURE);
    }

    char line[256];
    int NUM_OBSTACLES = 0;

    while (fgets(line, sizeof(line), file))
    {
        if (sscanf(line, "NUM_OBSTACLES = %d", &NUM_OBSTACLES) == 1)
        {
            // Se abbiamo trovato una riga che corrisponde al formato "NUM_OBSTACLES = %d",
            // interrompiamo il ciclo
            break;
        }
    }

    fclose(file);

    printf("NUM_OBSTACLES = %d\n", NUM_OBSTACLES);
    sleep(20);

    //struct Obstacle obstacles[NUM_OBSTACLES]; // Assume obstacles are initialized
    //struct Screen screen;
    struct World world;

    // Create a new window
    WINDOW *win = newwin(0, 0, 0, 0);

    while (1)
    {
        read(pipeso[PIPE_READ], &world.screen, sizeof(world.screen));
        printf("height: %d, width: %d\n", world.screen.height, world.screen.width);

    }

    // posizionare gli ostacoli intorno alla window di modo da fare i bordi

    // far si che gli ostacoli non si sovrappongano

    // generare ostacoli randomici

    // passare gli ostacoli al drone via pipe

    // Main loop
    // while (1)
    // {
    //     // ...

    //     // Calculate repulsion forces
    //     double fx = 0, fy = 0;
    //     for (int i = 0; i < NUM_OBSTACLES; ++i)
    //     {
    //         double dx = x - obstacles[i].x;
    //         double dy = y - obstacles[i].y;
    //         double distance = sqrt(dx * dx + dy * dy);
    //         if (distance != 0) // Avoid division by zero
    //         {
    //             fx += OBSTACLE_REPULSION_CONSTANT * dx / (distance * distance * distance);
    //             fy += OBSTACLE_REPULSION_CONSTANT * dy / (distance * distance * distance);
    //         }
    //     }

    //     // ...
    // }
    return 0;
}