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
#include <ncurses.h>
#include <stdbool.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define SHM_WRLD 34 // Define a key for the shared memory segment

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

    initscr();
    start_color();
    curs_set(0);
    timeout(100);
    srand(time(NULL));

    init_pair(1, COLOR_BLACK, COLOR_WHITE);

    struct Drone drone = {0, 0, 'W', 1};

    // Create a shared memory segment
    int shm_id = shmget(SHM_WRLD, sizeof(struct Drone), IPC_CREAT | 0666);
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

    shared_drone->symbol = drone.symbol;
    shared_drone->color_pair = drone.color_pair;
    while (1)
    {
        // Use the shared memory
        shared_drone->x = drone.x;
        shared_drone->y = drone.y;
        mvprintw(drone.y, drone.x, "%c", drone.symbol); // Print the drone symbol at the drone position
        refresh();                                      // Refresh the screen to show the changes
        clear();                                        // Clear the screen of all previously-printed characters

        sleep(3); // Wait for 5 seconds so you can see the output
    }
    // Detach the shared memory segment from our process's address space
    if (shmdt(shared_drone) == -1)
    {
        perror("shmdt");
        return -1;
    }
    endwin();
    return 0;
}