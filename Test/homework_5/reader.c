#include <stdio.h> 
#include <string.h> 
#include <fcntl.h> 
#include <sys/stat.h> 
#include <sys/types.h> 
#include <unistd.h> 
#include <stdlib.h>
#include "include/constants.h"

int main(int argc, char *argv[]) 
{
    int send_pipe_read;
    int send_pipe;
    int receive_pipe;
    int receive_pipe_write;
    int reader_num;
    int in_progress = 0;
    int number_received;

    char ask_char[MSG_LEN]; 
    ask_char[0] = 'A';
    char read_str[MSG_LEN];
    char format_string[MSG_LEN]="%d";
    char filename_string[MSG_LEN];

    FILE * fp;

    

    if(argc == 3){
        sscanf(argv[1],"%i", &reader_num);
        sscanf(argv[2], "%d %d %d %d", &send_pipe_read, &send_pipe, &receive_pipe, &receive_pipe_write);
    } else {
        printf("wrong args\n"); 
        return -1;
    }

    close(send_pipe_read);
    close(receive_pipe_write);

    sprintf(filename_string, "./log/reader_%i.txt", reader_num);

    printf("Reader number %i\n", reader_num);
    printf("Sending on pipe %i \n", send_pipe);
    printf("Receiving on pipe %i \n", receive_pipe);
    //clears contents of log file
    fp = fopen(filename_string, "w");
    fclose(fp);


    while (1) 
    {   
        // send ask
        if(!in_progress){
            printf("Requested new information\n");
            write(send_pipe, ask_char, strlen(ask_char) + 1);
            in_progress = 1;
        }
        else //wait for reply
        {
            read(receive_pipe, read_str, MSG_LEN);
            if (read_str[0] == 'R')
            {
                printf("Value has not changed \n");
            }
            else
            {
                sscanf(read_str, format_string, &number_received);
                printf("Value has changed to %i \n", number_received);
                fp = fopen(filename_string, "a");
                fprintf(fp, "Logged %i in cell %i \n", number_received, reader_num);
                fclose(fp);
            }
            in_progress = 0;
        }
        sleep(2);
    } 
    return 0; 
} 