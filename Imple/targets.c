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

struct Target
{
    int x;
    int y;
};

//leggere da file file_para numero di target

struct Target targets[NUM_TARGETS]; // Assume targets are initialized

// far si che i target non si sovrappongano

// generare target randomici numerati da 1 a 9

// passare i target al drone via pipe passando per il server e controllare che il drone non si sovrapponga ai target

// far sparire i target una volta raggiunti dal drone leggendo da pipe dal server
 
// se passa un tot di tempo e il drone non ha raggiunto il target, il target si sposta in un'altra posizione randomica 

// Main loop
while (1)
{
    // ...

    // Calculate attraction forces
    double fx = 0, fy = 0;
    for (int i = 0; i < NUM_TARGETS; ++i)
    {
        double dx = targets[i].x - x;
        double dy = targets[i].y - y;
        double distance = sqrt(dx * dx + dy * dy);
        if (distance != 0) // Avoid division by zero
        {
            fx += TARGET_ATTRACTION_CONSTANT * dx / distance;
            fy += TARGET_ATTRACTION_CONSTANT * dy / distance;
        }
    }

    // ...
}