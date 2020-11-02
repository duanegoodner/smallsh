// initial version of parsing code below taken from:
// https://github.com/brenns10/lsh/blob/407938170e8b40d231781576e05282a41634848c/src/main.c
// to at least help with troubleshooting. Made major modifications to get current form.

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "command.h"
#include "utilities.h"


struct command* get_command(char* expand_wc, char* expand_repl) {
    int n_inputs = 0;
    struct command *curr_command;
    
    char* curr_line = get_input_line();
    char** inputs = parse_input_line(curr_line, &n_inputs);

    if (n_inputs == 0) {
        curr_command = NULL;
    } else {
        curr_command = build_prelim_command(inputs, &n_inputs);
    }    
    free(curr_line);
    free(inputs);

    expand_var(curr_command, expand_wc, expand_repl);

    return curr_command;
}

void expand_var(struct command* curr_command, char* old_str, char* new_str) {
    
    if (curr_command != NULL) {
        for (int arg_index = 0; arg_index < curr_command->arg_count; arg_index++) {
            char* ss_ptr = strstr(curr_command->args[arg_index], old_str);
            if (ss_ptr != NULL){
                curr_command->args[arg_index] = dsubstr_replace_all(curr_command->args[arg_index], old_str, new_str);
            }
        }
    }    
} 

char* get_input_line(void) {
    char *curr_line = NULL;
    size_t len = 0;
    ssize_t nread;  
    nread = getline(&curr_line, &len, stdin);
    return curr_line;  
}

#define DELIMITERS " \t\r\n\a"
char** parse_input_line(char *curr_line, int *n_inputs) {
    int index = 0;
    char** inputs = malloc(MAX_ARGS + 7);
    char* token;

    token = strtok(curr_line, DELIMITERS);

    while (token != NULL) {
        inputs[index] = token;
        token = strtok(NULL, DELIMITERS);
        index++;
    }

    *n_inputs = index;
    inputs[index] = NULL;

    return inputs;
}

#define REDIRECT_OUT ">"
bool is_redirect_out(char* input) {
    if (strcmp(input, REDIRECT_OUT)) {
        return false;
    } else {
        return true;
    }
}

#define REDIRECT_IN "<"
bool is_redirect_in(char* input) {
    if (strcmp(input, REDIRECT_IN)) {
        return false;
    } else {
        return true;
    }
}

#define BG_FLAG "&"
bool is_bg_command(char** inputs, int *n_inputs) {
    if (strcmp(inputs[*n_inputs - 1], BG_FLAG)) {
        return false;
    } else {
        return true;
    }
}

struct command *build_prelim_command(char** inputs, int *n_inputs) {
    
    struct command *curr_command = malloc(sizeof(struct command));
    int index;
    int index_limit = *n_inputs;
    int arg_count = 0;
    
    curr_command->next = NULL;
    curr_command->process_id = -5;  // initialize to "safe" val. this mimics example at:
    // https://repl.it/@cs344/51zombieexc 



    // check if curr_command is intended to run in background
    if (is_bg_command(inputs, n_inputs)) {
        curr_command->background = true;
        index_limit--;
    } else {
        curr_command->background = false;
    }

    
    // Gather redirect info and count number of actual args.
    // Redirect symbols and filenames will not go into args.
    for (index = 0; index < index_limit; index++) {
        if (is_redirect_in(inputs[index])) {
            curr_command->input_redirect = calloc(strlen(inputs[index + 1]), sizeof(char));
            strcpy(curr_command->input_redirect, inputs[index + 1]);
            index++;
        } else if (is_redirect_out(inputs[index])) {
            curr_command->output_redirect = calloc(strlen(inputs[index + 1]), sizeof(char));
            strcpy(curr_command->output_redirect, inputs[index + 1]);
            index++;
        } else {
            arg_count++; 
        }
    }
    curr_command->arg_count = arg_count;

    // need (arg_count + 1) elements so we can have NULL as final array element 
    char** args = malloc(arg_count * sizeof(char*));
   
    for (index = 0; index < arg_count; index++) {
        args[index] = calloc(strlen(inputs[index]), sizeof(char));
        strcpy(args[index], inputs[index]);
    }
    
    args[arg_count] = NULL;
    curr_command->args = args;
    
    return curr_command;
}

void free_command(struct command* curr_command) {
    
    if (curr_command != NULL) {
        // for (int index = curr_command->arg_count; index >= 0; index--) {
        //     if (curr_command->args[index] != NULL){
        //         free(curr_command->args[index]);
        //     }  
        // }
        // if (curr_command->args != NULL)
        // {
        //     free(curr_command->args);
        // }
        // if (curr_command->input_redirect != NULL) {
        //     free(curr_command->input_redirect);
        // }
        // if (curr_command->output_redirect != NULL) {
        //     free(curr_command->output_redirect);
        // }       

        free(curr_command);
    }
    
}

#define COMMENT_CHAR '#'
bool is_comment(struct command *curr_command) {
    if (curr_command->args[0][0] == COMMENT_CHAR) {
        return true;
    } else {
        return false;
    }
}

bool is_null(struct command *curr_command) {
    if (curr_command->args[0] == NULL) {
        return true;
    } else {
        return false;
    }
}
