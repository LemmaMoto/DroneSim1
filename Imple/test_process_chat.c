#include <stdio.h>
#include <signal.h>
#include <unistd.h>

void handle_watchdog_signal(int signum) {
    // Signal handler for the watchdog signal (SIGUSR1)
    printf("Program 1 received watchdog signal. Responding...\n");
    fflush(stdout);

    // Send a signal to the watchdog process indicating activity
    kill(getppid(), SIGUSR1);
}

int main() {
    // Set up the signal handler for the watchdog signal (SIGUSR1)
    struct sigaction sa;
    sa.sa_handler = handle_watchdog_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGUSR1, &sa, NULL);

    printf("Program 1 is running and ready to receive watchdog signal.\n");

    while (1) {
        // Simulate the program being active by performing some task
        sleep(1);
    }

    return 0;
}

