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
#include "process_mgmt.h"
#include "signal_handling.h"

#define NO_FG_RUN_YET "No foreground processes run yet, but per instructions, report exit value"
#define LAST_FG_TERMINATED "The last foreground process was terminated by signal"
#define LAST_FG_EXITED "The last foreground process exited normally with exit value"

struct command *bg_list_head = NULL;
struct command *bg_list_tail = NULL;

int last_fg_endsig = 0;
char* last_fg_endmsg = NO_FG_RUN_YET;
bool last_fg_terminated = false;

bool bg_launch_allowed = true;
bool potential_zombies = false;


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




void force_report_last_fg_end(void) {
    printf("%s %d\n", last_fg_endmsg, last_fg_endsig);
    last_fg_terminated = false; 
}

void add_bg_node(struct command *curr_command) {
// TODO: make generic functions for handling linked lists
    if (bg_list_head == NULL) {
        bg_list_head = curr_command;
        bg_list_tail = curr_command;
    } else {
        bg_list_tail->next = curr_command;
        bg_list_tail = bg_list_tail->next; 
    }
}

void start_tracking_bg(struct command *curr_command) {
    add_bg_node(curr_command);
    printf("background pid is %d\n", curr_command->process_id);
    fflush(stdout);
}

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
void remove_zombies(void) {
    int bgchild_status;

    struct command* curr_node = bg_list_head;
    struct command* prev_node = NULL;
    struct command* dead_node = NULL;

    while (curr_node != NULL) {
        if (waitpid(curr_node->process_id, &bgchild_status, WNOHANG)) {
            char* process_id_str = malloc_atoi(curr_node->process_id);
            int process_id_str_len = strlen_int(curr_node->process_id);
            write(STDOUT_FILENO, BG_DONE_MSG_START, 16);
            write(STDOUT_FILENO, process_id_str, process_id_str_len);
            write(STDOUT_FILENO, BG_DONE_MSG_END, 10);
            free(process_id_str);
            if (WIFEXITED(bgchild_status)) {
                int exit_status = WEXITSTATUS(bgchild_status);
                char* exit_status_str = malloc_atoi(exit_status);
                int exit_status_str_len = strlen_int(exit_status);
                write(STDOUT_FILENO, BG_EXIT_MSG, 11);
                write(STDOUT_FILENO, exit_status_str, exit_status_str_len);
                write(STDOUT_FILENO, NEWLINE_C_PROMPT, 3);
                free(exit_status_str);
            } else {
                int term_signal = WTERMSIG(bgchild_status);
                char* term_signal_str = malloc_atoi(term_signal);
                int term_signal_str_len = strlen_int(term_signal);
                write(STDOUT_FILENO, BG_TERM_MSG, 21);
                write(STDOUT_FILENO, term_signal_str, term_signal_str_len);
                write(STDOUT_FILENO, NEWLINE, 1);
                free(term_signal_str);
            }
            kill(curr_node->process_id, SIGKILL);
            dead_node = curr_node;
            curr_node = curr_node->next;
            remove_bgpid_node(dead_node, prev_node);  // this function calls free(dead_node)
            continue; // already advanced curr_node
        }
        curr_node = curr_node->next;
    }

    potential_zombies = false;  // TBD if this'll be used. If decide no, then delete.
}

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

void set_fgchild_redirect(struct command *curr_command) {
    if (curr_command->output_redirect != NULL) {
            redirect_ouptut(curr_command->output_redirect);
        }
    if (curr_command->input_redirect != NULL) {
        redirect_input(curr_command->input_redirect);
        }
}

int launch_child_proc(struct command* curr_command) {
    int child_status;
    curr_command->process_id = fork();

    if (curr_command->process_id == -1) {
        perror("fork() failed.");
        exit(1);
    } else if (curr_command->process_id == 0) {
        if (bg_launch_allowed && curr_command->background) {
            set_bgchild_sighandlers();
            set_bgchild_redirect(curr_command);
        } else {
            set_fgchild_sighandlers();
            set_fgchild_redirect(curr_command);
        }

        execvp(curr_command->args[0], curr_command->args);

        fprintf(stderr, "%s: no such file or directory\n", curr_command->args[0]);
        exit(1);        
    } else {
        if (bg_launch_allowed && curr_command->background) {
            start_tracking_bg(curr_command);
        } else {
            curr_command->process_id = waitpid(curr_command->process_id, &child_status, 0);
            get_fg_status(child_status);
            free_command(curr_command);

        }
    }
    
    return 1;

}