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
    char symbol;
};
struct Target
{
    int x;
    int y;
    char symbol;
    bool is_active;
    bool is_visible;
};

struct World
{
    struct Drone drone;
    struct Obstacle obstacle[9];
    struct Screen screen;
    struct Target target[9];
};

int pipesw[2];
int pipews[2];
int pipesd_s[2];

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

    // Define a signal set
    sigset_t set;

    // Initialize the signal set to empty
    sigemptyset(&set);

    // Add SIGUSR1 to the set
    sigaddset(&set, SIGUSR1);

    // Block SIGUSR1
    if (sigprocmask(SIG_BLOCK, &set, NULL) < 0)
    {
        perror("sigprocmask"); // Print an error message if the signal can't be blocked
        return -1;
    }
    // Set up sigaction for receiving signals from the watchdog process
    struct sigaction p_action;
    p_action.sa_flags = SA_SIGINFO;
    p_action.sa_sigaction = watchdog_handler;
    if (sigaction(SIGUSR1, &p_action, NULL) < 0)
    {
        perror("sigaction"); // Print an error message if the signal can't be set up
    }

    // Unblock SIGUSR1
    if (sigprocmask(SIG_UNBLOCK, &set, NULL) < 0)
    {
        perror("sigprocmask"); // Print an error message if the signal can't be unblocked
        return -1;
    }

    int process_num;
    if (argc == 8)
    {
        sscanf(argv[1], "%d", &process_num);
        sscanf(argv[2], "%d", &pipesw[PIPE_READ]);
        sscanf(argv[3], "%d", &pipesw[PIPE_WRITE]);
        sscanf(argv[4], "%d", &pipews[PIPE_READ]);
        sscanf(argv[5], "%d", &pipews[PIPE_WRITE]);
        sscanf(argv[6], "%d", &pipesd_s[PIPE_READ]);
        sscanf(argv[7], "%d", &pipesd_s[PIPE_WRITE]);
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
    printf("pipe read %d \n", pipesd_s[PIPE_READ]);
    printf("pipe write %d \n", pipesd_s[PIPE_WRITE]);

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

    init_pair(1, COLOR_WHITE, COLOR_BLACK);
    init_pair(2, COLOR_BLUE, COLOR_BLACK);
    init_pair(3, COLOR_RED, COLOR_BLACK);
    init_pair(4, COLOR_GREEN, COLOR_BLACK);

    struct World world;
    int height, width;
    while (1)
    {

        clear();

        // Read data from the pipe
        if (read(pipesw[PIPE_READ], &world.drone, sizeof(world.drone)) == -1)
        {
            perror("read drone");
            continue;
        }
        if (read(pipesw[PIPE_READ], &world.obstacle, sizeof(world.obstacle)) == -1)
        {
            perror("read obstacle");
            continue;
        }
        if (read(pipesw[PIPE_READ], &world.target, sizeof(world.target)) == -1)
        {
            perror("read target");
            continue;
        }

        getmaxyx(win, height, width);
        world.screen.height = height;
        world.screen.width = width;
        mvprintw(3, 3, "Screen: height: %d, width: %d", world.screen.height, world.screen.width);
        clear();

        // Print the target symbols at their positions if they're within the window dimensions
        for (int i = 0; i < 9; i++)
        {
            if ((world.target[i].y < height && world.target[i].x < width) && world.target[i].is_visible)
            {
                attron(COLOR_PAIR(4));
                mvprintw(world.target[i].y, world.target[i].x, "%c", world.target[i].symbol);
                attroff(COLOR_PAIR(4));
            }
        }

        for (int i = 0; i < 20; i++)
        {
            if (world.obstacle[i].y < height && world.obstacle[i].x < width)
            {
                attron(COLOR_PAIR(3));
                mvprintw(world.obstacle[i].y, world.obstacle[i].x, "%c", world.obstacle[i].symbol);
                attroff(COLOR_PAIR(3));
            }
        }

        if (world.drone.y < height && world.drone.x < width)
        {
            attron(COLOR_PAIR(world.drone.color_pair));
            mvprintw(world.drone.y, world.drone.x, "%c", world.drone.symbol);
            attroff(COLOR_PAIR(world.drone.color_pair));
        }

        refresh();

        // Write the screen dimensions to the pipe
        if (write(pipews[PIPE_WRITE], &world.screen, sizeof(world.screen)) == -1)
        {
            perror("write screen");
            continue;
        }
        else
        {
            fsync(pipews[PIPE_WRITE]) == -1;
        }

        if (write(pipesd_s[PIPE_WRITE], &world.screen, sizeof(world.screen)) == -1)
        {
            perror("write screen");
            continue;
        }
        else
        {
            fsync(pipesd_s[PIPE_WRITE]) == -1;
        }

        refresh(); // Refresh the screen to show the changes
    }
    endwin();
    return 0;
}