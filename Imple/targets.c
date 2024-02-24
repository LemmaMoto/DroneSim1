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

#define TARGET_ATTRACTION_CONSTANT 1.0

#define PIPE_READ 0
#define PIPE_WRITE 1

#define portno 50002

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

void error(char *msg)
{
    FILE *logFile = fopen("log/targets/error_log_targets.txt", "a");
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

char *targetsToString(struct Target *targets, int num_targets)
{
    // Allocate memory for the string
    char *str = malloc(1024 * sizeof(char));
    if (str == NULL)
    {
        error("ERROR allocating memory for string");
    }

    // Start the string with 'O' and the number of obstacles
    sprintf(str, "T[%d]", num_targets);

    // Add each obstacle to the string
    for (int i = 0; i < num_targets; i++)
    {
        char targetStr[1024];

        // Format the obstacle's coordinates as a string
        sprintf(targetStr, "%.3f,%.3f|", (float)targets[i].x, (float)targets[i].y);

        // Add the obstacle string to the main string
        strcat(str, targetStr);
    }
    strcat(str, "\0");

    return str;
}

int main(int argc, char *argv[])
{
    char *targetsStr = malloc(1024 * sizeof(char));
    free(targetsStr);

    int process_num;
    int sockfd, n;

    struct sockaddr_in serv_addr;
    struct hostent *server;

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
    FILE *file = fopen("file_para.txt", "r");
    if (file == NULL)
    {
        error("Unable to open file");
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
    char buffer[1024] = "TI";
    n = write(sockfd, buffer, strlen(buffer));
    if (n < 0)
    {
        error("ERROR writing to socket");
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
            targetsStr = targetsToString(world.target, NUM_TARGETS);
            printf("targetsStr: %s\n", targetsStr);

            int n = write(sockfd, targetsStr, strlen(targetsStr));
            if (n < 0)
            {
                error("ERROR writing to socket");
            }
            free(targetsStr);
        }
    }
    return 0;
}