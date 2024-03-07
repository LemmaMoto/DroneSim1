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
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <errno.h>

#define PIPE_READ 0
#define PIPE_WRITE 1

#define portno 50000

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

    int sockfd, newsockfd, clilen, pid;
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

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");
    bzero((char *)&serv_addr, sizeof(serv_addr));
    //  portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    int yes = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
    {
        error("setsockopt");
        // Handle error
    }
    while (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        error("ERROR on binding");
        sleep(1); // Wait for 1 second before trying again
    }
    listen(sockfd, 5);
    clilen = sizeof(cli_addr);
    char buffer_screen1[512];
    buffer_screen1[0] = '\0';
    char buffer_screen2[512];
    buffer_screen2[0] = '\0';
    char merged_buffer[1024];
    merged_buffer[0] = '\0';
    char finestra[1024] = "38.000,151.000";
    while (1)
    {
        read(pipews[PIPE_READ], &world.screen, sizeof(world.screen));
        read(pipeds[PIPE_READ], &world.drone, sizeof(world.drone));

        merged_buffer[0] = '\0';
        sprintf(buffer_screen1, "%.3f", (double)world.screen.height);
        sprintf(buffer_screen2, "%.3f", (double)world.screen.width);
        strcat(merged_buffer, buffer_screen1);
        strcat(merged_buffer, ",");
        strcat(merged_buffer, buffer_screen2);
        printf("merged_buffer: %s\n", merged_buffer);

        // read(pipeos[PIPE_READ], &world.obstacle, sizeof(world.obstacle));
        // read(pipets[PIPE_READ], &world.target, sizeof(world.target));

        read(pipeis[PIPE_READ], &command, sizeof(command));
        write(pipeis[PIPE_WRITE], &command, sizeof(command));
        printf("command: %c\n", command);
        command = '0';
        printf("screen height: %d, screen width: %d\n", world.screen.height, world.screen.width);

        for (int i = 0; i < 9; i++)
        {
            if (world.target[i].is_active == true)
            {
                printf("target %d x: %d, y: %d, is_active: %d\n", i, world.target[i].x, world.target[i].y, world.target[i].is_active);
            }
        }

        write(pipesw[PIPE_WRITE], &world.drone, sizeof(world.drone));
        fsync(pipesw[PIPE_WRITE]);

        write(pipesw[PIPE_WRITE], &world.target, sizeof(world.target));
        fsync(pipesw[PIPE_WRITE]);

        // write(pipesw[PIPE_WRITE], &world.obstacle, sizeof(world.obstacle));
        // fsync(pipesw[PIPE_WRITE]);

        write(pipesd[PIPE_WRITE], &world.obstacle, sizeof(world.obstacle));
        fsync(pipesd[PIPE_WRITE]);

        write(pipesd_t[PIPE_WRITE], &world.target, sizeof(world.target));
        fsync(pipesd_t[PIPE_WRITE]);

        // write(pipesd_s[PIPE_WRITE], &world.screen, sizeof(world.screen));
        // fsync(pipesd_s[PIPE_WRITE]);

        // write(pipeso[PIPE_WRITE], &world, sizeof(world));
        // fsync(pipeso[PIPE_WRITE]);

        // write(pipest[PIPE_WRITE], &world, sizeof(world));
        // fsync(pipest[PIPE_WRITE]);

        printf("x: %d, y: %d\n", world.drone.x, world.drone.y);
        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
        if (newsockfd < 0)
        {
            error("ERROR on accept");
        }
        pid = fork();
        printf("pid: %d\n", pid);
        if (pid < 0)
        {
            error("ERROR on fork");
        }
        if (pid == 0)
        {
            // close(sockfd);
            int n;
            char buffer[1024];
            char buffer_check[1024];
            char buffer2[1024];
            while (1)
            {
                bzero(buffer, sizeof(buffer));
                sleep(1);
                n = read(newsockfd, buffer, sizeof(buffer));
                // int n2 = read(newsockfd, buffer2, 1024);
                printf("buffer: %s\n", buffer);
                // printf("buffer2: %s\n", buffer2);
                if (n < 0)
                {
                    // error("ERROR reading from socket");
                }
                if (n >= 0)
                {
                    switch (buffer[0])
                    {
                    case 'O':
                        if (buffer[1] == 'I')
                        {
                            n = write(newsockfd, buffer, sizeof(buffer)); // Send the message back to the client echo
                            printf("MERGED NUFFER DIM FINESTRA: %s\n", finestra);
                            sleep(1);
                            n = write(newsockfd, finestra, sizeof(finestra));
                            
                        }
                        else
                        {
                            int num_obstacles = buffer[2] - '0';
                            int j = 4;
                            for (int i = 0; i < num_obstacles; i++)
                            {
                                bool is_x = true;
                                char sub_buffer_x[7] = ""; // Buffer for the substring
                                char sub_buffer_y[7] = ""; // Buffer for the substring
                                int x_index = 0, y_index = 0;

                                for (; j < strlen(buffer); j++)
                                {
                                    if (buffer[j] == ',')
                                    {
                                        is_x = false;
                                        sub_buffer_x[x_index] = '\0';
                                        x_index = 0;
                                    }
                                    else if (buffer[j] == '|')
                                    {
                                        sub_buffer_y[y_index] = '\0';
                                        world.obstacle[i].x = (int)atof(sub_buffer_x);
                                        world.obstacle[i].y = (int)atof(sub_buffer_y);
                                        // printf("sub_buffer_x: %s, sub_buffer_y: %s\n", sub_buffer_x, sub_buffer_y);
                                        j++;
                                        break;
                                    }
                                    else if (is_x)
                                    {
                                        sub_buffer_x[x_index++] = buffer[j];
                                    }
                                    else
                                    {
                                        sub_buffer_y[y_index++] = buffer[j];
                                    }
                                }

                                // If the y-coordinate of the last obstacle hasn't been processed yet, process it
                                if (i == num_obstacles - 1 && y_index > 0)
                                {
                                    sub_buffer_y[y_index] = '\0';
                                    world.obstacle[i].x = (int)atof(sub_buffer_x);
                                    world.obstacle[i].y = (int)atof(sub_buffer_y);
                                }
                                world.obstacle[i].symbol = '#';
                                printf("obstacle %d x: %d, y: %d\n", i, world.obstacle[i].x, world.obstacle[i].y);
                            }
                        }
                        break;
                    case 'T':
                        if (buffer[1] == 'I')
                        {
                            printf("Here is the message: %s\n", buffer);
                        }
                        else
                        {
                            int num_targets = buffer[3];
                            for (int i = 0; i < num_targets; i++)
                            {
                                char sub_buffer_x[7];                          // Buffer for the substring
                                char sub_buffer_y[7];                          // Buffer for the substring
                                strncpy(sub_buffer_x, &buffer[5 + i * 16], 6); // Copy 6 characters starting at index 5
                                sub_buffer_x[6] = '\0';                        // Null-terminate the substring

                                strncpy(sub_buffer_y, &buffer[13 + i * 16], 6); // Copy 6 characters starting at index 13
                                sub_buffer_y[6] = '\0';                         // Null-terminate the substring

                                world.target[i].x = atof(sub_buffer_x);
                                world.target[i].y = atof(sub_buffer_y);
                            }
                        }
                        break;
                    default:
                        if (strcmp(buffer, buffer_check) != 0)
                        {
                            n = write(newsockfd, buffer, sizeof(buffer));
                        }
                        else if (strcmp(buffer, buffer_check) == 0)
                        {
                            bzero(buffer, 1024);
                        }
                        break;
                    }
                    for (int i = 0; i < 9; i++)
                    {
                        printf("obstacle %d x: %d, y: %d\n", i, world.obstacle[i].x, world.obstacle[i].y);
                        printf("symbol: %c\n", world.obstacle[i].symbol);
                    }
                    write(pipesw[PIPE_WRITE], &world.obstacle, sizeof(world.obstacle));
                    fsync(pipesw[PIPE_WRITE]);
                }
                if (n < 0)
                {
                    // error("ERROR writing to socket");
                }
            }
            // exit(0);
        }
        else
        {
            // close(newsockfd);
        }
    }

    return 0;
}