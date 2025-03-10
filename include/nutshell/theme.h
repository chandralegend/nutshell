#ifndef NUTSHELL_THEME_H
#define NUTSHELL_THEME_H

#include <stdbool.h>

// Command with its output
typedef struct {
    char *name;      // Name of the command (key)
    char *command;   // Command to execute
    char *output;    // Cached output
} ThemeCommand;

// Theme segment structure
typedef struct {
    bool enabled;
    char *key;        // Add this field to store the segment name from JSON
    char *format;
    int command_count;
    ThemeCommand **commands;  // Array of commands for this segment
} ThemeSegment;

// Theme color mapping
typedef struct {
    char *reset;
    char *primary;
    char *secondary;
    char *error;
    char *warning;
    char *info;
    char *success;
} ThemeColors;

// Prompt configuration
typedef struct {
    char *format;
    char *icon;
} PromptConfig;

// Overall theme structure
typedef struct {
    char *name;
    char *description;
    ThemeColors *colors;
    PromptConfig *left_prompt;
    PromptConfig *right_prompt;
    bool multiline;
    char *prompt_symbol;
    char *prompt_symbol_color;
    ThemeSegment **segments;
    int segment_count;
} Theme;

// Theme management functions
void init_theme_system();
void cleanup_theme_system();
Theme *load_theme(const char *theme_name);
void free_theme(Theme *theme);
char *get_theme_prompt(Theme *theme);
char *expand_theme_format(Theme *theme, const char *format);
char *get_segment_output(Theme *theme, const char *segment_name);
void execute_segment_commands(ThemeSegment *segment);  // Added this function declaration

// Builtin theme command
int theme_command(int argc, char **argv);

// Current theme
extern Theme *current_theme;

#endif // NUTSHELL_THEME_H
