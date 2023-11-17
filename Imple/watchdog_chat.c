#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>

volatile sig_atomic_t watchdog_signal_received = 0;

void handle_watchdog_signal(int signum) {
    // Signal handler for the watchdog signal
    watchdog_signal_received = 1;
}

void send_watchdog_signal(pid_t pid) {
    // Function to send a watchdog signal to the given process
    if (kill(pid, SIGUSR1) == -1) {
        perror("Error sending watchdog signal");
        exit(EXIT_FAILURE);
    }
}

void start_watchdog() {
    // Set up the signal handler for SIGUSR1 (watchdog signal)
    struct sigaction sa;
    sa.sa_handler = handle_watchdog_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGUSR1, &sa, NULL);
}

void check_process_activity(pid_t pid) {
    // Check if the process is still active by sending the watchdog signal
    send_watchdog_signal(pid);

    // Wait for a short time to allow the process to respond
    usleep(500000); // 0.5 seconds (adjust as needed)

    if (!watchdog_signal_received) {
        // If the process did not respond to the watchdog signal, assume it's inactive
        printf("Process is not responding. Taking action...\n");

        // For demonstration, we'll exit the program
        exit(EXIT_FAILURE);
    }

    // Reset the flag for the next check
    watchdog_signal_received = 0;
}

void do_some_task() {
    // Function simulating the process to be monitored
    // Perform some task here
    // This function should reset the watchdog periodically
    // For demonstration, we're just printing messages
    printf("Doing some task...\n");
    sleep(2); // Simulate work (you can replace this with actual task)
}

int main() {
    start_watchdog();

    pid_t monitored_process_pid = getpid(); // Replace this with the PID of the process to monitor

    while (1) {
        do_some_task();
        check_process_activity(monitored_process_pid);
    }

    return 0;
}

