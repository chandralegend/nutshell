#ifndef NUTSHELL_CONFIG_H
#define NUTSHELL_CONFIG_H

#include <stdbool.h>

// Configuration structure to store settings
typedef struct {
    char *theme;            // Current theme name
    char **enabled_packages; // Array of enabled package names
    int package_count;      // Number of enabled packages
    char **aliases;         // Array of custom aliases
    char **alias_commands;  // Array of commands for each alias
    int alias_count;        // Number of aliases
    char **scripts;         // Array of custom script paths
    int script_count;       // Number of custom scripts
} Config;

// Global configuration
extern Config *global_config;

// Configuration functions
void init_config_system();
void cleanup_config_system();

// Load configuration from files (checks dir, user, system in that order)
bool load_config_files();  // Renamed from load_config to avoid conflict

// Save current configuration to user config file
bool save_config();

// Update specific configuration settings
bool set_config_theme(const char *theme_name);
bool add_config_package(const char *package_name);
bool remove_config_package(const char *package_name);
bool add_config_alias(const char *alias_name, const char *command);
bool remove_config_alias(const char *alias_name);
bool add_config_script(const char *script_path);
bool remove_config_script(const char *script_path);

// New functions for directory-level configuration
bool reload_directory_config();
void cleanup_config_values();

// Get configuration settings
const char *get_config_theme();
bool is_package_enabled(const char *package_name);
const char *get_alias_command(const char *alias_name);

#endif // NUTSHELL_CONFIG_H
