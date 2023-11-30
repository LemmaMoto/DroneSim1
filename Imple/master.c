#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "include/constants.h"

int main(int argc, char *argv[])
{
    // Declare PIDs for all child processes
    pid_t child_watchdog;
    pid_t child_process0;
    pid_t child_process1;
    pid_t child_process2;
    pid_t child_process3;

    // Prepare the log file name and remove any existing log file
    char logfile_name[256];
    sprintf(logfile_name, LOG_FILE_NAME);
    remove(logfile_name);

    // Open the log file for writing
    FILE *logfile = fopen(logfile_name, "w");
    if (logfile == NULL)
    {
        perror("Error opening log file");
        return -1;
    }

    // Create a pipe
    int pipefd[2];
    if (pipe(pipefd) == -1)
    {
        perror("Error creating pipe");
        return -1;
    }
    printf("Pipe created successfully\n");

    // Convert the pipe file descriptors to strings
    char pipe_read_str[10];
    char pipe_write_str[10];
    sprintf(pipe_read_str, "%d", pipefd[PIPE_READ]);
    sprintf(pipe_write_str, "%d", pipefd[PIPE_WRITE]);

    // Open PID files for writing
    fopen(PID_FILE_PW, "w");
    char *fnames[NUM_PROCESSES] = PID_FILE_SP;

    for (int i = 0; i < NUM_PROCESSES; i++)
    {
        fopen(fnames[i], "w");
    }

    int res;
    int num_children = 0;

    // Create the watchdog process
    child_watchdog = fork();
    if (child_watchdog < 0)
    {
        perror("Fork");
        return -1;
    }
    printf("Watchdog process created successfully with PID %d\n", child_watchdog);

    // If this is the watchdog process, replace it with the watchdog program
    if (child_watchdog == 0)
    {
        char *arg_list[] = {"konsole", "-e", "./watchdog", NULL};
        execvp("konsole", arg_list);
        perror("execvp failed for watchdog");
        return 0;
    }
    num_children += 1;

    // Create the first child process and replace it with the server program
    child_process0 = fork();
    if (child_process0 < 0)
    {
        perror("Fork");
        return -1;
    }
    printf("Child process 0 created successfully with PID %d\n", child_process0);

    if (child_process0 == 0)
    {
        char *arg_list[] = {"konsole", "-e", "./server", "0", NULL};
        execvp("konsole", arg_list);
        perror("execvp failed for watchdog");
        return 0;
    }
    num_children += 1;

    // Create the second child process and replace it with the drone program
    child_process1 = fork();
    if (child_process1 < 0)
    {
        perror("Fork"); // Print an error message if fork failed
        return -1;
    }
    printf("Child process 1 created successfully with PID %d\n", child_process1);

    if (child_process1 == 0)
    {
        // Prepare the argument list for the drone program
        char *arg_list[] = {"konsole", "-e", "./drone", "1", pipe_read_str, pipe_write_str, NULL};
        execvp("konsole", arg_list);                // Replace the current process image with the drone program
        perror("execvp failed for child_process1"); // Print an error message if execvp failed
        return 0;
    }
    num_children += 1; // Increment the number of child processes

    // Create the third child process and replace it with the input program
    child_process2 = fork();
    if (child_process2 < 0)
    {
        perror("Fork"); // Print an error message if fork failed
        return -1;
    }
    printf("Child process 2 created successfully with PID %d\n", child_process2);

    if (child_process2 == 0)
    {
        // Prepare the argument list for the input program
        char *arg_list[] = {"konsole", "-e", "./input", "2", pipe_read_str, pipe_write_str, NULL};
        execvp("konsole", arg_list);                // Replace the current process image with the input program
        perror("execvp failed for child_process2"); // Print an error message if execvp failed
        return 0;
    }
    num_children += 1; // Increment the number of child processes

    // Create the fourth child process and replace it with the world program
    child_process3 = fork();
    if (child_process3 < 0)
    {
        perror("Fork"); // Print an error message if fork failed
        return -1;
    }
    printf("Child process 3 created successfully with PID %d\n", child_process3);

    if (child_process3 == 0)
    {
        // Prepare the argument list for the world program
        char *arg_list[] = {"konsole", "-e", "./world", "3", NULL};
        execvp("konsole", arg_list);                // Replace the current process image with the world program
        perror("execvp failed for child_process3"); // Print an error message if execvp failed
        return 0;
    }
    num_children += 1; // Increment the number of child processes

    // Wait for all child processes to terminate
    int status;
    int last_non_zero_status = 0;
    for (int i = 0; i < num_children; i++)
    {
        pid_t pid = wait(&status); // Wait for a child process to terminate
        if (WIFEXITED(status))     // If the child process terminated normally
        {
            printf("Child process %d exited with status %d\n", pid, WEXITSTATUS(status)); // Print the PID and exit status of the terminated child process
            if (WEXITSTATUS(status) != 0)                                                 // If the exit status is not 0 (indicating an error)
            {
                last_non_zero_status = WEXITSTATUS(status); // Store the non-zero exit status
            }
        }
        else // If the child process did not terminate normally
        {
            printf("Child process %d did not exit successfully\n", pid); // Print an error message
        }
    }
    printf("All child processes terminated\n"); // Print a message indicating that all child processes have terminated

    close(pipefd[0]);                                                                  // Close the read end of the pipe
    close(pipefd[1]);                                                                  // Close the write end of the pipe
    printf("pipefd[0] = %d, pipefd[1] = %d\n", pipefd[PIPE_READ], pipefd[PIPE_WRITE]); // Print the file descriptors of the pipe ends
    printf("Pipe closed successfully\n");                                              // Print a message indicating that the pipe has been closed successfully

    return last_non_zero_status; // Return the last non-zero exit status of the child processes
}