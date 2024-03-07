/* Library with all the functions of the project*/
#include "arplib.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <ncurses.h>
#include <sys/types.h>
#include "../config/config.h"

// Function for spawn process
int spawn(const char *program, char **arg_list)
{
    pid_t child_pid = fork();
    if (child_pid != 0)
        // main process
        return child_pid;
    else
    {
        // child process
        if (execvp(program, arg_list) == -1)
        {
            error("master: exec ");
        }
    }
    return 1;
}
////////////////////////////////////////////////////////////////////////////

// function for write in logfile
void writeLog(const char *format, ...)
{
    // Open the log file for appending
    FILE *logfile = fopen("../log/logfile.txt", "a");
    if (logfile == NULL)
    {
        error("error opening logfile");
    }

    // Initialize the variable argument list
    va_list args;
    va_start(args, format);

    // Get the current time
    time_t current_time;
    time(&current_time);

    // Get the local time structure
    struct tm *local_time = localtime(&current_time);

    // Get the current time in the format HH:MM:SS
    char time_str[9];
    strftime(time_str, sizeof(time_str), "%H:%M:%S", local_time);

    // Print the time on a separate line
    fprintf(logfile, "%s =>\t", time_str);

    // Print the log message on a new line with a newline character
    vfprintf(logfile, format, args);
    fprintf(logfile, "\n");

    // Clean up the variable argument list
    va_end(args);

    // Flush the file stream to ensure the message is written immediately
    fflush(logfile);

    // Close the log file, handle errors if closing fails
    if (fclose(logfile) == -1)
    {
        error("fclose logfile");
        
    }
}
////////////////////////////////////////////////////////////////////////////

// function for write in logfile_sock
void writeLog_sock(const char *format, ...)
{
    // Open the log file for appending
    FILE *logfile = fopen("../log/logfile_sock.txt", "a");
    if (logfile == NULL)
    {
        error("error opening logfile_sock");
    }

    // Initialize the variable argument list
    va_list args;
    va_start(args, format);

    // Get the current time
    time_t current_time;
    time(&current_time);

    // Get the local time structure
    struct tm *local_time = localtime(&current_time);

    // Get the current time in the format HH:MM:SS
    char time_str[9];
    strftime(time_str, sizeof(time_str), "%H:%M:%S", local_time);

    // Print the time on a separate line
    fprintf(logfile, "%s =>\t", time_str);

    // Print the log message on a new line with a newline character
    vfprintf(logfile, format, args);
    fprintf(logfile, "\n");

    // Clean up the variable argument list
    va_end(args);

    // Flush the file stream to ensure the message is written immediately
    fflush(logfile);

    // Close the log file, handle errors if closing fails
    if (fclose(logfile) == -1)
    {
        error("fclose logfile_sock");
    }
}
////////////////////////////////////////////////////////////////////////////

// function for write in logfile_wd
void writeLog_wd(const char *format, ...)
{
    // Open the log file for appending
    FILE *logfile = fopen("../log/logfile_wd.txt", "a");
    if (logfile == NULL)
    {
        error("error opening logfile_wd");
    }

    // Initialize the variable argument list
    va_list args;
    va_start(args, format);

    // Get the current time
    time_t current_time;
    time(&current_time);

    // Get the local time structure
    struct tm *local_time = localtime(&current_time);

    // Get the current time in the format HH:MM:SS
    char time_str[9];
    strftime(time_str, sizeof(time_str), "%H:%M:%S", local_time);

    // Print the time on a separate line
    fprintf(logfile, "%s =>\t", time_str);

    // Print the log message on a new line with a newline character
    vfprintf(logfile, format, args);
    fprintf(logfile, "\n");

    // Clean up the variable argument list
    va_end(args);

    // Flush the file stream to ensure the message is written immediately
    fflush(logfile);

    // Close the log file, handle errors if closing fails
    if (fclose(logfile) == -1)
    {
        error("fclose logfile_wd");
    }
}
////////////////////////////////////////////////////////////////////////////

// return sign of arg X
int sign(int x)
{
    if (x < 0)
        return -1;
    else if (x > 0)
        return 1;
    else
        return 1;
}
////////////////////////////////////////////////////////////////////////////

// create a pipe and convert the value to string for send the fd, used only on master process
void create_pipe(int pipe_fd[], char string_pipe_fd[][20], char *descriptorName)
{
    // create the pipe
    if ((pipe(pipe_fd)) < 0)
    {
        error(descriptorName);
    }
    // convert fd pipe in str
    for (int i = 0; i < 2; i++)
    {
        sprintf(string_pipe_fd[i], "%d", pipe_fd[i]);

    }
}

// close file descriptor and write on logfile
void closeAndLog(int fd, const char *descriptorName)
{
    if (close(fd) < 0)
    {
        perror(descriptorName);
        // Assuming writeLog is a function for logging errors
        writeLog("==> ERROR ==> %s, %m", descriptorName);
        exit(EXIT_FAILURE);
    }
}

// function for print the error in writeLog and exit
void error(char *descriptorName)
{
    perror(descriptorName);
    // Assuming writeLog is a function for logging errors
    writeLog("==> ERROR ==> %s, %m", descriptorName);
    exit(EXIT_FAILURE);
}
////////////////////////////////////////////////////////////////////////////

// error for socket function
void error_sock(char *descriptorName)
{
    perror(descriptorName);
    // Assuming writeLog is a function for logging errors
    writeLog_sock("==> ERROR ==> %s, %m", descriptorName);
    exit(EXIT_FAILURE);
}
////////////////////////////////////////////////////////////////////////////

// error for socket function
void error_wd(char *descriptorName)
{
    perror(descriptorName);
    // Assuming writeLog is a function for logging errors
    writeLog_wd("==> ERROR ==> %s, %m", descriptorName);
    exit(EXIT_FAILURE);
}

////////////////////////////////////////////////////////////////////////////

// save the real pid of the process
// ARGS: 1) pipe array fd, 2) address of the variable to save the pid ex: &child_pids_received[i]
void recive_correct_pid(int pipe_fd[2], int *pid_address)
{
    // close the write file descriptor
    if (close(pipe_fd[1]) == -1)
    {
        perror("master: close RD");
        writeLog("==> ERROR ==> master: close pipe RD, %m ");
    }
    // read from pipe, blocking read
    if (read(pipe_fd[0], pid_address, sizeof(pid_t)) == -1)
    {
        perror("master: read");
        writeLog("==> ERROR ==> master: read, %m ");
    }
    if (close(pipe_fd[0]) == -1)
    {
        perror("master: close WR");
        writeLog("==> ERROR ==> master: close pipe WR, %m ");
    }
}
////////////////////////////////////////////////////////////////////////////
//           SOCKET FUNCTIONS                                           ////
////////////////////////////////////////////////////////////////////////////

// functions for parse the uncoming string
int string_parser_sock(char *string, char *first_arg, char *second_arg)
{
    // define the char that separate the arguments in the string
    char *separator = " ";
    char *arg;
    int ret_val;
    char temp[256];

    strcpy(temp, string);

    arg = strtok(temp, separator);
    strcpy(first_arg, arg);

    arg = strtok(NULL, separator);
    if (arg == NULL)
    {
        ret_val = 0;
    }
    else
    {
        ret_val = 1;
        strcpy(second_arg, arg);
    }

    return ret_val;
}

////////////////////////////////////////////////////////////////////////////

// function for unpack the pipe
void pipe_fd_init(int fd_array[][2], char *argv[], int indx_offset)
{
    int j = 0;
    for (int i = 0; i < 7; i++)
    {
        fd_array[i][0] = atoi(argv[j + indx_offset]);
        fd_array[i][1] = atoi(argv[j + indx_offset + 1]);
        j += 2;
    }
}

////////////////////////////////////////////////////////////////////////////

char parseMessage(const char message[], double array[20][2], int *array_size)
{
    char *msg = (char *)message;
    char id;
    if (sscanf(msg, "%c[%d]", &id, array_size) != 2 || (id != 'O' && id != 'T'))
    {
        writeLog_sock("Failed to parse array size or invalid ID");
        
    }

    msg = strchr(msg, '|');
    if (!msg)
    {
        writeLog_sock("Failed to find start of data");
    }

    for (int i = 0; i < *array_size; i++)
    {
        if (sscanf(msg, "|%lf,%lf", &array[i][0], &array[i][1]) != 2)
        {
            writeLog_sock("Failed to parse pair %d", i);
            return id;
        }

        msg = strchr(msg + 1, '|');
        if (!msg && i != *array_size - 1)
        {
            writeLog_sock("Failed to find separator after pair %d", i);
            return id;
        }
    }
    return id;
}


////////////////////////////////////////////////////////////////////////////

void data_conversion(char string_mat[][256], double reading_set[][2], int lenght)
{
    for (int i = 0; i < lenght; i++)
    {
        sprintf(string_mat[i], "%.3f,%.3f", reading_set[i][0], reading_set[i][1]);
        // save positon in a string in the form (y | x)
    }
}

////////////////////////////////////////////////////////////////////////////

void data_organizer(char string_mat[][256], char send_string[], int lenght, char *client_id)
{
    char header[30];
    bzero(send_string, MAX_MSG_LENGHT);

    sprintf(header, "%c[%d]", client_id[0], lenght);
    // insert the number of obj in the head of the message
    strcat(send_string, header);
    // insert item coords and pipe in the send sendstring
    for (int i = 0; i < lenght; i++)
    {
        strcat(send_string, string_mat[i]);
        if (i < (lenght - 1))
        { // avoid add a pipe after the last element
            strcat(send_string, "|");
        }
    }
}