#include <stdio.h>
#include <stdbool.h>

#define MAX_CLL 2048
#define MAX_ARGS 512



#ifndef THERE_IS_A_COMMAND
#define THERE_IS_A_COMMAND

struct command
{
    int arg_count;
    char *args[MAX_ARGS];
    char *input_redirect;
    char *output_redirect;
    bool background;
    pid_t process_id;
    struct command *next;
};



struct command *get_command(char* expand_wc, char* expand_repl);
char* get_input_line(void);
char** parse_input_line(char *curr_line, int *n_inputs);
struct command *build_unexpanded_command(char** inputs, int *n_inputs);
void get_argc_and_redirs(struct command* curr_command, char** inputs, int index_limit);
void populate_args(struct command* curr_command, char** inputs);
void expand_var(struct command* curr_command, char* old_str, char* new_str);

bool is_comment(char** inputs);
bool is_redirect_out(char* input);
bool is_redirect_in(char* input);
bool bg_command_check(char** inputs, int *n_inputs); 

void free_command (struct command* curr_command);

#endif