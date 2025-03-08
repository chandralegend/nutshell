#include <nutshell/core.h>
#include <ctype.h>
#include <string.h>
#include <nutshell/utils.h>

// Function prototype
char* trim_whitespace(char* str);

ParsedCommand *parse_command(char *input) {
    if (!input) return NULL;
    
    input = trim_whitespace(input);
    ParsedCommand *cmd = malloc(sizeof(ParsedCommand));
    if (!cmd) return NULL;
    
    cmd->args = malloc(MAX_ARGS * sizeof(char *));
    if (!cmd->args) {
        free(cmd);
        return NULL;
    }
    
    cmd->input_file = cmd->output_file = NULL;
    cmd->background = 0;
    
    int arg_count = 0;
    char *token, *saveptr;
    
    // Simpler tokenization approach
    token = strtok_r(input, " \t", &saveptr);
    while (token != NULL && arg_count < MAX_ARGS - 1) {
        if (strcmp(token, "<") == 0) {
            token = strtok_r(NULL, " \t", &saveptr);
            if (!token) {
                DEBUG_LOG("Error: Missing input file after <");
                break;
            }
            cmd->input_file = strdup(token);
        } else if (strcmp(token, ">") == 0) {
            token = strtok_r(NULL, " \t", &saveptr);
            if (!token) {
                DEBUG_LOG("Error: Missing output file after >");
                break;
            }
            cmd->output_file = strdup(token);
        } else if (strcmp(token, "&") == 0) {
            cmd->background = 1;
        } else {
            cmd->args[arg_count++] = strdup(token);
        }
        token = strtok_r(NULL, " \t", &saveptr);
    }
    cmd->args[arg_count] = NULL;
    
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