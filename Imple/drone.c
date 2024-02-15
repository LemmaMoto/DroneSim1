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
#define OBSTACLE_REPULSION_CONSTANT 1000.0
#define TARGET_ATTRACTION_CONSTANT 500
#define INPUT_FORCE 1000.0

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
    struct Obstacle obstacle[20];
    struct Screen screen;
    struct Target target[9];
};

int pipedi[2];
int pipesd[2];
int pipeds[2];
int pipesd_t[2];
int pipesd_s[2];

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

double compute_distance(float x1, float y1, float x2, float y2)
{
    float d = sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1));
    return d;
}
void error(char *msg)
{
    FILE *logFile = fopen("log/drone/error_log_drone.txt", "a");
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
        sscanf(argv[2], "%d", &pipedi[PIPE_READ]);
        sscanf(argv[3], "%d", &pipedi[PIPE_WRITE]);
        sscanf(argv[4], "%d", &pipesd[PIPE_READ]);
        sscanf(argv[5], "%d", &pipesd[PIPE_WRITE]);
        sscanf(argv[6], "%d", &pipeds[PIPE_READ]);
        sscanf(argv[7], "%d", &pipeds[PIPE_WRITE]);
        sscanf(argv[8], "%d", &pipesd_t[PIPE_READ]);
        sscanf(argv[9], "%d", &pipesd_t[PIPE_WRITE]);
        sscanf(argv[10], "%d", &pipesd_s[PIPE_READ]);
        sscanf(argv[11], "%d", &pipesd_s[PIPE_WRITE]);
    }
    else
    {
        printf("wrong args\n");
        return -1;
    }

    printf("pipedi[0] = %d, pipedi[1] = %d\n", pipedi[PIPE_READ], pipedi[PIPE_WRITE]);
    printf("pipesd[0] = %d, pipesd[1] = %d\n", pipesd[PIPE_READ], pipesd[PIPE_WRITE]);
    printf("pipeds[0] = %d, pipeds[1] = %d\n", pipeds[PIPE_READ], pipeds[PIPE_WRITE]);
    printf("pipesd_t[0] = %d, pipesd_t[1] = %d\n", pipesd_t[PIPE_READ], pipesd_t[PIPE_WRITE]);
    printf("pipesd_s[0] = %d, pipesd_s[1] = %d\n", pipesd_s[PIPE_READ], pipesd_s[PIPE_WRITE]);

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
    double fx, fy;
    double prev_x = world.drone.x, prev_y = world.drone.y;
    static double prev_vx = 0, prev_vy = 0;
    double Fx = fx;
    double Fy = fy;

    double distance = 0;
    double dx = 0;
    double dy = 0;
    float distance_board = 0;

    while (1)
    {
        fx = 0;
        fy = 0;

        char command = '\0';
        printf("Reading from pipe\n");

        if (read(pipesd_s[PIPE_READ], &world.screen, sizeof(world.screen)) == -1)
        {
            error("read screen");
            continue;
        }

        int board_x0 = 0;
        int board_y0 = 0;
        int board_x1 = world.screen.width;
        int board_y1 = world.screen.height;

        float dist_board_x_y = world.drone.y;
        float dist_board_y_x = world.drone.x;

        float distance1 = compute_distance(world.drone.x, world.drone.y, board_x0, dist_board_x_y);
        float distance2 = compute_distance(world.drone.x, world.drone.y, board_x1, dist_board_x_y);
        float distance3 = compute_distance(world.drone.x, world.drone.y, dist_board_y_x, board_y0);
        float distance4 = compute_distance(world.drone.x, world.drone.y, dist_board_y_x, board_y1);

        float distance_board = distance1;
        int i = 0;

        if (distance2 < distance_board)
        {
            distance_board = distance2;
            i = 1;
        }
        if (distance3 < distance_board)
        {
            distance_board = distance3;
            i = 2;
        }
        if (distance4 < distance_board)
        {
            distance_board = distance4;
            i = 3;
        }

        mvprintw(30, 2, "distance1 = %f\n", distance1);
        mvprintw(31, 2, "distance2 = %f\n", distance2);
        mvprintw(32, 2, "distance3 = %f\n", distance3);
        mvprintw(33, 2, "distance4 = %f\n", distance4);
        mvprintw(34, 2, "distance_board = %f\n", distance_board);

        if (read(pipesd[PIPE_READ], &world.obstacle, sizeof(world.obstacle)) == -1)
        {
            error("read obstacle");
            continue;
        }
        int closest_obstacle_index = -1;
        double min_distance = DBL_MAX;

        // Find the closest obstacle
        for (int i = 0; i < 20; ++i)
        {
            dx = world.drone.x - world.obstacle[i].x;
            dy = world.drone.y - world.obstacle[i].y;
            distance = sqrt(dx * dx + dy * dy); // Use absolute value to get the distance

            if (distance < min_distance)
            {
                min_distance = distance;
                closest_obstacle_index = i;
            }
        }

        // Now calculate the distance from the closest obstacle
        dx = world.drone.x - world.obstacle[closest_obstacle_index].x;
        dy = world.drone.y - world.obstacle[closest_obstacle_index].y;
        distance = sqrt(dx * dx + dy * dy);

        // mvprintw(25, 25, "world.obstacle[%d].x = %d\n", closest_obstacle_index, world.obstacle[closest_obstacle_index].x);
        // mvprintw(30, 30, "world.obstacle[%d].y = %d\n", closest_obstacle_index, world.obstacle[closest_obstacle_index].y);

        if (distance_board < distance)
        {
            distance = distance_board;
            if (i == 0)
            {
                dx = world.drone.x - board_x0;
                dy = 0;
            }
            else if (i == 1)
            {
                dx = world.drone.x - board_x1;
                dy = 0;
            }
            else if (i == 2)
            {
                dx = 0;
                dy = world.drone.y - board_y0;
            }
            else if (i == 3)
            {
                dx = 0;
                dy = world.drone.y - board_y1;
            }
        }
        mvprintw(28, 25, "distance = %f\n", distance);
        refresh();
        double repulsion_force = 0;
        double angle = 0;
        if (distance < 4) // Check if the obstacle is within the circle of radius 2
        {
            repulsion_force = OBSTACLE_REPULSION_CONSTANT * ((1 / distance) - (1 / 4)) * (1 / (distance * distance));
            angle = atan2(dy, dx); // Calculate the angle

            // Apply the repulsion force in the opposite direction of fx
            Fx = fx + repulsion_force * cos(angle);
            Fy = fy + repulsion_force * sin(angle);

            refresh();
        }
        else
        {
            Fx = fx;
            Fy = fy;
        }
        if (repulsion_force != 0)
        {
            mvprintw(20, 25, "repulsion_force = %f\n", repulsion_force);
            mvprintw(21, 25, "angle = %f\n", angle);
            refresh();
        }
        if (read(pipesd_t[PIPE_READ], &world.target, sizeof(world.target)) == -1)
        {
            error("read target");
            continue;
        }
        int closest_target_index = -1;
        double min_distance_t = DBL_MAX;

        // Find the closest target
        for (int i = 0; i < 9; ++i)
        {
            if (world.target[i].is_active && world.target[i].is_visible) // Check if the target is active
            {
                double dx = world.drone.x - world.target[i].x;
                double dy = world.drone.y - world.target[i].y;
                double distance = sqrt(dx * dx + dy * dy); // Use absolute value to get the distance

                if (distance < min_distance_t)
                {
                    min_distance_t = distance;
                    closest_target_index = i;
                }
            }
            mvprintw(0 + i, 40, "world.target[%d].x = %d\n", i, world.target[i].x);
            mvprintw(0 + i + 9, 40, "world.target[%d].y = %d\n", i, world.target[i].y);
            // mvprintw(25+i+9, 25, "world.target[%d].is_active = %d\n", i, world.target[i].is_active);
            // mvprintw(26+i+9, 25, "world.target[%d].is_visible = %d\n", i, world.target[closest_target_index].is_visible);
        }
        double attractive_force = 0;
        double angleA = 0;
        // Now calculate the distance from the closest target
        if (world.target[closest_target_index].is_active && world.target[closest_target_index].is_visible) // Check if the closest target is active
        {

            double dx = world.drone.x - world.target[closest_target_index].x;
            double dy = world.drone.y - world.target[closest_target_index].y;
            double distance_target = sqrt(dx * dx + dy * dy);

            if (distance_target < 4 && distance_target > 1) // Check if the target is within the circle of radius 2
            {
                attractive_force = -TARGET_ATTRACTION_CONSTANT * (distance_target);
                angleA = atan2(dy, dx); // Calculate the angle

                // Apply the repulsion force in the opposite direction of fx
                fx = fx + attractive_force * cos(angleA);
                fy = fy + attractive_force * sin(angleA);

                min_distance_t = DBL_MAX;
                closest_target_index = closest_target_index + 1;
                refresh();
            }
            else
            {
                fx = Fx;
                fy = Fy;
            }
            if (attractive_force != 0)
            {
                mvprintw(20, 25, "attractive_force = %f\n", attractive_force);
                mvprintw(21, 25, "angleA = %f\n", angleA);
                refresh();
            }
            mvprintw(29, 25, "distance target = %f\n", distance_target);
            mvprintw(22, 25, "closest_target_index = %d\n", closest_target_index);

            // mvprintw(24, 24, "world.target[%d].x = %d\n", closest_target_index, world.target[closest_target_index].x); // Corrected here
            // mvprintw(29, 29, "world.target[%d].y = %d\n", closest_target_index, world.target[closest_target_index].y); // Corrected here
            refresh();
        }

        int bytesRead = read(pipedi[PIPE_READ], &command, sizeof(char));
        if (bytesRead > 0)
        {
            switch (command)
            {
            case 'x':
                fy += INPUT_FORCE; // forza verso USx
                fx -= INPUT_FORCE;
                break;
            case 'c':
                fy += INPUT_FORCE; // forza verso U
                break;
            case 'v':
                fy += INPUT_FORCE; // forza verso UDx
                fx += INPUT_FORCE;
                break;
            case 's':
                fx -= INPUT_FORCE; // forza verso Sx
                break;
            case 'd':
                fy = 0; // annulla forza
                fx = 0;
                break;
            case 'f':
                fx += INPUT_FORCE; // forza verso Dx
                break;
            case 'w':
                fy -= INPUT_FORCE; // forza verso DSx
                fx -= INPUT_FORCE;
                break;
            case 'e':
                fy -= INPUT_FORCE; // forza verso D
                break;
            case 'r':
                fy -= INPUT_FORCE; // forza verso DDx
                fx += INPUT_FORCE;
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
            error("read input:");
        }
        // clear();
        refresh();

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
