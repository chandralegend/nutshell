#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE

#include <nutshell/core.h>
#include <nutshell/ai.h>
#include <nutshell/utils.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

// Mock API key for testing
#define TEST_API_KEY "test_api_key_12345"

// External declaration for the function we're using from openai.c
extern int set_api_key_command(int argc, char **argv);

// Test API key management
void test_api_key_functions() {
    printf("Testing API key functions...\n");
    
    // Initially no key should be set
    assert(has_api_key() == false);
    
    // Set a key and verify
    set_api_key(TEST_API_KEY);
    assert(has_api_key() == true);
    
    // Check that key is persisted in home directory
    char key_path[512];
    char *home = getenv("HOME");
    snprintf(key_path, sizeof(key_path), "%s/.nutshell/openai_key", home);
    
    FILE *key_file = fopen(key_path, "r");
    assert(key_file != NULL);
    
    char buffer[256];
    fgets(buffer, sizeof(buffer), key_file);
    buffer[strcspn(buffer, "\n")] = 0;  // Remove newline
    
    assert(strcmp(buffer, TEST_API_KEY) == 0);
    fclose(key_file);
    
    printf("API key functions test passed\n");
}

// Test init and cleanup
void test_init_and_cleanup() {
    printf("Testing AI initialization and cleanup...\n");
    
    // Set a key first to ensure init will succeed
    set_api_key(TEST_API_KEY);
    
    // Initialize AI
    bool init_result = init_ai_integration();
    assert(init_result == true);
    
    // Cleanup should not crash
    cleanup_ai_integration();
    
    // After cleanup, key should be reset
    assert(has_api_key() == false);
    
    printf("Initialization and cleanup test passed\n");
}

// Test API key command (built-in command for setting API key)
void test_api_key_command() {
    printf("Testing set-api-key command...\n");
    
    // Clean up any existing key
    cleanup_ai_integration();
    
    // Test with valid args
    char *args[] = {"set-api-key", "new_test_key_6789"};
    int result = set_api_key_command(2, args);
    assert(result == 0);
    assert(has_api_key() == true);
    
    // Test with invalid args (too few)
    char *invalid_args[] = {"set-api-key"};
    result = set_api_key_command(1, invalid_args);
    assert(result != 0);  // Should return error code
    
    printf("set-api-key command test passed\n");
}

int main() {
    printf("Running AI integration tests...\n");
    
    // Set testing mode
    setenv("NUTSHELL_TESTING", "1", 1);
    
    // Make sure we start with no API key
    reset_api_key_for_testing();
    
    // Initialize test environment
    // Create .nutshell directory if it doesn't exist
    char *home = getenv("HOME");
    if (home) {
        char dir_path[512];
        snprintf(dir_path, sizeof(dir_path), "%s/.nutshell", home);
        mkdir(dir_path, 0755);
        
        // Remove any existing API key file to ensure clean test state
        char key_path[512];
        snprintf(key_path, sizeof(key_path), "%s/.nutshell/openai_key", home);
        unlink(key_path);
    }
    
    // Run tests
    test_api_key_functions();
    test_init_and_cleanup();
    test_api_key_command();
    
    printf("All AI integration tests passed!\n");
    return 0;
}
