

# smallsh #

***A small Linux shell that implements some of the features found in widely used shell programs such as Bash and Zsh.***

<br>

## Features ##

* A command prompt
* Handling of blank lines and comments
* Expansion of the variable `$$`
* Execution of built-in `exit`, `cd`, and `status` using code that is part of the shell program.
* Execution of non-built-in commands by forking new process each time a non-built-in command is received. These processes can be run in either the foreground or background.
* Input and output redirection
* Custom signal handlers for SIGINT and SIGTSTP

## Specifications ##

### 1. Command Line `':'` ###

* Prompt symbol: `:`
* Command syntax: `command [arg1 arg2 ...] [< input_file] [> output_file] [&]`
* If the final argument of a non-built-in command is `&`, and the shell is NOT is foreground-only mode (see section __ below) the command is executed in the background.
* If an argument uses both input and output redirection, `< input_file` and be placed before or after `> output_file`.

### 2. Comments and Blank Lines ###

* Any line that begins with a `#` character is treated as a *comment*. Upon receiving either a comment or a blank line, the shell simply re-prompts for the next command.

### 3. Variable Expansion

* Any instance of `$$` in a command is expanded into the process ID of `smallsh`.

### 4. Built-in Commands

* `exit`: Does not take any arguments. This command kills any existing child processes that the shell has launched and then terminates the `smallsh`.
* `cd`: Changes the working directory of `smallsh`. If no arguments are passed, then the directory is changed to that specified in the HOME environment variable. Cann also take a single argument specifying the absolute or relative path of an existing directory to change to.
* `status`: prints either the exit status or the terminating signal of the last non-built-in foreground process run by the shell. If no such process has run, return exit status of `0`.
* Built-in commands always run in the foreground. If a valid built-in command is appended with `&`, the `&` is ignored, and the command still runs in the foreground.
* NOTE: A built-in command does NOT result in the creation of a child process.

### 5. Non-built-in Commands ###
* Any time a non-built-in command is received, `smallsh` creates a child process using `fork()` and then uses `execvp()` to attempt to change the program of the child process to that correspondig to the command.
* Any valid command that is either in the PATH variable or is a valid shell script will then be run, and the corresponding child process is terminated upon completion of the command's program.
* If the command to run cannot be found, an error message is printed, the child process is terminated, and the exit status is set to 1.

### 6. Input and Output Redirection
 * Any input and/or output redirection specified by a command is performed using `dup2()` prior calling `execvp()`.
 * Any file used for input redirection is opened in read-only mode.
 * Any file use for output redirection is opened in write-only mode. If the output file does not exist, it is created. If it already exists, it is truncated.
 * If an input or output file cannot be opened, an error is printed, and the exit status is set to 1.


 ### 7. Foreground and Background Execution

* All built-in commands as well as all non-built-in commands that do not end with `&` run in the **foreground**. The shell waits for a foreground command to complete before prompting the user for another command.
* Non-built-in commands thate end with `&` and are received with `smallsh` is not in foreground-only mode (see next section), run in the **background**. `smallsh` prompts the user for the next command immediately after forking the child process for a background command.
* The process ID of of a background command is printed when the process begins, and a message showing the process ID and exit status is printed when a background process terminates.
* Any background command without specified input redirection has its input redirected to dev/null. Similarly, any background command without specified output redirection has its output redirected to dev/null.


### 8. Custom Signal Handling ###

**SIGINT**
* SIGINT is ignored by the parent process and any background child processes. However, any foreground child process still follows the default Linux default response and terminates.
* While this behavior is expected from a shell program, without the custom handling that is implemented in **smallsh**, SIGINT would cause **smallsh** and ALL of its child processes to terminate since it runs as a foreground process in the shell from which it is launched.


**SIGTSTP**
* SIGSTP  causes the program to toggle in and out of *foreground-only mode*.
* The first time SIGTSTP is sent to an instance of **smallsh**, if no foreground process is running, the shell immediately displays the message `Entering foreground-only mode (& is now ignored)` and enters a state that does not allow new processes to be launched in the background. If a foreground process is running when the signal is sent, the message display and state change occur immediately after that foreground process completes.
* A subsequent SIGTSTP signal causes the shell to display the message `Exiting foreground-only mode` and return to a state that allows non-built-in processes to be launched in the background.
* All child processes (foreground and background) ignore SIGTSTP.






### Referenes ###


* [Write a Shell in C](https://brennan.io/2015/01/16/write-a-shell-in-c/)



