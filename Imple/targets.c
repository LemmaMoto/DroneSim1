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
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define TARGET_ATTRACTION_CONSTANT 1.0

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

int pipest[2];
int pipets[2];

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
    int sockfd;
    struct sockaddr_in serv_addr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        perror("ERROR opening socket");
        exit(1);
    }

    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(49900); // choose a port number

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) // replace with your server IP address
    {
        perror("ERROR on inet_pton");
        exit(1);
    }

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("ERROR on connect");
        exit(1);
    }
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
    if (argc == 6)
    {
        sscanf(argv[1], "%d", &process_num);
        sscanf(argv[2], "%d", &pipest[PIPE_READ]);
        sscanf(argv[3], "%d", &pipest[PIPE_WRITE]);
        sscanf(argv[4], "%d", &pipets[PIPE_READ]);
        sscanf(argv[5], "%d", &pipets[PIPE_WRITE]);
    }
    else
    {
        printf("Error: wrong number of arguments\n");
        exit(EXIT_FAILURE);
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

    // Read how long to sleep process for
    int sleep_durations[NUM_PROCESSES] = PROCESS_SLEEPS_US;
    int sleep_duration = sleep_durations[process_num];
    char *process_names[NUM_PROCESSES] = PROCESS_NAMES;
    process_name = process_names[process_num]; // added to logfile for readability
    FILE *file = fopen("file_para.txt", "r");
    if (file == NULL)
    {
        perror("Unable to open file");
        exit(EXIT_FAILURE);
    }

    char line[256];
    int NUM_TARGETS = 0;
    int refresh_time_targets = 0;

    while (fgets(line, sizeof(line), file))
    {
        if (sscanf(line, "NUM_TARGETS = %d", &NUM_TARGETS) == 1)
        {
            // Se abbiamo trovato una riga che corrisponde al formato "NUM_OBSTACLES = %d",
            // interrompiamo il ciclo
            continue;
        }
        else if (sscanf(line, "refresh_time_targets = %d", &refresh_time_targets) == 1)
        {
            break;
        }
    }

    fclose(file);

    printf("NUM_TARGETS = %d\n", NUM_TARGETS);
    printf("refresh_time_targets = %d\n", refresh_time_targets);

    struct World world;
    int current_num_targets = NUM_TARGETS;
    struct Target current_targets[current_num_targets];
    time_t last_spawn_time = 0;
    int tot_borders;
    int first;
    int border_prec = 0;

    for (int i = 0; i < current_num_targets; i++)
    {
        if (i == 0)
        {
            current_targets[i].is_active = true;
            current_targets[i].is_visible = true;
        }
        else
        {
            current_targets[i].is_active = false;
            current_targets[i].is_visible = true;
        }
    }

    while (1)
    {
        if (recv(sockfd, &world, sizeof(world), 0) == -1)
        {
            perror("recv");
            continue;
        }
        else
        {
            tot_borders = 2 * (world.screen.height - 2) + 2 * (world.screen.width - 2);

            if (tot_borders != border_prec)
            {
                first = 2;
            }
            border_prec = tot_borders;

            time_t current_time = time(NULL);

            for (int i = 0; i < current_num_targets; i++)
            {
                // If the target is visible, generate a random position for it
                if (current_targets[i].is_visible && (current_time - last_spawn_time >= refresh_time_targets || first > 0))
                {
                    do
                    {
                        printf("ENTRATO NEL DO\n");
                        current_targets[i].x = rand() % (world.screen.width - 4) + 2;
                        current_targets[i].y = rand() % (world.screen.height - 4) + 2;
                    } while (current_targets[i].x == world.drone.x && current_targets[i].y == world.drone.y);

                    current_targets[i].symbol = '0' + i;

                    // Copy the target to the world's targets
                    world.target[i] = current_targets[i];
                }

                printf("drone.x: %d, drone.y: %d\n", world.drone.x, world.drone.y);
                if (current_targets[i].is_active)
                {
                    printf("target.x: %d, target.y: %d\n", world.target[i].x, world.target[i].y);
                }

                // If the drone is in the same position as the active target, make the target inactive and invisible
                if ((int)world.drone.x == (int)world.target[i].x && (int)world.drone.y == (int)world.target[i].y && current_targets[i].is_active)
                {
                    current_targets[i].is_active = false;
                    current_targets[i].is_visible = false;

                    // If there is a next target, make it active
                    if (i + 1 < current_num_targets)
                    {
                        current_targets[i + 1].is_active = true;
                    }
                    world.target[i] = current_targets[i];
                }
            }

            if (current_time - last_spawn_time >= refresh_time_targets || first > 0)
            {
                last_spawn_time = current_time;
            }

            first--;
            // write(pipets[PIPE_WRITE], &world.target, sizeof(world.target));
            // fsync(pipets[PIPE_WRITE]);

            if (send(sockfd, &world.target, sizeof(world.target), 0) == -1)
            {
                perror("ERROR sending targets");
                continue;
            }
        }
    }
    close(sockfd);

    return 0;
}