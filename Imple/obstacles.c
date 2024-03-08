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

#define portno 50000

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

int main(int argc, char *argv[])
{
    sleep(2);
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
    printf("Published pid %d \n", process_id);

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
        // exit(0);
    }
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(portno);
    while (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        error("ERROR connecting");
        sleep(1); // Wait for 1 second before trying again
    }

    char buffer_echo[1024];
    printf("Sending OI\n");
    bool var = true;
    // to keep sending the OI command until the server responds with the same string
    while (var)
    {
        bzero(buffer, sizeof(buffer));
        strcpy(buffer, "OI");
        n = write(sockfd, buffer, sizeof(buffer));
        if (n < 0)
        {
            // error("ERROR writing to socket");
        }
        sleep(1);                                // PROVARE AD AUMENTARE LO SLEEP
        bzero(buffer_echo, sizeof(buffer_echo)); // Clear the echo buffer before reading into it
        int n_r = read(sockfd, buffer_echo, sizeof(buffer_echo) - 1);
        if (n_r < 0)
        {
            error("ERROR reading from socket");
        }
        else if (n_r == 0)
        {
            error("STRANO ERRORE");
        }
        printf("ECHOOOOOOOO: %s\n", buffer_echo);
        printf("BUUUUUUFFEEERR: %s\n", buffer);
        if (strcmp(buffer, buffer_echo) == 0)
        {
            printf("UGUALI");
            var = false;
        }
    }
    bzero(buffer, sizeof(buffer));
    while (buffer[0] == '\0')
    {
        n = read(sockfd, buffer, sizeof(buffer));
        printf("LEGGENDO DIM FINESTRA \n");
        printf("DIMENSIONE FINESTRA: %s\n", buffer);
        if (n < 0)
        {
            error("ERROR reading from socket");
        }
        n = write(sockfd, buffer, sizeof(buffer));
    }

    while (1)
    {
        sleep(1);
        bzero(buffer, sizeof(buffer));
        n = read(sockfd, buffer, sizeof(buffer));
        if (n < 0)
        {
            error("ERROR reading from socket");
        }

        printf("\nECHO %s", buffer);
        if (strcmp(buffer, buffer_echo) == 0)
        {
            printf("\nEcho OK");
        }
        printf("STRINGA CHE ARRIVA PER DEBUG: %s\n", buffer);

        char height_str[8];
        height_str[0] = '\0';
        char width_str[8];
        width_str[0] = '\0';
        float height, width;

        strncpy(height_str, buffer, 7);
        height_str[8] = '\0'; // Null-terminate the string
        height = atof(height_str);

        strncpy(width_str, buffer + 7, 7);
        width_str[8] = '\0'; // Null-terminate the string
        width = atof(width_str);
        printf("height: %f, width: %f\n", height, width);

        if (height >= 1 && width >= 1)
        {
            world.screen.height = height;
            world.screen.width = width;

            tot_borders = 2 * (world.screen.height - 2) + 2 * (world.screen.width - 2);

            char obs_pos[1024];
            char str[64];
            sprintf(str, "%d", NUM_OBSTACLES);
            strcpy(&obs_pos[2], str);
            if (NUM_OBSTACLES > 9 && NUM_OBSTACLES < 100)
            {
                obs_pos[0] = 'O';
                obs_pos[1] = '[';
                obs_pos[2] = str[0];
                obs_pos[3] = str[1];
                obs_pos[4] = ']';
            }
            else if (NUM_OBSTACLES < 10)
            {
                obs_pos[0] = 'O';
                obs_pos[1] = '[';
                strcpy(&obs_pos[2], str);
                obs_pos[3] = ']';
            }
            char obstacle_x[8], obstacle_y[8];
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
                    // do
                    // {
                    isSamePosition = 0;
                    world.obstacle[i].x = rand() % (world.screen.width - 4) + 2;
                    world.obstacle[i].y = rand() % (world.screen.height - 4) + 2;
                    sprintf(obstacle_x, "%.3f", (float)world.obstacle[i].x);
                    sprintf(obstacle_y, "%.3f", (float)world.obstacle[i].y);
                    strcat(obs_pos, obstacle_x);
                    strcat(obs_pos, ",");
                    strcat(obs_pos, obstacle_y);
                    if (i < NUM_OBSTACLES - 1)
                    {
                        strcat(obs_pos, "|");
                    }

                    // Check if the obstacle is in the same position as the drone
                    //     if (world.obstacle[i].x == world.drone.x && world.obstacle[i].y == world.drone.y)
                    //     {
                    //         isSamePosition = 1;
                    //     }

                    //     // Check if the obstacle is in the same position as any of the targets
                    //     for (int j = 0; j < current_num_targets; j++)
                    //     {
                    //         if (world.obstacle[i].x == world.target[j].x && world.obstacle[i].y == world.target[j].y)
                    //         {
                    //             printf("DEBUGGGGGGGGGGGGGG\n");
                    //             isSamePosition = 1;
                    //             break;
                    //         }
                    //     }
                    // } while (isSamePosition);
                }
                last_spawn_time = current_time;
            }
            first--;
            // generare ostacoli randomici
            printf("tot_borders: %d\n", tot_borders);
            printf("i: %d\n", i);
            printf("obs_pos: %s\n", obs_pos);
            n = write(sockfd, obs_pos, strlen(obs_pos));
        }
        else
        {
            printf("0 height and width\n");
            // exit(EXIT_FAILURE);
        }
        printf("height: %d, width: %d\n", world.screen.height, world.screen.width);
        printf("buffer echo : %s", buffer_echo);

        // posizionare gli ostacoli intorno alla window di modo da delimitare i bordi
    }

    // passare gli ostacoli al drone via pipe

    return 0;
}