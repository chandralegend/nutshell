#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE
#include <nutshell/core.h>
#include <nutshell/utils.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// Replace the debug flag with a macro that uses environment variable
#define EXEC_DEBUG(fmt, ...) \
    do { if (getenv("NUT_DEBUG_EXEC")) fprintf(stderr, "EXEC: " fmt "\n", ##__VA_ARGS__); } while(0)

// Forward declarations
extern int install_pkg_command(int argc, char **argv);

static void handle_redirection(ParsedCommand *cmd);

// Debug helper function
static void debug_print_command(ParsedCommand *cmd) {
    if (!getenv("NUT_DEBUG_EXEC")) return;
    
    EXEC_DEBUG("Executing command: '%s'", cmd->args[0]);
    for (int i = 0; cmd->args[i]; i++) {
        EXEC_DEBUG("  Arg %d: '%s'", i, cmd->args[i]);
    }
    
    if (cmd->input_file) {
        EXEC_DEBUG("  Input from: %s", cmd->input_file);
    }
    if (cmd->output_file) {
        EXEC_DEBUG("  Output to: %s", cmd->output_file);
    }
    if (cmd->background) {
        EXEC_DEBUG("  Running in background");
    }
}

void execute_command(ParsedCommand *cmd) {
    if (!cmd || !cmd->args[0]) return;
    
    debug_print_command(cmd);

    // Handle builtin commands without forking
    if (strcmp(cmd->args[0], "cd") == 0) {
        if (cmd->args[1]) chdir(cmd->args[1]);
        return;
    }
     
    if (strcmp(cmd->args[0], "exit") == 0) {
        exit(EXIT_SUCCESS);
    }
    
    if (strcmp(cmd->args[0], "install-pkg") == 0) {
        int argc = 0;
        while (cmd->args[argc]) argc++;
        install_pkg_command(argc, cmd->args);
        return;
    }

    // Look up the command in our registry
    const CommandMapping *mapping = find_command(cmd->args[0]);
    if (getenv("NUT_DEBUG_EXEC") && mapping) {
        EXEC_DEBUG("Command '%s' found in registry as '%s' (builtin: %s)", 
                cmd->args[0], mapping->unix_cmd, 
                mapping->is_builtin ? "yes" : "no");
    }
    
    // Create a clean array for arguments
    char *clean_args[MAX_ARGS];
    int i = 0;
    
    if (mapping) {
        EXEC_DEBUG("Using mapped command %s (builtin: %s)", 
                mapping->unix_cmd, mapping->is_builtin ? "yes" : "no");
        
        if (mapping->is_builtin) {
            // For system commands (builtins), replace the command name but keep arg structure
            clean_args[0] = strdup(mapping->unix_cmd);
            for (i = 1; cmd->args[i] && i < MAX_ARGS - 1; i++) {
                clean_args[i] = strdup(cmd->args[i]);
            }
        } else {
            // For custom scripts, use the script path as command and preserve original args
            clean_args[0] = strdup(mapping->unix_cmd);
            for (i = 1; cmd->args[i] && i < MAX_ARGS - 1; i++) {
                clean_args[i] = strdup(cmd->args[i]);
            }
        }
    } else {
        // Regular system command - keep all args unchanged
        for (i = 0; cmd->args[i] && i < MAX_ARGS - 1; i++) {
            clean_args[i] = strdup(cmd->args[i]);
        }
    }
    clean_args[i] = NULL;  // Ensure NULL termination
    
    if (getenv("NUT_DEBUG_EXEC")) {
        EXEC_DEBUG("Final command array:");
        for (int j = 0; clean_args[j]; j++) {
            EXEC_DEBUG("  clean_args[%d] = '%s'", j, clean_args[j]);
        }
    }

    // Fork and execute
    pid_t pid = fork();
    
    if (pid < 0) {
        perror("fork");
        goto cleanup;
    }
    
    if (pid == 0) { // Child process
        // Set up any redirections
        handle_redirection(cmd);
        
        if (mapping && !mapping->is_builtin) {
            // For custom scripts, use direct execution with path
            EXEC_DEBUG("Executing script with execv: %s", clean_args[0]);
            execv(clean_args[0], clean_args);
        } else {
            // For system commands and built-ins, use PATH lookup
            EXEC_DEBUG("Executing command with execvp: %s", clean_args[0]);
            execvp(clean_args[0], clean_args);
        }
        
        // If we get here, execution failed
        fprintf(stderr, "ERROR: Failed to execute '%s': %s\n", 
                clean_args[0], strerror(errno));
        
        // Free memory before exit
        for (int j = 0; clean_args[j]; j++) {
            free(clean_args[j]);
        }
        
        exit(EXIT_FAILURE);
    } else {
        // Parent process
        if (!cmd->background) {
            int status;
            waitpid(pid, &status, 0);
            if (getenv("NUT_DEBUG_EXEC")) {
                if (WIFEXITED(status)) {
                    EXEC_DEBUG("Child exited with status %d", WEXITSTATUS(status));
                } else if (WIFSIGNALED(status)) {
                    EXEC_DEBUG("Child killed by signal %d", WTERMSIG(status));
                }
            }
        }
    }

cleanup:
    // Free the argument array in the parent
    for (int j = 0; clean_args[j]; j++) {
        free(clean_args[j]);
    }
}

static void handle_redirection(ParsedCommand *cmd) {
    if (cmd->input_file) {
        int fd = open(cmd->input_file, O_RDONLY);
        if (fd == -1) {
            perror("open");
            exit(EXIT_FAILURE);
        }
        dup2(fd, STDIN_FILENO);
        close(fd);
    }
    
    if (cmd->output_file) {
        int fd = open(cmd->output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd == -1) {
            perror("open");
            exit(EXIT_FAILURE);
        }
        dup2(fd, STDOUT_FILENO);
        close(fd);
    }
}