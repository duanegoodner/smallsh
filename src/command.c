#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "command.h"
#include "utilities.h"

#define COMMENT_CHAR '#'

// Gets input entered by user, converts to a command struct.
// See comments in command.h file for description of command struct members.
struct command* get_command(char* expand_wc, char* expand_repl) {
    
    // initialize number of inputs to zero
    // note: this is not number of args. it is number of space-delimited
    // elements entered at the command line.
    int n_inputs = 0;
    
    // initialize command structure. values of structure members will
    // be initialized and/or set later by helper functions.
    struct command *curr_command;
    
    // get command line input
    char* curr_line = get_input_line();

    // parse into an array of strings (parse_input_line() delimtits by whitespace)
    // also keep track of 
    char** inputs = parse_input_line(curr_line, &n_inputs);

 
    // check if user simply entered a blank line or a comment
    if (n_inputs == 0 || (is_comment(inputs))) {
        
        // if yes, return NULL value of command to main
        curr_command = NULL;
        return curr_command;
    
    // if not a blank line or command, populate members of command struct
    } else {
        curr_command = build_unexpanded_command(inputs, &n_inputs);
    }

    // Since build_unexpanded_command allocated and copied to new memory
    // when building command struct, we can now free memory pointed to by 
    // curr_line and members of inputs array.
    // Taking this approach (allocating memory other than that provided by getline()
    // and parsed by parse_input_line() because strange things might happen if we
    // pass tokens from original string to later functions. 
    // Note: Need to think about best order to free curr_line and inputs. 
    // Seems that may be best to free inputs first since it is built by parsing
    // curr_line. But will be best to just run a test with debugger and watch
    // how memory is freed.
    free(curr_line);
    free(inputs);

    // convert args of command struct as needed based on variable expansion rules
    expand_var(curr_command, expand_wc, expand_repl);

    return curr_command;
}

// 
struct command *build_unexpanded_command(char** inputs, int *n_inputs) {
    struct command *curr_command = malloc(sizeof(struct command));
    
    int index_limit = *n_inputs;
    int arg_count = 0;
    curr_command->next = NULL;
    curr_command->process_id = -5;  // initialize to "safe" val. this mimics example at:
    // https://repl.it/@cs344/51zombieexc 

    curr_command->input_redirect = NULL;
    curr_command->output_redirect = NULL;

    
    // if final element of input array is "&", user has requested a 
    // background command. Note: whether or not command actually runs 
    // in background will depend on status of global variable bg_launch_allowed
    // that is toggled on/off by SIGTSTP signal handler.
    curr_command->background = bg_command_check(inputs, n_inputs);


    index_limit = index_limit - (int) curr_command->background;
    get_argc_and_redirs(curr_command, inputs, index_limit);
    populate_args(curr_command, inputs);

    return curr_command;
}

void populate_args(struct command* curr_command, char** inputs) {
    for (int index = 0; index < curr_command->arg_count; index++) {
        curr_command->args[index] = calloc(strlen(inputs[index]), sizeof(char));
        strcpy(curr_command->args[index], inputs[index]);
    }

    curr_command->args[curr_command->arg_count] = NULL;
}

void get_argc_and_redirs(struct command* curr_command, char** inputs, int index_limit) {
 
    int arg_count = 0;

    // Gather redirect info and count number of actual args.
    // Redirect symbols and filenames will not go into args.
    for (int index = 0; index < index_limit; index++) {
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
bool bg_command_check(char** inputs, int *n_inputs) {
    if (strcmp(inputs[*n_inputs - 1], BG_FLAG)) {
        return false;
    } else {
        return true;
    }
}

void free_command(struct command* curr_command) {
    
    if (curr_command != NULL) {
        for (int index = 0; index < curr_command->arg_count; index++) {
            if (curr_command->args[index] != NULL){
                free(curr_command->args[index]);
            }  
        }
        if (curr_command->input_redirect != NULL) {
            free(curr_command->input_redirect);
        }
        if (curr_command->output_redirect != NULL) {
            free(curr_command->output_redirect);
        }       

        free(curr_command);
    }
    
}

bool is_comment(char **inputs) {
    if (inputs[0][0] == COMMENT_CHAR) {
        return true;
    } else {
        return false;
    }
}

