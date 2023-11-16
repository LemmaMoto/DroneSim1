#include <stdio.h>
#include <string.h> 
#include <fcntl.h> 
#include <sys/stat.h> 
#include <sys/types.h> 
#include <unistd.h> 
#include <stdlib.h>
#include <sys/wait.h>


int main(int argc, char *argv[])
{
    // Pids for all children
    pid_t child_writer0;
    pid_t child_writer1;
    pid_t child_reader0;
    pid_t child_reader1;
    pid_t child_server;

    int res;
    int num_children;

    // Server
    child_server = fork();
    if (child_server < 0) {
        perror("Fork");
        return -1;
    }

    if (child_server == 0) {
        char * arg_list[] = { "konsole", "-e", "./server", NULL };
        execvp("konsole", arg_list);
	return 0;
    }
    num_children += 1;

    // writer 0
    child_writer0 = fork();
    if (child_writer0 < 0) {
        perror("Fork");
        return -1;
    }

    if (child_writer0 == 0) {
        char * arg_list[] = { "konsole", "-e", "./writer", "0", NULL };
        execvp("konsole", arg_list);
	return 0;
    }
    num_children += 1;
    
    // writer 1
    child_writer1 = fork();
    if (child_writer1 < 0) {
        perror("Fork");
        return -1;
    }

    if (child_writer1 == 0) {
        char * arg_list[] = { "konsole", "-e", "./writer", "1", NULL };
        execvp("konsole", arg_list);
	return 0;
    }
    num_children += 1;

    // reader 0
    child_reader0 = fork();
    if (child_reader0 < 0) {
        perror("Fork");
        return -1;
    }

    if (child_reader0 == 0) {
        char * arg_list[] = { "konsole", "-e", "./reader", "0", NULL };
        execvp("konsole", arg_list);
	return 0;
    }
    num_children += 1;

    // reader 1
    child_reader1 = fork();
    if (child_reader1 < 0) {
        perror("Fork");
        return -1;
    }

    if (child_reader1 == 0) {
        char * arg_list[] = { "konsole", "-e", "./reader", "1", NULL };
        execvp("konsole", arg_list);
	return 0;
    }
    num_children += 1;
    
    //wait for all children to terminate
    for(int i = 0; i < num_children; i ++){
        wait(&res);
    }

    return 0;
}
