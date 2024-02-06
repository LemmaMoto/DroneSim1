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

#define portno 4444

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

void error(char *msg)
{
    perror(msg);
    // exit(0);
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

    int sockfd, n;

    struct sockaddr_in serv_addr;
    struct hostent *server;

    char buffer[1024];

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
        exit(EXIT_FAILURE);
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

    // leggere da file file_para numero di ostacoli
    FILE *file = fopen("file_para.txt", "r");
    if (file == NULL)
    {
        perror("Unable to open file");
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
        // exit(0);
    }
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(portno);
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        error("ERROR connecting");
    }
    char buffer_echo[1024];
    printf("Sending OI");
    bzero(buffer, 1024);
    strcpy(buffer, "OI");
    strcpy(buffer_echo, buffer);
    n = write(sockfd, buffer, strlen(buffer));

    while (1)
    {
        sleep(1);
        if (n < 0)
        {
            // error("ERROR writing to socket");
        }
        bzero(buffer, 1024);
        n = read(sockfd, buffer, 1024);
        if (n < 0)
        {
            // error("ERROR reading from socket");
        }

        printf("\nECHO %s", buffer);
        if (strcmp(buffer, buffer_echo) == 0)
        {
            printf("\nEcho OK");
        }
        // n = write(sockfd, buffer, sizeof(buffer));

        char height_str[8];
        char width_str[8];
        float height, width;

        strncpy(height_str, buffer, 7);
        height_str[8] = '\0'; // Null-terminate the string
        height = atof(height_str);

        strncpy(width_str, buffer + 7, 7);
        width_str[8] = '\0'; // Null-terminate the string
        width = atof(width_str);

        if (height > 0 && width > 0)
        {
            world.screen.height = height;
            world.screen.width = width;
        }
        else
        {
            printf("0 height and width\n");
            // exit(EXIT_FAILURE);
        }

        printf("height: %d, width: %d\n", world.screen.height, world.screen.width);
        printf("buffer echo : %s", buffer_echo);

        // posizionare gli ostacoli intorno alla window di modo da delimitare i bordi

        tot_borders = 2 * (world.screen.height - 2) + 2 * (world.screen.width - 2);

        if (tot_borders != border_prec)
        {
            first = 2;
        }
        border_prec = tot_borders;

        int i = 0;

        // generare ostacoli randomici
        time_t current_time = time(NULL);
        if (current_time - last_spawn_time >= refresh_time_obstacles || first > 0)
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
            }
            last_spawn_time = current_time;
        }
        first--;
        // generare ostacoli randomici
        printf("tot_borders: %d\n", tot_borders);
        printf("i: %d\n", i);
    }

    // passare gli ostacoli al drone via pipe

    return 0;
}