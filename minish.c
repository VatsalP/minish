//
// Created by Vatsal Parekh on 9/18/18.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <editline/readline.h>

#include "minish.h"


int main(int argc, char ** argv) {
    puts("Mini Shell Version 0.0.1");
    puts("Ctrl+C to exit");

    while (1) {
        char * input = readline(PROMPT);
        add_history(input);
        if (strcmp(input, "exit") == 0) {
            // cleanup();
            free(input);
            break;
        }
        else
            execute(input);
        free(input);
    }
    return 0;
}


/***
 * Free mem allocated to argument struct
 * @param args arguments struct to be freed
 */
void split_args_delete(arguments * args) {
    /*
    for (int i = 0; i < MAX_ARGS; i++) {
        char * arg = args->arg_var[i];
        if (arg != NULL) {
            free(arg);
        }
    }
     */
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
    args->arg_var = calloc(MAX_ARGS, sizeof(char *));

    char * token, * str;

    str = strdup(input);
    while((token = strsep(&str," \t\r\n"))) {
        args->arg_var[args->arg_count] = token;
        args->arg_count += 1;
    }
    free(str);
    return args;
}

/***
 * Execute user provided command
 *
 * @param input - user input
 */
void execute(const char * input) {
    arguments * args = split_args(input);
    /*
    printf("Arg count %d\n", args->arg_count);
    for (int i = 0; i < args->arg_count; i++) {
        fprintf(stdout, "%s ", args->arg_var[i]);
    }
    */

    
    split_args_delete(args);
}