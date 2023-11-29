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
#include <math.h>
#include <float.h>


#define PIPE_READ 0
#define PIPE_WRITE 1
#define OBSTACLE_REPULSION_CONSTANT 100.0
#define TARGET_ATTRACTION_CONSTANT 1000

#define T 0.01

// Initialize position and velocity
double x, y;
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

struct Screen
{
    int height;
    int width;
};

struct World
{
    struct Drone drone;
    struct Obstacle obstacle[700];
    struct Screen screen;
    struct Target target[9];
};

int pipedi[2];
int pipesd[2];
int pipeds[2];
int pipesd_t[2];

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
    if (argc == 10)
    {
        sscanf(argv[1], "%d", &process_num);
        sscanf(argv[2], "%d", &pipedi[PIPE_READ]);
        sscanf(argv[3], "%d", &pipedi[PIPE_WRITE]);
        sscanf(argv[4], "%d", &pipesd[PIPE_READ]);
        sscanf(argv[5], "%d", &pipesd[PIPE_WRITE]);
        sscanf(argv[6], "%d", &pipeds[PIPE_READ]);
        sscanf(argv[7], "%d", &pipeds[PIPE_WRITE]);
        sscanf(argv[8], "%d", &pipesd_t[PIPE_READ]);
        sscanf(argv[9], "%d", &pipesd_t[PIPE_WRITE]);
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

    FILE *file = fopen("file_para.txt", "r");
    if (file == NULL)
    {
        printf("Could not open file_para.txt for reading\n");
        return 1;
    }
    struct World world;
    float M, K;

    char label[256];
    while (fscanf(file, "%s", label) != EOF)
    {
        if (strcmp(label, "drone.symbol") == 0)
        {
            fscanf(file, " = '%c'", &world.drone.symbol);
        }
        else if (strcmp(label, "drone.color_paire") == 0)
        {
            fscanf(file, " = %hd", &world.drone.color_pair);
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
            fscanf(file, " = %d", &world.drone.x);
        }
        else if (strcmp(label, "drone.y") == 0)
        {
            fscanf(file, " = %d", &world.drone.y);
        }
    }

    fclose(file);

    printf("drone.symbol = %c\n", world.drone.symbol);
    printf("drone.color_paire = %hd\n", world.drone.color_pair);
    printf("M = %f\n", M);
    printf("K = %f\n", K);
    printf("drone.x = %d\n", world.drone.x);
    printf("drone.y = %d\n", world.drone.y);
    write(pipeds[PIPE_WRITE], &world.drone, sizeof(world.drone));
    double fx = 0, fy = 0;
    double prev_x = world.drone.x, prev_y = world.drone.y;
    static double prev_vx = 0, prev_vy = 0;
    double Fx = fx;
    double Fy = fy;

    while (1)
    {
        char command = '\0';
        printf("Reading from pipe\n");
        if (read(pipesd[PIPE_READ], &world.obstacle, sizeof(world.obstacle)) == -1)
        {
            perror("read obstacle");
            continue;
        }
        else
        {
            int closest_obstacle_index = -1;
            double min_distance = DBL_MAX;

            // Find the closest obstacle
            for (int i = 0; i < 700; ++i)
            {
                double dx = world.drone.x - world.obstacle[i].x;
                double dy = world.drone.y - world.obstacle[i].y;
                double distance = sqrt(dx * dx + dy * dy); // Use absolute value to get the distance

                if (distance < min_distance)
                {
                    min_distance = distance;
                    closest_obstacle_index = i;
                }
            }

            // Now calculate the distance from the closest obstacle
            double dx = world.drone.x - world.obstacle[closest_obstacle_index].x;
            double dy = world.drone.y - world.obstacle[closest_obstacle_index].y;
            double distance = sqrt(dx * dx + dy * dy);

            if (distance < 4) // Check if the obstacle is within the circle of radius 2
            {
                double repulsion_force = OBSTACLE_REPULSION_CONSTANT * ((1 / distance) - (1 / 4)) * (1 / (distance * distance));
                double angle = atan2(dy, dx); // Calculate the angle

                // Apply the repulsion force in the opposite direction of fx
                fx = fx + repulsion_force * cos(angle);
                fy = fy + repulsion_force * sin(angle);

                printf("repulsion_force = %f\n", repulsion_force);
            }
            else
            {
                fx = Fx;
                fy = Fy;
            }

            mvprintw(20, 20, "distance = %f\n", distance);
            mvprintw(25, 25, "world.obstacle[%d].x = %d\n", closest_obstacle_index, world.obstacle[closest_obstacle_index].x);
            mvprintw(30, 30, "world.obstacle[%d].y = %d\n", closest_obstacle_index, world.obstacle[closest_obstacle_index].y);
            refresh();
        }

        if (read(pipesd_t[PIPE_READ], &world.target, sizeof(world.target)) == -1)
        {
            perror("read target");
            continue;
        }
        else
        {
            int closest_target_index = -1;
            double min_distance = DBL_MAX;

            // Find the closest target
            for (int i = 0; i < 9; ++i)
            {
                if (world.target[i].is_active && world.target[i].is_visible) // Check if the target is active
                {
                    double dx = world.drone.x - world.target[i].x;
                    double dy = world.drone.y - world.target[i].y;
                    double distance = sqrt(dx * dx + dy * dy); // Use absolute value to get the distance

                    if (distance < min_distance)
                    {
                        min_distance = distance;
                        closest_target_index = i;
                    }
                }
            }

            // Now calculate the distance from the closest target
            if (world.target[closest_target_index].is_active && world.target[closest_target_index].is_visible) // Check if the closest target is active
            {
                refresh();
                double dx = world.drone.x - world.target[closest_target_index].x;
                double dy = world.drone.y - world.target[closest_target_index].y;
                double distance_target = sqrt(dx * dx + dy * dy);

                if (distance_target < 4) // Check if the target is within the circle of radius 2
                {
                    double attractive_force = -TARGET_ATTRACTION_CONSTANT * (distance_target);
                    double angleA = atan2(dy, dx); // Calculate the angle

                    // Apply the repulsion force in the opposite direction of fx
                    fx = fx + attractive_force * cos(angleA);
                    fy = fy + attractive_force * sin(angleA);

                    printf("attractive_force = %f\n", attractive_force);
                }
                else
                {
                    fx = Fx;
                    fy = Fy;
                }
                mvprintw(19, 19, "distance target = %f\n", distance_target);
                mvprintw(24, 24, "world.target[%d].x = %d\n", closest_target_index, world.target[closest_target_index].x); // Corrected here
                mvprintw(29, 29, "world.target[%d].y = %d\n", closest_target_index, world.target[closest_target_index].y); // Corrected here
                refresh();
            }
        }

        int bytesRead = read(pipedi[PIPE_READ], &command, sizeof(char));
        if (bytesRead > 0)
        {
            switch (command)
            {
            case 'x':
                fy += 100.0; // forza verso USx
                fx -= 100.0;
                break;
            case 'c':
                fy += 100.0; // forza verso U
                break;
            case 'v':
                fy += 100.0; // forza verso UDx
                fx += 100.0;
                break;
            case 's':
                fx -= 100.0; // forza verso Sx
                break;
            case 'd':
                fy = 0; // annulla forza
                fx = 0;
                break;
            case 'f':
                fx += 100.0; // forza verso Dx
                break;
            case 'w':
                fy -= 100.0; // forza verso DSx
                fx -= 100.0;
                break;
            case 'e':
                fy -= 100.0; // forza verso D
                break;
            case 'r':
                fy -= 100.0; // forza verso DDx
                fx += 100.0;
                break;
            case 'b':
                fx = 0; // annulla forza
                fy = 0;
                vx = 0; // annulla velocità
                vy = 0;
                prev_x = world.drone.x; // annulla posizione
                prev_y = world.drone.y;
                prev_vx = 0; // annulla velocità
                prev_vy = 0;
                break;
            case 'a':

                fx = 0; // annulla forza
                fy = 0;
                vx = 0; // annulla velocità
                vy = 0;
                world.drone.x = 10; // ann
                world.drone.y = 10;
                prev_x = world.drone.x; // annulla posizione
                prev_y = world.drone.y;
                prev_vx = 0; // annulla velocità
                prev_vy = 0;

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
            perror("read input:");
        }
        clear();

        // Update velocity and position
        double ax = (fx / M) - (K * vx);
        double ay = (fy / M) - (K * vy);
        vx = prev_vx + ax * T;
        vy = prev_vy + ay * T;

        vx = fmax(fmin(vx, 100), -100);
        vy = fmax(fmin(vy, 100), -100);

        double new_x = prev_x + vx * T;
        double new_y = prev_y + vy * T;

        // Store the current position and velocity for the next iteration
        prev_x = new_x;
        prev_y = new_y;
        prev_vx = vx;
        prev_vy = vy;

        mvprintw(1, 0, "x: %.2f, y: %.2f\n", new_x, new_y);
        mvprintw(2, 0, "vx: %f, vy: %f\n", vx, vy);

        world.drone.x = (int)new_x;
        world.drone.y = (int)new_y;
        world.drone.symbol = world.drone.symbol;
        world.drone.color_pair = world.drone.color_pair;
        write(pipeds[PIPE_WRITE], &world.drone, sizeof(world.drone));
        fsync(pipeds[PIPE_WRITE]);

        refresh();
    }
    endwin();
    return 0;
}

// while (1)
//  {
//      // ...

//     // Calculate repulsion forces
//     double fx = 0, fy = 0;
//     for (int i = 0; i < NUM_OBSTACLES; ++i)
//     {
//         double dx = x - obstacles[i].x;
//         double dy = y - obstacles[i].y;
//         double distance = sqrt(dx * dx + dy * dy);
//         if (distance != 0) // Avoid division by zero
//         {
//             fx += OBSTACLE_REPULSION_CONSTANT * dx / (distance * distance * distance);
//             fy += OBSTACLE_REPULSION_CONSTANT * dy / (distance * distance * distance);
//         }
//     }