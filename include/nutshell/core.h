#ifndef NUTSHELL_CORE_H
#define NUTSHELL_CORE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <readline/readline.h>

#define MAX_ARGS 64
#define MAX_CMD_LEN 1024
#define PROMPT_MAX 256

typedef struct CommandMapping {
    char *unix_cmd;
    char *nut_cmd;
    bool is_builtin;
} CommandMapping;

typedef struct CommandRegistry {
    CommandMapping *commands;
    size_t count;
} CommandRegistry;

typedef struct ParsedCommand {
    char **args;
    char *input_file;
    char *output_file;
    bool background;
} ParsedCommand;

// Registry functions
void init_registry();
void register_command(const char *unix_cmd, const char *nut_cmd, bool is_builtin);
const CommandMapping *find_command(const char *input_cmd);
void free_registry();

// Parser functions
ParsedCommand *parse_command(char *input);
void free_parsed_command(ParsedCommand *cmd);

// Executor functions
void execute_command(ParsedCommand *cmd);

// Shell core
void shell_loop();
char *get_prompt();
void handle_sigint(int sig);

// Add to the function declarations section
void load_packages_from_dir(const char *dir_path);

#endif // NUTSHELL_CORE_H