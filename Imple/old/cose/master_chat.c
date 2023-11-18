#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

int main() {
    pid_t watchdog_pid, process_pid;

    // Start the watchdog process
    watchdog_pid = fork();

    if (watchdog_pid == 0) {
        // This is the watchdog process
        // The process_pid is not defined at this point, so we can't use it here
        char * arg_list[] = { "konsole", "-e", "./watchdog_chat", NULL};
        execvp("konsole", arg_list);
        perror("Error starting watchdog process");
        return 1;
    } else if (watchdog_pid > 0) {
        // This is the master process
        printf("Watchdog process started with PID: %d\n", watchdog_pid);

        // Start the following process
        process_pid = fork();

        if (process_pid == 0) {
            // This is the following process
            char * arg_list2[] = { "konsole", "-e", "./test_process_chat", NULL};
            execvp("konsole", arg_list2);
            perror("Error starting test process");
            return 1;
        } else if (process_pid > 0) {
            // Master process
            printf("Following process started with PID: %d\n", process_pid);

            // Pass the PID of the following process to the watchdog
            kill(watchdog_pid, SIGUSR1);

            // Wait for the processes to finish
            wait(NULL);
            wait(NULL);
        } else {
            perror("Error forking following process");
            return 1;
        }
    } else {
        perror("Error forking watchdog process");
        return 1;
    }

    return 0;
}