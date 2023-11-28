#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <time.h>
#include <ncurses.h>
#include <errno.h>
#include "include/constants.h"
#include <sys/ipc.h>
#include <sys/shm.h>
#include <math.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <fcntl.h>

#define SHM_DRN 12 // Define a shared memory key

#define PIPE_READ 0  // Define the read end of the pipe
#define PIPE_WRITE 1 // Define the write end of the pipe

#define T 0.01 // Define a time constant

// Initialize position and velocity
double x = 0, y = 0;
double vx = 0, vy = 0;

// Initialize forces
double fx = 0, fy = 0;

pid_t watchdog_pid;                     // Process ID for the watchdog process
pid_t process_id;                       // Process ID for the current process
char *process_name;                     // Name of the current process
struct timeval prev_t;                  // Previous time value
char logfile_name[256] = LOG_FILE_NAME; // Log file name
char command;                           // Command character

struct Drone // Define a Drone structure
{
    int x;
    int y;
    char symbol;
    short color_pair;
};
struct Drone drone; // Create a Drone instance

int pipefd[2];     // File descriptors for the pipe
fd_set read_fds;   // Set of file descriptors for select()
struct timeval tv; // Time value for select()
int retval;        // Return value

// logs time update to file
void log_receipt(struct timeval tv)
{
    FILE *lf_fp = fopen(logfile_name, "a"); // Open the log file
    if (lf_fp == NULL)
    {
        perror("fopen"); // Print an error message if the file can't be opened
        return;
    }
    fprintf(lf_fp, "%d %ld %ld\n", process_id, tv.tv_sec, tv.tv_usec); // Write the process ID and time to the log file
    int result = fclose(lf_fp);                                        // Close the log file
    if (result != 0)
    {
        perror("fclose"); // Print an error message if the file can't be closed
    }
}

void watchdog_handler(int sig, siginfo_t *info, void *context) // Signal handler for the watchdog process
{
    if (info->si_pid == watchdog_pid) // If the signal was sent by the watchdog process
    {
        gettimeofday(&prev_t, NULL); // Get the current time
        log_receipt(prev_t);         // Log the receipt of the signal
    }
}

int main(int argc, char *argv[]) // Main function
{
    // Define a signal set
    sigset_t set;

    // Initialize the signal set to empty
    sigemptyset(&set);

    // Add SIGUSR1 to the set
    sigaddset(&set, SIGUSR1);

    // Block SIGUSR1
    if (sigprocmask(SIG_BLOCK, &set, NULL) < 0)
    {
        perror("sigprocmask"); // Print an error message if the signal can't be blocked
        return -1;
    }
    // Set up sigaction for receiving signals from the watchdog process
    struct sigaction p_action;
    p_action.sa_flags = SA_SIGINFO;
    p_action.sa_sigaction = watchdog_handler;
    if (sigaction(SIGUSR1, &p_action, NULL) < 0)
    {
        perror("sigaction"); // Print an error message if the signal can't be set up
    }

    // Unblock SIGUSR1
    if (sigprocmask(SIG_UNBLOCK, &set, NULL) < 0)
    {
        perror("sigprocmask"); // Print an error message if the signal can't be unblocked
        return -1;
    }

    // Process command line arguments
    int process_num;
    if (argc == 4)
    {
        sscanf(argv[1], "%d", &process_num);        // Get the process number
        sscanf(argv[2], "%d", &pipefd[PIPE_READ]);  // Get the read end of the pipe
        sscanf(argv[3], "%d", &pipefd[PIPE_WRITE]); // Get the write end of the pipe
    }
    else
    {
        printf("wrong args\n"); // Print an error message if the arguments are incorrect
        return -1;
    }

    printf("pipefd[0] = %d, pipefd[1] = %d\n", pipefd[PIPE_READ], pipefd[PIPE_WRITE]); // Print the file descriptors for the pipe
    // close(pipefd[PIPE_WRITE]); // Close the write end of the pipe

    // Initialize the screen
    initscr();
    cbreak();             // Disable line buffering
    noecho();             // Disable echoing of input characters
    keypad(stdscr, TRUE); // Enable the keypad of the user's terminal

    // Publish your pid
    process_id = getpid(); // Get the process ID

    char *fnames[NUM_PROCESSES] = PID_FILE_SP; // Array of file names

    FILE *pid_fp = fopen(fnames[process_num], "w"); // Open the file for writing
    if (pid_fp == NULL)                             // If the file couldn't be opened
    {
        perror("fopen"); // Print an error message
        return -1;       // Return an error code
    }
    fprintf(pid_fp, "%d", process_id); // Write the process ID to the file
    int result = fclose(pid_fp);       // Close the file
    if (result != 0)                   // If the file couldn't be closed
    {
        perror("fclose"); // Print an error message
    }
    printf("Published pid %d \n", process_id); // Print the process ID

    // Read watchdog pid
    FILE *watchdog_fp = NULL; // File pointer for the watchdog process
    struct stat sbuf;         // Stat structure for file information

    /* call stat, fill stat buffer, validate success */
    if (stat(PID_FILE_PW, &sbuf) == -1) // If the stat call fails
    {
        perror("error-stat"); // Print an error message
        return -1;            // Return an error code
    }
    // waits until the file has data
    while (sbuf.st_size <= 0) // While the file is empty
    {
        if (stat(PID_FILE_PW, &sbuf) == -1) // If the stat call fails
        {
            perror("error-stat"); // Print an error message
            return -1;            // Return an error code
        }
        usleep(50000); // Sleep for 50 milliseconds
    }

    watchdog_fp = fopen(PID_FILE_PW, "r"); // Open the file for reading
    if (watchdog_fp == NULL)               // If the file couldn't be opened
    {
        perror("fopen"); // Print an error message
        return -1;       // Return an error code
    }
    fscanf(watchdog_fp, "%d", &watchdog_pid);   // Read the watchdog process ID from the file
    printf("watchdog pid %d \n", watchdog_pid); // Print the watchdog process ID
    result = fclose(watchdog_fp);               // Close the file
    if (result != 0)                            // If the file couldn't be closed
    {
        perror("fclose"); // Print an error message
    }

    // Read how long to sleep process for
    int sleep_durations[NUM_PROCESSES] = PROCESS_SLEEPS_US; // Array of sleep durations
    int sleep_duration = sleep_durations[process_num];      // Sleep duration for the current process
    char *process_names[NUM_PROCESSES] = PROCESS_NAMES;
    process_name = process_names[process_num]; // added to logfile for readability

    // Create a shared memory segment
    int shm_id = shmget(SHM_DRN, sizeof(struct Drone), IPC_CREAT | 0666);
    if (shm_id < 0)
    {
        perror("shmget");
        return -1;
    }
    sem_t *semaphore = sem_open("/semaphore", O_CREAT, 0644, 1);
    if (semaphore == SEM_FAILED)
    {
        perror("sem_open/semaphore");
        exit(EXIT_FAILURE);
    }
    // Attach the shared memory segment to our process's address space
    struct Drone *shared_drone = (struct Drone *)shmat(shm_id, NULL, 0);
    if (shared_drone == (struct Drone *)-1)
    {
        perror("shmat");
        return -1;
    }

    FILE *file = fopen("file_para.txt", "r");
    if (file == NULL)
    {
        perror("Could not open file_para.txt for reading\n");
        return 1;
    }
    struct Drone drone;
    float M, K;

    char label[256];
    while (fscanf(file, "%s", label) != EOF)
    {
        if (strcmp(label, "drone.symbol") == 0)
        {
            fscanf(file, " = '%c'", &drone.symbol);
        }
        else if (strcmp(label, "drone.color_pair") == 0)
        {
            fscanf(file, " = %hd", &drone.color_pair);
        }
        else if (strcmp(label, "M") == 0)
        {
            fscanf(file, " = %f", &M);
        }
        else if (strcmp(label, "K") == 0)
        {
            fscanf(file, " = %f", &K);
        }
        else if (strcmp(label, "drone.x") == 0)
        {
            fscanf(file, " = %d", &drone.x);
        }
        else if (strcmp(label, "drone.y") == 0)
        {
            fscanf(file, " = %d", &drone.y);
        }
    }

    result = fclose(file);
    if (result != 0)
    {
        perror("fclose");
    }

    printf("drone.symbol = %c\n", drone.symbol);
    printf("drone.color_pair = %hd\n", drone.color_pair);
    printf("M = %f\n", M);
    printf("K = %f\n", K);
    printf("drone.x = %d\n", drone.x);
    printf("drone.y = %d\n", drone.y);

    // Assign shared drone parameters
    shared_drone->x = drone.x;
    shared_drone->y = drone.y;
    shared_drone->symbol = drone.symbol;
    shared_drone->color_pair = drone.color_pair;

    // Initialize variables
    double prev_x = drone.x, prev_y = drone.y;
    double prev_vx = 0, prev_vy = 0;
    double Fx = fx;
    double Fy = fy;

    while (1)
    {
        char command = '\0';
        printf("Reading from pipe\n");
        int bytesRead = read(pipefd[PIPE_READ], &command, sizeof(char));
        printf("Read %d bytes\n", bytesRead);
        printf("Command: %c\n", command);
        if (bytesRead > 0)
        {
            switch (command)
            {
            case 'x':
                fy += 100.0; // forza verso USx
                fx -= 100.0;
                break;
            case 'c':
                fy += 100.0; // forza verso U
                break;
            case 'v':
                fy += 100.0; // forza verso UDx
                fx += 100.0;
                break;
            case 's':
                fx -= 100.0; // forza verso Sx
                break;
            case 'd':
                fy = 0; // annulla forza
                fx = 0;
                break;
            case 'f':
                fx += 100.0; // forza verso Dx
                break;
            case 'w':
                fy -= 100.0; // forza verso DSx
                fx -= 100.0;
                break;
            case 'e':
                fy -= 100.0; // forza verso D
                break;
            case 'r':
                fy -= 100.0; // forza verso DDx
                fx += 100.0;
                break;
            case 'a':
                break;
            case 'b':
                fx = 0; // annulla forza
                fy = 0;
                vx = 0; // annulla velocità
                vy = 0;
                prev_x = drone.x; // annulla posizione
                prev_y = drone.y;
                prev_vx = 0; // annulla velocità
                prev_vy = 0;
                break;
            case '\0':
                command = '\0'; // Comando non valido
                break;
            }
            mvprintw(0, 0, "Comando inviato: %c\n", command);
        }
        else if (bytesRead == 0)
        {
            perror("Nothing to read\n");
        }
        else
        {
            perror("read");
        }

        // Update velocity and position
        double ax = (fx / M) - (K * vx);
        double ay = (fy / M) - (K * vy);
        vx = prev_vx + ax * T;
        vy = prev_vy + ay * T;

        vx = fmax(fmin(vx, 100), -100);
        vy = fmax(fmin(vy, 100), -100);

        double new_x = prev_x + vx * T;
        double new_y = prev_y + vy * T;

        // Store the current position and velocity for the next iteration
        prev_x = new_x;
        prev_y = new_y;
        prev_vx = vx;
        prev_vy = vy;

        mvprintw(1, 0, "x: %.2f, y: %.2f\n", new_x, new_y);
        mvprintw(2, 0, "vx: %f, vy: %f\n", vx, vy);

        refresh();

        if (sem_wait(semaphore) < 0)
        {
            perror("sem_wait");
            exit(EXIT_FAILURE);
        }

        // Use the shared memory
        shared_drone->x = (int)new_x;
        shared_drone->y = (int)new_y;
        shared_drone->symbol = drone.symbol;
        shared_drone->color_pair = drone.color_pair;

        if (sem_post(semaphore) < 0)
        {
            perror("sem_post");
            exit(EXIT_FAILURE);
        }

        clear(); // Clear the screen of all previously-printed characters
    }
    endwin();
    if (sem_close(semaphore) < 0)
    {
        perror("sem_close");
        exit(EXIT_FAILURE);
    }

    if (sem_unlink("/semaphore") < 0)
    {
        perror("sem_unlink");
        exit(EXIT_FAILURE);
    }
    // Detach the shared memory segment from our process's address space
    if (shmdt(shared_drone) == -1)
    {
        perror("shmdt");
        return -1;
    }
    // Close the read end of the pipe
    if (close(pipefd[PIPE_READ]) == -1)
    {
        perror("close");
        return -1;
    }

    // Close the write end of the pipe
    if (close(pipefd[PIPE_WRITE]) == -1)
    {
        perror("close");
        return -1;
    }

    // Close the write end of the pipe when you're done with it
    // close(pipefd[PIPE_WRITE]);
    return 0;
}