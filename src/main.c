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

#define C_PROMPT ": "
#define NEWLINE "\n"
#define NEWLINE_C_PROMPT "\n: "
#define COMMENT_CHAR "#"
#define DEFAULT_DIR "HOME"

#define NO_FG_RUN_YET "No foreground processes run yet, but per instructions, report exit status"
#define LAST_FG_TERMINATED "The last foreground process was terminated by signal"
#define LAST_FG_EXITED "The last foreground process exited normally with exit status"

#define NEWLINE "\n"

// TO DO: add fflush() after every print statement


struct command *bg_list_head = NULL;
struct command *bg_list_tail = NULL;

int last_fg_endsig = 0;
char* last_fg_endmsg = NO_FG_RUN_YET;
bool last_fg_terminated = false;

bool bg_launch_allowed = true;
bool potential_zombies = false;

int cd_bltin(struct command* cd_command);
int status_bltin(struct command* status_command);
int exit_bltin(struct command* exit_command);

char *bltin_funct_names[] = {
    "cd",
    "status",
    "exit"
};

int (*bltin_funct_ptrs[]) (struct command*) = {
    &cd_bltin,
    &status_bltin,
    &exit_bltin
};

int get_num_bltins() {
    return sizeof(bltin_funct_names) / sizeof (char*);
}

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

void killall_bgprocs(void) {
    // TO DO: change var names to be consistent with usage in zombie kill funct
    // Should be OK not doing and free() since program exits immediately after this funct runs
    // Still consider doing a full free (and use part of zombie killing funct)
    while (bg_list_head != NULL) {
        kill(bg_list_head->process_id, SIGKILL);
        bg_list_head = bg_list_head->next;
    }
}

int cd_bltin(struct command* cd_command) {
    
    int chdir_return;
    
    if (cd_command->arg_count == 1) {
        char* default_dir = getenv("HOME");
        chdir_return = chdir(default_dir);
    } else {
        chdir_return = chdir(cd_command->args[1]);
    }

    // for testing only:
    //char* new_dir = getcwd(NULL, 90);

    return 1;
}

int status_bltin(struct command* status_command) {
    
    printf("%s %d\n", last_fg_endmsg, last_fg_endsig);

    return 1;

}

int exit_bltin(struct command* exit_command) {

    killall_bgprocs();

    return 0;
}


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

// declare now since next function needs it.
// TODO: clean up declarations & function organization
void remove_zombies(void);

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

int launch_foreground(struct command* curr_command) {

    int fgchild_status;
    curr_command->process_id = fork();

    if (curr_command->process_id == -1) {
        perror("fork() failed.");
        exit(1);
    
    // child branch
    } else if (curr_command->process_id == 0) {
        
        // add signal handlers
        set_fgchild_sighandlers();
        
        // deal with redirects
        if (curr_command->output_redirect != NULL) {
            redirect_ouptut(curr_command->output_redirect);
        }
        if (curr_command->input_redirect != NULL) {
        redirect_input(curr_command->input_redirect);
        }
              
        // use execv to load and run new program
        execvp(curr_command->args[0], curr_command->args);

        // if execv fails:
        fprintf(stderr, "%s: no such file or directory\n", curr_command->args[0]);
        exit(1);

    // parent branch
    } else {
        curr_command->process_id = waitpid(curr_command->process_id, &fgchild_status, 0);
        if (WIFEXITED(fgchild_status)) {   // consider making status a member of command struct
            last_fg_endmsg = LAST_FG_EXITED;
            last_fg_endsig = WEXITSTATUS(fgchild_status);
        } else {
            last_fg_endmsg = LAST_FG_TERMINATED;
            last_fg_endsig = WTERMSIG(fgchild_status);
            last_fg_terminated = true;
        }
        free_command(curr_command);
    }

    return 1; // need this val because run_flag = 1 causes main while loop to repeat
    // TO DO: consider making run_flag = 0 cause while loop to continue
    // so that everything except for built-in exit can return 0 (more conventional)
}

void force_report_last_fg_end(void) {
    printf("%s %d\n", last_fg_endmsg, last_fg_endsig);
    last_fg_terminated = false; 
}

// struct bgpid_node* create_bg_pidnode(pid_t process_id) {
    
//     struct bgpid_node* new_bg_pid_node = malloc(sizeof(struct bgpid_node));
//     new_bg_pid_node->process_id = process_id;
//     new_bg_pid_node->next = NULL;

//     return new_bg_pid_node;
// }

void add_bg_node(struct command *curr_command) {
// TODO: make generic functions for handling linked lists
    if (bg_list_head == NULL) {
        bg_list_head = curr_command;
        bg_list_tail = curr_command;
    } else {
        bg_list_tail->next = curr_command;
        bg_list_tail = bg_list_tail->next;  // could use new_bg_pid_node
    }
}

void start_tracking_bg(struct command *curr_command) {
    add_bg_node(curr_command);
    printf("background pid is %d\n", curr_command->process_id);
    fflush(stdout);
}

void remove_bgpid_node(struct command* curr_node, struct command* prev_node) {
// consider adding a return value that confirms removal is successful
// consider changing variable name to dead node to match calling function name
    // write(STDOUT_FILENO, "\n1b\n", 4);
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

#define BG_DONE_MSG_START "background pid "
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
            write(STDOUT_FILENO, BG_DONE_MSG_START, 15);
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

#define DEFAULT_BG_REDIRECT "/dev/null" //may need to be char* ?
int launch_background(struct command* curr_command) {
// foreground and background launch codes pretty similar. mayber refactor into one funct?
// keep separate at least until confirmed both work.

    int bgchild_status;
    curr_command->process_id = fork();

    if (curr_command->process_id == -1) {
        perror("fork() failed.");
        exit(1);
    
    //child branch
    } else if (curr_command->process_id == 0) {

        //add signal handlers:
        set_bgchild_sighandlers();

        //set up redirects
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

        //execv call
        execvp(curr_command->args[0], curr_command->args);

        //handle execv failure
        fprintf(stderr, "could not find command %s\n", curr_command->args[0]);
        exit(1);        

    } else {
        start_tracking_bg(curr_command);
    }

    return 1;
}


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
        else if (bg_launch_allowed && curr_command->background) {
            run_flag = launch_background(curr_command);
        }
        else {
            run_flag = launch_foreground(curr_command);
        }
     }

    return 0;
}
