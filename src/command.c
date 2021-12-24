#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "definitions.h"
#include "globals.h"
#include "command.h"
#include "utilities.h"


// Gets input entered by user, converts to a command struct.
struct command* get_command() {

    // get command line input and convert to input struct
    struct input* curr_input = malloc(sizeof(struct input));
    char* curr_line = get_input_line();
    populate_input_struct(curr_line, curr_input);

    // curr_input->parsed_words = parse_input_line(curr_line, &curr_input->num_words);

    // Values of command structure members will be initialized and/or set later
    struct command *curr_command = malloc(sizeof(struct command));

    // check if user simply entered a blank line or a comment
    if (curr_input->num_words == 0 || (is_comment(curr_input->parsed_words))) {

        // if yes, return NULL value of command to main
        curr_command = NULL;
        return curr_command;

    // if not a blank line or command, populate members of command struct
    } else {
        populate_command(curr_command, curr_input);
    }

    /* Unsure of correct order to free since curr_input.parsed points to various
    parts of block that curr_line points to, but order below seems to work.  */
    free(curr_line);
    free(curr_input->parsed_words);
    free(curr_input);

    return curr_command;
}

void populate_input_struct(char *curr_line, struct input* curr_input) {
    int word_index = 0;

    curr_input->parsed_words = malloc(MAX_ARGS + MAX_EXTRA_WORDS);
    char* token;

    // tokenize the input string and count the number of tokens
    token = strtok(curr_line, DELIMITERS);
    while (token != NULL) {
        curr_input->parsed_words[word_index] = token;
        token = strtok(NULL, DELIMITERS);
        word_index++;
    }

    curr_input->num_words = word_index;

    curr_input->parsed_words = realloc(
        curr_input->parsed_words, sizeof(
            curr_input->parsed_words) * (word_index + 1));
    curr_input->parsed_words[word_index] = NULL;
}

// Read a line of input entered at command line, and return a pointer to dynamicall allocated memory
// where this data are stored.
char* get_input_line(void) {
    char *curr_line = NULL;
    size_t len = 0;
    ssize_t nread;
    nread = getline(&curr_line, &len, stdin);
    return curr_line;
}

// Takes an input string (probably obtained by getline()), counts the number of whitespace delimited
// elements, and returns an array of tokens pointing to each whitespace delimited element of the
// input string.
char** parse_input_line(char *curr_line, int *n_inputs) {
    int index = 0;
    // Start out with array capable of handling max input size in specs.
    // The +7 is for possible non-arg elements.
    char** inputs = malloc(MAX_ARGS + 7);
    char* token;

    // tokenize the input string and count the number of tokens
    token = strtok(curr_line, DELIMITERS);
    while (token != NULL) {
        inputs[index] = token;
        token = strtok(NULL, DELIMITERS);
        index++;
    }

    // place value corresponding to number of tokens in integer pointed to by
    *n_inputs = index;

    // trim size of inputs (dynamic array of char pointers) to only whats's needed
    inputs = realloc(inputs, index + 1);

    // set final slement of inputs to NULL for compatibility with execvp
    inputs[index] = NULL;

    return(inputs);
}

void populate_command(struct command* curr_command, struct input* curr_input) {

    // index_limit determines how many items get copied into command args
    int index_limit = curr_input->num_words;

    // background commands will be in linked list for tracking
    // since only need .next for bg procs, could use smaller struct here
    curr_command->next = NULL;

    // initialize pid to value that can't be confused with real pid
    curr_command->process_id = -5;

    // will update these later if have redirects. initializing to NULL helps
    // avoid double-free
    curr_command->input_redirect = NULL;
    curr_command->output_redirect = NULL;

    // set bg flag, and if true, reduce copy index by 1 b/c won't need the '&'
    curr_command->background = bg_command_check(curr_input->parsed_words, curr_input->num_words);
    index_limit = index_limit - (int) curr_command->background;

    // algo to set redirects also gives arg counts, so set all in 1 funct
    get_argc_and_redirs(curr_command, curr_input->parsed_words, index_limit);

    populate_args(curr_command->arg_count, curr_command->args, curr_input->parsed_words);

    char* expand_repl = malloc_atoi(getpid());
    expand_var(curr_command, VAR_EXPAND, expand_repl);
    free(expand_repl);
}

// takes appropriate elemnts of inputs array and copies them to commmand structs
// args array
void populate_args(int arg_count, char** args, char** inputs) {

    // total number of copy operations = command structs arg_count
    for (int index = 0; index < arg_count; index++) {

        // for each entry that will be copied, allocate new memory, and have and
        // element of command.args point to it.
        args[index] = calloc(strlen(inputs[index]), sizeof(char));

        // copy string from inputs array to command.args array
        strcpy(args[index], inputs[index]);
    }

    // enter NULL as final (used) element in command.args for use with execvp
    args[arg_count] = NULL;
}

// Counts number of elements in inputs array that need to be copied into args, and gets
// filenames of input/output redirects if specified. These pieces of info are entered
// in corresponding members of command struct.
void get_argc_and_redirs(struct command* curr_command, char** inputs, int index_limit) {

    // initialize arg counter to zero
    int arg_count = 0;

    // Gather redirect info and count number of actual args.
    // Redirect symbols and filenames will not go into args.
    // Start by iterating over each element of inputs (except for final element if it is
    // and '&' used to indicate background process)
    for (int index = 0; index < index_limit; index++) {
        // If we find an input redirect symbol in inputs, set the next elements of inputs
        // to be the input redirect path, and advance loop counter by an exta unit.
        if (is_redirect_in(inputs[index])) {
            curr_command->input_redirect = calloc(strlen(inputs[index + 1]), sizeof(char));
            strcpy(curr_command->input_redirect, inputs[index + 1]);
            index++;
        // Execute analogous instructions if we find and output redirect symbol
        } else if (is_redirect_out(inputs[index])) {
            curr_command->output_redirect = calloc(strlen(inputs[index + 1]), sizeof(char));
            strcpy(curr_command->output_redirect, inputs[index + 1]);
            index++;
        // If current element of inputs array is not for input/output redirect, it counts
        // as an arg (note we already avoid incorrectly counting an & at end of inputs
        // by setting index_limit)
        } else {
            arg_count++;
        }
    }
    // Set command's .arg_count member equal to counter in current function.
    // Could simplifiy by just using curr_command.arg_count inside loop.
    curr_command->arg_count = arg_count;
}

// Takes a populated command structure and modifies elements of args as needed based on
// variable expansion specification. In current implementation, just have one expansion
// rule. old_str = substring to be replaced and news_str = substing to replace with.
void expand_var(struct command* curr_command, char* old_str, char* new_str) {

    // make sure we don't try this on a NULL command. Program structure should prevent this from
    // happening, but still good to be safe.
    if (curr_command != NULL) {
        // vist each element of command's .args member
        for (int arg_index = 0; arg_index < curr_command->arg_count; arg_index++) {
            // use C-library strstr function to check if old_str is a substring of arg element
            char* ss_ptr = strstr(curr_command->args[arg_index], old_str);
            // if there is a substring match, strstr returns a pointer to start of substring in
            // the element of input. If no match, returns NULL. If there is a match, use dsubstr_replace_all
            // to create modified args element.
            if (ss_ptr != NULL){
                curr_command->args[arg_index] = dsubstr_replace_all(curr_command->args[arg_index], old_str, new_str);
            }
        }
    }
}





// checks if element of inputs is an output redirect symbol
bool is_redirect_out(char* input) {
    if (strcmp(input, REDIRECT_OUT)) {
        return false;
    } else {
        return true;
    }
}

// checks if element of inputs is an input redirect symbol
bool is_redirect_in(char* input) {
    if (strcmp(input, REDIRECT_IN)) {
        return false;
    } else {
        return true;
    }
}

// checks if final element of inputs is a background process indicator
bool bg_command_check(char** inputs, int n_inputs) {
    if (strcmp(inputs[n_inputs - 1], BG_FLAG)) {
        return false;
    } else {
        return true;
    }
}

// frees memory of a command structure.
// Note: since currently using static array for .args, only need to free elements of args,
// and do not need to free args array itself (that gets taken care of by the line 'free(command)' )
void free_command(struct command* curr_command) {

    // Make sure we don't try to free a NULL command, otherwise may have double free() fault.
    if (curr_command != NULL) {
        // iterate over each *populated* element of args array
        for (int index = 0; index < curr_command->arg_count; index++) {
            // if entry is not NULL, free its memory
            if (curr_command->args[index] != NULL){
                free(curr_command->args[index]);
            }
        }
        // free the pointers to the redirect filename strings
        if (curr_command->input_redirect != NULL) {
            free(curr_command->input_redirect);
        }
        if (curr_command->output_redirect != NULL) {
            free(curr_command->output_redirect);
        }

        // free the pointer to the actual command structure
        free(curr_command);
    }
}

// checks if first element of input string incidates that command is a comment
bool is_comment(char **inputs) {
    if (inputs[0][0] == COMMENT_CHAR) {
        return true;
    } else {
        return false;
    }
}

