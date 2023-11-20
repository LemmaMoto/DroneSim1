#include <stdio.h>
#include <string.h> 
#include <fcntl.h> 
#include <sys/stat.h> 
#include <sys/types.h> 
#include <unistd.h> 
#include <stdlib.h>
#include <sys/wait.h>

static void spawn(char ** arg_list)
{
    execvp ("konsole", arg_list);
    perror("exec failed");
    exit(1);
}


int main(int argc, char *argv[])
{
    //0 and 1 are W0 and W1, 2 and 3 are R0 and R1
    pid_t child_pids[4];
    pid_t server_pid;

    int num_pipes = 8;
    int fds[num_pipes][2];
    char id[3];

    for (int i = 0; i < num_pipes; i++) {
        pipe(fds[i]);

        if (fds[i] < 0) {
            perror("pipe()");
            exit(1);
        }
    }

    int res;
    char program_args[4][80];

    //initializing programs_args for each child (readers and writers)
    for (int i = 0; i < num_pipes/2; i++)
        sprintf(program_args[i], "%d %d %d %d", fds[i*2][0], fds[i*2][1], fds[(i*2)+1][0], fds[(i*2)+1][1]);

    //server
    server_pid = fork();
    if (server_pid < 0) {
        perror("Fork");
        return -1;
    }

    if (server_pid == 0) {
        //in the child
        char * arg_list[] = { "konsole", "-e", "./server", program_args[0], program_args[1], program_args[2], program_args[3], NULL };
        spawn(arg_list);
	    return 0;
    }

    //readers and writers
    for (int i = 0; i < 4; i++) {
        child_pids[i] = fork();

        if(child_pids[i] < 0) {
            perror("fork()");
            exit(1);
        }

        if(child_pids[i] == 0) {
            //in the child
            if(i == 0 || i == 1){
                //spawning W0 and W1
                sprintf(id, "%d", i);
                char * arg_list[] = { "konsole", "-e", "./writer", id, program_args[i], NULL };
                spawn(arg_list);
                return 0;
            }
            else {
                //spawning R0 and R1
                sprintf(id, "%d", i-2); //since i=2 or i=3, but id=0 for R0 and id=1 for R1
                char * arg_list[] = { "konsole", "-e", "./reader", id, program_args[i], NULL };
                spawn(arg_list);
                return 0;
            }
        }
    }

    //wait for all children to terminate
    for(int i = 0; i < 5; i ++){
        wait(&res);
    }

    return 0;
}
