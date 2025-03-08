#include <nutshell/core.h>
#include <nutshell/utils.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// Test fixture for the parser
void test_basic_parsing() {
    printf("Testing basic command parsing...\n");
    
    char test_cmd[] = "echo hello world";
    ParsedCommand *cmd = parse_command(test_cmd);
    
    assert(cmd != NULL);
    assert(cmd->args != NULL);
    assert(cmd->args[0] != NULL);
    assert(strcmp(cmd->args[0], "echo") == 0);
    assert(cmd->args[1] != NULL);
    assert(strcmp(cmd->args[1], "hello") == 0);
    assert(cmd->args[2] != NULL);
    assert(strcmp(cmd->args[2], "world") == 0);
    assert(cmd->args[3] == NULL);
    assert(cmd->input_file == NULL);
    assert(cmd->output_file == NULL);
    assert(cmd->background == 0);
    
    free_parsed_command(cmd);
    printf("Basic parsing test passed!\n");
}

void test_redirection() {
    printf("Testing redirection parsing...\n");
    
    char test_cmd[] = "cat file.txt > output.txt";
    ParsedCommand *cmd = parse_command(test_cmd);
    
    assert(cmd != NULL);
    assert(cmd->args != NULL);
    assert(cmd->args[0] != NULL);
    assert(strcmp(cmd->args[0], "cat") == 0);
    assert(cmd->args[1] != NULL);
    assert(strcmp(cmd->args[1], "file.txt") == 0);
    assert(cmd->args[2] == NULL);
    assert(cmd->output_file != NULL);
    assert(strcmp(cmd->output_file, "output.txt") == 0);
    
    free_parsed_command(cmd);
    printf("Redirection test passed!\n");
}

void test_null_input() {
    printf("Testing NULL input handling...\n");
    
    ParsedCommand *cmd = parse_command(NULL);
    assert(cmd == NULL);
    
    printf("NULL input test passed!\n");
}

int main() {
    printf("Running parser tests...\n");
    
    // Initialize anything needed for tests
    init_registry();
    
    // Run the tests
    test_basic_parsing();
    test_redirection();
    test_null_input();
    
    // Clean up
    free_registry();
    
    printf("All parser tests passed!\n");
    return 0;
}
