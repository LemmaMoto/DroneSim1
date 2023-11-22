#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <time.h>
#include "include/constants.h"
#include <sys/ipc.h>
#include <sys/shm.h>

#define SHM_DRN 12 // Define a key for the shared memory segment
#define SHM_WRLD 34

pid_t watchdog_pid;
pid_t process_id;
char *process_name;
struct timeval prev_t;
char logfile_name[256] = LOG_FILE_NAME;

struct Drone
{
    int x;
    int y;
    int x_1;
    int y_1;
    int x_2;
    int y_2;
    char symbol;
    short color_pair;
};

// logs time update to file
void log_receipt(struct timeval tv)
{
    FILE *lf_fp = fopen(logfile_name, "a");
    fprintf(lf_fp, "%d %ld %ld\n", process_id, tv.tv_sec, tv.tv_usec);
    fclose(lf_fp);
}

void watchdog_handler(int sig, siginfo_t *info, void *context)
{
    printf("received signal \n");
    if (info->si_pid == watchdog_pid)
    {
        gettimeofday(&prev_t, NULL);
        log_receipt(prev_t);
    }
}

int main(int argc, char *argv[])
{

    struct Drone drone = {0, 0, 0, 0, 0, 0, 'W', 1};
    int process_num;
    if (argc == 2)
    {
        sscanf(argv[1], "%d", &process_num);
    }
    else
    {
        printf("wrong args\n");
        return -1;
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

    // Set up sigaction for receiving signals from processes
    struct sigaction p_action;
    p_action.sa_flags = SA_SIGINFO;
    p_action.sa_sigaction = watchdog_handler;
    if (sigaction(SIGUSR1, &p_action, NULL) < 0)
    {
        perror("sigaction");
    }

    // Read how long to sleep process for
    int sleep_durations[NUM_PROCESSES] = PROCESS_SLEEPS_US;
    int sleep_duration = sleep_durations[process_num];
    char *process_names[NUM_PROCESSES] = PROCESS_NAMES;
    process_name = process_names[process_num]; // added to logfile for readability

    int shm_id = shmget(SHM_DRN, sizeof(struct Drone), IPC_CREAT | 0666);
    if (shm_id < 0)
    {
        perror("shmget");
        return -1;
    }

    // Attach the shared memory segment to our process's address space
    struct Drone *shared_drone = (struct Drone *)shmat(shm_id, NULL, 0);
    if (shared_drone == (struct Drone *)-1)
    {
        perror("shmat");
        return -1;
    }

    int shm_id2 = shmget(SHM_WRLD, sizeof(struct Drone), IPC_CREAT | 0666);
    if (shm_id2 < 0)
    {
        perror("shmget");
        return -1;
    }

    // Attach the shared memory segment to our process's address space
    struct Drone *shared_wrld = (struct Drone *)shmat(shm_id2, NULL, 0);
    if (shared_wrld == (struct Drone *)-1)
    {
        perror("shmat");
        return -1;
    }

    // Use the shared memory
    drone.symbol = shared_drone->symbol;
    drone.color_pair = shared_drone->color_pair;
    shared_wrld->symbol = drone.symbol;
    shared_wrld->color_pair = drone.color_pair;

    while (1)
    {

        drone.x = shared_drone->x;
        drone.y = shared_drone->y;

        // printf("x: %d, y: %d, symbol: %c, color_pair: %d\n", drone.x, drone.y, drone.symbol, drone.color_pair);
        sleep(0.5);
        shared_wrld->x = drone.x;
        shared_wrld->y = drone.y;

        printf("x: %d, y: %d, symbol: %c, color_pair: %d\n", shared_wrld->x, shared_wrld->y, shared_wrld->symbol, shared_wrld->color_pair);
        sleep(0.5);
        drone.x_2 = shared_drone->x_1;
        drone.y_2 = shared_drone->y_1;
        drone.x_1 = shared_drone->x;
        drone.y_1 = shared_drone->y;
    }

    // Detach the shared memory segment from our process's address space
    if (shmdt(shared_drone) == -1)
    {
        perror("shmdt");
        return -1;
    }

    // Detach the shared memory segment from our process's address space
    if (shmdt(shared_wrld) == -1)
    {
        perror("shmdt");
        return -1;
    }

    return 0;
}