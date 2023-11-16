#include <stdio.h> 
#include <string.h> 
#include <fcntl.h> 
#include <sys/stat.h> 
#include <sys/types.h> 
#include <sys/select.h>
#include <unistd.h> 
#include <stdlib.h>
#include "include/constants.h"
#include <semaphore.h>
#include <sys/mman.h>


int main(int argc, char *argv[]) 
{
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
    // copy initial data into cells
    // memcpy(shm_ptr, cells, shared_seg_size);
    // post semaphore
    sem_post(sem_id);

    // every two seconds print the current values in the server (for debugging)
    while (1) 
    {      
        sem_wait(sem_id);
        memcpy(cells, shm_ptr, shared_seg_size);
        sem_post(sem_id);
        printf("serving %i %i\n", cells[0], cells[1]);

        sleep(2);
    } 

    // clean up
    shm_unlink(SHMOBJ_PATH);
    sem_close(sem_id);
    sem_unlink(SEM_PATH);
    munmap(shm_ptr, shared_seg_size);

    return 0; 
} 