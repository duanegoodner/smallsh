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

#define ENTER_FG_ONLY_MSG "\nEntering foreground-only mode (& is now ignored)\n"
#define EXIT_FG_ONLY_MSG "\nExiting foreground-only mode\n"
void handle_SIGTSTP (int signo) {
    //TO DO: modify so handler only changes bool, then use separate function
    //to output messages at start of main while loop.
    bg_launch_allowed = !bg_launch_allowed;
    if (!bg_launch_allowed) {
        write(STDOUT_FILENO, ENTER_FG_ONLY_MSG, 50);
    } else {
        write(STDOUT_FILENO, EXIT_FG_ONLY_MSG, 30);
    }
    write(STDOUT_FILENO, C_PROMPT, 2);       
}

void handle_SIGCHLD (int signo) {
    remove_zombies();
}

void set_shell_sighandlers() {
    struct sigaction ignore_action = {0};
    ignore_action.sa_handler = SIG_IGN;
    sigaction(SIGINT, &ignore_action, NULL);

    struct sigaction SIGTSTP_action = {0};
    SIGTSTP_action.sa_handler = handle_SIGTSTP;
    SIGTSTP_action.sa_flags = SA_RESTART;
    sigaction(SIGTSTP, &SIGTSTP_action, NULL);

    struct sigaction SIGCHLD_action = {0};
    SIGCHLD_action.sa_handler = handle_SIGCHLD;
    SIGCHLD_action.sa_flags = SA_RESTART;
    sigaction(SIGCHLD, &SIGCHLD_action, NULL);
}

void set_fgchild_sighandlers() {
    struct sigaction SIGINT_action = {0};
    SIGINT_action.sa_handler = SIG_DFL;
    SIGINT_action.sa_flags = SA_RESTART; // Is SA_RESTART flag necessary???
    sigaction(SIGINT, &SIGINT_action, NULL);

    struct sigaction ignore_action = {0};
    ignore_action.sa_handler = SIG_IGN;
    sigaction(SIGTSTP, &ignore_action, NULL);
}

void set_bgchild_sighandlers() {
    struct sigaction ignore_action = {0};
    ignore_action.sa_handler = SIG_IGN;
    sigaction(SIGTSTP, &ignore_action, NULL);
}
