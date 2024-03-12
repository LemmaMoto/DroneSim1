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
#include <stdarg.h>

#define TARGET_ATTRACTION_CONSTANT 1.0

#define PIPE_READ 0
#define PIPE_WRITE 1


void error(char *msg)
{
    FILE *logFile = fopen("log/targets/error_log_targets.txt", "a");
    if (logFile != NULL)
    {
        time_t now = time(NULL);
        char timeStr[64]; // Buffer to hold the time string
        strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", localtime(&now));
        fprintf(logFile, "%s - %s: %s\n", timeStr, msg, strerror(errno));
        fclose(logFile);
    }
    else
    {
        perror("ERROR opening log file");
    }
}

void Printf(char *format, ...)
{
    FILE *logFile = fopen("log/targets/log_targets.txt", "a");
    if (logFile != NULL)
    {
        time_t now = time(NULL);
        char timeStr[64]; // Buffer to hold the time string
        strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", localtime(&now));
        fprintf(logFile, "%s - ", timeStr);

        va_list args;
        va_start(args, format);
        vfprintf(logFile, format, args);
        va_end(args);

        fprintf(logFile, "\n");
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
    int sockfd, n;

    struct sockaddr_in serv_addr;
    struct hostent *server;

    char buffer[256];

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
        Printf("Error: wrong number of arguments\n");
        exit(EXIT_FAILURE);
    }

    // Publish your pid
    process_id = getpid();
    Printf("Published pid %d \n", process_id);

    FILE *file = fopen("file_para.txt", "r");
    if (file == NULL)
    {
        error("Unable to open file");
        exit(EXIT_FAILURE);
    }

    char line[256];
    int NUM_TARGETS = 0;
    int refresh_time_targets = 0;
    char ip_addr_string[100];
    int portno = 0;

    while (fgets(line, sizeof(line), file))
    {
        if (sscanf(line, "NUM_TARGETS = %d", &NUM_TARGETS) == 1)
        {
            // Se abbiamo trovato una riga che corrisponde al formato "NUM_TARGETS = %d",
            // interrompiamo il ciclo
            continue;
        }
        else if (sscanf(line, "refresh_time_targets = %d", &refresh_time_targets) == 1)
        {
            continue;
        }
        else if (sscanf(line, "ip = %s", ip_addr_string) == 1)
        {
            continue;
        }
        else if (sscanf(line, "portno = %d", &portno) == 1)
        {
            break;
        }
    }

    fclose(file);

    Printf("NUM_TARGETS = %d\n", NUM_TARGETS);
    Printf("refresh_time_targets = %d\n", refresh_time_targets);

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
    server = gethostbyname(ip_addr_string);
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
    Printf("Sending TI\n");
    bool var = true;
    // to keep sending the OI command until the server responds with the same string
    while (var)
    {
        bzero(buffer, sizeof(buffer));
        strcpy(buffer, "TI");
        n = write(sockfd, buffer, sizeof(buffer));
        if (n < 0)
        {
            // error("ERROR writing to socket");
        }
        // sleep(1);                                // PROVARE AD AUMENTARE LO SLEEP
        bzero(buffer_echo, sizeof(buffer_echo)); // Clear the echo buffer before reading into it
        int n_r = read(sockfd, buffer_echo, sizeof(buffer_echo));
        if (n_r < 0)
        {
            error("ERROR reading from socket");
        }
        else if (n_r == 0)
        {
            error("STRANO ERRORE");
        }
        Printf("ECHOOOOOOOO: %s\n", buffer_echo);
        Printf("BUUUUUUFFEEERR: %s\n", buffer);
        if (strcmp(buffer, buffer_echo) == 0)
        {
            Printf("UGUALI");
            var = false;
        }
    }
    Printf("1\n");
    bzero(buffer, sizeof(buffer));
    while (buffer[0] == '\0')
    {
        n = read(sockfd, buffer, sizeof(buffer));
        Printf("LEGGENDO DIM FINESTRA \n");
        Printf("DIMENSIONE FINESTRA: %s\n", buffer);
        if (n < 0)
        {
            error("ERROR reading from socket");
        }
        n = write(sockfd, buffer, sizeof(buffer));
    }

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
    Printf("height: %f, width: %f\n", height, width);

    bool echo_ok = false;
    char buffer_echo_trg_pos[1024];

    world.screen.height = height;
    world.screen.width = width;

    tot_borders = 2 * (world.screen.height - 2) + 2 * (world.screen.width - 2);
    char trg_pos[1024];
    char str[64];
    sprintf(str, "%d", NUM_TARGETS);
    strcpy(&trg_pos[2], str);
    while (1)
    {

        bzero(trg_pos, sizeof(trg_pos));
        if (NUM_TARGETS >= 10 && NUM_TARGETS < 100)
        {
            trg_pos[0] = 'T';
            trg_pos[1] = '[';
            trg_pos[2] = str[0];
            trg_pos[3] = str[1];
            trg_pos[4] = ']';
        }
        else if (NUM_TARGETS < 10)
        {
            trg_pos[0] = 'T';
            trg_pos[1] = '[';
            strcpy(&trg_pos[2], str);
            trg_pos[3] = ']';
        }
        char target_x[8], target_y[8];
        if (tot_borders != border_prec)
        {
            first = 2;
        }
        border_prec = tot_borders;
        int i = 0;

        // generare ostacoli randomici
        time_t current_time = time(NULL);
        srand(current_time + 15);
        if (current_time - last_spawn_time >= refresh_time_targets || first > 0)
        {
            for (; i < NUM_TARGETS; i++)
            {
                world.target[i].x = rand() % (world.screen.width - 4) + 2;
                world.target[i].y = rand() % (world.screen.height - 4) + 2;
                sprintf(target_x, "%.3f", (float)world.target[i].x);
                sprintf(target_y, "%.3f", (float)world.target[i].y);
                strcat(trg_pos, target_x);
                strcat(trg_pos, ",");
                strcat(trg_pos, target_y);
                if (i < NUM_TARGETS - 1)
                {
                    strcat(trg_pos, "|");
                }
            }
            last_spawn_time = current_time;
            echo_ok = false;
        }
        first--;
        while (!echo_ok)
        {
            Printf("trg_pos: %s\n", trg_pos);
            n = write(sockfd, trg_pos, sizeof(trg_pos));
            if (n < 0)
            {
                error("ERROR writing to socket");
            }
            n = read(sockfd, buffer_echo_trg_pos, sizeof(buffer_echo_trg_pos));
            if (n < 0)
            {
                error("ERROR writing to socket");
            }
            if (strcmp(buffer_echo_trg_pos, trg_pos) == 0)
            {
                echo_ok = true;
                Printf("target POSITION ECHO CORRETTO\n");
            }
            else
            {
                Printf("target POSITION ECHO NON CORRETTO\n");
            }
        }
    }
    return 0;
}