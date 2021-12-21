#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include "command.h"
#include "built_ins.h"
#include "definitions.h"
#include "globals.h"


// global variable that is an array for storing pointers to strings with names of
// the built-in functions
// REFERENCE: Following approach provided at:
// https://github.com/brenns10/lsh
char *bltin_funct_names[] = {
    "cd",
    "status",
    "exit"
};

// global variable that stores pointers to built in functions
// REFERENCE: Following approach provided at:
// https://github.com/brenns10/lsh
int (*bltin_funct_ptrs[]) (struct command*) = {
    &cd_bltin,
    &status_bltin,
    &exit_bltin
};

// built in cd function. changes current directory.
// if no directory specified, changes to HOME
int cd_bltin(struct command* cd_command) {

    int chdir_return;
    if (cd_command->arg_count == 1) {
        char* default_dir = getenv(DEFAULT_DIR);
        chdir_return = chdir(default_dir);
    } else {
        chdir_return = chdir(cd_command->args[1]);
    }
    return 1;
}

// built in status function. reports exit value / termination status of
// last foreground process
int status_bltin(struct command* status_command) {
    printf("%s %d\n", last_fg_endmsg, last_fg_endsig);
    return 1;
}

// built in exit function. kills any active background processes then returns
// value of 0 to main(). This will cause the while loop in main() to exit.
// REFERENCE: Following approach provided at:
// https://github.com/brenns10/lsh
int exit_bltin(struct command* exit_command) {
    while (bg_list_head != NULL) {
        kill(bg_list_head->process_id, SIGKILL);
        bg_list_head = bg_list_head->next;
    }
    return 0;
}

// returns number of built in functions.
// REFERENCE: Following approach provided at:
// https://github.com/brenns10/lsh
int get_num_bltins() {
    return sizeof(bltin_funct_names) / sizeof (char*);
}

// Checks to see if a string corresponds to a built in command.
// If it does, returns index of that command in name and function
// pointer arrays. If not present, returns -1;
// Note: This approach was NOT used by S. Brennan at https://github.com/brenns10/lsh
int get_bltin_index(struct command* curr_command) {
    int bltin_index = -1;
    int num_bltins = get_num_bltins();
    for (int index = 0; index < num_bltins; index++) {
        int comparison = strcmp(curr_command->args[0], bltin_funct_names[index]);
        if (comparison == 0) {
            bltin_index = index;
        }
    }
    return bltin_index;
}

