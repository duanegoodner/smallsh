#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include "command.h"
#include "utilities.h"
#include "globals.h"
#include "process_mgmt.h"
#include "signal_handling.h"

#define NO_FG_RUN_YET "exit value"
#define LAST_FG_TERMINATED "The last foreground process was terminated by signal"
#define LAST_FG_EXITED "exit value"

// initialize linked list where commands that get launched as background processes are stored
struct command *bg_list_head = NULL;
struct command *bg_list_tail = NULL;

// initalize variables for status of the last foreground process
int last_fg_endsig = 0;
char* last_fg_endmsg = NO_FG_RUN_YET;
bool last_fg_terminated = false;

// intialize variable that will allow/prevent a trailing '&' to actuallly launch a bg process
bool bg_launch_allowed = true;

// keeps track of whether or not SIGCHILD signal has been received, which could indicat
// there are zombie processes (though SIGCHILD could be from foregrond???)
// in current implementation, this bool variable may not be necessary. consider deleting.
bool potential_zombies = false;

// sets the output redirect path of the process it is called within
// REFERENCE: Used code provided in course module: https://canvas.oregonstate.edu/courses/1784217/modules/items/19893106
int redirect_ouptut(char* new_out_path) {
    int out_fd = open(new_out_path, O_WRONLY | O_CREAT | O_TRUNC, 0640);
    if (out_fd == -1) {
        fprintf(stderr, "Failed open %s for output", new_out_path);
        exit(1);
    }
    int dup2_result = dup2(out_fd, 1);
    if (dup2_result == -1) {
        perror("output dup2 error");
        exit(2);
    }
}

// sets the output redirect path of the process from within which its called
// REFERENCE: Used code provided in course module:
// https://canvas.oregonstate.edu/courses/1784217/modules/items/19893106
void redirect_input(char* new_in_path) {
    int in_fd = open(new_in_path, O_RDONLY);
    if (in_fd == -1) {
        fprintf(stderr, "cannot open %s for input\n", new_in_path);
        fflush(stdout);
        exit(1);
    }

    int dup2_result = dup2(in_fd, 0);
    if (dup2_result == -1) {
        fprintf(stderr, "input dup2 error");
    }
}

// checks the exit status of a child process. in current implementation only used to check foregound processes
// REFERENCE: Used code provided in course module:
// https://canvas.oregonstate.edu/courses/1784217/modules/items/19893096
void get_fg_status(int child_status) {
    if (WIFEXITED(child_status)) {   // consider making status a member of command struct
            last_fg_endmsg = LAST_FG_EXITED;
            last_fg_endsig = WEXITSTATUS(child_status);
        } else {
            last_fg_endmsg = LAST_FG_TERMINATED;
            last_fg_endsig = WTERMSIG(child_status);
            last_fg_terminated = true;
        }
}

// Prnigs exit/termination status of last foreground process and resets global termination var to false.
void force_report_last_fg_end(void) {
    printf("%s %d\n", last_fg_endmsg, last_fg_endsig);
    last_fg_terminated = false; 
}

// adds command struct to linked list that stores commands running in background
void add_bg_node(struct command *curr_command) {
    if (bg_list_head == NULL) {
        bg_list_head = curr_command;
        bg_list_tail = curr_command;
    } else {
        bg_list_tail->next = curr_command;
        bg_list_tail = bg_list_tail->next; 
    }
}

// calls function to add command to background process to bg_list
// and reports process id to standard out.
void start_tracking_bg(struct command *curr_command) {
    add_bg_node(curr_command);
    printf("background pid is %d\n", curr_command->process_id);
    fflush(stdout);
}

// removes a command structure for the linked list where commands corresponding
// to active background processes are stored.
void remove_bgpid_node(struct command* curr_node, struct command* prev_node) {
// consider adding a return value that confirms removal is successful
    if (curr_node == bg_list_head && curr_node == bg_list_tail) {
        bg_list_head = NULL;
        bg_list_tail = NULL;
    } else if (curr_node == bg_list_head) {
        bg_list_head = curr_node->next;
        curr_node->next = NULL; // unnecessary???
    } else {
        prev_node->next = curr_node->next;
        curr_node->next = NULL; // unnecessary???
        if (curr_node == bg_list_head) {
            bg_list_tail = prev_node;
        } 
    }
    free_command(curr_node);
}

#define BG_DONE_MSG_START "\nbackground pid "
#define BG_DONE_MSG_END " is done: "
#define BG_EXIT_MSG "exit value "
#define BG_TERM_MSG "terminated by signal "
// called during iteration through bg process LL. checks status of each node 
// in list, and if the process corresponding to that node has ended, kills process
// and removes node from the list
void remove_zombies(void) {
    int bgchild_status;

    struct command* curr_node = bg_list_head;
    struct command* prev_node = NULL;
    struct command* dead_node = NULL;

    while (curr_node != NULL) {
        // first check if process is done
        if (waitpid(curr_node->process_id, &bgchild_status, WNOHANG)) {
            char* process_id_str = malloc_atoi(curr_node->process_id);
            int process_id_str_len = strlen_int(curr_node->process_id);
            
            // if it is done report to standard out.
            write(STDOUT_FILENO, BG_DONE_MSG_START, 16);
            write(STDOUT_FILENO, process_id_str, process_id_str_len);
            write(STDOUT_FILENO, BG_DONE_MSG_END, 10);
            free(process_id_str);
            
            // then check if exited normally
            if (WIFEXITED(bgchild_status)) {
                int exit_status = WEXITSTATUS(bgchild_status);
                char* exit_status_str = malloc_atoi(exit_status);
                int exit_status_str_len = strlen_int(exit_status);

                // if exited normally report this along with exit status
                write(STDOUT_FILENO, BG_EXIT_MSG, 11);
                write(STDOUT_FILENO, exit_status_str, exit_status_str_len);
                write(STDOUT_FILENO, NEWLINE_C_PROMPT, 3);
                free(exit_status_str);

            // if process did not exit normally, get termination status 
            } else {
                int term_signal = WTERMSIG(bgchild_status);
                char* term_signal_str = malloc_atoi(term_signal);
                int term_signal_str_len = strlen_int(term_signal);
                
                // report termination status to standard out
                write(STDOUT_FILENO, BG_TERM_MSG, 21);
                write(STDOUT_FILENO, term_signal_str, term_signal_str_len);
                write(STDOUT_FILENO, NEWLINE, 1);
                free(term_signal_str);
            }
            // kill proces, and remove from linked list where background processes are stored
            kill(curr_node->process_id, SIGKILL);
            dead_node = curr_node;
            curr_node = curr_node->next;
            remove_bgpid_node(dead_node, prev_node);  // this function calls free(dead_node)
            
            // if we did kill a process, we're already at next node to check, so no need to advance
            // call command that advances to next node
            continue;
        }

        // advance to next node
        curr_node = curr_node->next;
    }
    // any zombies that were in linked list have now been removed
    potential_zombies = false;  // TBD if actually need this variable.
}

// set background process input/output redirect. if no path specified, then set to /dev/null
#define DEFAULT_BG_REDIRECT "/dev/null"
void set_bgchild_redirect(struct command *curr_command) {
    if (curr_command->output_redirect == NULL) {
            redirect_ouptut(DEFAULT_BG_REDIRECT);
        } else {
            redirect_ouptut(curr_command->output_redirect);
        }
        if (curr_command->input_redirect == NULL) {
            redirect_input(DEFAULT_BG_REDIRECT);
        } else {
            redirect_input(curr_command->input_redirect);
        }
}

// sest foreground child process redirect if corresponding members of command struct not NULL
void set_fgchild_redirect(struct command *curr_command) {
    if (curr_command->output_redirect != NULL) {
            redirect_ouptut(curr_command->output_redirect);
        }
    if (curr_command->input_redirect != NULL) {
        redirect_input(curr_command->input_redirect);
        }
}

// Launches a child process. 
// REFERENCE: Structure of this function borrows heavily from code provided in course module:
// https://canvas.oregonstate.edu/courses/1784217/modules/items/19893097
int launch_child_proc(struct command* curr_command) {
    int child_status;
    curr_command->process_id = fork();

    // handle error
    if (curr_command->process_id == -1) {
        perror("fork() failed.");
        exit(1);
    } else if (curr_command->process_id == 0) {
        
        // child branch
        // set sig handlers
        if (bg_launch_allowed && curr_command->background) {
            set_bgchild_sighandlers();
            set_bgchild_redirect(curr_command);
        } else {
            set_fgchild_sighandlers();
            set_fgchild_redirect(curr_command);
        }
        
        // call execvp using command struct's args array
        execvp(curr_command->args[0], curr_command->args);
        
        // handle error if new program did not load/start
        fprintf(stderr, "%s: no such file or directory\n", curr_command->args[0]);
        exit(1);

    } else {
        // if process was launched as a background process, start tracking it
        if (bg_launch_allowed && curr_command->background) {
            start_tracking_bg(curr_command);
        
        } else {
            // if process was launched in foreground, wait for it, and report status if
            // terminates (rather than exits)
            curr_command->process_id = waitpid(curr_command->process_id, &child_status, 0);
            get_fg_status(child_status);
            if (last_fg_terminated) {
                force_report_last_fg_end();
            }

            // free memory immediately if process was a foreground process (which is now done) 
            free_command(curr_command);
        }
    }

    // return 1 so main while loop continues 
    return 1;

}