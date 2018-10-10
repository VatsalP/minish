//
// Created by Vatsal Parekh on 9/18/18.
//
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>

#include "minish.h"

// Input buffer
static char buffer[2048];

// List of builtin commands, followed by their corresponding functions.
char * builtin_str[] = {
        "builtin",
        "kill",
        "exit"
};

void (* builtin_func[]) (char **) = {
        &minish_builtin,
        &minish_kill,
        &minish_exit
};


int main(int argc, char ** argv) {
    puts("Mini Shell Version 0.0.1");
    puts("builtin to see the builtins");

    if ((signal(SIGINT, signal_handler) == SIG_ERR) ||
            (signal(SIGCHLD, signal_handler) == SIG_ERR)
    ) {
        perror("Can't catch sigint");
        exit(EXIT_FAILURE);
    }

    for (;;) {
        char * input = read_line(PROMPT);
        int built_in = 0;

        for (int i = 0; i < minish_num_builtin(); i++) {
            if (strcmp(&input[0], builtin_str[i]) == 0) {
                built_in = 1;
                (*builtin_func[i])(&input);
            }
        }
        if (!built_in && (strcmp("", input) != 0)) {
            pipe_info * store = split_pipe_args(input);
            execute_pipe(store);
            //arguments *args = split_args(input);
            //execute(args);
        }
        free(input);
    }
}


void minish_builtin(char ** args) {
    puts("Mini Shell Version 0.0.1");
    puts("Builtins are:");
    for (int i = 0; i < minish_num_builtin(); i++) {
        printf("%d: %s\n", (i+1), builtin_str[i]);
    }
}

void minish_kill(char ** args) {
    pid_t pid = (pid_t) atoi(args[1]);
    if (!pid)
        puts("Need a valid pid");

    if (pid && kill(pid, SIGKILL) == -1)
        perror("Error while using kill:");
}

void minish_exit(char ** args) {
    //killpg(0, SIGKILL);
    exit(EXIT_SUCCESS);
}

/***
 * To find how many builtins are there
 * @return no. of builtins
 */
int minish_num_builtin() {
    return sizeof(builtin_str)/ sizeof(char *);
}


/***
 * To handle ctrl+c
 * @param signal
 */
void signal_handler(int signal) {
    // Does nothing
    if (signal == SIGCHLD)
        wait(NULL);
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
 * Deallocate/Free mem of pipe_info *
 * @param store - pointer to pipe_info
 */
void split_pipe_args_delete(pipe_info * store) {
    //free(store->pipe_command);
    //ls
    free(store);
}


/***
 * Create pipe_info struct and execute structs
 *
 * @param input - input string
 * @return status of last foreground command
 */
pipe_info * split_pipe_args(const char * input) {
    pipe_info * store = malloc(sizeof(pipe_info));
    store->pipe_count = 0;
    store->pipe_command = calloc(MAX_PIPE, sizeof(pipe_info *));

    char * token, * str;
    if (strcmp(input, "") != 0) {
        str = strdup(input);
        int i = 0; // index counter for pipe_commands
        while((token = strsep(&str, "|"))) {
            if (i > 0) token = token + i; // workaround...
            store->pipe_command[i] = * (split_args(token)); // derefrencing... +)
            store->pipe_count = ++i;
        }
        free(str);
    }
    return store;
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
        while ((token = strsep(&str, " \t\r\n")) != NULL) {
            if (strlen(token) == 0) continue;
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
  * Execute ze pipeline xd
  *
  * @param store
  * @param pipe_index
  * @param pipe_last
  */
void execute_pipe(pipe_info * store) {
    // for command without pipe
    if (store->pipe_count == 1) {
        execute(&store->pipe_command[0], 0, 1, -1, -1, 1);
    }
    else {
        int pipe_fd[2], input = -1;
        int i = 0;
        // for loop ze commands
        for(; i < (store->pipe_count-1); i++) {
            if (pipe(pipe_fd) == -1) {
                perror("PIPE Error");
                exit(EXIT_FAILURE);
            }
            // meh
            execute(&store->pipe_command[i], i, 0, input, pipe_fd[1], 0);
            close(pipe_fd[1]); // close write in parent
            input = pipe_fd[0]; // keep read end for next child
        }
        // last command in pipe
        execute(&store->pipe_command[i], i, 1, input, 1, 0);
        close(input);
    }
    while (waitpid(-1, NULL, 0)) {
        if (errno == ECHILD) {
            // means we are done and there are no more child processes
            break;
        }
    }
    split_pipe_args_delete(store);
}

/***
 * Execute user provided command
 *
 * @param input - user input
 */
void execute(
        arguments * args, int pipe_index, int pipe_last,
        int pipe_read, int pipe_write, int not_pipe
        ) {
    pid_t pid;
    int status;

    if (args->arg_count > 0) {
        pid = fork();
        if (pid == 0) {

            // read pipe end
            if (!(not_pipe) && pipe_read > -1 && pipe_index != 0) {
                if (dup2(pipe_read, 0) == -1) {
                    perror("Error while using dup on pipe read end");
                    exit(EXIT_FAILURE);
                }
                close(pipe_read);
            }

            // write pipe end
            if (!(not_pipe) && pipe_write != 1) {
                if (dup2(pipe_write, 1) == -1) {
                    perror("Error while using dup on pipe write end");
                    exit(EXIT_FAILURE);
                }
                close(pipe_write);
            }

            // for input redirection
            if (args->input && pipe_index == 0) {
                int fd = open(args->arg_var[args->input + 1], O_RDONLY | O_CLOEXEC);
                if (dup2(fd, 0) == -1) {
                    perror("Error while using dup");
                    exit(EXIT_FAILURE);
                }
                args->arg_var[(args->input)++] = NULL;
                args->arg_var[args->input] = NULL;
            }

            // for output redirection
            if (args->output && pipe_last) {
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
            if (!args->background && not_pipe)
                waitpid(pid, &status, 0);
        }
    }
    //split_args_delete(args);
}