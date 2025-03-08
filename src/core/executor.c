#include <nutshell/core.h>
#include <nutshell/utils.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

void execute_command(ParsedCommand *cmd);

static void handle_redirection(ParsedCommand *cmd);

void execute_command(ParsedCommand *cmd) {

    if (!sanitize_command(cmd->args[0])) {
        print_error("Command blocked by security policy");
        return;
    }

    if (cmd->args[0] == NULL) return;

    // Handle builtins
    if (strcmp(cmd->args[0], "cd") == 0) {
        if (cmd->args[1]) chdir(cmd->args[1]);
        return;
    }
     
    if (strcmp(cmd->args[0], "exit") == 0) {
        exit(EXIT_SUCCESS);
    }

    // Fork and execute
    pid_t pid = fork();
    if (pid == 0) {
        handle_redirection(cmd);
        
        const CommandMapping *mapping = find_command(cmd->args[0]);
        if (mapping) {
            if (mapping->is_builtin) {
                execvp(mapping->unix_cmd, cmd->args);
            } else {
                // Handle Nutlang commands
                char *nut_args[MAX_ARGS];
                nut_args[0] = mapping->unix_cmd;
                for (int i = 1; cmd->args[i]; i++) {
                    nut_args[i] = cmd->args[i];
                }
                execvp(nut_args[0], nut_args);
            }
        } else {
            execvp(cmd->args[0], cmd->args);
        }
        perror("execvp");
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        if (!cmd->background) {
            waitpid(pid, NULL, 0);
        }
    } else {
        perror("fork");
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