#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include "include/constants.h"

void Read(int fd, char *msg_received, int msg_length)
{
    ssize_t bytes_read;
    bytes_read = read(fd, msg_received, msg_length);

    if (bytes_read < 0)
    {
        perror("read()");
        exit(0);
    }
}

void Write(int fd, char *str, int str_len)
{
    ssize_t bytes_written;

    bytes_written = write(fd, str, str_len);
    if (bytes_written < 0)
    {
        perror("write()");
        exit(0);
    }
}

int main(int argc, char *argv[])
{
    int fds[2][2];
    int id;
    int number_received;

    bool communicating = false;

    char ask_char[MSG_LEN];
    ask_char[0] = ASK_CHAR;

    char msg_received[MSG_LEN];

    char filename_string[MSG_LEN];
    char filedirectory_string[MSG_LEN];

    FILE *F0;

    if (argc == 2)
    {
        sscanf(argv[1], "%d", &id);
        // fds[0] is communication from R to server, fds[1] viceversa
        sscanf(argv[2], "%d %d %d %d", &fds[0][0], &fds[0][1], &fds[1][0], &fds[1][1]);
    }
    else
    {
        perror("arg_list");
        exit(1);
    }

    close(fds[0][0]); // close read end of R0->S
    close(fds[1][1]); // close write end of S->R0

    time_t now = time(NULL);
    struct tm *timenow;
    timenow = gmtime(&now);

    // logfile building
    sprintf(filename_string, "./log/reader_%i/%i-%i-%i_%i:%i:%i.txt", id, timenow->tm_year + 1900, timenow->tm_mon, timenow->tm_mday, timenow->tm_hour, timenow->tm_min, timenow->tm_sec);

    printf("I am reader R%d\n", id);

    // opening and closing logfile F0 to clear it
    F0 = fopen(filename_string, "w");
    fclose(F0);

    while (1)
    {
        // ask to read from server
        if (!communicating)
        {
            printf("Requested new information\n");
            Write(fds[0][1], ask_char, strlen(ask_char) + 1);
            communicating = true;
        }
        else // wait for reply
        {
            Read(fds[1][0], msg_received, MSG_LEN);
            if (msg_received[0] == REJ_CHAR)
            {
                printf("Not reading: cell[%d] not changed.\n", id);
            }
            else
            {
                sscanf(msg_received, "%d", &number_received);
                printf("Read allowed: cell[%d] is %d\n", id, number_received);

                // open logfile in append mode
                F0 = fopen(filename_string, "a");
                fprintf(F0, "cell[%d] = %d\n", number_received, id);
                fclose(F0);
            }
            communicating = false;
        }
        sleep(2);
    }
    return 0;
}