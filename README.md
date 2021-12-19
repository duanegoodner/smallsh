

# smallsh #

This program runs a small Linux shell (`smallsh`) that implements some of the features found in widely used shell programs such as Bash and Zsh.

## Features ##

* A command prompt
* Handling of blank lines and comments
* Expansion of the variable `$$`
* Execution of built-in `exit`, `cd`, and `status` using code that is part of the shell program.
* Execution of non-built-in commands by forking new process each time a non-built-in command is received. These processes can be run in either the foreground or background.
* Input and output redirection
* Custom signal handlers for SIGINT and SIGTSTP




* A command prompt using the syntax:
`command [arg1 arg2 ...] [< input_file] [output_file] [&]`
* Treats lines prepended with a `#` character as comments and ignores them. Also ignores blank lines.
* Expands the variable `$$` into the shell's process ID
* Executes `exit`, `cd`, and `status` commands in the foreground using code built into the shell
    * `exit` does not take any arguments. When this command is called, the program kills any child processes or jobs it has launched and then terminates itself.
    * `cd` changes the working directory of `smallsh` to a relative or absolute path provided as the argument.
    * `status` does not take any argments. This command prints out the exit status or terminating signal of the most recently run non-built-in foreground process. If the current shell session has not yet run any such processes, exit status 0 is displayed.
* Runs any non-built-in command that is found in the PATH variable or is a valid shell script as a newly forked process.
    * If the final argument of a non-built-in command is `&`, and program is not in foreground-only mode (see below), the command will run  in the background. Otherwise, it will run in the foreground.
    * If the command fails because it is not found, an error message is displayed, and the exit status is set to 1.
*




Features of he user to run any of three built-in commands (status, cd, and exit) as well as any other command found in the . Any other command found int the user's .  Non-built-in command can be run as either foreground or background processes. Built in commands can only run in the foreground. Child processes can run in either the background (using standard '&' at end of command) or background
(if no '&' is used). The program supports input and output redirection
for child processes (but not for built in commands) using the standard
'<' for input redirection and '>' for output redirection. A '#' character
at the start of a line causes that line to be interpreted as a comment.
The program supports a simple variable expansion feature by expanding
the substring '$$' to the shell process id. The program also supports
signal handling features specified in the section titled:
"Assignmet 3: smallsh (Portfolio Assignment)" in the course
website: https://canvas.oregonstate.edu/courses/1784217/modules

The general structure of the main() function follows the approach
described by S. Brennan at the web page "Tutorial - Write a Shell in C"
(Link: https://brennan.io/2015/01/16/write-a-shell-in-c/) and in
the Github repository (also owned by S Brennan) at:
https://github.com/brenns10/lsh. Brennan's clever use of an array
of pointers to built-in functions is also implemented in the current
program.
