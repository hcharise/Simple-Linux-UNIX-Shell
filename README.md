# Simple Linux/UNIX Shell
An interactive shell in C that mirrors a Linux/UNIX shell, accepting and executing fundamental commands. Enhances the user's ability to interact with the operating system in a command-line experience, enabling efficient and precise control over system resources, file structures, and processes.

Project was completed in June 2023 for Syracuse CPS 500 Computer Organization and Operating Systems with Professor William Katsak. Makefile, argset.c, and argset.h provided by professor, as well as two sections in the simplesh.c file (indicated by comments).

<img width="571" alt="image" src="https://github.com/hcharise/Simple-Linux-UNIX-Shell/assets/110205350/c64cc26c-f853-4227-84bb-533d2769bde8">

# Overall Structure
The main function repeatedly gets input from the user in the command line within a while loop. This while loop's condition is always true, so it can only be ended with the use of the exit command, which uses the system call exit(0). Otherwise, the command line parses the user's input and follows the appropriate commands. The built-in commands included in this simple shell include: exit, cd, version, and echo. The simple shell also allows for comments (#), the use of piped commands, and external commands.

# Program Functions and Descriptions
## Non-Forking Commands
The exit and cd commands do not fork because they must be executed in the existing process. The function **exit_bi** simply uses the system call exit(0). The function **cd_bi** checks that a directory path was provided as an argument; then uses the chdir() system call to move to the designated directory. All other commands execute after forking to a new process.

## Pipes
After forking but before executing the commands, the program checks for a pipe. If there is no pipe, the **no_pipe** function calls the forked command in the child process while the parent waits.

If the user did input a pipe, the **with_pipe** function forks again so there are three processes (child, parent, grandparent). The child process executes the first command using the forked_commands function while the parent and grandparent both wait. Then the parent process executes the second command using the forked_commands function while the grandparent (initial proceess) waits.

## Forking Commands
The **forked_commands** function handles the version, echo, and external commands. Each of these is executed in their own function: **version_bi**, **echo_bi**, **exec_wrapper** (which calls execvp()).


