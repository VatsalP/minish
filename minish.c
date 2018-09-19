//
// Created by Vatsal Parekh on 9/18/18.
//
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

#include "minish.h"


static char buffer[2048];


int main(int argc, char ** argv) {
    puts("Mini Shell Version 0.0.1");
    puts("Ctrl+C to exit");

    while (1) {
        char * input = read_line(PROMPT);

        if (strcmp(input, "exit") == 0) { // Builtins xd
            // cleanup();
            free(input);
            break;
        }
        else {
            arguments *args = split_args(input);
            execute(args);
        }
        free(input);
    }
    return 0;
}


/***
 * Read line from user input
 * @param prompt "minish> "
 * @return char pointer to beginning of user inputted string
 */
char * read_line(char * prompt) {
    fputs(prompt, stdout);
    fgets(buffer, 2048, stdin);
    char * copy = malloc(strlen(buffer) + 1);
    strcpy(copy, buffer);
    copy[strlen(copy)-1] = '\0';
    return copy;
}

/***
 * Free mem allocated to argument struct
 * @param args arguments struct to be freed
 */
void split_args_delete(arguments * args) {
    free(args->arg_var);
    free(args);
}

/***
 * Create arguments struct and store input args in it
 *
 * @param input - command entered by user
 * @return prepared arguments struct
 */
arguments * split_args(const char * input) {
    arguments * args = malloc(sizeof(arguments));
    args->arg_count = 0;
    args->background = 0;
    args->input = 0;
    args->output = 0;
    args->arg_var = calloc(MAX_ARGS, sizeof(char *));

    char * token, * str;
    if (strcmp(input, "") != 0) {
        str = strdup(input);
        while ((token = strsep(&str, " \t\r\n"))) {
            if (strcmp(token, "<") == 0) {
                args->input = args->arg_count;
            }
            else if (strcmp(token, ">") == 0) {
                args->output = args->arg_count;
            }
            args->arg_var[args->arg_count] = token;
            args->arg_count += 1;
        }
        // check for background
        if (strcmp(args->arg_var[args->arg_count - 1], "&") == 0) {
            args->background = 1;
            args->arg_count -= 1;
        }
        args->arg_var[args->arg_count] = NULL;
        free(str);

    }
    return args;
}

/***
 * Execute user provided command
 *
 * @param input - user input
 */
void execute(arguments * args) {
    pid_t pid;
    int status;

    if (args->arg_count > 0) {
        pid = fork();
        if (pid == 0) {
            if (args->input) {
                int fd = open(args->arg_var[args->input + 1], O_RDONLY | O_CLOEXEC);
                if (dup2(fd, 0) == -1) {
                    perror("Error while using dup");
                    exit(EXIT_FAILURE);
                }
                args->arg_var[(args->input)++] = NULL;
                args->arg_var[args->input] = NULL;
            }
            if (args->output) {
                int fd = open(args->arg_var[args->output + 1], O_WRONLY | O_CLOEXEC | O_CREAT, 0644);
                if (dup2(fd, 1) == -1) {
                    perror("Error while using dup");
                    exit(EXIT_FAILURE);
                }
                args->arg_var[(args->output)++] = NULL;
                args->arg_var[args->output] = NULL;
            }
            if (execvp(
                    args->arg_var[0],
                    args->arg_var
            ) == -1) {
                perror("Error while executing exec");
                exit(EXIT_FAILURE);
            };
        } else if (pid == -1) {
            perror("Error while creating child");
            exit(EXIT_FAILURE);
        } else { // Parent ze shell
            if (!args->background)
                wait(&status);
        }
    }
    split_args_delete(args);
}