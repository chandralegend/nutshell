#include <nutshell/core.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

static CommandRegistry *registry = NULL;

void init_registry() {
    registry = malloc(sizeof(CommandRegistry));
    registry->commands = NULL;
    registry->count = 0;
    
    // Default commands
    register_command("exit", "roast", true);
    register_command("ls", "peekaboo", false);
    register_command("cd", "hop", true);
}

// Change int to bool to match header declaration
void register_command(const char *unix_cmd, const char *nut_cmd, bool is_builtin) {
    registry->count++;
    registry->commands = realloc(registry->commands, 
                               registry->count * sizeof(CommandMapping));
    
    CommandMapping *cmd = &registry->commands[registry->count - 1];
    cmd->unix_cmd = strdup(unix_cmd);
    cmd->nut_cmd = strdup(nut_cmd);
    cmd->is_builtin = is_builtin;
}

const CommandMapping *find_command(const char *input_cmd) {
    // Use size_t instead of int for loop counter
    for (size_t i = 0; i < registry->count; i++) {
        if (strcmp(registry->commands[i].nut_cmd, input_cmd) == 0 ||
            strcmp(registry->commands[i].unix_cmd, input_cmd) == 0) {
            return &registry->commands[i];
        }
    }
    return NULL;
}

void free_registry() {
    for (size_t i = 0; i < registry->count; i++) {
        free(registry->commands[i].unix_cmd);
        free(registry->commands[i].nut_cmd);
    }
    free(registry->commands);
    free(registry);
}

void print_command_registry() {
    // Use size_t instead of int for loop counter
    for (size_t i = 0; i < registry->count; i++) {
        printf("Unix Command: %s, Nut Command: %s, Builtin: %s\n",
               registry->commands[i].unix_cmd,
               registry->commands[i].nut_cmd,
               registry->commands[i].is_builtin ? "true" : "false");
    }
}