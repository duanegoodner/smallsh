#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include "command.h"
#include "utilities.h"
#include "globals.h"
#include "built_ins.h"
#include "process_mgmt.h"
#include "signal_handling.h"






int main(void) {

    int run_flag = 1;
    pid_t shell_pid = getpid();
    char* shell_pid_str = int_to_dynstr(shell_pid);
    char expand_str[] = "$$";          //TO DO: Consider making this global and or a constant

    set_shell_sighandlers();
    
    while (run_flag) {
        
        // TODO: Check for and respond to completed bg processes
        // TODO: Modify SIGSTP handler so that it only changes bool, then put msg function here
        // TODO: Consider placing pointers to any pre-prompt messages in an array?? 
        if (last_fg_terminated) {
            force_report_last_fg_end();
        }
        // if (potential_zombies) {
        //     remove_zombies();
        // }
        printf(C_PROMPT);
        fflush(stdout);  
        struct command *curr_command = get_command(expand_str, shell_pid_str);
        
        // Check if for empty line or comment character
        if (curr_command == NULL || is_comment(curr_command)) {
                free_command(curr_command);
            continue;
        }
              
        //Check if command is a "built-in" (and execute if it is)
        int bltin_index = get_bltin_index(curr_command);
        if (bltin_index >= 0) {
            run_flag = (*bltin_funct_ptrs[bltin_index]) (curr_command);
        } 
        // else if (bg_launch_allowed && curr_command->background) {
        //     run_flag = launch_background(curr_command);
        // }
        // else {
        //     run_flag = launch_foreground(curr_command);
        // }
        else {
            launch_child_proc(curr_command);
        }
     }

    return 0;
}
