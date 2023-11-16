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
#include "include/constants.h"

int main(int argc, char *argv[]) 
{
    int reader_num;
    int in_progress = 0;
    int number_received;

    char filename_string[80];
    char filedirectory_string[80];

    FILE * fp;

    // extract reader number
    if(argc == 2){
        sscanf(argv[1],"%i", &reader_num);
    } else {
        printf("wrong args\n"); 
        return -1;
    }

    int cells[2];
    int shared_seg_size = (1 * sizeof(cells));

    // make the log file name
    time_t now = time(NULL);
    struct tm *timenow;
    timenow = gmtime(&now);

    // There should be a check that those folders exist but I haven't done that
    sprintf(filename_string, "./log/reader_%i/%i-%i-%i_%i:%i:%i.txt", reader_num, timenow->tm_year + 1900, timenow->tm_mon, timenow->tm_mday, timenow->tm_hour, timenow->tm_min, timenow->tm_sec);
    //clears contents of log file
    fp = fopen(filename_string, "w");
    fclose(fp);

    // iniitialize pointers to semaphore and shared memory
    sem_t * sem_id = sem_open(SEM_PATH, 0);

    int shmfd  = shm_open(SHMOBJ_PATH, O_RDONLY, S_IRWXU | S_IRWXG);
    if (shmfd < 0)
    {
        perror("shm_open");
        return -1;
    }
    void* shm_ptr = mmap(NULL, shared_seg_size, PROT_READ, MAP_SHARED, shmfd, 0);


    while (1) 
    {     
        // wait for semaphore
        sem_wait(sem_id);
        // copy memory to local cells
        memcpy(cells, shm_ptr, shared_seg_size);
        // post semaphore as soon as we are done
        sem_post(sem_id);

        // if the value has changed, log to log file and print to console
        if(cells[reader_num] != number_received)
        {
            number_received = cells[reader_num];
            printf("Value has changed to %i \n", number_received);
            fp = fopen(filename_string, "a");
            fprintf(fp, "Logged %i in cell %i \n", number_received, reader_num);
            fclose(fp);
        }
        sleep(2);
    } 

    // clean up
    shm_unlink(SHMOBJ_PATH);
    sem_close(sem_id);
    sem_unlink(SEM_PATH);
    return 0; 
} 