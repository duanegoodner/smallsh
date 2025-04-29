# smallsh

*A lightweight Linux shell implemented in C, mimicking essential features of Bash and Zsh.*

---

## About

**smallsh** is a custom-built shell created for an undergraduate Operating Systems course. It supports core functionality like command execution, input/output redirection, background processes, and custom signal handling. The goal was to closely follow the behavior of typical Unix shells while managing processes and signals explicitly.


## Features

- Custom command prompt (`:`)
- Built-in commands: `exit`, `cd`, `status`
- Execution of non-built-in commands
- Foreground and background process management
- Input (`<`) and output (`>`) redirection
- Variable expansion for `$$` (current shell PID)
- Custom handling of `SIGINT` (Ctrl+C) and `SIGTSTP` (Ctrl+Z)
- Foreground-only mode toggle


## Getting Started

### Prerequisites
- GCC or any C compiler
- Linux-based environment (Ubuntu, Fedora, etc.)

### Building the Shell

Clone the repository and build the project using `make`:

```bash
make
```

This will compile the source files and produce the `smallsh` executable.


## Usage

Start the shell:

```bash
./smallsh
```

At the `:` prompt, enter commands in the following format:

```bash
command [arg1 arg2 ...] [< input_file] [> output_file] [&]
```

Example:

```bash
ls -la > output.txt &
```

Use `exit` to terminate the shell.


## Technical Details

### Commands
- **Built-ins**:
  - `exit`: Terminates the shell, killing any background processes.
  - `cd [directory]`: Changes the current working directory. Defaults to `$HOME` if no directory is given.
  - `status`: Prints the exit status or terminating signal of the last foreground process.

- **Non-built-ins**:
  - Forks a new child process.
  - Executes via `execvp()`.
  - Errors (like command not found) are reported, and the exit status is set accordingly.

### Input/Output Redirection
- `< input_file` redirects stdin.
- `> output_file` redirects stdout.
- Background processes default to `/dev/null` if no redirection is specified.

### Background Processes
- Appending `&` at the end of a command runs it in the background.
- Background process PID is displayed immediately.
- Completion and exit status of background processes are reported asynchronously.

### Variable Expansion
- `$$` is replaced by the process ID (PID) of the running `smallsh` instance.


## Signal Handling

### `SIGINT` (Ctrl+C)
- Ignored by the shell itself and background processes.
- Foreground child processes terminate normally upon receiving SIGINT.

### `SIGTSTP` (Ctrl+Z)
- Toggles **foreground-only mode**:
  - In foreground-only mode, background execution requests (`&`) are ignored.
  - Toggle messages are displayed accordingly.
- Shell and all child processes ignore SIGTSTP unless intended for toggling.


## References
- [Write a Shell in C](https://brennan.io/2015/01/16/write-a-shell-in-c/)

---

## License

This project is open for educational use. Feel free to explore and adapt it for learning purposes.

