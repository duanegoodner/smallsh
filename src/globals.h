
#include "command.h"

#define C_PROMPT ": "
#define NEWLINE "\n"
#define NEWLINE_C_PROMPT "\n: "
#define COMMENT_CHAR "#"
#define DEFAULT_DIR "HOME"

#ifndef HAVE_GLOBAL_VARS
#define HAVE_GOBAL_VARS

extern char *bltin_funct_names[];
extern int (*bltin_funct_ptrs[]) (struct command*);

extern struct command *bg_list_head;
extern struct command *bg_list_tail;

extern int last_fg_endsig;
extern char* last_fg_endmsg;
extern bool last_fg_terminated;

extern bool bg_launch_allowed;
extern bool potential_zombies;


#endif