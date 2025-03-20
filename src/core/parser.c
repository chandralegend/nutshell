#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE

#include <nutshell/core.h>
#include <ctype.h>
#include <string.h>
#include <nutshell/utils.h>

// Replace the debug flag with a macro that uses environment variable
#define PARSER_DEBUG(fmt, ...) \
    do { if (getenv("NUT_DEBUG_PARSER")) fprintf(stderr, "PARSER: " fmt "\n", ##__VA_ARGS__); } while(0)

// Function prototype
char* trim_whitespace(char* str);

ParsedCommand *parse_command(char *input) {
    if (!input) return NULL;
    
    PARSER_DEBUG("Parsing command: '%s'", input);
    
    // Make a copy of the input to avoid modifying the original
    char *input_copy = strdup(input);
    if (!input_copy) return NULL;
    
    char *original_input_copy = input_copy;
    input_copy = trim_whitespace(input_copy);
    if (strlen(input_copy) == 0) {
        PARSER_DEBUG("Empty command after trimming");
        free(original_input_copy);
        original_input_copy = NULL;
        return NULL;
    }
    
    ParsedCommand *cmd = calloc(1, sizeof(ParsedCommand));
    if (!cmd) {
        PARSER_DEBUG("Failed to allocate ParsedCommand");
        free(original_input_copy);
        original_input_copy = NULL;
        return NULL;
    }
    
    // Use calloc to ensure all entries are initialized to NULL
    cmd->args = calloc(MAX_ARGS, sizeof(char *));
    if (!cmd->args) {
        PARSER_DEBUG("Failed to allocate args array");
        free(original_input_copy);
        original_input_copy = NULL;
        free(cmd);
        return NULL;
    }
    int arg_count = 0;
    char *token, *saveptr = NULL;
    // Tokenize and process input
    token = strtok_r(input_copy, " \t", &saveptr);
    while (token != NULL && arg_count < MAX_ARGS - 1) {
        // Check if token is a special character
        if (strcmp(token, "<") == 0) {
            token = strtok_r(NULL, " \t", &saveptr);
            if (token) {
                cmd->input_file = strdup(token);
                PARSER_DEBUG("Input file: %s", cmd->input_file);
            } else {
                PARSER_DEBUG("Missing input file after <");
            }
        } else if (strcmp(token, ">") == 0) {
            token = strtok_r(NULL, " \t", &saveptr);
            if (token) {
                cmd->output_file = strdup(token);
                PARSER_DEBUG("Output file: %s", cmd->output_file);
            } else {
                PARSER_DEBUG("Missing output file after >");
            }
        } else if (strcmp(token, "&") == 0) {
            cmd->background = true;
            PARSER_DEBUG("Background process");
        } else {
            // Regular argument
            cmd->args[arg_count] = strdup(token);
            PARSER_DEBUG("Arg[%d] = '%s'", arg_count, cmd->args[arg_count]);
            arg_count++;
        }
        
        // Get next token
        token = strtok_r(NULL, " \t", &saveptr);
    }
    // Ensure NULL termination
    cmd->args[arg_count] = NULL;
    
    PARSER_DEBUG("Command parsed with %d arguments", arg_count);

    if(original_input_copy){
        free(original_input_copy);
        original_input_copy = NULL;
    }


    return cmd;
}

void free_parsed_command(ParsedCommand *cmd) {
    if (!cmd) return;
    
    if (cmd->args) {
        for (int i = 0; cmd->args[i]; i++) 
            free(cmd->args[i]);
        free(cmd->args);
    }
    
    free(cmd->input_file);
    free(cmd->output_file);
    free(cmd);
}
