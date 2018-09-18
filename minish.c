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


static char buffer[2048]; // max no. of bytes that can be filled by input

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


arguments * split_args(const char * input) {
    arguments * args = malloc(sizeof(arguments));
    args->arg_count = 0;
    args->arg_var = calloc(MAX_ARGS, sizeof(char *));
    return args;
}

void execute(const char * input) {
    arguments * args = split_args(input);
    printf("LOL LMAO LOL LOLOLOL %d\n", args->arg_count);
}