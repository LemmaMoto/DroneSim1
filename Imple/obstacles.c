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
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>

#define OBSTACLE_REPULSION_CONSTANT 1.0

#define PIPE_READ 0
#define PIPE_WRITE 1

#define portno 50001

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

int pipeso[2];
int pipeos[2];

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
void error(char *msg)
{
    FILE *logFile = fopen("log/obstacles/error_log_obstacles.txt", "a");
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
char *obstaclesToString(struct Obstacle *obstacles, int num_obstacles)
{
    // Allocate memory for the string
    char *str = malloc(1024 * sizeof(char));
    if (str == NULL)
    {
        error("ERROR allocating memory for string");
    }

    // Start the string with 'O' and the number of obstacles
    sprintf(str, "O[%d]", num_obstacles);

    // Add each obstacle to the string
    for (int i = 0; i < num_obstacles; i++)
    {
        char obstacleStr[1024];

        // Format the obstacle's coordinates as a string
        sprintf(obstacleStr, "%.3f,%.3f", (float)obstacles[i].x, (float)obstacles[i].y);

        // Add the obstacle string to the main string
        // If this is not the first obstacle, add a '|' before the obstacle string
        if (i > 0)
        {
            strcat(str, "|");
        }
        strcat(str, obstacleStr);
    }

    return str;
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

    int sockfd;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    char buffer[256];
    int process_num;
    if (argc == 6)
    {
        sscanf(argv[1], "%d", &process_num);
        sscanf(argv[2], "%d", &pipeso[PIPE_READ]);
        sscanf(argv[3], "%d", &pipeso[PIPE_WRITE]);
        sscanf(argv[4], "%d", &pipeos[PIPE_READ]);
        sscanf(argv[5], "%d", &pipeos[PIPE_WRITE]);
    }
    else
    {
        printf("Error: wrong number of arguments\n");
    }

    printf("process num %d \n", process_num);

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

    // leggere da file file_para numero di ostacoli
    FILE *file = fopen("file_para.txt", "r");
    if (file == NULL)
    {
        error("Unable to open file");
        exit(EXIT_FAILURE);
    }

    char line[256];
    int NUM_OBSTACLES = 0;
    int refresh_time_obstacles = 0;

    while (fgets(line, sizeof(line), file))
    {
        if (sscanf(line, "NUM_OBSTACLES = %d", &NUM_OBSTACLES) == 1)
        {
            // Se abbiamo trovato una riga che corrisponde al formato "NUM_OBSTACLES = %d",
            // interrompiamo il ciclo
            continue;
        }
        else if (sscanf(line, "refresh_time_obstacles = %d", &refresh_time_obstacles) == 1)
        {
            break;
        }
    }

    fclose(file);

    printf("NUM_OBSTACLES = %d\n", NUM_OBSTACLES);
    printf("refresh_time_obstacles = %d\n", refresh_time_obstacles);
    // struct Obstacle obstacles[NUM_OBSTACLES]; // Assume obstacles are initialized
    // struct Screen screen;
    struct World world;

    // Create a new window
    WINDOW *win = newwin(0, 0, 0, 0);
    int tot_borders;
    time_t last_spawn_time = 0;
    int first;
    int border_prec = 0;
    int current_num_targets = 9;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");
    server = gethostbyname("localhost");
    if (server == NULL)
    {
        fprintf(stderr, "ERROR, no such host\n");
        exit(0);
    }
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(portno);
    while (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        error("ERROR connecting");
        sleep(1);
    }

    while (1)
    {
        if (read(sockfd, &world, sizeof(world)) == -1)
        {
            error("read");
            continue;
        }
        else
        {

            // printf("height: %d, width: %d\n", world.screen.height, world.screen.width);

            // posizionare gli ostacoli intorno alla window di modo da delimitare i bordi

            tot_borders = 2 * (world.screen.height - 2) + 2 * (world.screen.width - 2);

            if (tot_borders != border_prec)
            {
                first = 2;
            }
            border_prec = tot_borders;

            int i = 0;

            // generare ostacoli randomici
            if ((time(NULL) - last_spawn_time >= refresh_time_obstacles) || first > 0)
            {
                for (; i < NUM_OBSTACLES; i++)
                {
                    int isSamePosition;
                    do
                    {
                        isSamePosition = 0;
                        world.obstacle[i].x = rand() % (world.screen.width - 4) + 2;
                        world.obstacle[i].y = rand() % (world.screen.height - 4) + 2;

                        // Check if the obstacle is in the same position as the drone
                        if (world.obstacle[i].x == world.drone.x && world.obstacle[i].y == world.drone.y)
                        {
                            isSamePosition = 1;
                        }

                        // Check if the obstacle is in the same position as any of the targets
                        for (int j = 0; j < current_num_targets; j++)
                        {
                            if (world.obstacle[i].x == world.target[j].x && world.obstacle[i].y == world.target[j].y)
                            {
                                isSamePosition = 1;
                                break;
                            }
                        }
                    } while (isSamePosition);

                    world.obstacle[i].symbol = '#';
                    printf("x: %d, y: %d\n", world.obstacle[i].x, world.obstacle[i].y);
                }
                last_spawn_time = time(NULL);
                first--;
                char *obstaclesStr = obstaclesToString(world.obstacle, NUM_OBSTACLES);
                printf("obstaclesStr: %s\n", obstaclesStr);
                int n = write(sockfd, obstaclesStr, strlen(obstaclesStr));
                if (n < 0)
                {
                    error("ERROR writing to socket");
                }
                free(obstaclesStr);
            }
            // generare ostacoli randomici
            printf("tot_borders: %d\n", tot_borders);
            printf("i: %d\n", i);
        }
    }

    // passare gli ostacoli al drone via pipe
    close(sockfd);
    return 0;
}