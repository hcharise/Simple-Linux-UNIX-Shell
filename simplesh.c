// Hannah Ashton

#include <stdio.h>
#include <stdlib.h>
#include <readline/readline.h>
#include<readline/history.h>
#include "argset.h"
#include <string.h>
#include <unistd.h>
#include <errno.h>

void exit_bi(char **argv, char argc) {
    exit(0);
} // end exit_bi()

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
        if (chdir(path) == -1) {
            fprintf (stderr, "chdir failed - %s\n", strerror (errno));
        }
    }
} // end cd_bi()

void version_bi(char **argv, char argc) {
    fprintf(stdout, "simplesh v0.1 - Hannah Ashton\n");
    exit(0);
} // end version_bi()

void echo_bi(char **argv, char argc) {
    for (int i = 1; i < argc; i++) {
            fprintf(stdout, "%s ", argv[i]);
        }
    fprintf(stdout, "\n");
    exit(0);
} // end echo_bi()

void exec_wrapper(char **argv, char argc) {
    if (execvp(argv[0], argv) == -1) {
        fprintf(stderr, "Error accessing external command: %s\n", strerror (errno));
        exit(-1);
    }
} // end exec_wrapper()

// For forked commands (all but comments, cd, and exit)
int forked_commands(argset_t *argset, int set) {
    if (strcmp(argset->sets[set].argv[0], "version") == 0) { // VERSION
        version_bi(argset->sets[set].argv, argset->sets[0].argc);
    } else if (strcmp(argset->sets[set].argv[0], "echo") == 0) { // ECHO
        echo_bi(argset->sets[set].argv, argset->sets[0].argc);
    } else {
        exec_wrapper(argset->sets[set].argv, argset->sets[0].argc); // EXEC to EXTERNALS
    }
    exit(0);
} // end forked_commands()

void no_pipe(argset_t *argset, int pid) {
    // Check if process is parent or child
    if (pid == 0) { // Child, call forked_commands
        if(forked_commands(argset, 0) == -1) {
            fprintf(stderr, "Error reading command.\n");
            exit(-1);
        }
    } else { // Parent, so wait
        waitpid(pid, NULL, 0);
    }
}

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
        if(forked_commands(argset, 0) == -1) {
            fprintf(stderr, "Error reading command.\n");
            exit(-1);
        }
    } else { // New parent, so wait
        close(pipefd[1]); // Close write end of pipe
        dup2(pipefd[0], 0); // Close stdin and instead get input from read end of pipe
        waitpid(pid_2, NULL, 0); // Wait for it's child (grandchild), then does it's own command
        if(forked_commands(argset, 1) == -1) {
            fprintf(stderr, "Error reading command.\n");
            exit(-1);
        }
    }
    } else { // Grandparent (original/shell parent), so wait for it's child (from first fork)
        waitpid(pid, NULL, 0);
    }
}

int main(int argc, char **argv) {

    // Repeating while loop, only terminates with exit command/error
    int repeat = 1;
    while (repeat) {
        
        // Prompt user with $ and convert line to argset
        char *line = readline("$ ");
        add_history(line);
        if (line == NULL) {
            fprintf(stderr, "Error in readline\n");
            exit(-1);
        }
        argset_t  *argset = build_argset(line);

        // Check for no command or comment
        if (argset == NULL || *argset->sets[0].argv[0] == '#') {
            continue;
        }

        // COMMANDS WITH NO FORKING
        // Check if this is a built in process that should not fork (exit or cd)
        if (strcmp(argset->sets[0].argv[0], "exit") == 0) { // EXIT
            exit_bi(argset->sets[0].argv, argset->sets[0].argc);
        } else if (strcmp(argset->sets[0].argv[0], "cd") == 0) { // CD
            cd_bi(argset->sets[0].argv, argset->sets[0].argc);
            continue; // Return to start of while loop to prompt user
        } // End built in processes that do not fork

        // Fork to start child process since not an exit/cd
        int pid;
        pid = fork();
        
        // COMMANDS WITH FORKING
        if (argset->count == 1) { // NO PIPE
            no_pipe(argset, pid);
        } else { // WITH A PIPE
            with_pipe(argset, pid);
        }
    }
    return 0;
}
