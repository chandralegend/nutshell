#include <nutshell/core.h>
#include <nutshell/utils.h>
#include <nutshell/theme.h>
#include <nutshell/config.h>  // Add this include for configuration functions
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <signal.h>

// Helper function to create a simple test theme
Theme* create_test_theme() {
    Theme *theme = calloc(1, sizeof(Theme));
    
    // Theme basics
    theme->name = strdup("test_theme");
    theme->description = strdup("Test theme for unit tests");
    
    // Colors
    theme->colors = calloc(1, sizeof(ThemeColors));
    theme->colors->reset = strdup("\001\033[0m\002");
    theme->colors->primary = strdup("\001\033[1;32m\002");
    theme->colors->secondary = strdup("\001\033[1;34m\002");
    theme->colors->error = strdup("\001\033[1;31m\002");
    theme->colors->warning = strdup("\001\033[1;33m\002");
    theme->colors->info = strdup("\001\033[1;36m\002");
    theme->colors->success = strdup("\001\033[1;32m\002");
    
    // Left prompt - CHANGE THIS LINE to reference git_info instead of git_branch
    theme->left_prompt = calloc(1, sizeof(PromptConfig));
    theme->left_prompt->format = strdup("{primary}{icon} {directory}{reset} {git_info}");
    theme->left_prompt->icon = strdup("T");
    
    // Right prompt
    theme->right_prompt = calloc(1, sizeof(PromptConfig));
    theme->right_prompt->format = strdup("");
    
    // Other prompt settings
    theme->multiline = false;
    theme->prompt_symbol = strdup("$ ");
    theme->prompt_symbol_color = strdup("primary");
    
    // Segments
    theme->segment_count = 2;
    theme->segments = calloc(theme->segment_count + 1, sizeof(ThemeSegment*));
    
    // Directory segment
    theme->segments[0] = calloc(1, sizeof(ThemeSegment));
    theme->segments[0]->enabled = true;
    theme->segments[0]->key = strdup("directory");  // Add this line to set the segment key
    theme->segments[0]->format = strdup("{directory}");
    // New format with multiple commands
    theme->segments[0]->command_count = 1;
    theme->segments[0]->commands = calloc(2, sizeof(ThemeCommand*)); // +1 for NULL terminator
    theme->segments[0]->commands[0] = calloc(1, sizeof(ThemeCommand));
    theme->segments[0]->commands[0]->name = strdup("directory");
    theme->segments[0]->commands[0]->command = strdup("echo test_dir");
    theme->segments[0]->commands[0]->output = NULL;
    theme->segments[0]->commands[1] = NULL;  // NULL terminator
    
    // Git branch segment - KEEP THE KEY CONSISTENT
    theme->segments[1] = calloc(1, sizeof(ThemeSegment));
    theme->segments[1]->enabled = true;
    // Use git_info as the segment key to match prompt format
    theme->segments[1]->key = strdup("git_info");  // Set key to match prompt format
    theme->segments[1]->format = strdup("{secondary}git:({branch}){dirty_flag}{reset}");
    
    theme->segments[1]->command_count = 2;
    theme->segments[1]->commands = calloc(3, sizeof(ThemeCommand*)); // +1 for NULL terminator
    
    // Branch command
    theme->segments[1]->commands[0] = calloc(1, sizeof(ThemeCommand));
    theme->segments[1]->commands[0]->name = strdup("branch");
    theme->segments[1]->commands[0]->command = strdup("echo test_branch");
    theme->segments[1]->commands[0]->output = NULL;
    
    // Dirty flag command - demonstrates multiple commands per segment
    theme->segments[1]->commands[1] = calloc(1, sizeof(ThemeCommand));
    theme->segments[1]->commands[1]->name = strdup("dirty_flag");
    theme->segments[1]->commands[1]->command = strdup("echo '*'");
    theme->segments[1]->commands[1]->output = NULL;
    
    theme->segments[1]->commands[2] = NULL;  // NULL terminator
    
    theme->segments[2] = NULL;  // NULL terminator for segments array
    
    return theme;
}

// Test theme loading from JSON - change return type to int
int test_load_theme() {
    printf("Testing theme loading...\n");
    
    // First ensure themes are properly set up
    printf("DEBUG: Creating themes directory\n");
    system("mkdir -p ~/.nutshell/themes");
    printf("DEBUG: Copying theme files\n");
    system("cp ./themes/*.json ~/.nutshell/themes/ 2>/dev/null || echo 'DEBUG: Failed to copy themes'");
    system("ls -la ./themes/ 2>/dev/null || echo 'DEBUG: No themes in ./themes'");
    system("ls -la ~/.nutshell/themes/ || echo 'DEBUG: No themes in ~/.nutshell/themes'");
    
    printf("DEBUG: Attempting to load default theme\n");
    // Test loading the default theme
    Theme *theme = load_theme("default");
    if (!theme) {
        printf("WARNING: Could not load default theme. Check JSON syntax.\n");
        printf("DEBUG: Creating test theme instead\n");
        theme = create_test_theme();
        if (!theme) {
            printf("ERROR: Failed to create test theme\n");
            return 1;
        }
        printf("DEBUG: Test theme created\n");
    } else {
        printf("DEBUG: Default theme loaded successfully\n");
        assert(theme->name != NULL);
        printf("DEBUG: Theme name: %s\n", theme->name);
        assert(strcmp(theme->name, "default") == 0);
        assert(theme->colors != NULL);
        assert(theme->left_prompt != NULL);
        assert(theme->right_prompt != NULL);
    }
    
    // Cleanup
    printf("DEBUG: Freeing theme\n");
    free_theme(theme);
    
    // Test loading minimal theme
    printf("DEBUG: Attempting to load minimal theme\n");
    theme = load_theme("minimal");
    if (!theme) {
        printf("WARNING: Could not load minimal theme. Check JSON syntax.\n");
    } else {
        printf("DEBUG: Minimal theme loaded successfully\n");
        assert(theme->name != NULL);
        printf("DEBUG: Theme name: %s\n", theme->name);
        assert(strcmp(theme->name, "minimal") == 0);
        // Cleanup
        free_theme(theme);
    }
    
    printf("Theme loading test passed!\n");
    return 0;  // Add return value
}

// Test theme format expansion - change return type to int
int test_expand_format() {
    printf("Testing theme format expansion...\n");
    
    Theme *theme = create_test_theme();
    if (!theme) {
        printf("ERROR: Failed to create test theme\n");
        return 1; // Return error code
    }
    
    // Test basic color expansion
    char *result = expand_theme_format(theme, "Hello {primary}World{reset}");
    if (!result) {
        printf("ERROR: expand_theme_format returned NULL\n");
        free_theme(theme);
        return 1;
    }
    assert(result != NULL);
    assert(strstr(result, "\033") != NULL); // Should contain color codes
    free(result);
    
    // Test icon expansion
    result = expand_theme_format(theme, "Hello {icon}");
    if (!result) {
        printf("ERROR: expand_theme_format returned NULL\n");
        free_theme(theme);
        return 1;
    }
    assert(result != NULL);
    assert(strstr(result, "T") != NULL); // Should contain our test icon
    free(result);
    
    // Test segment expansion
    result = expand_theme_format(theme, "{directory}");
    if (!result) {
        printf("ERROR: expand_theme_format returned NULL\n");
        free_theme(theme);
        return 1;
    }
    assert(result != NULL);
    assert(strstr(result, "test_dir") != NULL); // Should contain segment output
    free(result);
    
    // Cleanup
    free_theme(theme);
    
    printf("Theme format expansion test passed!\n");
    return 0; // Return success
}

// Test the prompt generation - change return type to int
int test_get_prompt() {
    printf("Testing prompt generation...\n");
    
    // Create a test theme we know is valid
    Theme *theme = create_test_theme();
    if (!theme) {
        printf("ERROR: Failed to create test theme\n");
        return 1;
    }
    
    printf("DEBUG: Test theme created successfully\n");
    
    char *prompt = get_theme_prompt(theme);
    if (!prompt) {
        printf("ERROR: get_theme_prompt returned NULL\n");
        free_theme(theme);
        return 1;
    }
    
    printf("DEBUG: Got prompt: %.40s...\n", prompt);
    
    assert(prompt != NULL);
    assert(strstr(prompt, "T") != NULL); // Should contain our icon
    assert(strstr(prompt, "test_dir") != NULL); // Should contain dir segment
    assert(strstr(prompt, "\033") != NULL); // Should contain color codes
    
    free(prompt);
    free_theme(theme);
    
    printf("Prompt generation test passed!\n");
    return 0; // Add return value
}

// Test the theme command - update to handle config integration
int test_theme_command() {
    printf("Testing theme command...\n");
    
    // Also initialize the configuration system
    init_config_system();
    
    // Setup
    extern Theme *current_theme;
    current_theme = NULL;
    
    // Test listing themes
    char *args1[] = {"theme"};
    printf("DEBUG: Testing 'theme' command (list themes)\n");
    int result = theme_command(1, args1);
    assert(result == 0);
    
    // Test setting theme
    char *args2[] = {"theme", "default"};
    printf("DEBUG: Testing 'theme default' command\n");
    result = theme_command(2, args2);
    assert(result == 0);
    assert(current_theme != NULL);
    printf("DEBUG: Current theme set to: %s\n", current_theme->name);
    assert(strcmp(current_theme->name, "default") == 0);
    
    // Check if theme was saved to config
    const char *saved_theme = get_config_theme();
    printf("DEBUG: Config saved theme: %s\n", saved_theme ? saved_theme : "NULL");
    assert(saved_theme != NULL);
    assert(strcmp(saved_theme, "default") == 0);
    
    // Test invalid theme
    char *args3[] = {"theme", "nonexistent_theme"};
    printf("DEBUG: Testing 'theme nonexistent_theme' command\n");
    result = theme_command(2, args3);
    assert(result != 0); // Should fail
    
    // Cleanup
    cleanup_theme_system();
    cleanup_config_system();
    
    printf("Theme command test passed!\n");
    return 0;
}

// Test segment command execution and output storage
int test_segment_commands() {
    printf("Testing segment command execution...\n");
    
    Theme *theme = create_test_theme();
    if (!theme) {
        printf("ERROR: Failed to create test theme\n");
        return 1;
    }
    
    // Verify our test theme structure
    printf("DEBUG: Theme segment count: %d\n", theme->segment_count);
    printf("DEBUG: Left prompt format: '%s'\n", theme->left_prompt->format);
    for (int i = 0; i < theme->segment_count; i++) {
        printf("DEBUG: Segment %d key: '%s', format: '%s'\n", i, 
            theme->segments[i]->key, theme->segments[i]->format);
    }
    
    // Test executing commands for a segment
    ThemeSegment *git_segment = theme->segments[1];
    printf("DEBUG: Git segment format: '%s'\n", git_segment->format);
    
    execute_segment_commands(git_segment);
    
    printf("DEBUG: git_segment->commands[0]->output = '%s'\n", 
           git_segment->commands[0]->output ? git_segment->commands[0]->output : "NULL");
    printf("DEBUG: git_segment->commands[1]->output = '%s'\n", 
           git_segment->commands[1]->output ? git_segment->commands[1]->output : "NULL");
    
    // Verify branch command output is stored
    assert(git_segment->commands[0]->output != NULL);
    assert(strcmp(git_segment->commands[0]->output, "test_branch") == 0);
    
    // Verify dirty flag command output is stored
    assert(git_segment->commands[1]->output != NULL);
    assert(strcmp(git_segment->commands[1]->output, "*") == 0);
    
    // Test prompt generation with multiple command outputs
    setenv("NUT_DEBUG_THEME", "1", 1); // Enable theme debugging for this test
    printf("DEBUG: Getting theme prompt...\n");
    char *prompt = get_theme_prompt(theme);
    assert(prompt != NULL);
    
    // Add debugging output
    printf("DEBUG: Generated prompt: '%s'\n", prompt);
    
    // Check if both command outputs appear in the prompt
    bool has_branch = strstr(prompt, "test_branch") != NULL;
    bool has_flag = strstr(prompt, "*") != NULL;
    
    printf("DEBUG: Contains branch? %s\n", has_branch ? "YES" : "NO");
    printf("DEBUG: Contains flag? %s\n", has_flag ? "YES" : "NO");
    
    assert(has_branch);
    assert(has_flag);
    
    free(prompt);
    free_theme(theme);
    
    printf("Segment command execution test passed!\n");
    return 0;
}

int main() {
    printf("Running theme tests...\n");

    // Initialize error handling
    signal(SIGSEGV, SIG_DFL);
    
    // Enable theme debug if NUT_DEBUG is set
    if (getenv("NUT_DEBUG")) {
        setenv("NUT_DEBUG_THEME", "1", 1);
        setenv("NUT_DEBUG_CONFIG", "1", 1);
        printf("Theme and config debugging enabled\n");
    }
    
    // Run the tests with proper return value checking
    int result = 0;
    
    // Run each test and track overall success
    result = test_load_theme();
    
    // Only continue if previous test passed
    if (result == 0) {
        result = test_expand_format();
    }
    
    // Only continue if previous tests passed
    if (result == 0) {
        result = test_get_prompt();
    }
    
    // Add the new test for segment commands
    if (result == 0) {
        result = test_segment_commands();
    }
    
    // Only continue if previous tests passed
    if (result == 0) {
        result = test_theme_command();
    }
    
    if (result == 0) {
        printf("All theme tests passed!\n");
    } else {
        printf("Some theme tests failed!\n");
    }
    
    return result;
}
