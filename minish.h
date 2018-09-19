//
// Created by Vatsal Parekh on 9/18/18.
//

#ifndef MINISH_MINISH_H
#define MINISH_MINISH_H

#define PROMPT "minish> "
#define MAX_ARGS 66

typedef struct {
    int arg_count;
    char ** arg_var;
} arguments;

arguments * split_args(const char * input);
void split_args_delete(arguments * args);
void execute(arguments * args);

#endif //MINISH_MINISH_H
