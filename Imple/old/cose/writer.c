#include <stdio.h> 
#include <string.h> 
#include <fcntl.h> 
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/stat.h> 
#include <sys/types.h> 
#include <unistd.h> 
#include <stdlib.h>
#include <time.h>
#include <signal.h> // Added this line
#include "include/constants.h"

void handle_watchdog_signal(int signum) {
    printf("Received watchdog signal. Responding...\n");
    fflush(stdout);
    kill(getppid(), SIGUSR1);
}

int main(int argc, char *argv[]) 
{
    // Set up the signal handler for the watchdog signal (SIGUSR2)
    struct sigaction sa;
    sa.sa_handler = handle_watchdog_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGUSR1, &sa, NULL);

    printf("Process is running and ready to receive watchdog signal.\n");

    int send_int = 1;
    int writer_num = -1;

    // extract writer number
    if(argc == 2){
        sscanf(argv[1],"%i", &writer_num);
    } else {
        printf("wrong number of arguments\n"); 
        return -1;
    }

    int cells[2] = {0, 0};
    int shared_seg_size = (1 * sizeof(cells));

    // create semaphore ids
    sem_t * sem_id = sem_open(SEM_PATH, 0);

    // create shared memory pointer
    int shmfd  = shm_open(SHMOBJ_PATH, O_RDWR, S_IRWXU | S_IRWXG);
    if (shmfd < 0)
    {
        perror("shm_open");
        return -1;
    }
    void* shm_ptr = mmap(NULL, shared_seg_size, PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0);

    srandom(writer_num * SEED_MULTIPLIER);

    while (1) 
    {   
        // wait for semaphore
        sem_wait(sem_id);
        // copy cells
        memcpy(cells, shm_ptr, shared_seg_size);
        // if your cell value is less than the other cell value, write a random number
        if(can_write(cells, writer_num))
        {
            send_int = random() % MAX_NUMBER;
            cells[writer_num] = send_int;
            // copy local cells to memory
            memcpy(shm_ptr, cells, shared_seg_size);
            printf("Wrote %i to server in cell %i\n", send_int, writer_num);
        }
        // post semaphore
        sem_post(sem_id);
        sleep(0.5);
    } 
    // clean up
    sem_close(sem_id);
    return 0; 
} 