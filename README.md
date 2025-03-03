# Custom Shell Program

This is a custom shell program written in C that allows users to execute commands, navigate directories, handle basic built-in commands (such as `exit`, `quit`, and `cd`), and supports output redirection to files. It also allows batch mode for reading commands from a file.

## Features

- **Interactive Mode**: Prompts users with `msh>` to enter commands.
- **Built-in Commands**:
  - `exit` or `quit`: Exits the shell program.
  - `cd <directory>`: Changes the current working directory.
- **Output Redirection**: Allows redirection of command output to a file using the `>` operator.
- **Batch Mode**: If a filename is provided as an argument, the shell reads commands from that file instead of standard input.
- **Command Searching**: The shell searches for commands in the directories `/bin/`, `/usr/bin/`, `/usr/local/bin/`, and the current directory (`./`).
- **Error Handling**: Includes basic error handling for invalid commands, output redirection errors, and file access issues.

## Compilation

To compile the program, use the following command:

```bash
make
