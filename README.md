# Linux Shell with Job Control, Pipes, and History

This program is a Unix/Linux shell that supports job control, pipes, and a history mechanism. It is an extended version of a basic shell, capable of executing commands, managing processes, handling pipelines, and maintaining a history of commands.

## Features
- **Basic Shell Functionality**: Prompt, read command, parse input, and execute commands.
- **Job Control**: Fork processes, manage background and foreground processes.
- **Redirection**: Support for standard input/output redirection.
- **Signals**: Handle signals such as `SIGTSTP`, `SIGINT`, and `SIGCONT`.
- **Pipes**: Implement pipelines for chaining commands.
- **Process Manager**: Manage running and suspended processes.
- **History Mechanism**: Store and retrieve previous command lines.

## Getting Started
1. **Compilation**: Compile the shell program using the provided makefile.
   ```sh
   make myshell
   ```
## Usage
Enter commands at the prompt and observe the shell's behavior. Use the following commands and features to interact with the shell:

- `cd <directory>`: Change the current working directory.
- `quit`: Exit the shell.
- `procs`: List all running and suspended processes.
- `suspend <process_id>`: Suspend a running process.
- `wake <process_id>`: Wake up a sleeping process.
- `kill <process_id>`: Terminate a running or sleeping process.
- `history`: Display the command history.
- `!!`: Repeat the last command.
- `!n`: Repeat the command at index `n` in the history.

## Examples
- Running a command in the background:
  ```sh
  sleep 10 &
   ```
## Redirecting Input/Output
You can redirect the input and output of commands using the following symbols:

- `<`: Redirects the standard input of a command.
  ```sh
  cat < file.txt
   ```
## Redirection
You can redirect the standard input, output, and error of commands using the following symbols:

- `<`: Redirects the standard input of a command.
  ```sh
  cat < file.txt
   ```
- `>`: Redirects the standard output of a command, overwriting the existing content of the output file.
  ```sh
  ls > files.txt
   ```
- `>>`: Redirects the standard output of a command, appending to the existing content of the output file.
  ```sh
  echo "new line" >> file.txt
   ```
- `2>`: Redirects the standard error output of a command.
   ```sh
  command_that_errors 2> error.log
   ```
### Enjoy!
