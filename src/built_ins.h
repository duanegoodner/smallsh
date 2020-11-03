
#include "globals.h"

#ifndef HAVE_ANY_BUILTIN_FUNCTS
#define HAVE_ANY_BUILTIN_FUNCTS

int cd_bltin(struct command* cd_command);
int status_bltin(struct command* status_command);
int exit_bltin(struct command* exit_command);

int get_num_bltins();
int get_bltin_index(struct command* curr_command);

#endif