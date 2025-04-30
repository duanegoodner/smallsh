#define _POSIX_C_SOURCE 200809L     // Enables full POSIX definitions

#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>       // for memset

#include "globals.h"
#include "definitions.h"
#include "process_mgmt.h"
#include "signal_handling.h"


// handle SIGSTP by toggling global bg_launch_allowed on/off and reporting the change
// to standard out.
void handle_SIGTSTP (int signo) {

    (void)signo;

    bg_launch_allowed = !bg_launch_allowed;
    if (!bg_launch_allowed) {
        write(STDOUT_FILENO, ENTER_FG_ONLY_MSG, 50);
    } else {
        write(STDOUT_FILENO, EXIT_FG_ONLY_MSG, 30);
    }
    write(STDOUT_FILENO, C_PROMPT, 2);
}

// handles SIGCHLD by checking for and removing zombie processes
void handle_SIGCHLD (int signo) {
    (void)signo;
    remove_zombies();
}

// sets signal handlers for the shell process
void set_shell_sighandlers() {

    //ignore SIGINT
    struct sigaction ignore_action;
    memset(&ignore_action, 0, sizeof(ignore_action));
    ignore_action.sa_handler = SIG_IGN;
    sigaction(SIGINT, &ignore_action, NULL);

    // Register SIGSTP handler
    struct sigaction SIGTSTP_action;
    memset(&SIGTSTP_action, 0, sizeof(SIGTSTP_action));
    SIGTSTP_action.sa_handler = handle_SIGTSTP;
    SIGTSTP_action.sa_flags = SA_RESTART;
    sigaction(SIGTSTP, &SIGTSTP_action, NULL);

    // register SIGCHLD handler
    struct sigaction SIGCHLD_action;
    memset(&SIGCHLD_action, 0, sizeof(SIGCHLD_action));
    SIGCHLD_action.sa_handler = handle_SIGCHLD;
    SIGCHLD_action.sa_flags = SA_RESTART;
    sigaction(SIGCHLD, &SIGCHLD_action, NULL);
}

// sets sig handlers for foreground child process
void set_fgchild_sighandlers() {
    // register SIGINT to follow default action
    struct sigaction SIGINT_action;
    memset(&SIGINT_action, 0, sizeof(SIGINT_action));
    SIGINT_action.sa_handler = SIG_DFL;
    SIGINT_action.sa_flags = SA_RESTART; // Is SA_RESTART flag necessary???
    sigaction(SIGINT, &SIGINT_action, NULL);

    // register SIGTSTP to be ignored
    struct sigaction ignore_action;
    memset(&ignore_action, 0, sizeof(ignore_action));
    ignore_action.sa_handler = SIG_IGN;
    sigaction(SIGTSTP, &ignore_action, NULL);
}

// sets signal handlers for background child process
void set_bgchild_sighandlers() {
    // register SIGTSTP to be ignored
    struct sigaction ignore_action;
    memset(&ignore_action, 0, sizeof(ignore_action));
    ignore_action.sa_handler = SIG_IGN;
    sigaction(SIGTSTP, &ignore_action, NULL);
}