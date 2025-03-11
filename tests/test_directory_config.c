#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE

#include <nutshell/config.h>
#include <nutshell/utils.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <sys/stat.h>
#include <libgen.h>

// Helper function to create a test directory structure with configs
static void create_test_directory_structure() {
    // Create a temporary test directory structure
    char *home = getenv("HOME");
    if (!home) {
        printf("ERROR: HOME environment variable not set\n");
        return;
    }
    
    char test_root[512];
    snprintf(test_root, sizeof(test_root), "%s/.nutshell/test_dirs", home);
    
    // Create directories
    mkdir(test_root, 0755);
    char parent_dir[512], child_dir[512], grandchild_dir[512];
    
    snprintf(parent_dir, sizeof(parent_dir), "%s/parent", test_root);
    mkdir(parent_dir, 0755);
    
    snprintf(child_dir, sizeof(child_dir), "%s/parent/child", test_root);
    mkdir(child_dir, 0755);
    
    snprintf(grandchild_dir, sizeof(grandchild_dir), "%s/parent/child/grandchild", test_root);
    mkdir(grandchild_dir, 0755);
    
    // Create config files - the filename must match exactly what find_directory_config looks for
    char parent_config[512], child_config[512];
    snprintf(parent_config, sizeof(parent_config), "%s/.nutshell.json", parent_dir);
    snprintf(child_config, sizeof(child_config), "%s/.nutshell.json", child_dir);
    
    printf("DEBUG: Creating parent config at: %s\n", parent_config);
    
    // Parent directory config
    FILE *fp = fopen(parent_config, "w");
    if (fp) {
        fprintf(fp, "{\n"
                   "  \"theme\": \"parent_theme\",\n"
                   "  \"packages\": [\"parent_pkg\"],\n"
                   "  \"aliases\": {\n"
                   "    \"parent_alias\": \"echo parent\"\n"
                   "  }\n"
                   "}\n");
        fclose(fp);
        printf("Created parent config: %s\n", parent_config);
    } else {
        printf("ERROR: Failed to create parent config at %s\n", parent_config);
        perror("Reason");
    }
    
    printf("DEBUG: Creating child config at: %s\n", child_config);
    
    // Child directory config
    fp = fopen(child_config, "w");
    if (fp) {
        fprintf(fp, "{\n"
                   "  \"theme\": \"child_theme\",\n"
                   "  \"packages\": [\"child_pkg\"],\n"
                   "  \"aliases\": {\n"
                   "    \"child_alias\": \"echo child\"\n"
                   "  }\n"
                   "}\n");
        fclose(fp);
        printf("Created child config: %s\n", child_config);
    } else {
        printf("ERROR: Failed to create child config at %s\n", child_config);
        perror("Reason");
    }
    
    // Verify files were created properly
    struct stat st;
    if (stat(parent_config, &st) == 0) {
        printf("DEBUG: Parent config file exists and is %lld bytes\n", (long long)st.st_size);
    } else {
        printf("DEBUG: Parent config file does not exist!\n");
    }
    
    if (stat(child_config, &st) == 0) {
        printf("DEBUG: Child config file exists and is %lld bytes\n", (long long)st.st_size);
    } else {
        printf("DEBUG: Child config file does not exist!\n");
    }
}

// Helper function to back up and restore user's config
static void backup_user_config(bool restore) {
    char *home = getenv("HOME");
    if (!home) return;
    
    char config_file[512], backup_file[512];
    snprintf(config_file, sizeof(config_file), "%s/.nutshell/config.json", home);
    snprintf(backup_file, sizeof(backup_file), "%s/.nutshell/config.json.bak", home);
    
    if (restore) {
        // Restore the backup
        printf("DEBUG: Restoring user config from backup\n");
        struct stat st;
        if (stat(backup_file, &st) == 0) {
            // If backup exists, restore it
            rename(backup_file, config_file);
        }
    } else {
        // Create backup and remove config
        printf("DEBUG: Backing up user config\n");
        struct stat st;
        if (stat(config_file, &st) == 0) {
            // If config exists, make backup
            rename(config_file, backup_file);
        }
    }
}

// Test loading directory-specific config
static void test_directory_config_loading() {
    printf("Testing directory config loading...\n");
    
    // Back up any existing user config
    backup_user_config(false);
    
    // Create the test directory structure
    create_test_directory_structure();
    
    char *home = getenv("HOME");
    char test_root[512], parent_dir[512], child_dir[512], grandchild_dir[512];
    snprintf(test_root, sizeof(test_root), "%s/.nutshell/test_dirs", home);
    snprintf(parent_dir, sizeof(parent_dir), "%s/parent", test_root);
    snprintf(child_dir, sizeof(child_dir), "%s/parent/child", test_root);
    snprintf(grandchild_dir, sizeof(grandchild_dir), "%s/parent/child/grandchild", test_root);
    
    // Save current directory
    char cwd[512];
    getcwd(cwd, sizeof(cwd));
    
    // Test parent directory config
    printf("Testing in parent directory: %s\n", parent_dir);
    
    // Make sure the config file exists
    char parent_config[512];
    snprintf(parent_config, sizeof(parent_config), "%s/.nutshell.json", parent_dir);
    printf("DEBUG: Checking parent config file: %s\n", parent_config);
    
    FILE *check = fopen(parent_config, "r");
    if (check) {
        char buffer[512] = {0};
        size_t bytes_read = fread(buffer, 1, sizeof(buffer) - 1, check);
        fclose(check);
        printf("DEBUG: Parent config content (%zu bytes):\n%s\n", bytes_read, buffer);
    } else {
        printf("DEBUG: Cannot open parent config for reading!\n");
        perror("Reason");
    }
    
    // Execute the actual test
    if (chdir(parent_dir) != 0) {
        printf("DEBUG: Failed to chdir to %s\n", parent_dir);
        perror("chdir");
        return;
    }
    
    printf("DEBUG: Current directory after chdir: ");
    system("pwd");
    system("ls -la");
    
    // Initialize with debug enabled
    setenv("NUT_DEBUG_CONFIG", "1", 1);
    init_config_system();
    
    printf("DEBUG: After init_config_system()\n");
    
    assert(global_config != NULL);
    printf("DEBUG: global_config is not NULL\n");
    
    // Check if theme was loaded
    printf("DEBUG: global_config->theme = '%s'\n", 
           global_config->theme ? global_config->theme : "NULL");
    
    // Make sure the theme is not NULL
    assert(global_config->theme != NULL);
    
    // Check if it matches what we expect 
    assert(strcmp(global_config->theme, "parent_theme") == 0);
    
    // Verify the alias from parent config
    const char *parent_alias = get_alias_command("parent_alias");
    assert(parent_alias != NULL);
    assert(strcmp(parent_alias, "echo parent") == 0);
    cleanup_config_system();
    
    // Test child directory config
    printf("Testing in child directory: %s\n", child_dir);
    chdir(child_dir);
    init_config_system();
    assert(global_config != NULL);
    assert(global_config->theme != NULL);
    assert(strcmp(global_config->theme, "child_theme") == 0);
    
    // Verify the alias from child config
    const char *child_alias = get_alias_command("child_alias");
    assert(child_alias != NULL);
    assert(strcmp(child_alias, "echo child") == 0);
    cleanup_config_system();
    
    // Test grandchild directory - should inherit from child
    printf("Testing in grandchild directory (should inherit from child): %s\n", grandchild_dir);
    chdir(grandchild_dir);
    init_config_system();
    assert(global_config != NULL);
    assert(global_config->theme != NULL);
    assert(strcmp(global_config->theme, "child_theme") == 0);
    cleanup_config_system();
    
    // Test config reload on directory change
    // Start in child directory
    printf("Testing config reload on directory change\n");
    chdir(child_dir);
    init_config_system();
    assert(global_config != NULL);
    assert(strcmp(global_config->theme, "child_theme") == 0);
    
    // Change to parent directory and reload
    chdir(parent_dir);
    reload_directory_config();
    assert(strcmp(global_config->theme, "parent_theme") == 0);
    
    // Change to grandchild directory and reload
    chdir(grandchild_dir);
    reload_directory_config();
    assert(strcmp(global_config->theme, "child_theme") == 0);
    
    cleanup_config_system();
    
    // Restore original directory and user config
    chdir(cwd);
    backup_user_config(true);
    
    printf("Directory config loading test passed!\n");
}

int main() {
    printf("Running directory config tests...\n");
    
    // Set debugging if needed
    const char *debug_env = getenv("NUT_DEBUG");
    if (debug_env && strcmp(debug_env, "1") == 0) {
        setenv("NUT_DEBUG_CONFIG", "1", 1);
        printf("Config debugging enabled\n");
    }
    
    // Run tests
    test_directory_config_loading();
    
    printf("All directory config tests passed!\n");
    return 0;
}
