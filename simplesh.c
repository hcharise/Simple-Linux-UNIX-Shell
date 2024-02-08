/*
 Hannah Ashton
 Syracuse University
 CPS 500 Computer Organization and Operating Systems
 Professor William Katsak
 June 2023
 
 This program is a simple Linus/UNIX shell that executes basic
 functions, including built ins and external executables.
*/

// Section below provided by Professor WK
#include <stdio.h>
#include <stdlib.h>
#include <readline/readline.h>
#include "argset.h"
// End section provided by Professor WK

#include <readline/history.h> // Allows user to use ^ arrow to copy previous commands
#include <string.h> // Allows for the handling of strings
#include <unistd.h> // Allows for fork() command to be used
#include <errno.h> // Used to retrive error conditions

// Exits the simple shell by calling exit system call
void exit_bi(char **argv, char argc) {
    exit(0);
} // end exit_bi()

// Changes the directory if valid directory path was provided
void cd_bi(char **argv, char argc) {
    // Check that there is a file path
    if (argv[1] == NULL) {
        fprintf(stderr, "No file path provided.\n");
    } else {
        // Concatenate path in case spaces exist in path to directory
        char path[50];
        memset(path, '\0', 50 * sizeof(char));
        for (int i = 1; i < argc; i++) {
            if (i > 1) {
                strcat(path, " ");
            }
            strcat(path, argv[i]);
        }
        if (chdir(path) == -1) { // If error returned, print message to user
            fprintf (stderr, "chdir failed - %s\n", strerror (errno));
        }
    }
} // end cd_bi()

// Prints the version of the simple shell
void version_bi(char **argv, char argc) {
    fprintf(stdout, "simplesh v0.1 - Hannah Ashton\n");
    exit(0);
} // end version_bi()

// Echo's the user's input (prints it back to them)
void echo_bi(char **argv, char argc) {
    for (int i = 1; i < argc; i++) {
            fprintf(stdout, "%s ", argv[i]);
        }
    fprintf(stdout, "\n");
    exit(0);
} // end echo_bi()

// Wrapper to call external commands
void exec_wrapper(char **argv, char argc) {
    if (execvp(argv[0], argv) == -1) { // If error returned from execvp, print meessage to user
        fprintf(stderr, "Error accessing external command: %s\n", strerror (errno));
        exit(-1);
    }
} // end exec_wrapper()

// For forked commands (all but comments, cd, and exit)
int forked_commands(argset_t *argset, int set) {
    if (strcmp(argset->sets[set].argv[0], "version") == 0) { // VERSION
        version_bi(argset->sets[set].argv, argset->sets[0].argc); // Prints version info
    } else if (strcmp(argset->sets[set].argv[0], "echo") == 0) { // ECHO
        echo_bi(argset->sets[set].argv, argset->sets[0].argc); // Prints user's input back to them
    } else {
        exec_wrapper(argset->sets[set].argv, argset->sets[0].argc); // EXEC to EXTERNALS; allows for external commands
    }
    exit(0);
} // end forked_commands()

// Calls forked_commands with child while parent waits, when no pipe is used
void no_pipe(argset_t *argset, int pid) {
    // Check if process is parent or child
    if (pid == 0) { // Child, call forked_commands
        if(forked_commands(argset, 0) == -1) { // If forked commands returns error, print message to user
            fprintf(stderr, "Error reading command.\n");
            exit(-1);
        }
    } else { // Parent, so wait until child is done executing
        waitpid(pid, NULL, 0);
    }
} // end no_pipe()

// Calls forked_commands with child while parent waits, when pipe is used
void with_pipe(argset_t *argset, int pid) {
    if (pid == 0) { // Child, so create pipe, fork again,and run forked_commands on self and new child

    // Create pipe
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        fprintf(stderr, "Error in pipe.\n");
    }

    // Fork to start new child process
    int pid_2 = fork();
    
    if (pid_2 == 0) { // New child (grandchild), so run forked built ins or exec() - will execute the first command
        close(pipefd[0]); // Close read end of pipe, since this process only writes
        dup2(pipefd[1], 1); // Close stdout and redirect stdout to write end of pipe
        if(forked_commands(argset, 0) == -1) { // If forked commands return error, print message to user
            fprintf(stderr, "Error reading command.\n");
            exit(-1);
        }
    } else { // New parent, so wait
        close(pipefd[1]); // Close write end of pipe
        dup2(pipefd[0], 0); // Close stdin and instead get input from read end of pipe
        waitpid(pid_2, NULL, 0); // Wait for it's child (grandchild), then does it's own command
        if(forked_commands(argset, 1) == -1) { // If forked commands return error, print message to user
            fprintf(stderr, "Error reading command.\n");
            exit(-1);
        }
    }
    } else { // Grandparent (original/shell parent), so wait for it's child (from first fork)
        waitpid(pid, NULL, 0);
    }
} // end with_pipe()

// MAIN; runs the simple shell
int main(int argc, char **argv) {

    // Repeating while loop, only terminates with exit command/error
    int repeat = 1;
    while (repeat) {
        
        // Section below provided by Professor WK
        // HA - Prompt user with $ and convert line to argset
        char *line = readline("$ ");
        add_history(line); // HA - adds the current line into the line history, allowing user to use ^ arrow to use again
        if (line == NULL) {
            fprintf(stderr, "Error in readline\n");
            exit(-1);
        }
        argset_t  *argset = build_argset(line);
        // Check for no command or comment
        if (argset == NULL || *argset->sets[0].argv[0] == '#') {
            continue;
        }
        // End section provided by Professor WK

        // COMMANDS WITH NO FORKING
        // Check if this is a built in process that should not fork (exit or cd)
        if (strcmp(argset->sets[0].argv[0], "exit") == 0) { // EXIT
            exit_bi(argset->sets[0].argv, argset->sets[0].argc); // Exits simple shell
        } else if (strcmp(argset->sets[0].argv[0], "cd") == 0) { // CD
            cd_bi(argset->sets[0].argv, argset->sets[0].argc); // Changes directory
            continue; // Return to start of while loop to prompt user
        } // End built in processes that do not fork

        // Fork to start child process since not an exit/cd
        int pid;
        pid = fork();
        
        // COMMANDS WITH FORKING
        if (argset->count == 1) { // NO PIPE
            no_pipe(argset, pid); // handles commands that do not require piping
        } else { // WITH A PIPE
            with_pipe(argset, pid); // handleees commands that require piping
        }
    }
    return 0;
} // end main()
