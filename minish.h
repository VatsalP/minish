//
// Created by Vatsal Parekh on 9/18/18.
//
#include <sys/types.h>

#ifndef MINISH_MINISH_H
#define MINISH_MINISH_H

#define PROMPT "minish> "
#define MAX_ARGS 66
#define MAX_PIPE 66
#define MAX_BACKGROUND 66


typedef struct {
    int arg_count;
    int background;
    int input;
    int output;
    char ** arg_var;
} arguments;

typedef struct {
    int pipe_count;
    int background;
    arguments * pipe_command;
} pipe_info;

char * trim_white_space(char * str);
int is_in_array(pid_t pid, pid_t arr[], int max);

void minish_bg(char * args);
void minish_cd(char * args);
void minish_builtin(char * args);
void minish_kill(char * args);
void minish_exit(char * args);

int minish_num_builtin();

void signal_handler(int signal);

char * read_line(char * prompt);
pipe_info * split_pipe_args(const char * input);
arguments * split_args(const char * input, arguments *);
void split_pipe_args_delete(pipe_info * store);
void execute(
        arguments * args, int pipe_index, int pipe_last,
        int pipe_read, int pipe_write, int not_pipe,
        int pipe_background
);
void execute_pipe(pipe_info * store);

#endif //MINISH_MINISH_H
