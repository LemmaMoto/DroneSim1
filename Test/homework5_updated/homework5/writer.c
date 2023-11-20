#include <stdio.h> 
#include <string.h> 
#include <fcntl.h> 
#include <sys/stat.h> 
#include <sys/types.h> 
#include <sys/select.h>
#include <unistd.h> 
#include <stdlib.h>
#include <stdbool.h>
#include "include/constants.h"

void Read(int fd, char* msg_received, int msg_length) {
    ssize_t bytes_read;
    bytes_read = read(fd, msg_received, msg_length);

    if (bytes_read < 0) {
        perror("read()");
        exit(0);
    }
}

void Write(int fd, char* str, int str_len) {
    ssize_t bytes_written;

    bytes_written = write(fd, str, str_len);
    if (bytes_written < 0) {
        perror("write()");
        exit(0);
    }
}

int main(int argc, char *argv[]) 
{
    int fds[2][2];
    int id;

    bool communicating = false;

    char ask_char[MSG_LEN]; 
    ask_char[0] = ASK_CHAR;

    char msg_received[MSG_LEN];
    char ack_char = ACK_CHAR;
    
    char msg_sent[MSG_LEN];

    int num_to_send;

    if(argc == 3){
        sscanf(argv[1],"%d", &id);
        sscanf(argv[2], "%d %d %d %d", &fds[0][0], &fds[0][1], &fds[1][0], &fds[1][1]);
    } else {
        perror("arg_list");
        exit(1);
    }
    //closing unused ends of pipes
    close(fds[0][0]);
    close(fds[1][1]);

    printf("I am writer W%d \n", id);

    srandom(id * SEED_MULTIPLIER);

    while (1) 
    {   
        //ask to write
        if(!communicating){
            printf("Request to write on cell[%d]\n", id);
            Write(fds[0][1], ask_char, strlen(ask_char) + 1);
            communicating = true;
        }
        else {
            //allow check
            Read(fds[1][0], msg_received, MSG_LEN);
            if(msg_received[0] == ACK_CHAR) {
                num_to_send = random() % MAX_NUMBER;
                sprintf(msg_sent, "%d", num_to_send);

                printf("Write request accepted: sent %d to server \n", num_to_send);

                Write(fds[0][1], msg_sent, strlen(msg_sent)+1);
            }

            else if (msg_received[0] == REJ_CHAR) {
                printf("Write request rejected\n");
            }
            
            communicating = false;
        }
        sleep(2);
    } 
    return 0; 
} 