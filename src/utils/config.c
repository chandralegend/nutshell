#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE

#include <nutshell/config.h>
#include <nutshell/utils.h>
#include <jansson.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <limits.h>  // For PATH_MAX constant
#include <libgen.h>  // For dirname function
#include <errno.h>   // Add this include for errno

// Configuration debug macro
#define CONFIG_DEBUG(fmt, ...) \
    do { if (getenv("NUT_DEBUG_CONFIG")) fprintf(stderr, "CONFIG: " fmt "\n", ##__VA_ARGS__); } while(0)

// Global configuration instance
Config *global_config = NULL;

// Configuration file names
static const char *DIR_CONFIG_FILE = ".nutshell.json";
static const char *USER_CONFIG_DIR = "/.nutshell";
static const char *USER_CONFIG_FILE = "/.nutshell/config.json";
static const char *SYSTEM_CONFIG_FILE = "/usr/local/nutshell/config.json";

// Initialize configuration system
void init_config_system() {
    CONFIG_DEBUG("Initializing configuration system");
    
    // Create empty configuration structure
    global_config = calloc(1, sizeof(Config));
    if (!global_config) {
        fprintf(stderr, "Error: Failed to initialize configuration system\n");
        return;
    }
    
    // Ensure user config directory exists
    char *home = getenv("HOME");
    if (home) {
        char config_dir[512];
        snprintf(config_dir, sizeof(config_dir), "%s%s", home, USER_CONFIG_DIR);
        
        struct stat st = {0};
        if (stat(config_dir, &st) == -1) {
            // Create directory if it doesn't exist
            CONFIG_DEBUG("Creating user config directory %s", config_dir);
            mkdir(config_dir, 0700);
        }
    }
    
    // Load configuration from files
    load_config_files();  // Updated function name
}

// Clean up configuration resources
void cleanup_config_system() {
    if (!global_config) return;
    
    free(global_config->theme);
    
    for (int i = 0; i < global_config->package_count; i++) {
        free(global_config->enabled_packages[i]);
    }
    free(global_config->enabled_packages);
    
    for (int i = 0; i < global_config->alias_count; i++) {
        free(global_config->aliases[i]);
        free(global_config->alias_commands[i]);
    }
    free(global_config->aliases);
    free(global_config->alias_commands);
    
    for (int i = 0; i < global_config->script_count; i++) {
        free(global_config->scripts[i]);
    }
    free(global_config->scripts);
    
    free(global_config);
    global_config = NULL;
}

// Load JSON file into configuration
static bool load_config_from_file(const char *path) {
    CONFIG_DEBUG("Attempting to load config from: %s", path);
    
    // Check if file exists
    struct stat st;
    if (stat(path, &st) != 0) {
        CONFIG_DEBUG("Config file not found: %s", path);
        return false;
    }
    
    // Open file
    FILE *file = fopen(path, "r");
    if (!file) {
        CONFIG_DEBUG("Failed to open config file: %s", path);
        return false;
    }
    
    // Read file contents
    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char *data = malloc(length + 1);
    if (!data) {
        CONFIG_DEBUG("Failed to allocate memory for config data");
        fclose(file);
        return false;
    }
    
    fread(data, 1, length, file);
    data[length] = '\0';
    fclose(file);
    
    // Parse JSON
    json_error_t error;
    json_t *root = json_loads(data, 0, &error);
    free(data);
    
    if (!root) {
        CONFIG_DEBUG("JSON parse error: %s (line: %d, col: %d)", 
                   error.text, error.line, error.column);
        return false;
    }
    
    // Extract theme setting
    json_t *theme_json = json_object_get(root, "theme");
    if (json_is_string(theme_json)) {
        free(global_config->theme);
        global_config->theme = strdup(json_string_value(theme_json));
        CONFIG_DEBUG("Loaded theme: %s", global_config->theme);
    }
    
    // Extract packages
    json_t *packages_json = json_object_get(root, "packages");
    if (json_is_array(packages_json)) {
        // Free existing packages
        for (int i = 0; i < global_config->package_count; i++) {
            free(global_config->enabled_packages[i]);
        }
        free(global_config->enabled_packages);
        
        // Allocate new packages array
        size_t package_count = json_array_size(packages_json);
        global_config->enabled_packages = calloc(package_count, sizeof(char*));
        global_config->package_count = package_count;
        
        // Load each package
        for (size_t i = 0; i < package_count; i++) {
            json_t *pkg = json_array_get(packages_json, i);
            if (json_is_string(pkg)) {
                global_config->enabled_packages[i] = strdup(json_string_value(pkg));
                CONFIG_DEBUG("Loaded package: %s", global_config->enabled_packages[i]);
            }
        }
    }
    
    // Extract aliases
    json_t *aliases_json = json_object_get(root, "aliases");
    if (json_is_object(aliases_json)) {
        // Free existing aliases
        for (int i = 0; i < global_config->alias_count; i++) {
            free(global_config->aliases[i]);
            free(global_config->alias_commands[i]);
        }
        free(global_config->aliases);
        free(global_config->alias_commands);
        
        // Allocate new aliases array
        size_t alias_count = json_object_size(aliases_json);
        global_config->aliases = calloc(alias_count, sizeof(char*));
        global_config->alias_commands = calloc(alias_count, sizeof(char*));
        global_config->alias_count = alias_count;
        
        // Load each alias
        const char *key;
        json_t *value;
        int i = 0;
        
        json_object_foreach(aliases_json, key, value) {
            if (json_is_string(value)) {
                global_config->aliases[i] = strdup(key);
                global_config->alias_commands[i] = strdup(json_string_value(value));
                CONFIG_DEBUG("Loaded alias: %s -> %s", 
                           global_config->aliases[i], global_config->alias_commands[i]);
                i++;
            }
        }
    }
    
    // Extract scripts
    json_t *scripts_json = json_object_get(root, "scripts");
    if (json_is_array(scripts_json)) {
        // Free existing scripts
        for (int i = 0; i < global_config->script_count; i++) {
            free(global_config->scripts[i]);
        }
        free(global_config->scripts);
        
        // Allocate new scripts array
        size_t script_count = json_array_size(scripts_json);
        global_config->scripts = calloc(script_count, sizeof(char*));
        global_config->script_count = script_count;
        
        // Load each script
        for (size_t i = 0; i < script_count; i++) {
            json_t *script = json_array_get(scripts_json, i);
            if (json_is_string(script)) {
                global_config->scripts[i] = strdup(json_string_value(script));
                CONFIG_DEBUG("Loaded script: %s", global_config->scripts[i]);
            }
        }
    }
    
    json_decref(root);
    CONFIG_DEBUG("Successfully loaded config from %s", path);
    return true;
}

// Check up the directory tree for config files
static bool find_directory_config(char *result_path, size_t max_size) {
    char current_dir[PATH_MAX] = {0};
    char config_path[PATH_MAX] = {0};
    
    // Get the absolute path of the current directory
    if (!getcwd(current_dir, sizeof(current_dir))) {
        CONFIG_DEBUG("Failed to get current directory");
        return false;
    }
    
    CONFIG_DEBUG("Searching for directory config starting from: %s", current_dir);
    CONFIG_DEBUG("Looking for file named: %s", DIR_CONFIG_FILE);
    
    // Start from the current directory and go up until root
    char *dir_path = strdup(current_dir);
    if (!dir_path) {
        CONFIG_DEBUG("Failed to allocate memory for dir_path");
        return false;
    }
    
    while (dir_path && strlen(dir_path) > 0) {
        // Create the config file path
        snprintf(config_path, sizeof(config_path), "%s/%s", dir_path, DIR_CONFIG_FILE);
        CONFIG_DEBUG("Checking for config at: %s", config_path);
        
        // Check if config file exists
        if (access(config_path, R_OK) == 0) {
            CONFIG_DEBUG("Found directory config at: %s", config_path);
            
            // Check if we can actually open and read the file
            FILE *check = fopen(config_path, "r");
            if (check) {
                char buffer[256];
                size_t bytes_read = fread(buffer, 1, sizeof(buffer) - 1, check);
                buffer[bytes_read] = '\0';
                fclose(check);
                CONFIG_DEBUG("First %zu bytes of config: %.100s...", bytes_read, buffer);
            } else {
                CONFIG_DEBUG("WARNING: Found config but cannot open for reading: %s", strerror(errno));
            }
            
            strncpy(result_path, config_path, max_size);
            free(dir_path);
            return true;
        } else {
            CONFIG_DEBUG("Config file not found at: %s (%s)", config_path, strerror(errno));
        }
        
        // Go up one directory level - completely rewritten to avoid memory issues
        char *parent_dir = strdup(dir_path);
        if (!parent_dir) {
            CONFIG_DEBUG("Failed to allocate memory for parent_dir");
            free(dir_path);
            return false;
        }
        
        // Use dirname() on the copy
        char *dirname_result = dirname(parent_dir);
        
        // If we've reached the root directory
        if (strcmp(dir_path, dirname_result) == 0 || 
            strcmp(dirname_result, "/") == 0) {
            CONFIG_DEBUG("Reached root directory or can't go up further");
            free(parent_dir);
            free(dir_path);
            return false;
        }
        
        // Replace current path with parent
        free(dir_path);
        dir_path = strdup(dirname_result);
        free(parent_dir);
        
        if (!dir_path) {
            CONFIG_DEBUG("Failed to allocate memory for new dir_path");
            return false;
        }
    }
    
    CONFIG_DEBUG("No directory config found in path");
    if (dir_path) {
        free(dir_path);
    }
    return false;
}

// Load configuration from files with improved directory hierarchy search
bool load_config_files() {
    bool loaded_any = false;
    
    // Track which configuration sources were loaded
    bool dir_loaded = false;
    bool user_loaded = false;
    bool system_loaded = false;
    
    // Load in reverse precedence order: system (lowest) -> user -> directory (highest)
    
    // Check for system config first (lowest precedence)
    if (load_config_from_file(SYSTEM_CONFIG_FILE)) {
        CONFIG_DEBUG("Loaded system config from: %s", SYSTEM_CONFIG_FILE);
        system_loaded = true;
        loaded_any = true;
    }
    
    // Check for user config next (medium precedence)
    char user_config[512];
    char *home = getenv("HOME");
    if (home) {
        snprintf(user_config, sizeof(user_config), "%s%s", home, USER_CONFIG_FILE);
        if (load_config_from_file(user_config)) {
            CONFIG_DEBUG("Loaded user config from: %s", user_config);
            user_loaded = true;
            loaded_any = true;
        }
    }
    
    // Finally, try to find directory-specific config (highest precedence)
    char dir_config_path[PATH_MAX] = {0};
    if (find_directory_config(dir_config_path, sizeof(dir_config_path))) {
        if (load_config_from_file(dir_config_path)) {
            CONFIG_DEBUG("Loaded directory-specific config from: %s", dir_config_path);
            dir_loaded = true;
            loaded_any = true;
            
            // Store the directory where we found the config
            if (global_config) {
                char *dir_name = strdup(dirname(dir_config_path));
                CONFIG_DEBUG("Setting active config directory to: %s", dir_name);
                // Store this path for later reference
                free(dir_name); // Free the temporary string
            }
        }
    }
    
    CONFIG_DEBUG("Config loading summary: directory=%s, user=%s, system=%s",
              dir_loaded ? "yes" : "no", 
              user_loaded ? "yes" : "no", 
              system_loaded ? "yes" : "no");
              
    return loaded_any;
}

// Force reload of configuration based on current directory
bool reload_directory_config() {
    CONFIG_DEBUG("Reloading configuration for current directory");
    
    // Temporarily store current theme to preserve it if not overridden
    char *current_theme = global_config && global_config->theme ? 
                         strdup(global_config->theme) : NULL;
    
    // Clear existing configuration but don't free the struct
    cleanup_config_values();
    
    // Reload configuration from files
    bool result = load_config_files();
    
    // If we had a theme before and no new theme was loaded, restore it
    if (current_theme && global_config && !global_config->theme) {
        global_config->theme = current_theme;
    } else {
        free(current_theme);
    }
    
    return result;
}

// Clean up only the values inside the configuration, not the struct itself
void cleanup_config_values() {
    if (!global_config) return;
    
    free(global_config->theme);
    global_config->theme = NULL;
    
    for (int i = 0; i < global_config->package_count; i++) {
        free(global_config->enabled_packages[i]);
    }
    free(global_config->enabled_packages);
    global_config->enabled_packages = NULL;
    global_config->package_count = 0;
    
    for (int i = 0; i < global_config->alias_count; i++) {
        free(global_config->aliases[i]);
        free(global_config->alias_commands[i]);
    }
    free(global_config->aliases);
    free(global_config->alias_commands);
    global_config->aliases = NULL;
    global_config->alias_commands = NULL;
    global_config->alias_count = 0;
    
    for (int i = 0; i < global_config->script_count; i++) {
        free(global_config->scripts[i]);
    }
    free(global_config->scripts);
    global_config->scripts = NULL;
    global_config->script_count = 0;
}

// Save current configuration to user config file
bool save_config() {
    if (!global_config) return false;
    
    CONFIG_DEBUG("Saving configuration");
    
    char user_config[512];
    char *home = getenv("HOME");
    if (!home) {
        CONFIG_DEBUG("HOME environment variable not set, can't save config");
        return false;
    }
    
    snprintf(user_config, sizeof(user_config), "%s%s", home, USER_CONFIG_FILE);
    
    // Create JSON structure
    json_t *root = json_object();
    
    // Save theme
    if (global_config->theme) {
        json_object_set_new(root, "theme", json_string(global_config->theme));
    }
    
    // Save packages
    if (global_config->package_count > 0) {
        json_t *packages = json_array();
        for (int i = 0; i < global_config->package_count; i++) {
            json_array_append_new(packages, json_string(global_config->enabled_packages[i]));
        }
        json_object_set_new(root, "packages", packages);
    }
    
    // Save aliases
    if (global_config->alias_count > 0) {
        json_t *aliases = json_object();
        for (int i = 0; i < global_config->alias_count; i++) {
            json_object_set_new(aliases, global_config->aliases[i], 
                             json_string(global_config->alias_commands[i]));
        }
        json_object_set_new(root, "aliases", aliases);
    }
    
    // Save scripts
    if (global_config->script_count > 0) {
        json_t *scripts = json_array();
        for (int i = 0; i < global_config->script_count; i++) {
            json_array_append_new(scripts, json_string(global_config->scripts[i]));
        }
        json_object_set_new(root, "scripts", scripts);
    }
    
    // Write JSON to file
    char *json_str = json_dumps(root, JSON_INDENT(2));
    json_decref(root);
    
    if (!json_str) {
        CONFIG_DEBUG("Failed to generate JSON");
        return false;
    }
    
    FILE *file = fopen(user_config, "w");
    if (!file) {
        CONFIG_DEBUG("Failed to open config file for writing: %s", user_config);
        free(json_str);
        return false;
    }
    
    fputs(json_str, file);
    fclose(file);
    free(json_str);
    
    CONFIG_DEBUG("Configuration saved to %s", user_config);
    return true;
}

// Update theme in configuration
bool set_config_theme(const char *theme_name) {
    if (!global_config || !theme_name) return false;
    
    CONFIG_DEBUG("Setting config theme to %s", theme_name);
    
    free(global_config->theme);
    global_config->theme = strdup(theme_name);
    
    return save_config();
}

// Add package to configuration
bool add_config_package(const char *package_name) {
    if (!global_config || !package_name) return false;
    
    CONFIG_DEBUG("Adding package: %s", package_name);
    
    // Check if package is already in config
    for (int i = 0; i < global_config->package_count; i++) {
        if (strcmp(global_config->enabled_packages[i], package_name) == 0) {
            CONFIG_DEBUG("Package %s already exists in config", package_name);
            return true; // Already exists
        }
    }
    
    CONFIG_DEBUG("Package count before adding: %d", global_config->package_count);
    
    // Add new package
    char **new_packages = realloc(global_config->enabled_packages, 
                            (global_config->package_count + 1) * sizeof(char*));
    
    if (!new_packages) {
        CONFIG_DEBUG("Failed to reallocate package array");
        return false;
    }
    
    global_config->enabled_packages = new_packages;
    global_config->enabled_packages[global_config->package_count] = strdup(package_name);
    global_config->package_count++;
    
    CONFIG_DEBUG("Package count after adding: %d", global_config->package_count);
    CONFIG_DEBUG("Added package: %s", global_config->enabled_packages[global_config->package_count-1]);
    
    return save_config();
}

// Remove package from configuration
bool remove_config_package(const char *package_name) {
    if (!global_config || !package_name) return false;
    
    for (int i = 0; i < global_config->package_count; i++) {
        if (strcmp(global_config->enabled_packages[i], package_name) == 0) {
            // Found the package, remove it
            free(global_config->enabled_packages[i]);
            
            // Shift remaining packages
            for (int j = i; j < global_config->package_count - 1; j++) {
                global_config->enabled_packages[j] = global_config->enabled_packages[j+1];
            }
            
            global_config->package_count--;
            return save_config();
        }
    }
    
    return false; // Package not found
}

// Add alias to configuration
bool add_config_alias(const char *alias_name, const char *command) {
    if (!global_config || !alias_name || !command) return false;
    
    // Check if alias already exists
    for (int i = 0; i < global_config->alias_count; i++) {
        if (strcmp(global_config->aliases[i], alias_name) == 0) {
            // Update existing alias
            free(global_config->alias_commands[i]);
            global_config->alias_commands[i] = strdup(command);
            return save_config();
        }
    }
    
    // Add new alias
    global_config->aliases = realloc(global_config->aliases, 
                                  (global_config->alias_count + 1) * sizeof(char*));
    global_config->alias_commands = realloc(global_config->alias_commands, 
                                         (global_config->alias_count + 1) * sizeof(char*));
    
    global_config->aliases[global_config->alias_count] = strdup(alias_name);
    global_config->alias_commands[global_config->alias_count] = strdup(command);
    global_config->alias_count++;
    
    return save_config();
}

// Remove alias from configuration
bool remove_config_alias(const char *alias_name) {
    if (!global_config || !alias_name) return false;
    
    for (int i = 0; i < global_config->alias_count; i++) {
        if (strcmp(global_config->aliases[i], alias_name) == 0) {
            // Found the alias, remove it
            free(global_config->aliases[i]);
            free(global_config->alias_commands[i]);
            
            // Shift remaining aliases
            for (int j = i; j < global_config->alias_count - 1; j++) {
                global_config->aliases[j] = global_config->aliases[j+1];
                global_config->alias_commands[j] = global_config->alias_commands[j+1];
            }
            
            global_config->alias_count--;
            return save_config();
        }
    }
    
    return false; // Alias not found
}

// Add script to configuration
bool add_config_script(const char *script_path) {
    if (!global_config || !script_path) return false;
    
    // Check if script already exists
    for (int i = 0; i < global_config->script_count; i++) {
        if (strcmp(global_config->scripts[i], script_path) == 0) {
            return true; // Already exists
        }
    }
    
    // Add new script
    global_config->scripts = realloc(global_config->scripts, 
                                  (global_config->script_count + 1) * sizeof(char*));
    global_config->scripts[global_config->script_count] = strdup(script_path);
    global_config->script_count++;
    
    return save_config();
}

// Remove script from configuration
bool remove_config_script(const char *script_path) {
    if (!global_config || !script_path) return false;
    
    for (int i = 0; i < global_config->script_count; i++) {
        if (strcmp(global_config->scripts[i], script_path) == 0) {
            // Found the script, remove it
            free(global_config->scripts[i]);
            
            // Shift remaining scripts
            for (int j = i; j < global_config->script_count - 1; j++) {
                global_config->scripts[j] = global_config->scripts[j+1];
            }
            
            global_config->script_count--;
            return save_config();
        }
    }
    
    return false; // Script not found
}

// Get theme from configuration
const char *get_config_theme() {
    return global_config ? global_config->theme : NULL;
}

// Check if package is enabled
bool is_package_enabled(const char *package_name) {
    if (!global_config || !package_name) return false;
    
    for (int i = 0; i < global_config->package_count; i++) {
        if (strcmp(global_config->enabled_packages[i], package_name) == 0) {
            return true;
        }
    }
    
    return false;
}

// Get alias command
const char *get_alias_command(const char *alias_name) {
    if (!global_config || !alias_name) return NULL;
    
    for (int i = 0; i < global_config->alias_count; i++) {
        if (strcmp(global_config->aliases[i], alias_name) == 0) {
            return global_config->alias_commands[i];
        }
    }
    
    return NULL;
}
