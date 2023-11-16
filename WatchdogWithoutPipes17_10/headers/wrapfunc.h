#ifndef WRAPFUNC
#define WRAPFUNC
#include <stddef.h>
#include <sys/select.h>

int m_read(int fd, void *buf, size_t nbytes);
int m_write(int fd, void *buf, size_t nbytes);
int m_fork();
int m_select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);
int m_open(const char* file, int oflag);
int m_pipe(int *pipedes);
#endif // !WRAPFUNC
