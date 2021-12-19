#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include "command.h"
#include "utilities.h"
#include "globals.h"
#include "built_ins.h"
#include "process_mgmt.h"
#include "signal_handling.h"

#define VAR_EXPAND "$$"


int main(void) {

    // This retains value of 1 until built-in exit command is called.
    int run_flag = 1;

    // get shell pid and convert to a string (for use in variable expansion)
    pid_t shell_pid = getpid();
    char* shell_pid_str = malloc_atoi(shell_pid);

    // set signal handlers for the shell
    set_shell_sighandlers();

    // Shell continues prompting and executing commnads until run_flag becomes 0
    // The non-exit built-in functions as well as calls to launch a child process
    // all return 1. The built-in exit function returns 0
    while (run_flag) {

        // display the prompt
        printf(C_PROMPT);
        fflush(stdout);

        // Build a command struct based on user input and variable expansion.
        // If user enters blank line or comment, NULL is returned.
        // Note: Could provide support for additonal variable expansion by
        // modifying get_command to handle two array args instead of just a
        // pair of strings.
        struct command *curr_command = get_command(VAR_EXPAND, shell_pid_str);

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
