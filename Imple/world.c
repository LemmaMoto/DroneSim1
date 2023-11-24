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

struct Screen
{
    int height;
    int width;
};

struct Obstacle
{
    int x;
    int y;
};
struct World
{
    struct Drone drone;
    struct Obstacle obstacle;
    struct Screen screen;
};

int pipesw[2];
int pipews[2];

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
    if (argc == 6)
    {
        sscanf(argv[1], "%d", &process_num);
        sscanf(argv[2], "%d", &pipesw[PIPE_READ]);
        sscanf(argv[3], "%d", &pipesw[PIPE_WRITE]);
        sscanf(argv[4], "%d", &pipews[PIPE_READ]);
        sscanf(argv[5], "%d", &pipews[PIPE_WRITE]);
    }
    else
    {
        printf("wrong args\n");
        return -1;
    }

    printf("process num %d \n", process_num);
    printf("pipe read %d \n", pipesw[PIPE_READ]);
    printf("pipe write %d \n", pipesw[PIPE_WRITE]);
    printf("pipe read %d \n", pipews[PIPE_READ]);
    printf("pipe write %d \n", pipews[PIPE_WRITE]);

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

    WINDOW *win;
    win = initscr();
    start_color();
    curs_set(0);
    timeout(100);
    srand(time(NULL));

    init_pair(1, COLOR_BLACK, COLOR_WHITE);

    struct World world;
    int height, width;
    while (1)
    {
        clear();
        read(pipesw[PIPE_READ], &world.drone, sizeof(world.drone));
        mvprintw(world.drone.y, world.drone.x, "%c", world.drone.symbol); // Print the drone symbol at the drone position
        getmaxyx(win, height, width);
        world.screen.height = height;
        world.screen.width = width;
        mvprintw(20, 20, "height: %d, width: %d\n", height, width);
        write(pipews[PIPE_WRITE], &world.screen, sizeof(world.screen));
        refresh(); // Refresh the screen to show the changes
    }
    endwin();
    return 0;
}