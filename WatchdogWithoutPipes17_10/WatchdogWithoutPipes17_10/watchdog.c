#include "headers/wrapfunc.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <wait.h>
#include <ncurses.h>

void summon(char **programArgs) {
    execvp("konsole", programArgs);
    perror("Execution failed");

    // avoid unwanted forking
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
    char *argsChild[] = {"konsole", "-e", "./child", "p", "p", NULL};
    int max_fd = -1;
    int pipes[2];

    fd_set master, reader;
    FD_ZERO(&master);
    FD_ZERO(&reader);

    for (int i = 0; i < 5; i++) {
        m_pipe(pipes);

        FD_SET(pipes[0], &master);

        if (max_fd < pipes[0])
            max_fd = pipes[0];

        pid_t pid = m_fork();

        if (!pid) {
            char str[100];
            char str2[100];
            sprintf(str, "%d", i+1);
            argsChild[3] = str;
            sprintf(str2, "%d %d", pipes[0], pipes[1]);
            argsChild[4] = str2;
            summon(argsChild);
        } else {
            close(pipes[1]);
            printf("Summoned child with pid %d\n", pid);
            fflush(stdout);
        }
    }
    printf("%d\n", max_fd);
    char received_msg[100];

    // Lazy way of assigning array dimension
    struct timespec reference[max_fd+1];
    for (int i= 0;i <= max_fd;i++) {
        clock_gettime(CLOCK_MONOTONIC_RAW, &reference[i]);
    }

    int nChilds = 5;
    while (1) {
        fflush(stdout);
        reader = master;
        m_select(max_fd + 1, &reader, NULL, NULL, NULL);
        fflush(stdout);
        for (int i = 0; i <= max_fd; i++) {
            if (FD_ISSET(i, &reader)) {
                int r_b = m_read(i, received_msg, 100);
                if(r_b == 0){
                    FD_CLR(i, &master);
                    close(i);
                    printf("Closed pipe %d\n", i);
                    if(!--nChilds){
                        printf("watchdog out\n");
                        fflush(stdout);
                        exit(EXIT_SUCCESS);
                    }
                    continue;
                }
                //printf("%s\n", received_msg);
                //fflush(stdout);
                
                uint64_t aux = reference[i].tv_sec;
                clock_gettime(CLOCK_MONOTONIC_RAW, &reference[i]);
                uint64_t delay = reference[i].tv_sec - aux;
                if(delay > 3){
                    printf("Error for child %s with a delay of %lu\n", received_msg, delay);
                    fflush(stdout);
                }else{
                    printf("All good for child %s with a delay of %lu\n",received_msg, delay);
                    fflush(stdout);
                }
            }
        }
    }

    int stat;

    for (int i = 0; i < 5; i++) {
        pid_t pid = wait(&stat);
        printf("Child %d terminated with status %d\n", pid, stat);
        fflush(stdout);
    }
    return EXIT_SUCCESS;
}
