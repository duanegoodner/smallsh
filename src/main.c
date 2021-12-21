#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include "command.h"
#include "utilities.h"
#include "definitions.h"
#include "globals.h"
#include "built_ins.h"
#include "process_mgmt.h"
#include "signal_handling.h"


int main(void) {

    // run_flag = 1 until built-in exit command is called
    int run_flag = 1;

    // set signal handlers for the shell
    set_shell_sighandlers();

    /* while loop continues prompting and executing commnads until run_flag is
    changed to 0. Non-exit built-in functions as well as calls to launch new
    processes return 1. */
    while (run_flag) {

        // display the prompt
        printf(C_PROMPT);
        fflush(stdout);

        /* Build a command struct based on user input and variable expansion.
        If user enters blank line or comment, NULL is returned. */
        struct command *curr_command = get_command();

        // Check for empty line or comment character
        if (curr_command == NULL) {
            continue;
        }

        //Check if command is a "built-in" (and execute if it is)
        int bltin_index = get_bltin_index(curr_command);
        if (bltin_index >= 0) {

            // cd and status built-ins will return 1. exit will return 0.
            run_flag = (*bltin_funct_ptrs[bltin_index]) (curr_command);
        }
        // If not a built-in command, launch a child process
        else {
            launch_child_proc(curr_command);
        }
     }

    return 0;
}
