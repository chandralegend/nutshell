#include <nutshell/core.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

// Debug flag - set to 1 to enable debug output
#define DEBUG_REGISTRY 1

static CommandRegistry *registry = NULL;
static const char* PACKAGES_DIR = "/.nutshell/packages";

// Function prototype for register_package_commands
bool register_package_commands(const char *pkg_dir, const char *pkg_name);

void init_registry() {
    registry = malloc(sizeof(CommandRegistry));
    registry->commands = NULL;
    registry->count = 0;
    
    // Default commands
    register_command("exit", "roast", true);
    register_command("ls", "peekaboo", true);  // Change to true - ls is a builtin command
    register_command("cd", "hop", true);
    
    // Register the package installer command
    register_command("install-pkg", "install-pkg", true);
    
    if (DEBUG_REGISTRY) {
        fprintf(stderr, "DEBUG: Initialized registry with default commands\n");
    }
    
    // Load installed packages
    char home_path[256];
    char *home = getenv("HOME");
    if (home) {
        snprintf(home_path, sizeof(home_path), "%s%s", home, PACKAGES_DIR);
        if (DEBUG_REGISTRY) {
            fprintf(stderr, "DEBUG: Loading packages from user dir: %s\n", home_path);
        }
        load_packages_from_dir(home_path);
    } else {
        if (DEBUG_REGISTRY) {
            fprintf(stderr, "DEBUG: HOME environment variable not set\n");
        }
    }
    
    // Also check system-wide packages if accessible
    if (DEBUG_REGISTRY) {
        fprintf(stderr, "DEBUG: Loading packages from system dir: /usr/local/nutshell/packages\n");
    }
    load_packages_from_dir("/usr/local/nutshell/packages");
}

// New function to scan and load packages from directories
void load_packages_from_dir(const char *dir_path) {
    DIR *dir = opendir(dir_path);
    if (!dir) return;
    
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR && 
            strcmp(entry->d_name, ".") != 0 && 
            strcmp(entry->d_name, "..") != 0) {
            
            register_package_commands(dir_path, entry->d_name);
        }
    }
    
    closedir(dir);
}

// Register commands from a specific package
bool register_package_commands(const char *pkg_dir, const char *pkg_name) {
    char script_path[512];
    snprintf(script_path, sizeof(script_path), "%s/%s/%s.sh", pkg_dir, pkg_name, pkg_name);
    
    // Check if the script exists
    struct stat st;
    if (stat(script_path, &st) == 0) {
        // Register command with the package name
        register_command(script_path, pkg_name, false);
        return true;
    }
    
    return false;
}

void register_command(const char *unix_cmd, const char *nut_cmd, bool is_builtin) {
    registry->count++;
    registry->commands = realloc(registry->commands, 
                               registry->count * sizeof(CommandMapping));
    
    CommandMapping *cmd = &registry->commands[registry->count - 1];
    cmd->unix_cmd = strdup(unix_cmd);
    cmd->nut_cmd = strdup(nut_cmd);
    cmd->is_builtin = is_builtin;
    
    if (DEBUG_REGISTRY) {
        fprintf(stderr, "DEBUG: Registered command: %s -> %s (builtin: %s)\n", 
                nut_cmd, unix_cmd, is_builtin ? "yes" : "no");
    }
}

const CommandMapping *find_command(const char *input_cmd) {
    if (DEBUG_REGISTRY) {
        fprintf(stderr, "DEBUG: Looking for command: %s\n", input_cmd);
    }
    
    for (size_t i = 0; i < registry->count; i++) {
        if (strcmp(registry->commands[i].nut_cmd, input_cmd) == 0 ||
            strcmp(registry->commands[i].unix_cmd, input_cmd) == 0) {
            
            if (DEBUG_REGISTRY) {
                fprintf(stderr, "DEBUG: Found command: %s -> %s (builtin: %s)\n", 
                        input_cmd, registry->commands[i].unix_cmd, 
                        registry->commands[i].is_builtin ? "yes" : "no");
            }
            return &registry->commands[i];
        }
    }
    
    if (DEBUG_REGISTRY) {
        fprintf(stderr, "DEBUG: Command not found: %s\n", input_cmd);
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