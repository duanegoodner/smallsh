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

    int run_flag = 1;
    pid_t shell_pid = getpid();
    char* shell_pid_str = malloc_atoi(shell_pid);

    set_shell_sighandlers();
    
    while (run_flag) {
           
        printf(C_PROMPT);
        fflush(stdout);  
        struct command *curr_command = get_command(VAR_EXPAND, shell_pid_str);
        
        // Check for empty line or comment character
        if (curr_command == NULL) {
            continue;
        }  
        //Check if command is a "built-in" (and execute if it is)
        int bltin_index = get_bltin_index(curr_command);
        if (bltin_index >= 0) {
            run_flag = (*bltin_funct_ptrs[bltin_index]) (curr_command);
        } 
        else {
            launch_child_proc(curr_command);
        }
     }
    return 0;
}
