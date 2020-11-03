#include "command.h"
#include "globals.h"


#ifndef WILL_RUN_PROCESSES
#define WILL_RUN_PROCESSES

void killall_bgprocs(void);
void remove_zombies(void);
int redirect_ouptut(char* new_out_path);
void redirect_input(char* new_in_path);
int launch_foreground(struct command* curr_command);
int launch_background(struct command* curr_command);
void force_report_last_fg_end(void);
void add_bg_node(struct command *curr_command);
void start_tracking_bg(struct command *curr_command);
void remove_bgpid_node(struct command* curr_node, struct command* prev_node);
void remove_zombies(void);

#endif