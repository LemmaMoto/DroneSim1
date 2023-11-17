#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>

volatile sig_atomic_t process1_active = 0;
volatile sig_atomic_t process2_active = 0;
volatile sig_atomic_t process3_active = 0; // Added for the third process

void handle_process1_signal(int signum) {
    process1_active = 1;
}

void handle_process2_signal(int signum) {
    process2_active = 1;
}

void handle_process3_signal(int signum) { // Handler for the third process
    process3_active = 1;
}

void send_watchdog_signal(pid_t pid) {
    if (kill(pid, SIGUSR1) == -1) {
        perror("Error sending watchdog signal");
        exit(EXIT_FAILURE);
    }
}

void start_watchdog() {
    struct sigaction sa1, sa2, sa3; 
    // Set up the signal handler for process 1 signal (SIGUSR1)
    sa1.sa_handler = handle_process1_signal;
    sigemptyset(&sa1.sa_mask);
    sa1.sa_flags = 0;
    sigaction(SIGUSR1, &sa1, NULL);

    // Set up the signal handler for process 2 signal (SIGUSR2)
    sa2.sa_handler = handle_process2_signal;
    sigemptyset(&sa2.sa_mask);
    sa2.sa_flags = 0;
    sigaction(SIGUSR1, &sa2, NULL);

    // Set up the signal handler for process 3 signal (SIGUSR3)
    sa3.sa_handler = handle_process3_signal;
    sigemptyset(&sa3.sa_mask);
    sa3.sa_flags = 0;
    sigaction(SIGUSR1, &sa3, NULL); // Assuming SIGUSR2 for the third process
}

void check_processes_activity(pid_t pid1, pid_t pid2, pid_t pid3) { // Added pid3 for the third process
    // Check if Process 1 is active by sending a watchdog signal
    process1_active = 0;
    send_watchdog_signal(pid1);
    usleep(500000); // Wait for response

    // Check if Process 2 is active by sending a watchdog signal
    process2_active = 0;
    send_watchdog_signal(pid2);
    usleep(500000); // Wait for response

    // Check if Process 3 is active by sending a watchdog signal
    process3_active = 0;
    send_watchdog_signal(pid3);
    usleep(500000); // Wait for response

    // Check if all processes responded
    if (!process1_active || !process2_active || !process3_active) { // Added check for the third process
        printf("At least one process is not responding. Taking action...\n");
        // Perform action when a process is not responding
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <pid1> <pid2> <pid3>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    start_watchdog();

    pid_t process1_pid = atoi(argv[1]); // PID of child_reader0
    pid_t process2_pid = atoi(argv[2]); // PID of child_writer0
    pid_t process3_pid = atoi(argv[3]); // PID of child_server

    while (1) {
        check_processes_activity(process1_pid, process2_pid, process3_pid);
        sleep(5); // Adjust the interval as needed
    }

    return 0;
}