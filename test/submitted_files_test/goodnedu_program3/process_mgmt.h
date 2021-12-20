#include "command.h"


#ifndef WILL_RUN_PROCESSES
#define WILL_RUN_PROCESSES

void killall_bgprocs(void);
void remove_zombies(void);
int redirect_ouptut(char* new_out_path);
void redirect_input(char* new_in_path);
int launch_child_proc(struct command* curr_command);
void set_bg_child_redirect(struct command* curr_command);
void set_fg_child_redirect(struct command* curr_command);
void get_fg_status(int child_status);
void force_report_last_fg_end(void);
void add_bg_node(struct command *curr_command);
void start_tracking_bg(struct command *curr_command);
void remove_bgpid_node(struct command* curr_node, struct command* prev_node);
void remove_zombies(void);

#endif