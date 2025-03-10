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

// External declarations for functions we're testing
extern int ask_ai_command(int argc, char **argv);
extern int set_api_key_command(int argc, char **argv);
extern int explain_command(int argc, char **argv);
extern int fix_command(int argc, char **argv);

// Mock implementation functions 
char *mock_nl_to_command(const char *natural_language_query) {
    if (strstr(natural_language_query, "find all pdf files")) {
        return strdup("find . -name \"*.pdf\"");
    } 
    else if (strstr(natural_language_query, "list directory")) {
        return strdup("peekaboo");
    }
    return strdup("echo 'Command not understood'");
}

char *mock_explain_command_ai(const char *command) {
    if (strstr(command, "find . -name")) {
        return strdup("This command searches for files with names ending in .pdf in the current directory and all subdirectories.");
    }
    return strdup("This is a mock explanation for testing purposes.");
}

char *mock_suggest_fix(const char *command, const char *error, int exit_status) {
    // Prevent unused parameter warnings
    (void)error;
    (void)exit_status;
    
    if (strstr(command, "gti")) {
        return strdup("Explanation: You typed 'gti' instead of 'git'.\n\nCorrected command:\ngit status");
    } else if (strstr(command, "cat nonexistent")) {
        return strdup("Explanation: The file 'nonexistent' does not exist.\n\nCorrected command:\ntouch nonexistent && cat nonexistent");
    }
    return strdup("This is a mock fix suggestion for testing purposes.");
}

// Test natural language to command conversion
void test_nl_to_command() {
    printf("Testing natural language to command conversion...\n");
    
    // Set up the mock implementation
    set_ai_mock_functions(mock_nl_to_command, NULL);
    
    // Test with various inputs
    char *result = nl_to_command("find all pdf files in this directory");
    assert(result != NULL);
    assert(strcmp(result, "find . -name \"*.pdf\"") == 0);
    free(result);
    
    result = nl_to_command("list directory");
    assert(result != NULL);
    assert(strcmp(result, "peekaboo") == 0);
    free(result);
    
    printf("nl_to_command test passed\n");
}

// Test command explanation
void test_explain_command() {
    printf("Testing command explanation...\n");
    
    // Set up the mock implementation
    set_ai_mock_functions(NULL, mock_explain_command_ai);
    
    char *result = explain_command_ai("find . -name \"*.pdf\"");
    assert(result != NULL);
    assert(strstr(result, "searches for files") != NULL);
    free(result);
    
    printf("explain_command_ai test passed\n");
}

// Test ask_ai_command using our mocks
void test_ask_command() {
    printf("Testing ask AI command...\n");
    
    // Set up the mock implementation
    set_ai_mock_functions(mock_nl_to_command, NULL);
    
    // Setup test arguments
    char *args[] = {"ask", "find", "all", "pdf", "files"};
    
    // Redirect stdout to capture output
    FILE *original_stdout = stdout;
    char temp_file[] = "/tmp/nutshell_test_output_XXXXXX";
    int fd = mkstemp(temp_file);
    assert(fd != -1);
    
    FILE *temp_fp = fdopen(fd, "w+");
    assert(temp_fp != NULL);
    
    stdout = temp_fp;
    
    // Call the command
    int result = ask_ai_command(5, args);
    
    // Reset stdout
    fflush(stdout);
    stdout = original_stdout;
    
    // Read captured output
    rewind(temp_fp);
    char output[1024] = {0};
    size_t bytes_read = fread(output, 1, sizeof(output) - 1, temp_fp);
    output[bytes_read] = '\0';
    
    fclose(temp_fp);
    unlink(temp_file);
    
    // Verify
    assert(result == 0);
    assert(strstr(output, "find . -name \"*.pdf\"") != NULL);
    
    printf("ask_ai_command test passed\n");
}

// Test fix_command using our mocks
void test_fix_command() {
    printf("Testing fix command...\n");
    
    // Direct assignment since we can't use set_ai_mock_functions for this
    extern char* (*suggest_fix_impl)(const char*, const char*, int);
    suggest_fix_impl = mock_suggest_fix;
    
    // Set up command history for testing
    extern CommandHistory cmd_history;
    cmd_history.last_command = strdup("gti status");
    cmd_history.last_output = strdup("Command 'gti' not found");
    cmd_history.has_error = true;
    cmd_history.exit_status = 127;
    
    // Setup test arguments
    char *args[] = {"fix"};
    
    // Redirect stdout to capture output
    FILE *original_stdout = stdout;
    char temp_file[] = "/tmp/nutshell_test_output_XXXXXX";
    int fd = mkstemp(temp_file);
    assert(fd != -1);
    
    FILE *temp_fp = fdopen(fd, "w+");
    assert(temp_fp != NULL);
    
    stdout = temp_fp;
    
    // Call the command
    int result = fix_command(1, args);
    
    // Reset stdout
    fflush(stdout);
    stdout = original_stdout;
    
    // Read captured output
    rewind(temp_fp);
    char output[1024] = {0};
    size_t bytes_read = fread(output, 1, sizeof(output) - 1, temp_fp);
    output[bytes_read] = '\0';
    
    fclose(temp_fp);
    unlink(temp_file);
    
    // Verify
    assert(result == 0);
    assert(strstr(output, "Analyzing: gti status") != NULL);
    assert(strstr(output, "You typed 'gti' instead of 'git'") != NULL);
    
    // Clean up
    free(cmd_history.last_command);
    free(cmd_history.last_output);
    cmd_history.last_command = NULL;
    cmd_history.last_output = NULL;
    
    printf("fix_command test passed\n");
}

int main() {
    printf("Running OpenAI commands tests...\n");
    
    // Set testing mode
    setenv("NUTSHELL_TESTING", "1", 1);
    
    // Ensure we start with a clean state
    reset_api_key_for_testing();
    
    // Set up API key - we'll use a fake one since we're using mock functions
    set_api_key("test_api_key");
    
    // Run tests
    test_nl_to_command();
    test_explain_command();
    test_ask_command();
    test_fix_command();
    
    // Reset mocks to use real functions for cleanup
    set_ai_mock_functions(NULL, NULL);
    
    printf("All OpenAI commands tests passed!\n");
    return 0;
}
