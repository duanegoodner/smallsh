#define _POSIX_C_SOURCE 200809L     // Enables full POSIX definitions

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "definitions.h"
#include "globals.h"
#include "command.h"
#include "utilities.h"

/*
    FUNCTION: get_command
    @desc: gets input from command line & converts to command struct
    @returns: pointer to a command struct in heap
 */
struct command* get_command(void) {

    struct input* curr_input = malloc(sizeof(struct input));
    struct command *curr_command = malloc(sizeof(struct command));

    char* curr_line = get_input_line();

    // convert raw line of input to an input struct in heap
    populate_input_struct(curr_line, curr_input);

    // if input is blank line or comment, command is NULL
    if (curr_input->num_words == 0 || (is_comment(curr_input->parsed_words))) {
        curr_command = NULL;
        return curr_command;
    // if not a blank line or command, populate command struct data members
    } else {
        populate_command(curr_command, curr_input);
    }

    // free all heap items except command struct
    free(curr_line);
    free(curr_input->parsed_words);
    free(curr_input);

    return curr_command;
}

/*
    FUNCTION: populate_input_struct
    @desc: uses info in curr_line string to populate an input struct
    @param: curr_line = pointer to string entered at command line
    @param: curr_input = pointer to input struct to be populated
    @returns: none
    @unfreed heap items: curr_input.parsed_words
 */
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

    // reduce size of curr_input.parsed words down to what's used
    curr_input->parsed_words = realloc(
        curr_input->parsed_words, sizeof(
            curr_input->parsed_words) * (word_index + 1));

    // having a NULL element at end helps with debugging and/or execvp later
    curr_input->parsed_words[word_index] = NULL;
}

/*
    FUNCTION: get_input_line
    @desc: reads line of input entered at command line, and saves in heap
    returns pointer.
    @returns: curr_line = pointer to saved string in heap.
    @unfreed heap items: curr_line
 */
char* get_input_line(void) {
    char *curr_line = NULL;
    size_t len = 0;
    ssize_t nread;
    nread = getline(&curr_line, &len, stdin);

    if (nread == -1) {
        perror("getline");
        free(curr_line); // avoid leak on error
        return NULL;
    }
    
    return curr_line;
}

/*
    FUNCTION: populate_command
    @desc: populates command struct with info from input struct. new copies of
    heap items in curr_input are created (so generally OK to free curr_input
    after calling).
    @param: curr_command = pointer to command struct to be populated
    @param: curr_input = pointer to input struct providing the info
    @returns: none
    @possible (new) unfreed heap items: curr_command.args,
    curr_command.input_redirect, curr_command.output_redirect
 */
void populate_command(struct command* curr_command, struct input* curr_input) {

    // index_limit determines how many items get copied into command args
    int index_limit = curr_input->num_words;

    // background commands get saved in linked list, so need a .next member
    curr_command->next = NULL;

    // initialize pid to value that can't be confused with real pid
    curr_command->process_id = -5;

    // these get updated later if needed. initializing to NULL to avoid double free
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

/*
    FUNCTION: populate_args
    @desc: uses arg_count to determine which items in parsed_words are args,
    and copies them to args.
    @param: arg_count = number of args in the command (does not include redirect
    or background syntax)
    @args: pointer to array of string pointers for command args
    @parsed_words: pointer to array of string pointers (most of, but not all,
    of which are command args)
    @new unfreed head items: each element of args points to newly allocated mem
 */
void populate_args(int arg_count, char** args, char** parsed_words) {

    // total number of copy operations = command structs arg_count
    for (int copy_index = 0; copy_index < arg_count; copy_index++) {

        // for each entry to be copied, allocate new memory
        args[copy_index] = calloc(strlen(parsed_words[copy_index]), sizeof(char));

        // copy string from inputs array to command.args array
        strcpy(args[copy_index], parsed_words[copy_index]);
    }

    // enter NULL as final (used) element in command.args for use with execvp
    args[arg_count] = NULL;
}

/*
    FUNCTION: get_argc_and_redirs
    @desc: Counts number of elements in inputs array that need to be copied
    into args, and gets filenames of input/output redirects if specified. These
    pieces of info are entered in corresponding members of command struct. Combining
    arc counting with getting redirect seems a bit messy, but doing one automatically
    gives the other.
    @param: curr_command = pointer to command structure being populated
    @param: parsed_words = pointer to array of words (most but not all of which
    are command args)
 */
void get_argc_and_redirs(struct command* curr_command, char** parsed_words, int index_limit) {

    // initialize arg counter to zero
    int arg_count = 0;

    for (int index = 0; index < index_limit; index++) {
        // If find redirect symbol, set next element as input & advance counter extra unit
        if (is_redirect_in(parsed_words[index])) {
            curr_command->input_redirect = calloc(strlen(parsed_words[index + 1]), sizeof(char));
            strcpy(curr_command->input_redirect, parsed_words[index + 1]);
            index++;
        // Analogous procedure if we find and output redirect symbol
        } else if (is_redirect_out(parsed_words[index])) {
            curr_command->output_redirect = calloc(strlen(parsed_words[index + 1]), sizeof(char));
            strcpy(curr_command->output_redirect, parsed_words[index + 1]);
            index++;
        // If current element of inputs array is not for redirection, it's  an arg
        } else {
            arg_count++;
        }
    }
    curr_command->arg_count = arg_count;
}

/*
    @desc: Takes a populated command structure and modifies elements of args as needed
    based on a variable expansion rule.
    @param: curr_command = pointer to command struct that may need variable expansion
    @param: old_str = string pattern that gets replaced
    @param: new_str = string pattern thta replaces the old one

 */
void expand_var(struct command* curr_command, char* old_str, char* new_str) {

    if (curr_command != NULL) {
        for (int arg_index = 0; arg_index < curr_command->arg_count; arg_index++) {

            // check if old_str is a substring of arg element
            char* ss_ptr = strstr(curr_command->args[arg_index], old_str);
            // if match if found, execute the replacement
            if (ss_ptr != NULL){
                curr_command->args[arg_index] = dsubstr_replace_all(curr_command->args[arg_index], old_str, new_str);
            }
        }
    }
}

/*
    FUNCTION: is_redirect_out
    @desc: checks string is an output redirect symbol
    @param: word = pointer to the string being checked
    @returns: true if word is a redirect, false otherwise
 */
bool is_redirect_out(char* word) {
    if (strcmp(word, REDIRECT_OUT)) {
        return false;
    } else {
        return true;
    }
}

/*
    FUNCTION: is_redirect_in
    @desc: checks string is an input redirect symbol
    @param: word = pointer to the string being checked
    @returns: true if word is an output redirect, false otherwise
 */
bool is_redirect_in(char* word) {
    if (strcmp(word, REDIRECT_IN)) {
        return false;
    } else {
        return true;
    }
}

/*
    FUNCTION: bg_command_check
    @desc: checks if final element in array of strings is the background
    process symbol.
    @param: inputs = pointer to array of string pointers being checked
    @returns: true if final element is the background symbol, false otherwise
 */
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

/*
    FUNCTION: free_command
    @desc: Frees a command structure from heap memory. First frees memory
    pointed to by data members, then frees struct itself. Note that some
    members are not heap pointers so don't require free. Use NULL guards to
    avoid double-free(). Called in process_mgmt module.
    @param: curr_command = pointer to command struct being freed
*/
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

/*
    FUNCTION: is_comment
    @desc: checks if first character in array of strings is a comment char.
    @param: inputs = pointer to array of string pointers being checked
    @returns: true if first char is comment char, false otherwise
 */
bool is_comment(char **inputs) {
    if (inputs[0][0] == COMMENT_CHAR) {
        return true;
    } else {
        return false;
    }
}

