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

#define SHM_DRN 12

#define PIPE_READ 0
#define PIPE_WRITE 1

#define M 1.0
#define K 0.1

// Initialize position and velocity
double x = 0, y = 0;
double vx = 0, vy = 0;

// Initialize forces
double fx = 0, fy = 0;

pid_t watchdog_pid;
pid_t process_id;
char *process_name;
struct timeval prev_t;
char logfile_name[256] = LOG_FILE_NAME;
char command;

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
struct Drone drone = {0, 0, 0, 0, 0, 0, 'W', 1};

int pipefd[2];
fd_set read_fds;
struct timeval tv;
int retval;

// logs time update to file
void log_receipt(struct timeval tv)
{
    FILE *lf_fp = fopen(logfile_name, "a");
    fprintf(lf_fp, "%d %ld %ld\n", process_id, tv.tv_sec, tv.tv_usec);
    fclose(lf_fp);
}

void watchdog_handler(int sig, siginfo_t *info, void *context)
{
    if (info->si_pid == watchdog_pid)
    {
        gettimeofday(&prev_t, NULL);
        log_receipt(prev_t);
    }
}

int main(int argc, char *argv[])
{
    // Set up sigaction for receiving signals from processes
    struct sigaction p_action;
    p_action.sa_flags = SA_SIGINFO;
    p_action.sa_sigaction = watchdog_handler;
    if (sigaction(SIGUSR1, &p_action, NULL) < 0)
    {
        perror("sigaction");
    }

    int process_num;
    if (argc == 4)
    {
        sscanf(argv[1], "%d", &process_num);
        sscanf(argv[2], "%d", &pipefd[PIPE_READ]);
        sscanf(argv[3], "%d", &pipefd[PIPE_WRITE]);
    }
    else
    {
        printf("wrong args\n");
        return -1;
    }

    printf("pipefd[0] = %d, pipefd[1] = %d\n", pipefd[PIPE_READ], pipefd[PIPE_WRITE]);
    // close(pipefd[PIPE_WRITE]);

    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);

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

    // Create a shared memory segment
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

    double x_1 = 0;
    double y_1 = 0;
    double x_2 = 0;
    double y_2 = 0;
    
    while (1)
    {
        x_1 = shared_drone->x_1;
        x_2 = shared_drone->x_2;
        y_1 = shared_drone->y_1;
        y_2 = shared_drone->y_2;
        shared_drone->symbol = drone.symbol;
        shared_drone->color_pair = drone.color_pair;

        // Read from pipe
        char command;
        printf("Reading from pipe\n");
        int bytesRead = read(pipefd[PIPE_READ], &command, sizeof(char));
        printf("Read %d bytes\n", bytesRead);
        printf("Command: %c\n", command);
        if (bytesRead > 0)
        {
            switch (command)
            {
            case 'x':
                fy += 1.0; // forza verso USx
                fx -= 1.0;
                break;
            case 'c':
                fy += 1.0; // forza verso U
                break;
            case 'v':
                fy += 1.0; // forza verso UDx
                fx += 1.0;
                break;
            case 's':
                fx -= 1.0; // forza verso Sx
                break;
            case 'd':
                fy = 0; // annulla forza
                fx = 0;
                break;
            case 'f':
                fx += 1.0; // forza verso Dx
                break;
            case 'w':
                fy -= 1.0; // forza verso DSx
                fx -= 1.0;
                break;
            case 'e':
                fy -= 1.0; // forza verso D
                break;
            case 'r':
                fy -= 1.0; // forza verso DDx
                fx += 1.0;
                break;
            }
            mvprintw(0, 0, "Comando inviato: %c\n", command);
        }
        else if (bytesRead == 0)
        {
            printf("Nothing to read\n");
        }
        else
        {
            perror("read");
        }
        endwin();

        // Update velocity and position
        double ax = fx / M;
        double ay = fy / M;
        if (ax == 0 && ay == 0)
        {
            vx = 0;
            vy = 0;
        }
        else
        {
            vx += ax;
            vy += ay;
            vx *= (1 - K);
            vy *= (1 - K);
        }

        //(vx/(1 - K)*M )-vx =  fx ;

        // Apply friction

        drone.x += vx;
        drone.y += vy;
        refresh();

        // Use the shared memory
        shared_drone->x = drone.x;
        shared_drone->y = drone.y;
        shared_drone->symbol = drone.symbol;
        shared_drone->color_pair = drone.color_pair;

        clear(); // Clear the screen of all previously-printed characters
    }

    // Detach the shared memory segment from our process's address space
    if (shmdt(shared_drone) == -1)
    {
        perror("shmdt");
        return -1;
    }

    // Close the write end of the pipe when you're done with it
    // close(pipefd[PIPE_WRITE]);
    return 0;
}