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
#include <errno.h>
#include <stdarg.h>

#define PIPE_READ 0
#define PIPE_WRITE 1

pid_t watchdog_pid;
pid_t process_id;
char *process_name;
struct timeval prev_t;
char logfile_name[256] = LOG_FILE_NAME;

void error(char *msg)
{
    FILE *logFile = fopen("log/world/error_log_world.txt", "a");
    if (logFile != NULL)
    {
        time_t now = time(NULL);
        char timeStr[20]; // Buffer to hold the time string
        strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", localtime(&now));
        fprintf(logFile, "%s - %s: %s\n", timeStr, msg, strerror(errno));
        fclose(logFile);
    }
    else
    {
        perror("ERROR opening log file");
    }
}
void Printf(char *format, ...)
{
    FILE *logFile = fopen("log/world/log_world.txt", "a");
    if (logFile != NULL)
    {
        time_t now = time(NULL);
        char timeStr[64]; // Buffer to hold the time string
        strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", localtime(&now));
        fprintf(logFile, "%s - ", timeStr);

        va_list args;
        va_start(args, format);
        vfprintf(logFile, format, args);
        va_end(args);

        fprintf(logFile, "\n");
        fclose(logFile);
    }
    else
    {
        perror("ERROR opening log file");
    }
}

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
    struct Target target[10];
};

int pipesw[2];
int pipews[2];
int pipesd_s[2];
int pipesw_o[2];
int pipesw_t[2];

// logs time update to file
void log_receipt(struct timeval tv)
{
    FILE *lf_fp = fopen(logfile_name, "a");
    fprintf(lf_fp, "%d %ld %ld\n", process_id, tv.tv_sec, tv.tv_usec);
    fclose(lf_fp);
}

void watchdog_handler(int sig, siginfo_t *info, void *context)
{
    // Printf("received signal \n");
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
        error("sigprocmask"); // Print an error message if the signal can't be blocked
        return -1;
    }
    // Set up sigaction for receiving signals from the watchdog process
    struct sigaction p_action;
    p_action.sa_flags = SA_SIGINFO;
    p_action.sa_sigaction = watchdog_handler;
    if (sigaction(SIGUSR1, &p_action, NULL) < 0)
    {
        error("sigaction"); // Print an error message if the signal can't be set up
    }

    // Unblock SIGUSR1
    if (sigprocmask(SIG_UNBLOCK, &set, NULL) < 0)
    {
        error("sigprocmask"); // Print an error message if the signal can't be unblocked
        return -1;
    }

    int process_num;
    if (argc == 12)
    {
        sscanf(argv[1], "%d", &process_num);
        sscanf(argv[2], "%d", &pipesw[PIPE_READ]);
        sscanf(argv[3], "%d", &pipesw[PIPE_WRITE]);
        sscanf(argv[4], "%d", &pipews[PIPE_READ]);
        sscanf(argv[5], "%d", &pipews[PIPE_WRITE]);
        sscanf(argv[6], "%d", &pipesd_s[PIPE_READ]);
        sscanf(argv[7], "%d", &pipesd_s[PIPE_WRITE]);
        sscanf(argv[8], "%d", &pipesw_o[PIPE_READ]);
        sscanf(argv[9], "%d", &pipesw_o[PIPE_WRITE]);
        sscanf(argv[10], "%d", &pipesw_t[PIPE_READ]);
        sscanf(argv[11], "%d", &pipesw_t[PIPE_WRITE]);
    }
    else
    {
        Printf("wrong args\n");
        return -1;
    }

    Printf("process num %d \n", process_num);
    Printf("pipe read %d \n", pipesw[PIPE_READ]);
    Printf("pipe write %d \n", pipesw[PIPE_WRITE]);
    Printf("pipe read %d \n", pipews[PIPE_READ]);
    Printf("pipe write %d \n", pipews[PIPE_WRITE]);
    Printf("pipe read %d \n", pipesd_s[PIPE_READ]);
    Printf("pipe write %d \n", pipesd_s[PIPE_WRITE]);

    // Publish your pid
    process_id = getpid();

    char *fnames[NUM_PROCESSES] = PID_FILE_SP;

    FILE *pid_fp = fopen(fnames[process_num], "w");
    fprintf(pid_fp, "%d", process_id);
    fclose(pid_fp);

    Printf("Published pid %d \n", process_id);

    // Read watchdog pid
    FILE *watchdog_fp = NULL;
    struct stat sbuf;

    /* call stat, fill stat buffer, validate success */
    if (stat(PID_FILE_PW, &sbuf) == -1)
    {
        error("error-stat");
        return -1;
    }
    // waits until the file has data
    while (sbuf.st_size <= 0)
    {
        if (stat(PID_FILE_PW, &sbuf) == -1)
        {
            error("error-stat");
            return -1;
        }
        usleep(50000);
    }

    watchdog_fp = fopen(PID_FILE_PW, "r");

    fscanf(watchdog_fp, "%d", &watchdog_pid);
    Printf("watchdog pid %d \n", watchdog_pid);
    fclose(watchdog_fp);

    // Read how long to sleep process for
    int sleep_durations[NUM_PROCESSES] = PROCESS_SLEEPS_US;
    int sleep_duration = sleep_durations[process_num];
    char *process_names[NUM_PROCESSES] = PROCESS_NAMES;
    process_name = process_names[process_num]; // added to logfile for readability

    FILE *file = fopen("file_para.txt", "r");
    if (file == NULL)
    {
        error("Unable to open file");
        exit(EXIT_FAILURE);
    }

    char line[256];
    int NUM_OBSTACLES = 0;
    int NUM_TARGETS = 0;
    int refresh_time_obstacles = 0;

    while (fgets(line, sizeof(line), file))
    {
        if (sscanf(line, "NUM_OBSTACLES = %d", &NUM_OBSTACLES) == 1)
        {
            // Se abbiamo trovato una riga che corrisponde al formato "NUM_OBSTACLES = %d",
            // interrompiamo il ciclo
            continue;
        }
        else if (sscanf(line, "NUM_TARGETS = %d", &NUM_TARGETS) == 1)
        {
            continue;
        }
        else if (sscanf(line, "refresh_time_obstacles = %d", &refresh_time_obstacles) == 1)
        {
            break;
        }
    }
    mvprintw(3, 3, "NUM_OBSTACLES = %d\n", NUM_OBSTACLES);
    refresh();
    // fclose(file);

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

    struct World world = {0};
    int height, width;
    int n;
    while (1)
    {
        Printf("world loop %d\n",sizeof(world));

        struct Drone droneTemp;
        n = read(pipesw[PIPE_READ], &droneTemp, sizeof(world.drone));
        if (n > 0)
        {
            
            world.drone = droneTemp;
            //Printf("drone x: AAAAAAA %d, y: %d , color: %d\n", world.drone.x, world.drone.y, world.drone.color_pair);
        }
        else
        {
            error("read drone");
        }

        
        struct Obstacle obstacleTemp[9];
        n = read(pipesw_o[PIPE_READ], &obstacleTemp, sizeof(world.obstacle));
        if (n > 0)
        {
            for (int i = 0; i < NUM_OBSTACLES; i++)
            {
                world.obstacle[i] = obstacleTemp[i];
                //Printf("obstacle AAAAAAA%d: x: %d, y: %d, symbol: %c\n", i, world.obstacle[i].x, world.obstacle[i].y, world.obstacle[i].symbol);
            }
        }
        else
        {
            error("read obstacle");
        }

        struct Target targetTemp[10];
        n = read(pipesw_t[PIPE_READ], &targetTemp, sizeof(world.target));
        if (n > 0)
        {
            for (int i = 0; i < NUM_TARGETS; i++)
            {
                world.target[i] = targetTemp[i];
                //Printf("target AAAAAA%d: x: %d, y: %d, symbol: %c, is_active: %d, is_visible: %d\n", i, world.target[i].x, world.target[i].y, world.target[i].symbol, world.target[i].is_active, world.target[i].is_visible);
            }
        }
        else
        {
            error("read target");
        }



        getmaxyx(win, height, width);
        world.screen.height = height;
        world.screen.width = width;
        clear();
        // mvprintw(3, 3, "Screen: height: %d, width: %d", world.screen.height, world.screen.width);

        for (int i = 1; i < NUM_TARGETS; i++)
        {
            Printf("target %d: x: %d, y: %d\n", i, world.target[i].x, world.target[i].y);
        }

        //Print the target symbols at their positions if they're within the window dimensions
        for (int i = 0; i < NUM_TARGETS; i++)
        {
            if ((world.target[i].y < height && world.target[i].x < width) && world.target[i].is_visible)
            {
                attron(COLOR_PAIR(4));
                mvprintw(world.target[i].y, world.target[i].x, "T" );
                attroff(COLOR_PAIR(4));
            }
        }
        

        for (int i = 1; i < NUM_OBSTACLES; i++)
        {
            if (world.obstacle[i].y < height && world.obstacle[i].x < width)
            {
                attron(COLOR_PAIR(3));
                mvprintw(world.obstacle[i].y, world.obstacle[i].x, "#");
                attroff(COLOR_PAIR(3));
            }
        }
        

        Printf("drone x: %d, y: %d\n", world.drone.x, world.drone.y);
        Printf("Height x: %d, width y: %d\n", height, width);
        if (world.drone.y < height && world.drone.x < width)
        {
            attron(COLOR_PAIR(world.drone.color_pair));
            mvprintw(world.drone.y, world.drone.x, "W");
            attroff(COLOR_PAIR(world.drone.color_pair));
        }

        refresh();

        // Write the screen dimensions to the pipe
        if (write(pipews[PIPE_WRITE], &world.screen, sizeof(world.screen)) == -1)
        {
            error("write screen");
            continue;
        }
        else
        {
            fsync(pipews[PIPE_WRITE]) == -1;
        }

        if (write(pipesd_s[PIPE_WRITE], &world.screen, sizeof(world.screen)) == -1)
        {
            error("write screen");
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