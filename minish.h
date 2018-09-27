//
// Created by Vatsal Parekh on 9/18/18.
//

#ifndef MINISH_MINISH_H
#define MINISH_MINISH_H

#define PROMPT "minish> "
#define MAX_ARGS 66

// TODO
// add input fd and output fd
// in struct
typedef struct {
    int arg_count;
    int background;
    int input;
    int output;
    char ** arg_var;
} arguments;

void keyboard_interrupt(int signal);

char * read_line(char * prompt);
arguments * split_args(const char * input);
void split_args_delete(arguments * args);
void execute(arguments * args);

#endif //MINISH_MINISH_H
