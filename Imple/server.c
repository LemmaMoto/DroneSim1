#include <stdio.h> 
#include <unistd.h> 
#include <signal.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h> 
#include <fcntl.h> 
#include <sys/stat.h> 
#include <sys/types.h> 
#include <sys/select.h>
#include "include/constants.h"
#include <semaphore.h>
#include <sys/mman.h>

pid_t watchdog_pid;
pid_t process_id;
struct timeval prev_t;

void watchdog_handler(int sig, siginfo_t *info, void *context)
{
    printf("received signal \n");
    if(info->si_pid == watchdog_pid){
        gettimeofday(&prev_t, NULL);
    }  
}

int main(int argc, char *argv[]) 
{
    // Publish your pid
    process_id = getpid();

    // Read watchdog pid
    FILE *watchdog_fp = fopen(PID_FILE_PW, "r");
    fscanf(watchdog_fp, "%d", &watchdog_pid);
    printf("watchdog pid %d \n", watchdog_pid);
    fclose(watchdog_fp);

    // Set up sigaction for receiving signals from processes
    struct sigaction p_action;
    p_action.sa_flags = SA_SIGINFO;
    p_action.sa_sigaction = watchdog_handler;
    if(sigaction(SIGUSR1, &p_action, NULL) < 0)
    {
        perror("sigaction");
    }

    // initialize semaphore
    sem_t * sem_id = sem_open(SEM_PATH, O_CREAT, S_IRUSR | S_IWUSR, 1);
    sem_init(sem_id, 1, 0); //initialized to 0 until shared memory is instantiated

    int cells[2] = {0, 1};
    int shared_seg_size = (1 * sizeof(cells));

    // create shared memory object
    int shmfd  = shm_open(SHMOBJ_PATH, O_CREAT | O_RDWR, S_IRWXU | S_IRWXG);
    if (shmfd < 0)
    {
        perror("shm_open");
        return -1;
    }
    // truncate size of shared memory
    ftruncate(shmfd, shared_seg_size);
    // map pointer
    void* shm_ptr = mmap(NULL, shared_seg_size, PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0);
    // post semaphore
    sem_post(sem_id);

    // every two seconds print the current values in the server (for debugging)
    while (1) 
    {      
        sem_wait(sem_id);
        memcpy(cells, shm_ptr, shared_seg_size);
        sem_post(sem_id);
        printf("serving %i %i\n", cells[0], cells[1]);

        pause(); // Wait for signals

        sleep(2);
    } 

    // clean up
    sem_close(sem_id);
    munmap(shm_ptr, shared_seg_size);

    return 0; 
} 
