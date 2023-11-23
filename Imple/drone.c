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

// #define M 1.0
// #define K 0.1
#define T 0.01

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
    char symbol;
    short color_pair;
};
struct Drone drone;

int pipedi[2];
int pipesd[2];
int pipeds[2];

struct timeval tv;

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
    if (argc == 8)
    {
        sscanf(argv[1], "%d", &process_num);
        sscanf(argv[2], "%d", &pipedi[PIPE_READ]);
        sscanf(argv[3], "%d", &pipedi[PIPE_WRITE]);
        sscanf(argv[4], "%d", &pipesd[PIPE_READ]);
        sscanf(argv[5], "%d", &pipesd[PIPE_WRITE]);
        sscanf(argv[6], "%d", &pipeds[PIPE_READ]);
        sscanf(argv[7], "%d", &pipeds[PIPE_WRITE]);
    }
    else
    {
        printf("wrong args\n");
        return -1;
    }

    printf("pipedi[0] = %d, pipedi[1] = %d\n", pipedi[PIPE_READ], pipedi[PIPE_WRITE]);
    printf("pipesd[0] = %d, pipesd[1] = %d\n", pipesd[PIPE_READ], pipesd[PIPE_WRITE]);
    printf("pipeds[0] = %d, pipeds[1] = %d\n", pipeds[PIPE_READ], pipeds[PIPE_WRITE]);

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

    FILE *file = fopen("file_para.txt", "r");
    if (file == NULL)
    {
        printf("Could not open file_para.txt for reading\n");
        return 1;
    }
    struct Drone drone;
    float M, K;

    char label[256];
    while (fscanf(file, "%s", label) != EOF)
    {
        if (strcmp(label, "drone.symbol") == 0)
        {
            fscanf(file, " = '%c'", &drone.symbol);
        }
        else if (strcmp(label, "drone.color_paire") == 0)
        {
            fscanf(file, " = %hd", &drone.color_pair);
        }
        else if (strcmp(label, "M") == 0)
        {
            fscanf(file, " = %f", &M);
        }
        else if (strcmp(label, "K") == 0)
        {
            fscanf(file, " = %f", &K);
        }
        else if (strcmp(label, "drone.x") == 0)
        {
            fscanf(file, " = %d", &drone.x);
        }
        else if (strcmp(label, "drone.y") == 0)
        {
            fscanf(file, " = %d", &drone.y);
        }
    }

    fclose(file);

    printf("drone.symbol = %c\n", drone.symbol);
    printf("drone.color_paire = %hd\n", drone.color_pair);
    printf("M = %f\n", M);
    printf("K = %f\n", K);
    printf("drone.x = %d\n", drone.x);
    printf("drone.y = %d\n", drone.y);
    sleep(10);
    // shared_drone->x = drone.x;
    // shared_drone->y = drone.y;
    // shared_drone->symbol = drone.symbol;
    // shared_drone->color_pair = drone.color_pair;
    write(pipeds[PIPE_READ], &drone, sizeof(drone));
    static double prev_x = 0, prev_y = 0;
    static double prev_vx = 0, prev_vy = 0;
    double Fx = fx;
    double Fy = fy;

    while (1)
    {
        char command = '\0';
        printf("Reading from pipe\n");
        int bytesRead = read(pipedi[PIPE_READ], &command, sizeof(char));
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
            case '\0':
                command = '\0'; // Comando non valido
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

        // Update velocity and position
        double ax = (fx / M) - (K * vx);
        double ay = (fy / M) - (K * vy);
        vx = prev_vx + ax * T;
        vy = prev_vy + ay * T;

        double new_x = prev_x + vx * T;
        double new_y = prev_y + vy * T;

        // Store the current position and velocity for the next iteration
        prev_x = new_x;
        prev_y = new_y;
        prev_vx = vx;
        prev_vy = vy;

        mvprintw(1, 0, "x: %.2f, y: %.2f\n", new_x, new_y);
        mvprintw(2, 0, "vx: %f, vy: %f\n", vx, vy);

        refresh();

        // Use the shared memory
        // shared_drone->x = (int)new_x;
        // shared_drone->y = (int)new_y;
        // shared_drone->symbol = drone.symbol;
        // shared_drone->color_pair = drone.color_pair;
        drone.x = (int)new_x;
        drone.y = (int)new_y;
        drone.symbol = drone.symbol;
        drone.color_pair = drone.color_pair;
        write(pipeds[PIPE_WRITE], &drone, sizeof(drone));

        clear(); // Clear the screen of all previously-printed characters
    }
    endwin();
    // Detach the shared memory segment from our process's address space
    if (shmdt(shared_drone) == -1)
    {
        perror("shmdt");
        return -1;
    }

    // Close the write end of the pipe when you're done with it
    // close(pipedi[PIPE_WRITE]);
    return 0;
}