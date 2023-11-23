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