#include <nutshell/core.h>
#include <ctype.h>
#include <string.h>
#include <nutshell/utils.h>

// Debug flag - set to 1 to enable debug output
#define DEBUG_PARSER 1

// Function prototype
char* trim_whitespace(char* str);

ParsedCommand *parse_command(char *input) {
    if (!input) return NULL;
    
    if (DEBUG_PARSER) {
        fprintf(stderr, "DEBUG: Parsing command: '%s'\n", input);
    }
    
    // Make a copy of the input to avoid modifying the original
    char *input_copy = strdup(input);
    if (!input_copy) return NULL;
    
    input_copy = trim_whitespace(input_copy);
    if (strlen(input_copy) == 0) {
        if (DEBUG_PARSER) fprintf(stderr, "DEBUG: Empty command after trimming\n");
        free(input_copy);
        return NULL;
    }
    
    ParsedCommand *cmd = calloc(1, sizeof(ParsedCommand));
    if (!cmd) {
        if (DEBUG_PARSER) fprintf(stderr, "DEBUG: Failed to allocate ParsedCommand\n");
        free(input_copy);
        return NULL;
    }
    
    // Use calloc to ensure all entries are initialized to NULL
    cmd->args = calloc(MAX_ARGS, sizeof(char *));
    if (!cmd->args) {
        if (DEBUG_PARSER) fprintf(stderr, "DEBUG: Failed to allocate args array\n");
        free(input_copy);
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
                if (DEBUG_PARSER) fprintf(stderr, "DEBUG: Input file: %s\n", cmd->input_file);
            } else {
                if (DEBUG_PARSER) fprintf(stderr, "DEBUG: Missing input file after <\n");
            }
        } else if (strcmp(token, ">") == 0) {
            token = strtok_r(NULL, " \t", &saveptr);
            if (token) {
                cmd->output_file = strdup(token);
                if (DEBUG_PARSER) fprintf(stderr, "DEBUG: Output file: %s\n", cmd->output_file);
            } else {
                if (DEBUG_PARSER) fprintf(stderr, "DEBUG: Missing output file after >\n");
            }
        } else if (strcmp(token, "&") == 0) {
            cmd->background = true;
            if (DEBUG_PARSER) fprintf(stderr, "DEBUG: Background process\n");
        } else {
            // Regular argument
            cmd->args[arg_count] = strdup(token);
            if (DEBUG_PARSER) fprintf(stderr, "DEBUG: Arg[%d] = '%s'\n", arg_count, cmd->args[arg_count]);
            arg_count++;
        }
        
        // Get next token
        token = strtok_r(NULL, " \t", &saveptr);
    }
    
    // Ensure NULL termination
    cmd->args[arg_count] = NULL;
    
    if (DEBUG_PARSER) {
        fprintf(stderr, "DEBUG: Command parsed with %d arguments\n", arg_count);
    }
    
    free(input_copy);
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