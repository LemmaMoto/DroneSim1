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
#define TARGET_ATTRACTION_CONSTANT 1.0

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
    char symbol;
};

struct Target
{
    int x;
    int y;
    char symbol;
};

struct Screen
{
    int height;
    int width;
};

struct World
{
    struct Drone drone;
    struct Obstacle obstacle[676]; // 512 + 50(NUM_OBSTACLES) +1 a caso
    struct Screen screen;
    struct Target target[9];
};

int pipest[2];
int pipets[2];

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
        sscanf(argv[2], "%d", &pipest[PIPE_READ]);
        sscanf(argv[3], "%d", &pipest[PIPE_WRITE]);
        sscanf(argv[4], "%d", &pipets[PIPE_READ]);
        sscanf(argv[5], "%d", &pipets[PIPE_WRITE]);
    }
    else
    {
        printf("Error: wrong number of arguments\n");
        exit(EXIT_FAILURE);
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
    FILE *file = fopen("file_para.txt", "r");
    if (file == NULL)
    {
        perror("Unable to open file");
        exit(EXIT_FAILURE);
    }

    char line[256];
    int NUM_TARGETS = 0;
    int refresh_time_targets = 0;

    while (fgets(line, sizeof(line), file))
    {
        if (sscanf(line, "NUM_TARGETS = %d", &NUM_TARGETS) == 1)
        {
            // Se abbiamo trovato una riga che corrisponde al formato "NUM_OBSTACLES = %d",
            // interrompiamo il ciclo
            continue;
        }
        else if (sscanf(line, "refresh_time_targets = %d", &refresh_time_targets) == 1){
            break;
        }

    }

    fclose(file);

    printf("NUM_TARGETS = %d\n", NUM_TARGETS);
    printf("refresh_time_targets = %d\n", refresh_time_targets);

    struct World world;
    time_t last_spawn_time = 0;
    int first = 6;
    while (1)
    {
        read(pipest[PIPE_READ], &world.target, sizeof(world.target));
        printf("target x: %d\n", world.target[0].x);

        time_t current_time = time(NULL);
        if (current_time - last_spawn_time >= refresh_time_targets || first > 0)
        {
            for (int i=0; i < NUM_TARGETS; i++)
            {   
                do
                {
                    world.target[i].x = rand() % (world.screen.width - 4) + 2;
                    world.target[i].y = rand() % (world.screen.height - 4) + 2;
                } while (world.target[i].x == world.drone.x && world.target[i].y == world.drone.y);

                world.target[i].symbol = '0' + i;
            }
            last_spawn_time = current_time;
        }
        first--;
        write(pipets[PIPE_WRITE], &world.target, sizeof(world.target));
        fsync(pipets[PIPE_WRITE]);
        // far si che i target non si sovrappongano

        // generare target randomici numerati da 1 a 9

        // passare i target al drone via pipe passando per il server e controllare che il drone non si sovrapponga ai target

        // far sparire i target una volta raggiunti dal drone leggendo da pipe dal server

        // se passa un tot di tempo e il drone non ha raggiunto il target, il target si sposta in un'altra posizione randomica

        // Main loop
        // while (1)
        // {
        //     // ...

        //     // Calculate attraction forces
        //     double fx = 0, fy = 0;
        //     for (int i = 0; i < NUM_TARGETS; ++i)
        //     {
        //         double dx = targets[i].x - x;
        //         double dy = targets[i].y - y;
        //         double distance = sqrt(dx * dx + dy * dy);
        //         if (distance != 0) // Avoid division by zero
        //         {
        //             fx += TARGET_ATTRACTION_CONSTANT * dx / distance;
        //             fy += TARGET_ATTRACTION_CONSTANT * dy / distance;
        //         }
        //     }

        //     // ...
        // }
    }
    return 0;
}