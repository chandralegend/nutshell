#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE

#include <nutshell/core.h>
#include <nutshell/ai.h>
#include <nutshell/utils.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// External declarations for functions we're testing
extern void register_ai_commands();
extern bool handle_ai_command(ParsedCommand *cmd);

// Test AI command registration
void test_ai_command_registration() {
    printf("Testing AI command registration...\n");
    
    // Initialize registry
    init_registry();
    
    // Register AI commands
    register_ai_commands();
    
    // Check if AI commands are registered correctly
    const CommandMapping *set_api_key_cmd = find_command("set-api-key");
    const CommandMapping *ask_cmd = find_command("ask");
    const CommandMapping *explain_cmd = find_command("explain");
    
    assert(set_api_key_cmd != NULL);
    assert(ask_cmd != NULL);
    assert(explain_cmd != NULL);
    
    assert(set_api_key_cmd->is_builtin == true);
    assert(ask_cmd->is_builtin == true);
    assert(explain_cmd->is_builtin == true);
    
    // Cleanup
    free_registry();
    
    printf("AI command registration test passed\n");
}

// Test handling AI commands
void test_ai_command_handling() {
    printf("Testing AI command handling...\n");
    
    // Create a parsed command for the set-api-key command
    ParsedCommand *cmd = calloc(1, sizeof(ParsedCommand));
    cmd->args = calloc(3, sizeof(char*));
    cmd->args[0] = strdup("set-api-key");
    cmd->args[1] = strdup("test_key");
    cmd->args[2] = NULL;
    
    // Test handling set-api-key command
    bool handled = handle_ai_command(cmd);
    assert(handled == true);
    
    // Clean up
    free_parsed_command(cmd);
    
    // Check that other commands aren't handled
    cmd = calloc(1, sizeof(ParsedCommand));
    cmd->args = calloc(2, sizeof(char*));
    cmd->args[0] = strdup("not_an_ai_command");
    cmd->args[1] = NULL;
    
    handled = handle_ai_command(cmd);
    assert(handled == false);
    
    free_parsed_command(cmd);
    
    printf("AI command handling test passed\n");
}

// Test integration with executor
void test_ai_integration_with_executor() {
    printf("Testing AI integration with executor...\n");
    
    // Create a test AI command
    ParsedCommand *cmd = calloc(1, sizeof(ParsedCommand));
    cmd->args = calloc(4, sizeof(char*));
    cmd->args[0] = strdup("ask");
    cmd->args[1] = strdup("list");
    cmd->args[2] = strdup("directory");
    cmd->args[3] = NULL;
    
    // Initialize registry and AI commands
    init_registry();
    register_ai_commands();
    
    // Verify command is found in registry
    const CommandMapping *ask_cmd = find_command("ask");
    assert(ask_cmd != NULL);
    
    // Cleanup
    free_parsed_command(cmd);
    free_registry();
    
    printf("AI integration with executor test passed\n");
}

int main() {
    printf("Running AI shell integration tests...\n");
    
    // Run tests
    test_ai_command_registration();
    test_ai_command_handling();
    test_ai_integration_with_executor();
    
    printf("All AI shell integration tests passed!\n");
    return 0;
}
