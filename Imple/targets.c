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


struct Target
{
    int x;
    int y;
};

int main(int argc, char *argv[])
{
    


FILE *file = fopen("file_para.txt", "r");
if (file == NULL) {
    perror("Unable to open file");
    exit(EXIT_FAILURE);
}

char line[256];
int NUM_TARGETS = 0;

while (fgets(line, sizeof(line), file)) {
    if (sscanf(line, "NUM_TARGETS = %d", &NUM_TARGETS) == 1) {
        // Se abbiamo trovato una riga che corrisponde al formato "NUM_OBSTACLES = %d",
        // interrompiamo il ciclo
        break;
    }
}

fclose(file);

printf("NUM_TARGETS = %d\n", NUM_TARGETS);
sleep(20);

struct Target targets[NUM_TARGETS]; // Assume targets are initialized

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