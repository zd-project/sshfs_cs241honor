#include <stdio.h>
#include <sys/types.h>

/*
Multithreaded SSH Project

Thread Safe Util

Author: Ziang Wan
Date: 11/7
*/

// print the shell prompt
void print_shell_prompt(void);

// split the str by whitespace and deep copy, null terminated
char **strsplit(const char *str, size_t expected_tokens, size_t* num_token);
// free the result of split args
void free_args(char **args);

// continuing read & write
ssize_t read_all_from_socket(int socket, char* buffer, size_t count);
// writing
ssize_t write_all_to_socket(int socket, char* buffer, size_t count);