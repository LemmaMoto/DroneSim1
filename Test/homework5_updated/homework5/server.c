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

void Read(int i, char* msg_received, int msg_length) {
    ssize_t bytes_read;
    bytes_read = read(i, msg_received, msg_length);

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
    int num_pipes = 8;
    int fds[num_pipes][2];

    fd_set reading;
    char msg_received[MSG_LEN];

    char ack_char[MSG_LEN]; 
    ack_char[0] = ACK_CHAR;

    char rej_char[MSG_LEN];
    rej_char[0] = REJ_CHAR;

    int cell[2] = {0,0};
    int tmp_cell[2] = {0,0};
    //int has_changed[2] = {0,0};
    bool zerochanged = false;
    bool firstchanged = false;

    char output_string[MSG_LEN];



    if(argc == 5){
        for (int i = 0; i < num_pipes/2; i++) {
            sscanf(argv[i+1], "%d %d %d %d", &fds[i*2][0], &fds[i*2][1], &fds[(i*2)+1][0], &fds[(i*2)+1][1]);
        }
    } 
    else {
        perror("arg_list");
        exit(1);
    }

    //closing unused ends of pipes
    for(int i = 0; i <  num_pipes; i+=2){
        close(fds[i][1]);
        close(fds[i+1][0]);
    }

    int max_fd = -1;
    for(int i = 0; i < num_pipes; i+=2) {
        if(fds[i][0] > max_fd){
            max_fd = fds[i][0];
        }
    }

    printf("Max fd is %d\n", max_fd);

    while (1) 
    {    
        //reset reading set
        FD_ZERO(&reading);

        for(int i = 0; i < num_pipes; i+=2)
            FD_SET(fds[i][0], &reading);

        int ret_val = select(max_fd + 1, &reading, NULL, NULL, NULL);

        for(int i = 0; i < max_fd + 1; i++){
            if(FD_ISSET(i, &reading)){
                if(i == fds[0][0]) {
                    //W0 case
                    Read(i, msg_received, MSG_LEN);

                    if (msg_received[0] == ASK_CHAR){
                        printf("W0 asked to write\n");

                        //allow check
                        if(cell[0] <= cell[1]) {
                            printf("W0 can write\n");
                            Write(fds[1][1], ack_char, strlen(ack_char) + 1); 
                        }
                        else {
                            printf("W0 can't write\n");
                            Write(fds[1][1], rej_char, strlen(rej_char) + 1); 
                        }
                    } else {
                        //W0 is writing
                        sscanf(msg_received, "%d", &tmp_cell[0]);
                        printf("W0: cell[0] = %d\n", tmp_cell[0]);
                        zerochanged = true;
                    }
                    
                }

                else if (i == fds[2][0])
                {
                    Read(i, msg_received, MSG_LEN);

                    if (msg_received[0] == ASK_CHAR){
                        //W1
                        printf("W1 asked to write\n");

                        //allow check
                        if(cell[1] <= cell [0]) {
                            printf("W1 can write\n");
                            Write(fds[3][1], ack_char, strlen(ack_char) + 1); 
                        }
                        else {
                            printf("W1 can't write\n");
                            Write(fds[3][1], rej_char, strlen(rej_char) + 1); 
                        }
                    } 
                    
                    else {
                        //W1 is writing
                        sscanf(msg_received, "%d", &tmp_cell[1]);
                        printf("W1: cell[1] = %d \n", tmp_cell[1]);
                        firstchanged = true;
                    }
                }
                else if (i == fds[4][0]) {
                    //R0 case
                    Read(i, msg_received, MSG_LEN);
                    if (msg_received[0] == ASK_CHAR){
                        printf("R0 asked to read\n");

                        //allow check
                        if(zerochanged){
                            printf("R0 can read cell[0], which is %d\n", tmp_cell[0]);
                            sprintf(output_string, "%d", tmp_cell[0]);
                            Write(fds[5][1], output_string, strlen(output_string) + 1);
                            zerochanged = false;
                        }
                        else
                        {
                            printf("R0 can't read since cell[0] has not changed\n");
                            Write(fds[5][1], rej_char, strlen(rej_char) + 1); 
                        }
                    }
                }
                else if (i == fds[6][0]) {
                    Read(i, msg_received, MSG_LEN);
                    if (msg_received[0] == ASK_CHAR){
                        printf("R1 asked to read\n");
                        if(firstchanged){
                            printf("R1 can read cell[1], which is %d\n", tmp_cell[1]);
                            sprintf(output_string, "%d", tmp_cell[1]);
                            Write(fds[7][1], output_string, strlen(output_string) + 1);
                            firstchanged = false;
                        }
                        else
                        {
                            printf("R1 can't read since cell[1] has not changed\n");
                            Write(fds[7][1], rej_char, strlen(rej_char) + 1); 
                        }
                    }
                }
            }
        }
        
        cell[0] = tmp_cell[0];
        cell[1] = tmp_cell[1];
    } 
    return 0; 
} 