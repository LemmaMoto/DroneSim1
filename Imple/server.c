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
#include <stdbool.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#define PIPE_READ 0
#define PIPE_WRITE 1

#define portno_obstacles 50001
#define portno_targets 50002

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
    struct Obstacle obstacle[20];
    struct Screen screen;
    struct Target target[9];
};
int pipesd[2];
int pipeds[2];
int pipeso[2];
int pipeos[2];
int pipest[2];
int pipets[2];
int pipesw[2];
int pipews[2];
int pipesd_t[2];
int pipeis[2];

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
void error(char *msg)
{
    FILE *logFile = fopen("log/server/error_log_server.txt", "a");
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
int stringToObstacles(char *str, struct Obstacle *obstacles)
{
    // Skip the 'O[' at the start of the string and get the number of obstacles
    int num_obstacles;
    sscanf(str, "O[%d]", &num_obstacles);

    // Find the start of the obstacle data
    char *obstacleData = strchr(str, ']');
    if (obstacleData != NULL)
    {
        // Skip the ']'
        obstacleData++;

        // Parse each obstacle
        for (int i = 0; i < num_obstacles; i++)
        {
            // Parse the obstacle's coordinates
            sscanf(obstacleData, "%d,%d", &obstacles[i].x, &obstacles[i].y);
            printf("obstacle %d x: %d, y: %d\n", i, obstacles[i].x, obstacles[i].y);

            // Find the start of the next obstacle
            obstacleData = strchr(obstacleData, '|');
            if (obstacleData != NULL)
            {
                // Skip the '|'
                obstacleData++;
            }
            else
            {
                // No more obstacles
                break;
            }
        }
    }

    return num_obstacles;
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

    int sockfd_obstacles, newsockfd_obstacles;
    int sockfd_targets, newsockfd_targets;
    int clilen, pid;
    struct sockaddr_in serv_addr, cli_addr;

    struct Drone drone;
    int process_num;
    if (argc == 22)
    {
        sscanf(argv[1], "%d", &process_num);
        sscanf(argv[2], "%d", &pipesd[PIPE_READ]);
        sscanf(argv[3], "%d", &pipesd[PIPE_WRITE]);
        sscanf(argv[4], "%d", &pipeds[PIPE_READ]);
        sscanf(argv[5], "%d", &pipeds[PIPE_WRITE]);
        sscanf(argv[6], "%d", &pipeso[PIPE_READ]);
        sscanf(argv[7], "%d", &pipeso[PIPE_WRITE]);
        sscanf(argv[8], "%d", &pipeos[PIPE_READ]);
        sscanf(argv[9], "%d", &pipeos[PIPE_WRITE]);
        sscanf(argv[10], "%d", &pipest[PIPE_READ]);
        sscanf(argv[11], "%d", &pipest[PIPE_WRITE]);
        sscanf(argv[12], "%d", &pipets[PIPE_READ]);
        sscanf(argv[13], "%d", &pipets[PIPE_WRITE]);
        sscanf(argv[14], "%d", &pipesw[PIPE_READ]);
        sscanf(argv[15], "%d", &pipesw[PIPE_WRITE]);
        sscanf(argv[16], "%d", &pipews[PIPE_READ]);
        sscanf(argv[17], "%d", &pipews[PIPE_WRITE]);
        sscanf(argv[18], "%d", &pipesd_t[PIPE_READ]);
        sscanf(argv[19], "%d", &pipesd_t[PIPE_WRITE]);
        sscanf(argv[20], "%d", &pipeis[PIPE_READ]);
        sscanf(argv[21], "%d", &pipeis[PIPE_WRITE]);
    }
    else
    {
        printf("wrong args\n");
        return -1;
    }
    printf("process_num: %d\n", process_num);
    printf("pipesd[PIPE_READ]: %d\n", pipesd[PIPE_READ]);
    printf("pipesd[PIPE_WRITE]: %d\n", pipesd[PIPE_WRITE]);
    printf("pipeds[PIPE_READ]: %d\n", pipeds[PIPE_READ]);
    printf("pipeds[PIPE_WRITE]: %d\n", pipeds[PIPE_WRITE]);
    printf("pipeso[PIPE_READ]: %d\n", pipeso[PIPE_READ]);
    printf("pipeso[PIPE_WRITE]: %d\n", pipeso[PIPE_WRITE]);
    printf("pipeos[PIPE_READ]: %d\n", pipeos[PIPE_READ]);
    printf("pipeos[PIPE_WRITE]: %d\n", pipeos[PIPE_WRITE]);
    printf("pipest[PIPE_READ]: %d\n", pipest[PIPE_READ]);
    printf("pipest[PIPE_WRITE]: %d\n", pipest[PIPE_WRITE]);
    printf("pipets[PIPE_READ]: %d\n", pipets[PIPE_READ]);
    printf("pipets[PIPE_WRITE]: %d\n", pipets[PIPE_WRITE]);
    printf("pipesw[PIPE_READ]: %d\n", pipesw[PIPE_READ]);
    printf("pipesw[PIPE_WRITE]: %d\n", pipesw[PIPE_WRITE]);
    printf("pipews[PIPE_READ]: %d\n", pipews[PIPE_READ]);
    printf("pipews[PIPE_WRITE]: %d\n", pipews[PIPE_WRITE]);
    printf("pipesd_t[PIPE_READ]: %d\n", pipesd_t[PIPE_READ]);
    printf("pipesd_t[PIPE_WRITE]: %d\n", pipesd_t[PIPE_WRITE]);
    printf("pipeis[PIPE_READ]: %d\n", pipeis[PIPE_READ]);

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

    struct World world;
    char command;

    sockfd_obstacles = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd_obstacles < 0)
        error("ERROR opening socket for obstacles");
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno_obstacles);
    if (bind(sockfd_obstacles, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR on binding for obstacles");
    listen(sockfd_obstacles, 5);
    newsockfd_obstacles = accept(sockfd_obstacles, (struct sockaddr *)&cli_addr, &clilen);
    if (newsockfd_obstacles < 0)
        error("ERROR on accept for obstacles");

    // Set up the socket for targets.c
    sockfd_targets = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd_targets < 0)
        error("ERROR opening socket for targets");
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno_targets);
    if (bind(sockfd_targets, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR on binding for targets");
    listen(sockfd_targets, 5);
    newsockfd_targets = accept(sockfd_targets, (struct sockaddr *)&cli_addr, &clilen);
    if (newsockfd_targets < 0)
        error("ERROR on accept for targets");
    clilen = sizeof(cli_addr);

    int n;
    char buffer[1024];
    int num_obstacles = 20;
    while (1)
    {
        n = read(pipews[PIPE_READ], &world.screen, sizeof(world.screen));
        if (n < 0)
            error("ERROR reading from pipews\n");

        n = read(pipeds[PIPE_READ], &world.drone, sizeof(world.drone));
        if (n < 0)
            error("ERROR reading from pipeds\n");

        sleep(1);
        n = read(newsockfd_obstacles, buffer, sizeof(buffer));
        if (n < 0)
            error("ERROR reading from newsockfd_obstacles\n");

        // printf("buffer: %s\n", buffer);

        struct Obstacle obstacles[20];
        // num_obstacles = stringToObstacles(buffer, obstacles);
        if (num_obstacles <= 0)
        {
            error("ERROR parsing obstacle string");
            continue;
        }
        for (int i = 0; i < num_obstacles; i++)
        {
            world.obstacle[i] = obstacles[i];
        }

        n = read(newsockfd_targets, &world.target, sizeof(world.target));
        if (n < 0)
            error("ERROR reading from newsockfd_targets\n");

        for (int i = 0; i < 20; i++)
        {
            if (world.obstacle[i].x != 0 && world.obstacle[i].y != 0)
            {
                printf("obstacle %d x: %d, y: %d\n", i, world.obstacle[i].x, world.obstacle[i].y);
            }
        }

        n = read(pipeis[PIPE_READ], &command, sizeof(command));
        if (n < 0)
            error("ERROR reading from pipeis\n");
        n = write(pipeis[PIPE_WRITE], &command, sizeof(command));
        if (n < 0)
            error("ERROR writing to pipeis\n");
        printf("command: %c\n", command);
        command = '0';
        printf("screen height: %d, screen width: %d\n", world.screen.height, world.screen.width);

        // for (int i = 0; i < 9; i++)
        // {
        //     if (world.target[i].is_active == true)
        //     {
        //         printf("target %d x: %d, y: %d, is_active: %d\n", i, world.target[i].x, world.target[i].y, world.target[i].is_active);
        //     }
        // }

        n = write(pipesw[PIPE_WRITE], &world.drone, sizeof(world.drone));
        if (n < 0)
            error("ERROR writing to pipesw\n");
        fsync(pipesw[PIPE_WRITE]);

        n = write(pipesw[PIPE_WRITE], &world.obstacle, sizeof(world.obstacle));
        if (n < 0)
            error("ERROR writing to pipesw\n");
        fsync(pipesw[PIPE_WRITE]);

        n = write(pipesw[PIPE_WRITE], &world.target, sizeof(world.target));
        if (n < 0)
            error("ERROR writing to pipesw\n");
        fsync(pipesw[PIPE_WRITE]);

        n = write(pipesd[PIPE_WRITE], &world.obstacle, sizeof(world.obstacle));
        if (n < 0)
            error("ERROR writing to pipesd\n");
        fsync(pipesd[PIPE_WRITE]);

        n = write(pipesd_t[PIPE_WRITE], &world.target, sizeof(world.target));
        if (n < 0)
            error("ERROR writing to pipesd_t\n");
        fsync(pipesd_t[PIPE_WRITE]);

        n = write(newsockfd_obstacles, &world, sizeof(world));
        if (n < 0)
            error("ERROR writing to newsockfd_obstacles\n");
        n = write(newsockfd_targets, &world, sizeof(world));
        if (n < 0)
            error("ERROR writing to newsockfd_targets\n");
    }
    close(sockfd_obstacles);
    close(sockfd_targets);
    return 0;
}