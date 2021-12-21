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

//  n_inputs is number of space-delimited elements entered (not number of args)
    int n_inputs = 0;

//  Values of command structure members will be initialized and/or set later
    struct command *curr_command;

    // get command line input
    char* curr_line = get_input_line();
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
    char* expand_repl = malloc_atoi(getpid());
    expand_var(curr_command, VAR_EXPAND, expand_repl);

    return curr_command;
}

// Takes array of elements of input line and builds command struct
// Params:  inputs = array of the space delimted elements entered at command line.
//          n_inputs = pointer to number of elements in inputs
struct command *build_unexpanded_command(char** inputs, int *n_inputs) {

    // allocate memory for command sruct
    struct command *curr_command = malloc(sizeof(struct command));

    // declare local to hold number of inputs elements so we can manipulate it
    // without messing with info held outside of function (that we might want later)
    int index_limit = *n_inputs;

    // initialize number of args (this is number of arguments in command. Same as
    // argc in conventional use. Note that file redirecty symbols, filenames, and a final
    // '&' character indicating background process do not get counted as part of argc.
    int arg_count = 0;

    // command struct has a .next member for use in linked list. set to null.
    curr_command->next = NULL;

    // Since .process_id is an int and not a pointer, can't initialize to NULL.
    // Instead, initialize as a "safe" value. Using -5 this mimics example at:
    // https://repl.it/@cs344/51zombieexc
    curr_command->process_id = -5;

    // set redirect filename pointers to NULL. Note: If this is not done,
    // we run the risk of a "double free()" when command struct is eventually freed.
    curr_command->input_redirect = NULL;
    curr_command->output_redirect = NULL;


    // if final element of input array is "&", user has requested a
    // background command. Note: whether or not command actually runs
    // in background will depend on status of global variable bg_launch_allowed
    // that is toggled on/off by SIGTSTP signal handler.
    curr_command->background = bg_command_check(inputs, n_inputs);

    // if inputs array ends with and '&' (which makes it a background command)
    // ww need to reduce the upper limit that will be used when we copy information
    // from inputs array into command structs **args array (so reduce index_limit by
    // 1 if command is a background command). For current approach to work, need to
    // have already set curr_command.background to either true or fale.
    // To be cautious, cast the boolean .background to an int before subtraction.
    index_limit = index_limit - (int) curr_command->background;

    // populste command structs arg_count and input/output redirect filename members
    get_argc_and_redirs(curr_command, inputs, index_limit);

    // populate command structs args array using appropriate elements of inputs array
    populate_args(curr_command, inputs);

    return curr_command;
}

// takes appropriate elemnts of inputs array and copies them to commmand structs
// args array
void populate_args(struct command* curr_command, char** inputs) {

    // total number of copy operations = command structs arg_count
    for (int index = 0; index < curr_command->arg_count; index++) {

        // for each entry that will be copied, allocate new memory, and have and
        // element of command.args point to it.
        curr_command->args[index] = calloc(strlen(inputs[index]), sizeof(char));

        // copy string from inputs array to command.args array
        strcpy(curr_command->args[index], inputs[index]);
    }

    // enter NULL as final element (that we care about) in command.args
    // Note: since currently using static rather than dynamic array for .args,
    // there will be many unused elements after this entry. But program knows where
    // to look in this array because command has an n_args member. Previouslly tried
    // using dynamica array for .args, but ran into problems when trying to free memory.
    // May revisit dyanamic array for args in future version.
    curr_command->args[curr_command->arg_count] = NULL;
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
// REFERENCE: The general approach of this function is similar to the lsh_split_line function
// provided by S. Brennan at: https://github.com/brenns10/lsh.
// Use any whitespace as the delimters
char** parse_input_line(char *curr_line, int *n_inputs) {
    int index = 0;
    // Start out with array capable of handling max input size specified in assignment
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

    // set trailing inputs (that we care about) to NULL. This may not be necessary since
    // command.args is what gets passed to execvp (and it is forced to have trailing NULL),
    // but leaving this here to be safe.
    inputs[index] = NULL;

    return inputs;
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
bool bg_command_check(char** inputs, int *n_inputs) {
    if (strcmp(inputs[*n_inputs - 1], BG_FLAG)) {
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

