#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int m_read(int fd, void *buf, size_t nbytes) {
    int ret = read(fd, buf, nbytes);
    if (ret < 0) {
        perror("Error on reading from file descriptor");
        fflush(stdout);
        getchar();
        exit(EXIT_FAILURE);
    }
    return ret;
}

int m_fork() {
    int ret = fork();
    if (ret < 0) {
        perror("Error on forking");
        fflush(stdout);
        getchar();
        exit(EXIT_FAILURE);
    }
    return ret;
}
int m_write(int fd, void *buf, size_t nbytes) {
    int ret = write(fd, buf, nbytes);
    if (ret < 0) {
        perror("Error on writing in file descriptor");
        fflush(stdout);
        getchar();
        exit(EXIT_FAILURE);
    }
    return ret;
}

int m_select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
             struct timeval *timeout) {
    int ret = select(nfds, readfds, writefds, exceptfds, timeout);
    if (ret < 0) {
        perror("Error on executing select");
        fflush(stdout);
        getchar();
        exit(EXIT_FAILURE);
    }
    return ret;
}

int m_open(const char* file, int oflag){
    int ret = open(file, oflag);
    if (ret < 0) {
        perror("Error on executing open");
        fflush(stdout);
        getchar();
        exit(EXIT_FAILURE);
    }
    return ret;
}
int m_pipe(int *pipedes) {
    int ret = pipe(pipedes);
    if (ret < 0) {
        perror("Error on executing open");
        fflush(stdout);
        getchar();
        exit(EXIT_FAILURE);
    }
    return ret;
}
