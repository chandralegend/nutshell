#include <nutshell/core.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

// Replace the debug flag with a macro that uses environment variable
#define REGISTRY_DEBUG(fmt, ...) \
    do { if (getenv("NUT_DEBUG_REGISTRY")) fprintf(stderr, "REGISTRY: " fmt "\n", ##__VA_ARGS__); } while(0)

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
    
    // Register theme command
    register_command("theme", "theme", true);
    
    REGISTRY_DEBUG("Initialized registry with default commands");
    
    // Load installed packages
    char home_path[256];
    char *home = getenv("HOME");
    if (home) {
        snprintf(home_path, sizeof(home_path), "%s%s", home, PACKAGES_DIR);
        REGISTRY_DEBUG("Loading packages from user dir: %s", home_path);
        load_packages_from_dir(home_path);
    } else {
        REGISTRY_DEBUG("HOME environment variable not set");
    }
    
    // Also check system-wide packages if accessible
    REGISTRY_DEBUG("Loading packages from system dir: /usr/local/nutshell/packages");
    load_packages_from_dir("/usr/local/nutshell/packages");
}

// New function to scan and load packages from directories
void load_packages_from_dir(const char *dir_path) {
    DIR *dir = opendir(dir_path);
    if (!dir) return;
    
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        // Use stat instead of d_type which is more portable
        char full_path[512];
        struct stat st;
        
        // Skip . and .. entries
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;
            
        // Build the full path
        snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);
        
        // Check if it's a directory using stat
        if (stat(full_path, &st) == 0 && S_ISDIR(st.st_mode)) {
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
    
    REGISTRY_DEBUG("Registered command: %s -> %s (builtin: %s)", 
            nut_cmd, unix_cmd, is_builtin ? "yes" : "no");
}

const CommandMapping *find_command(const char *input_cmd) {
    REGISTRY_DEBUG("Looking for command: %s", input_cmd);
    
    for (size_t i = 0; i < registry->count; i++) {
        if (strcmp(registry->commands[i].nut_cmd, input_cmd) == 0 ||
            strcmp(registry->commands[i].unix_cmd, input_cmd) == 0) {
            
            REGISTRY_DEBUG("Found command: %s -> %s (builtin: %s)", 
                    input_cmd, registry->commands[i].unix_cmd, 
                    registry->commands[i].is_builtin ? "yes" : "no");
            return &registry->commands[i];
        }
    }
    
    REGISTRY_DEBUG("Command not found: %s", input_cmd);
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