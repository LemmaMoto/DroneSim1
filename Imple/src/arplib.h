// header file of library "arplib.h"

#ifndef ARPLIB_H
#define ARPLIB_H

// Headers of functions

int spawn(const char *program, char **arg_list);

void writeLog(const char *format, ...);

void writeLog_sock(const char *format, ...);

void writeLog_wd(const char *format, ...);

int sign(int x);

void create_pipe(int pipe_fd[], char string_pipe_fd[][20], char *descriptorName);

void recive_correct_pid(int pipe_fd[2], int *pid_address);

void closeAndLog(int fd, const char *descriptorName);

void error(char *descriptorName);

void error_sock(char *descriptorName);

void error_wd(char *descriptorName);


/// SOCKET FUNCTIONS ///

int string_parser_sock(char *string, char *first_arg, char *second_arg);

void pipe_fd_init(int fd_array[][2], char *argv[], int indx_offset);

char parseMessage(const char message[], double array[20][2], int *array_size);

void data_conversion(char string_mat[][256], double reading_set[][2], int lenght);

void data_organizer(char string_mat[][256], char send_string[], int lenght, char *client_id);





#endif //ARPLIB_H