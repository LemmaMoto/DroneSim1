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

int pipesw[2];

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




struct Obstacle
{
    int x;
    int y;
};

//leggere da file file_para numero di ostacoli 

struct Obstacle obstacles[NUM_OBSTACLES]; // Assume obstacles are initialized

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