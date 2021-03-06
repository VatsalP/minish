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
#include <ctype.h>
#include <errno.h>
#include <unistd.h>

#include "minish.h"


// Background pid array
static pid_t background_array[MAX_BACKGROUND];
static pid_t pipe_pid = 0;

// Input buffer
static char buffer[2048];

// List of builtin commands, followed by their corresponding functions.
char * builtin_str[] = {
        "bg",
        "cd",
        "builtin",
        "kill",
        "exit"
};

// ayy
void (* builtin_func[]) (char *) = {
        &minish_bg,
        &minish_cd,
        &minish_builtin,
        &minish_kill,
        &minish_exit,
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
        pipe_pid = 0;

        for (int i = 0; i < minish_num_builtin(); i++) {
            if (memcmp(&input[0], builtin_str[i], strlen(builtin_str[i])) == 0) {
                built_in = 1;
                (* builtin_func[i])(input);
            }
        }
        if (!built_in && (strcmp("", input) != 0)) {
            pipe_info * store = split_pipe_args(input);
            execute_pipe(store);
        }
        free(input);
        input = NULL;
    }
}


void minish_bg(char * args) {
    for(int i = 0, j = 0; i < MAX_BACKGROUND; i++) {
        if(background_array[i]) {
            printf("%d: %d\n", j++, background_array[i]);
        }
    }
}

void minish_cd(char * args) {
    arguments * arg = malloc(sizeof(arguments));
    arg =  split_args(args, arg);
    if(chdir(arg->arg_var[1]) == -1)
        perror("Error while changing directory");
    free(arg->arg_var);
    free(arg);
    arg = NULL;
}


void minish_builtin(char * args) {
    puts("Mini Shell Version 0.0.1");
    puts("Builtins are:");
    for (int i = 0; i < minish_num_builtin(); i++) {
        printf("%d: %s\n", (i+1), builtin_str[i]);
    }
}

void minish_kill(char * args) {
    arguments * arg = malloc(sizeof(arguments));
    arg =  split_args(args, arg);
    pid_t pid = (pid_t) atoi(arg->arg_var[1]);
    if (!pid)
        puts("Need a valid pid");
    int proceed = 1;
    if (getpgrp() != getpgid(pid)) {
        pid_t pgid = getpgid(pid);
        int ret = is_in_array(pgid, background_array, MAX_BACKGROUND);
        if (ret) {
            int i = 0;
            while(background_array[i++] != pgid && i < MAX_BACKGROUND);
            background_array[--i] = 0;

        } else {
            int i = 0;
            while(background_array[i++] != pid && i < MAX_BACKGROUND);
            background_array[--i] = 0;
        }
        if (pid && killpg(pgid, SIGKILL) == -1)
            perror("Error while using killpg");
        proceed = 0;
    }

    if (proceed && pid && kill(pid, SIGKILL) == -1)
        perror("Error while using kill");

    free(arg->arg_var);
    free(arg);
    arg = NULL;
}

void minish_exit(char * args) {
    for(int i = 0; i < MAX_BACKGROUND; i++) {
        if(background_array[i]) {
            if(kill(background_array[i], SIGKILL) == -1) {
                fprintf(stderr, "Error while killing %d\n", background_array[i]);
                perror("");
            }
        }
    }
    free(args);
    args = NULL;
    exit(EXIT_SUCCESS);
}

/***
 * To find how many builtins are there
 * @return no. of builtins
 */
int minish_num_builtin() {
    return sizeof(builtin_str) / sizeof(char *);
}


/***
 * To handle ctrl+c
 * @param signal
 */
void signal_handler(int signal) {
    // Does nothing for sigint
    if (signal == SIGCHLD) {
        pid_t child_pid = wait(NULL);
        if (getpgrp() != getpgid(child_pid)) {
            int i = 0;
            while(background_array[i++] != child_pid && i < MAX_BACKGROUND);
            background_array[--i] = 0;
        }
    }

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
 * Utility function to check if value is in array or not
 *
 * @param pid
 * @param arr
 * @param max
 * @return 1 or 0 if yes or no
 */
int is_in_array(pid_t pid, pid_t arr[], int max) {
    for (int i = 0; i < max; i++) {
        if (pid == arr[i]) return 1;
    }
    return 0;
}


/***
 * Deallocate/Free mem of pipe_info *
 * @param store - pointer to pipe_info
 */
void split_pipe_args_delete(pipe_info * store) {
    for (int i = 0; i < MAX_PIPE; i++) {
        free(store->pipe_command[i].arg_var);
    }
    free(store->pipe_command);
    free(store);
    store = NULL;
}


/***
 * To trim whitespace
 * From https://stackoverflow.com/a/122721/5501519
 * @param str
 * @return string with whitespaces removed
 */
char * trim_white_space(char *str)
{
    char * end;

    // Trim leading space
    while(isspace((unsigned char) * str)) str++;

    if(* str == 0)  // All spaces?
        return str;

    // Trim trailing space
    end = str + strlen(str) - 1;
    while(end > str && isspace((unsigned char) * end)) end--;

    // Write new null terminator character
    end[1] = '\0';

    return str;
}

/***
 * Create pipe_info struct and execute structs
 *
 * @param input - input string
 * @return status of last foreground command
 */
pipe_info * split_pipe_args(const char * input) {
    pipe_info * store = (pipe_info *)malloc(sizeof(pipe_info));
    store->pipe_count = 0;
    store->background = 0;
    store->pipe_command = calloc(MAX_PIPE, sizeof(arguments));

    char * token, * str;
    if (strcmp(input, "") != 0) {
        str = strdup(input);
        int i = 0; // index counter for pipe_commands
        while((token = strsep(&str, "|"))) {
            token = trim_white_space(token);
            //if (i > 0) token = token + i; // workaround...
            store->pipe_command[i] = * split_args(token, &store->pipe_command[i]); // derefrencing... +)
            store->pipe_count = ++i;
        }
        free(str);
        str = NULL;
    }
    if (store->pipe_count > 1 && store->pipe_command[store->pipe_count-1].background)
        store->background = 1;
    return store;
}

/***
 * Create arguments struct and store input args in it
 *
 * @param input - command entered by user
 * @return prepared arguments struct
 */
arguments * split_args(const char * input, arguments * args) {
    //arguments * args = malloc(sizeof(arguments));
    args->arg_count = 0;
    args->background = 0;
    args->input = 0;
    args->output = 0;
    args->arg_var = calloc(MAX_ARGS, sizeof(char *));

    char * token, * str;
    if (strcmp(input, "") != 0) {
        str = strdup(input);
        while ((token = strsep(&str, " \t\r\n")) != NULL) {
            token = trim_white_space(token);
            if (strlen(token) == 0) continue;
            if (strcmp(token, "<") == 0) {
                args->input = args->arg_count;
            }
            else if (strcmp(token, ">") == 0) {
                args->output = args->arg_count;
            }
            if (strcmp(token, "&") == 0) {
                args->background = 1;
                continue;
            }
            args->arg_var[args->arg_count] = token;
            args->arg_count += 1;
        }
        args->arg_var[args->arg_count] = NULL;
        free(str);
        str = NULL;

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
        execute(&store->pipe_command[0], 0, 1, -1, -1, 1, 0);
    }
    else {
        int pipe_fd[2], input = -1;
        int i = 0;
        // for loop ze commands
        for (; i < (store->pipe_count - 1); i++) {
            if (pipe(pipe_fd) == -1) {
                perror("PIPE Error");
                exit(EXIT_FAILURE);
            }
            // meh
            execute(&store->pipe_command[i], i, 0, input, pipe_fd[1], 0, store->background);
            close(pipe_fd[1]); // close write in parent
            input = pipe_fd[0]; // keep read end for next child

        }
        // last command in pipe
        execute(&store->pipe_command[i], i, 1, input, 1, 0, store->background);
        close(input);

        if (!store->background) {
            while (waitpid(-1, NULL, 0)) {
                if (errno == ECHILD) {
                    // means we are done and there are no more child processes
                    break;
                }
            }
        }
    }
    split_pipe_args_delete(store);
}

/***
 * Execute user provided command
 *
 * @param args - command to exec
 * @param pipe_index - current pipe component
 * @param pipe_last - is it?
 * @param pipe_read - read side fd
 * @param pipe_write - write side fd
 * @param not_pipe - to be or not to be
 * @param pipe_background - meh
 */
void execute(
        arguments * args, int pipe_index, int pipe_last,
        int pipe_read, int pipe_write, int not_pipe,
        int pipe_background
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

            // now exec finally xd
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
            if (pipe_background) {
                if (!pipe_pid) {
                    pipe_pid = pid;
                    int i = 0;
                    while (background_array[i++] != 0 && i < MAX_BACKGROUND);
                    background_array[--i] = pipe_pid;
                }
                if(setpgid(pid, pipe_pid) == -1)
                    perror("Error while setting pgid");
            }

            if (args->background && not_pipe) {
                if(setpgid(pid, pid) == -1)
                    perror("Error while setting pgid");
                int i = 0;
                while(background_array[i++] != 0 && i < MAX_BACKGROUND);
                background_array[--i] = pid;
            }

            if (!args->background && not_pipe)
                waitpid(pid, &status, 0);
        }
    }
}