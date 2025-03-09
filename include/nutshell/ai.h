#ifndef NUTSHELL_AI_H
#define NUTSHELL_AI_H

#include <stdbool.h>

// Initialize AI integration
bool init_ai_integration();

// Convert natural language to shell command
char *nl_to_command(const char *natural_language_query);

// Get command explanation
char *explain_command_ai(const char *command);

// Get command suggestions based on context
char *suggest_commands(const char *context);

// Cleanup AI resources
void cleanup_ai_integration();

// Configuration
void set_api_key(const char *key);
bool has_api_key();

// Testing support - function pointer types
typedef char* (*NlToCommandFunc)(const char*);
typedef char* (*ExplainCommandFunc)(const char*);

// Set mock implementations for testing
void set_ai_mock_functions(NlToCommandFunc nl_func, ExplainCommandFunc explain_func);

// Function to reset API key state for testing
void reset_api_key_for_testing();

#endif // NUTSHELL_AI_H
