
#ifndef DEALING_WITH_SIGNALS
#define DEALING_WITH_SIGNALS

void handle_SIGTSTP (int signo);
void handle_SIGCHLD (int signo);
void set_shell_sighandlers(void);
void set_fgchild_sighandlers(void);
void set_bgchild_sighandlers(void);


#endif