#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE

#include <nutshell/theme.h>
#include <jansson.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>  // Add this include for struct stat, stat() function, and S_ISREG macro
#include <nutshell/config.h>

// Add this helper macro at the top of the file
#define THEME_DEBUG(fmt, ...) \
    do { if (getenv("NUT_DEBUG_THEME")) fprintf(stderr, "THEME: " fmt "\n", ##__VA_ARGS__); } while(0)

// Current theme
Theme *current_theme = NULL;

// Initialize the theme system
void init_theme_system() {
    // Try to load the default theme
    current_theme = load_theme("default");
    
    // If default theme failed, try minimal theme
    if (!current_theme) {
        current_theme = load_theme("minimal");
    }
    
    // If all themes failed, create a minimal hardcoded theme
    if (!current_theme) {
        fprintf(stderr, "Warning: Could not load any themes, using fallback\n");
        
        current_theme = calloc(1, sizeof(Theme));
        if (!current_theme) return;
        
        current_theme->name = strdup("fallback");
        current_theme->description = strdup("Fallback theme when no themes can be loaded");
        
        // Basic colors
        current_theme->colors = calloc(1, sizeof(ThemeColors));
        current_theme->colors->reset = strdup("\001\033[0m\002");
        current_theme->colors->primary = strdup("\001\033[1;32m\002");
        current_theme->colors->secondary = strdup("\001\033[1;34m\002");
        current_theme->colors->error = strdup("\001\033[1;31m\002");
        current_theme->colors->warning = strdup("\001\033[1;33m\002");
        current_theme->colors->info = strdup("\001\033[1;36m\002");
        current_theme->colors->success = strdup("\001\033[1;32m\002");
        
        // Simple prompt
        current_theme->left_prompt = calloc(1, sizeof(PromptConfig));
        current_theme->left_prompt->format = strdup("{primary}🥜 {dir}{reset}");
        current_theme->left_prompt->icon = NULL;
        
        current_theme->right_prompt = calloc(1, sizeof(PromptConfig));
        current_theme->right_prompt->format = strdup("");
        
        current_theme->prompt_symbol = strdup("➜ ");
        current_theme->prompt_symbol_color = strdup("primary");
        current_theme->multiline = false;
        
        // Directory segment
        current_theme->segment_count = 1;
        current_theme->segments = calloc(1, sizeof(ThemeSegment*));
        current_theme->segments[0] = calloc(1, sizeof(ThemeSegment));
        current_theme->segments[0]->enabled = true;
        current_theme->segments[0]->format = strdup("{dir}");
        
        // Create command array structure instead of single command
        current_theme->segments[0]->command_count = 1;
        current_theme->segments[0]->commands = calloc(2, sizeof(ThemeCommand*)); // +1 for NULL terminator
        current_theme->segments[0]->commands[0] = calloc(1, sizeof(ThemeCommand));
        current_theme->segments[0]->commands[0]->name = strdup("dir");
        current_theme->segments[0]->commands[0]->command = strdup("pwd | sed \"s|$HOME|~|\"");
        current_theme->segments[0]->commands[0]->output = NULL;
        current_theme->segments[0]->commands[1] = NULL;
    }
}

// Helper to replace strings
char *str_replace(const char *str, const char *find, const char *replace) {
    if (!str || !find || !replace) return NULL;
    
    size_t str_len = strlen(str);
    size_t find_len = strlen(find);
    size_t replace_len = strlen(replace);
    
    // Count occurrences of find in str
    size_t count = 0;
    const char *tmp = str;
    while ((tmp = strstr(tmp, find)) != NULL) {
        count++;
        tmp += find_len;
    }
    
    // Calculate new string length
    size_t result_len = str_len + (replace_len - find_len) * count + 1;
    
    // Allocate new string
    char *result = malloc(result_len);
    if (!result) return NULL;
    
    // Copy with replacements
    char *cur = result;
    const char *remainder = str;
    tmp = str;
    
    while ((tmp = strstr(remainder, find)) != NULL) {
        // Copy up to occurrence
        size_t prefix_len = tmp - remainder;
        memcpy(cur, remainder, prefix_len);
        cur += prefix_len;
        
        // Copy replacement
        memcpy(cur, replace, replace_len);
        cur += replace_len;
        
        // Update remainder
        remainder = tmp + find_len;
    }
    
    // Copy remainder
    strcpy(cur, remainder);
    
    return result;
}

// Load theme from JSON file
Theme *load_theme(const char *theme_name) {
    if (!theme_name) {
        THEME_DEBUG("theme_name is NULL");
        return NULL;
    }
    THEME_DEBUG("Attempting to load theme: %s", theme_name);
    
    char theme_path[256];
    snprintf(theme_path, sizeof(theme_path), "./themes/%s.json", theme_name);
    THEME_DEBUG("Trying path: %s", theme_path);
    
    FILE *file = fopen(theme_path, "r");
    if (!file) {
        THEME_DEBUG("File not found at %s, trying user directory", theme_path);
        // Try user directory
        char *home = getenv("HOME");
        if (home) {
            snprintf(theme_path, sizeof(theme_path), "%s/.nutshell/themes/%s.json", home, theme_name);
            THEME_DEBUG("Trying user path: %s", theme_path);
            file = fopen(theme_path, "r");
        }
    }
    
    if (!file) {
        THEME_DEBUG("Theme file not found: %s", theme_path);
        return NULL;
    }
    
    THEME_DEBUG("Theme file opened successfully");
    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    THEME_DEBUG("File length: %ld bytes", length);
    char *data = malloc(length + 1);
    if (!data) {
        THEME_DEBUG("Failed to allocate memory for file data");
        fclose(file);
        return NULL;
    }
    
    fread(data, 1, length, file);
    data[length] = '\0';
    fclose(file);
    
    THEME_DEBUG("File contents loaded, parsing JSON");
    
    // Print first few characters to ensure it's valid JSON
    THEME_DEBUG("First 40 chars: %.40s...", data);
    
    json_error_t error;
    // Use JSON_DECODE_ANY to prevent errors with escape sequences
    json_t *root = json_loads(data, JSON_DECODE_ANY, &error);
    
    if (!root) {
        THEME_DEBUG("JSON parse error: %s (line: %d, col: %d, pos: %d)", 
               error.text, error.line, error.column, error.position);
        free(data);
        return NULL;
    }
    
    THEME_DEBUG("JSON parsed successfully");
    free(data);
    
    Theme *theme = calloc(1, sizeof(Theme));
    if (!theme) {
        THEME_DEBUG("Failed to allocate memory for theme");
        json_decref(root);
        return NULL;
    }
    
    THEME_DEBUG("Loading theme properties");
    
    // Extract basic properties with safety checks
    json_t *name_json = json_object_get(root, "name");
    json_t *desc_json = json_object_get(root, "description");
    json_t *colors_json = json_object_get(root, "colors");
    json_t *prompt_json = json_object_get(root, "prompt");
    json_t *segments_json = json_object_get(root, "segments");
    
    if (!name_json || !json_is_string(name_json)) {
        THEME_DEBUG("Missing or invalid 'name' property");
        free(theme);
        json_decref(root);
        return NULL;
    }
    
    theme->name = strdup(json_string_value(name_json));
    theme->description = desc_json && json_is_string(desc_json) ? 
                         strdup(json_string_value(desc_json)) : 
                         strdup("No description available");
    
    // Load colors with safety checks
    theme->colors = calloc(1, sizeof(ThemeColors));
    if (colors_json && json_is_object(colors_json)) {
        json_t *reset = json_object_get(colors_json, "reset");
        json_t *primary = json_object_get(colors_json, "primary");
        json_t *secondary = json_object_get(colors_json, "secondary");
        json_t *error = json_object_get(colors_json, "error");
        json_t *warning = json_object_get(colors_json, "warning");
        json_t *info = json_object_get(colors_json, "info");
        json_t *success = json_object_get(colors_json, "success");
        
        theme->colors->reset = reset && json_is_string(reset) ? 
                              strdup(json_string_value(reset)) : 
                              strdup("\001\033[0m\002");
        theme->colors->primary = primary && json_is_string(primary) ? 
                                strdup(json_string_value(primary)) : 
                                strdup("\001\033[1;32m\002");
        theme->colors->secondary = secondary && json_is_string(secondary) ? 
                                  strdup(json_string_value(secondary)) : 
                                  strdup("\001\033[1;34m\002");
        theme->colors->error = error && json_is_string(error) ? 
                              strdup(json_string_value(error)) : 
                              strdup("\001\033[1;31m\002");
        theme->colors->warning = warning && json_is_string(warning) ? 
                                strdup(json_string_value(warning)) : 
                                strdup("\001\033[1;33m\002");
        theme->colors->info = info && json_is_string(info) ? 
                            strdup(json_string_value(info)) : 
                            strdup("\001\033[1;36m\002");
        theme->colors->success = success && json_is_string(success) ? 
                               strdup(json_string_value(success)) : 
                               strdup("\001\033[1;32m\002");
    } else {
        // Default colors if not specified
        theme->colors->reset = strdup("\001\033[0m\002");
        theme->colors->primary = strdup("\001\033[1;32m\002");
        theme->colors->secondary = strdup("\001\033[1;34m\002");
        theme->colors->error = strdup("\001\033[1;31m\002");
        theme->colors->warning = strdup("\001\033[1;33m\002");
        theme->colors->info = strdup("\001\033[1;36m\002");
        theme->colors->success = strdup("\001\033[1;32m\002");
    }
    
    // Load prompt configuration with safety checks
    theme->left_prompt = calloc(1, sizeof(PromptConfig));
    if (prompt_json && json_is_object(prompt_json)) {
        json_t *left_prompt = json_object_get(prompt_json, "left");
        
        if (left_prompt && json_is_object(left_prompt)) {
            json_t *left_format = json_object_get(left_prompt, "format");
            json_t *left_icon = json_object_get(left_prompt, "icon");
            
            theme->left_prompt->format = left_format && json_is_string(left_format) ? 
                                        strdup(json_string_value(left_format)) : 
                                        strdup("{primary}> {reset}");
            theme->left_prompt->icon = left_icon && json_is_string(left_icon) ? 
                                      strdup(json_string_value(left_icon)) : 
                                      strdup("🥜");
        } else {
            theme->left_prompt->format = strdup("{primary}> {reset}");
            theme->left_prompt->icon = strdup("🥜");
        }
        
        // Right prompt with safety checks
        theme->right_prompt = calloc(1, sizeof(PromptConfig));
        json_t *right_prompt = json_object_get(prompt_json, "right");
        
        if (right_prompt && json_is_object(right_prompt)) {
            json_t *right_format = json_object_get(right_prompt, "format");
            
            theme->right_prompt->format = right_format && json_is_string(right_format) ? 
                                         strdup(json_string_value(right_format)) : 
                                         strdup("");
        } else {
            theme->right_prompt->format = strdup("");
        }
        
        // Get multiline and prompt symbol settings
        json_t *multiline = json_object_get(prompt_json, "multiline");
        json_t *prompt_symbol = json_object_get(prompt_json, "prompt_symbol");
        json_t *prompt_symbol_color = json_object_get(prompt_json, "prompt_symbol_color");
        
        theme->multiline = multiline ? json_is_true(multiline) : false;
        theme->prompt_symbol = prompt_symbol && json_is_string(prompt_symbol) ? 
                              strdup(json_string_value(prompt_symbol)) : 
                              strdup("$ ");
        theme->prompt_symbol_color = prompt_symbol_color && json_is_string(prompt_symbol_color) ? 
                                    strdup(json_string_value(prompt_symbol_color)) : 
                                    strdup("primary");
    } else {
        // Default prompt settings if not specified
        theme->left_prompt->format = strdup("{primary}> {reset}");
        theme->left_prompt->icon = strdup("🥜");
        
        theme->right_prompt = calloc(1, sizeof(PromptConfig));
        theme->right_prompt->format = strdup("");
        
        theme->multiline = false;
        theme->prompt_symbol = strdup("$ ");
        theme->prompt_symbol_color = strdup("primary");
    }
    
    // Load segments with safety checks
    if (segments_json && json_is_object(segments_json)) {
        theme->segment_count = json_object_size(segments_json);
        theme->segments = calloc(theme->segment_count + 1, sizeof(ThemeSegment *)); // +1 for NULL safety
        
        const char *key;
        json_t *value;
        int i = 0;
        json_object_foreach(segments_json, key, value) {
            if (value && json_is_object(value) && i < theme->segment_count) {
                ThemeSegment *segment = calloc(1, sizeof(ThemeSegment));
                if (!segment) continue;
                
                // Store the segment key from JSON directly
                segment->key = strdup(key);
                THEME_DEBUG("Loading segment with key: %s", segment->key);
                
                json_t *enabled = json_object_get(value, "enabled");
                json_t *format = json_object_get(value, "format");
                
                segment->enabled = enabled ? json_is_true(enabled) : true;
                segment->format = format && json_is_string(format) ? 
                                strdup(json_string_value(format)) : 
                                strdup("");
                
                // Handle both old and new command formats
                json_t *command = json_object_get(value, "command");
                json_t *commands = json_object_get(value, "commands");
                
                if (commands && json_is_object(commands)) {
                    // New format with multiple commands
                    segment->command_count = json_object_size(commands);
                    segment->commands = calloc(segment->command_count + 1, sizeof(ThemeCommand*));
                    
                    const char *cmd_key;
                    json_t *cmd_value;
                    int j = 0;
                    
                    json_object_foreach(commands, cmd_key, cmd_value) {
                        if (cmd_value && json_is_string(cmd_value) && j < segment->command_count) {
                            ThemeCommand *cmd = calloc(1, sizeof(ThemeCommand));
                            if (!cmd) continue;
                            
                            cmd->name = strdup(cmd_key);
                            cmd->command = strdup(json_string_value(cmd_value));
                            cmd->output = NULL; // Will be filled when executed
                            
                            segment->commands[j++] = cmd;
                        }
                    }
                    segment->commands[j] = NULL; // NULL terminate
                } else if (command && json_is_string(command)) {
                    // Old format with a single command - create with key "default"
                    segment->command_count = 1;
                    segment->commands = calloc(2, sizeof(ThemeCommand*)); // +1 for NULL terminator
                    segment->commands[0] = calloc(1, sizeof(ThemeCommand));
                    
                    segment->commands[0]->name = strdup("default");
                    segment->commands[0]->command = strdup(json_string_value(command));
                    segment->commands[0]->output = NULL;
                    segment->commands[1] = NULL; // NULL terminate
                } else {
                    // No command specified
                    segment->command_count = 1;
                    segment->commands = calloc(2, sizeof(ThemeCommand*));
                    segment->commands[0] = calloc(1, sizeof(ThemeCommand));
                    
                    segment->commands[0]->name = strdup("default");
                    segment->commands[0]->command = strdup("echo ''");
                    segment->commands[0]->output = NULL;
                    segment->commands[1] = NULL;
                }
                
                theme->segments[i++] = segment;
            }
        }
        theme->segments[i] = NULL; // NULL terminate the array
    } else {
        // Default segment if none specified
        theme->segment_count = 1;
        theme->segments = calloc(2, sizeof(ThemeSegment*)); // +1 for NULL terminator
        theme->segments[0] = calloc(1, sizeof(ThemeSegment));
        theme->segments[0]->enabled = true;
        theme->segments[0]->format = strdup("{directory}");
        
        // Create single default command
        theme->segments[0]->command_count = 1;
        theme->segments[0]->commands = calloc(2, sizeof(ThemeCommand*));
        theme->segments[0]->commands[0] = calloc(1, sizeof(ThemeCommand));
        theme->segments[0]->commands[0]->name = strdup("directory");
        theme->segments[0]->commands[0]->command = strdup("pwd | sed \"s|$HOME|~|\"");
        theme->segments[0]->commands[0]->output = NULL;
        theme->segments[0]->commands[1] = NULL;
    }
    
    THEME_DEBUG("Theme loaded successfully: %s", theme->name);
    json_decref(root);
    return theme;
}

// Free theme resources
void free_theme(Theme *theme) {
    if (!theme) return;
    
    free(theme->name);
    free(theme->description);
    
    free(theme->colors->reset);
    free(theme->colors->primary);
    free(theme->colors->secondary);
    free(theme->colors->error);
    free(theme->colors->warning);
    free(theme->colors->info);
    free(theme->colors->success);
    free(theme->colors);
    
    free(theme->left_prompt->format);
    free(theme->left_prompt->icon);
    free(theme->left_prompt);
    
    free(theme->right_prompt->format);
    free(theme->right_prompt);
    
    free(theme->prompt_symbol);
    free(theme->prompt_symbol_color);
    
    for (int i = 0; i < theme->segment_count; i++) {
        free(theme->segments[i]->key);  // Free the segment key
        free(theme->segments[i]->format);
        
        // Free all commands in the segment
        for (int j = 0; j < theme->segments[i]->command_count; j++) {
            if (theme->segments[i]->commands[j]) {
                free(theme->segments[i]->commands[j]->name);
                free(theme->segments[i]->commands[j]->command);
                free(theme->segments[i]->commands[j]->output);
                free(theme->segments[i]->commands[j]);
            }
        }
        free(theme->segments[i]->commands);
        free(theme->segments[i]);
    }
    free(theme->segments);
    
    free(theme);
}

// Execute all commands for a segment and store the results
void execute_segment_commands(ThemeSegment *segment) {
    if (!segment || !segment->commands) return;
    
    for (int i = 0; i < segment->command_count; i++) {
        ThemeCommand *cmd = segment->commands[i];
        if (!cmd || !cmd->command) continue;
        
        // Free any previous output
        free(cmd->output);
        cmd->output = NULL;
        
        // Execute the command
        FILE *fp = popen(cmd->command, "r");
        if (!fp) continue;
        
        char buffer[256] = {0};
        if (fgets(buffer, sizeof(buffer), fp)) {
            // Remove trailing newline
            buffer[strcspn(buffer, "\n")] = 0;
            if (strlen(buffer) > 0) {
                cmd->output = strdup(buffer);
                THEME_DEBUG("Command '%s' output: '%s'", cmd->name, cmd->output);
            }
        }
        pclose(fp);
    }
}

// Get theme prompt
char *get_theme_prompt(Theme *theme) {
    if (!theme) return NULL;
    
    THEME_DEBUG("Generating prompt from template: %s", theme->left_prompt->format);
    
    // First process the left prompt
    char *left_prompt = strdup(theme->left_prompt->format);
    if (!left_prompt) return NULL;
    
    // Replace basic placeholders like colors and icon
    // Replace icon if present
    if (theme->left_prompt->icon) {
        char *temp = left_prompt;
        left_prompt = str_replace(temp, "{icon}", theme->left_prompt->icon);
        free(temp);
    }
    
    // Replace prompt symbol
    char *temp = left_prompt;
    left_prompt = str_replace(temp, "{prompt_symbol}", theme->prompt_symbol);
    free(temp);
    
    // Replace color codes
    temp = left_prompt;
    left_prompt = str_replace(temp, "{reset}", theme->colors->reset);
    free(temp);
    
    temp = left_prompt;
    left_prompt = str_replace(temp, "{primary}", theme->colors->primary);
    free(temp);
    
    temp = left_prompt;
    left_prompt = str_replace(temp, "{secondary}", theme->colors->secondary);
    free(temp);
    
    temp = left_prompt;
    left_prompt = str_replace(temp, "{error}", theme->colors->error);
    free(temp);
    
    temp = left_prompt;
    left_prompt = str_replace(temp, "{warning}", theme->colors->warning);
    free(temp);
    
    temp = left_prompt;
    left_prompt = str_replace(temp, "{info}", theme->colors->info);
    free(temp);
    
    temp = left_prompt;
    left_prompt = str_replace(temp, "{success}", theme->colors->success);
    free(temp);
    
    // Extract all segment placeholders from the prompt format
    THEME_DEBUG("Extracting segments from prompt format");
    
    // Process all segments in the theme definition
    for (int i = 0; i < theme->segment_count; i++) {
        ThemeSegment *segment = theme->segments[i];
        if (!segment || !segment->enabled || !segment->key) continue;
        
        // Create the placeholder from the segment key that was stored during load
        char placeholder[256];
        snprintf(placeholder, sizeof(placeholder), "{%s}", segment->key);
        THEME_DEBUG("Checking for placeholder: %s", placeholder);
        
        // Only process if the placeholder exists in the prompt
        if (strstr(left_prompt, placeholder)) {
            THEME_DEBUG("Processing segment: %s", segment->key);
            
            // Execute all commands for this segment to get live data
            execute_segment_commands(segment);
            
            // Create formatted segment with all replaced values
            char *formatted_segment = strdup(segment->format);
            if (!formatted_segment) continue;
            
            // Replace color codes first
            char *temp;
            temp = formatted_segment;
            formatted_segment = str_replace(temp, "{primary}", theme->colors->primary);
            free(temp);
            
            temp = formatted_segment;
            formatted_segment = str_replace(temp, "{secondary}", theme->colors->secondary);
            free(temp);
            
            temp = formatted_segment;
            formatted_segment = str_replace(temp, "{reset}", theme->colors->reset);
            free(temp);
            
            temp = formatted_segment;
            formatted_segment = str_replace(temp, "{info}", theme->colors->info);
            free(temp);
            
            temp = formatted_segment;
            formatted_segment = str_replace(temp, "{warning}", theme->colors->warning);
            free(temp);
            
            temp = formatted_segment;
            formatted_segment = str_replace(temp, "{error}", theme->colors->error);
            free(temp);
            
            temp = formatted_segment;
            formatted_segment = str_replace(temp, "{success}", theme->colors->success);
            free(temp);
            
            // Replace command outputs in the segment format
            bool has_output = false;
            for (int j = 0; j < segment->command_count; j++) {
                ThemeCommand *cmd = segment->commands[j];
                if (!cmd || !cmd->name || !cmd->output) continue;
                
                THEME_DEBUG("Command %s output: %s", cmd->name, cmd->output);
                
                // Replace {command_name} with its output
                char cmd_placeholder[256];
                snprintf(cmd_placeholder, sizeof(cmd_placeholder), "{%s}", cmd->name);
                
                temp = formatted_segment;
                formatted_segment = str_replace(temp, cmd_placeholder, cmd->output);
                free(temp);
                
                has_output = true;
            }
            
            // Only replace in the prompt if there's actual content
            if (has_output) {
                THEME_DEBUG("Replacing placeholder %s with: %s", placeholder, formatted_segment);
                
                temp = left_prompt;
                left_prompt = str_replace(temp, placeholder, formatted_segment);
                free(temp);
            } else {
                // Otherwise remove the placeholder
                temp = left_prompt;
                left_prompt = str_replace(temp, placeholder, "");
                free(temp);
            }
            
            free(formatted_segment);
        }
    }

    // Append the prompt symbol if it's not already in the format
    if (!strstr(theme->left_prompt->format, "{prompt_symbol}")) {
        char *prompt_color = NULL;
        
        // Get the color for the prompt symbol
        if (strcmp(theme->prompt_symbol_color, "primary") == 0) {
            prompt_color = theme->colors->primary;
        } else if (strcmp(theme->prompt_symbol_color, "secondary") == 0) {
            prompt_color = theme->colors->secondary;
        } else if (strcmp(theme->prompt_symbol_color, "error") == 0) {
            prompt_color = theme->colors->error;
        } else if (strcmp(theme->prompt_symbol_color, "warning") == 0) {
            prompt_color = theme->colors->warning;
        } else if (strcmp(theme->prompt_symbol_color, "info") == 0) {
            prompt_color = theme->colors->info;
        } else if (strcmp(theme->prompt_symbol_color, "success") == 0) {
            prompt_color = theme->colors->success;
        } else {
            prompt_color = theme->colors->reset;
        }
        
        // Create colored prompt symbol
        char colored_symbol[256];
        snprintf(colored_symbol, sizeof(colored_symbol), "%s%s%s ", 
                prompt_color, theme->prompt_symbol, theme->colors->reset);
        
        // Append to the prompt
        char *final_prompt = malloc(strlen(left_prompt) + strlen(colored_symbol) + 1);
        if (final_prompt) {
            sprintf(final_prompt, "%s%s", left_prompt, colored_symbol);
            free(left_prompt);
            left_prompt = final_prompt;
        }
    }
    
    THEME_DEBUG("Final prompt: %s", left_prompt);
    return left_prompt;
}

// Get segment output
char *get_segment_output(Theme *theme, const char *segment_name) {
    if (!theme || !segment_name) return NULL;
    
    for (int i = 0; i < theme->segment_count; i++) {
        ThemeSegment *segment = theme->segments[i];
        // Search through all commands in the segment
        for (int j = 0; j < segment->command_count; j++) {
            if (segment->commands[j] && strcmp(segment->commands[j]->command, segment_name) == 0) {
                FILE *fp = popen(segment->commands[j]->command, "r");
                if (!fp) return NULL;
                
                char buffer[128];
                fgets(buffer, sizeof(buffer), fp);
                pclose(fp);
                
                // Remove newline
                buffer[strcspn(buffer, "\n")] = '\0';
                
                return strdup(buffer);
            }
        }
    }
    
    return NULL;
}

// Replace placeholders in format string
char *expand_theme_format(Theme *theme, const char *format) {
    if (!theme || !format) return NULL;
    
    char *result = strdup(format);
    if (!result) return NULL;
    
    // Handle NULL parameters safely throughout the function
    char *temp = NULL;
    
    // Replace color codes with safety checks
    if (result && theme->colors) {
        if (theme->colors->primary && strstr(result, "{primary}")) {
            temp = result;
            result = str_replace(temp, "{primary}", theme->colors->primary);
            free(temp);
        }
        
        if (theme->colors->secondary && strstr(result, "{secondary}")) {
            temp = result;
            result = str_replace(temp, "{secondary}", theme->colors->secondary);
            free(temp);
        }
        
        if (theme->colors->error && strstr(result, "{error}")) {
            temp = result;
            result = str_replace(temp, "{error}", theme->colors->error);
            free(temp);
        }
        
        if (theme->colors->warning && strstr(result, "{warning}")) {
            temp = result;
            result = str_replace(temp, "{warning}", theme->colors->warning);
            free(temp);
        }
        
        if (theme->colors->info && strstr(result, "{info}")) {
            temp = result;
            result = str_replace(temp, "{info}", theme->colors->info);
            free(temp);
        }
        
        if (theme->colors->success && strstr(result, "{success}")) {
            temp = result;
            result = str_replace(temp, "{success}", theme->colors->success);
            free(temp);
        }
        
        if (theme->colors->reset && strstr(result, "{reset}")) {
            temp = result;
            result = str_replace(temp, "{reset}", theme->colors->reset);
            free(temp);
        }
    }
    
    // Replace icon placeholder
    if (result && theme->left_prompt->icon && strstr(result, "{icon}")) {
        temp = result;
        result = str_replace(temp, "{icon}", theme->left_prompt->icon);
        free(temp);
    }
    
    // Replace segment placeholders
    for (int i = 0; i < theme->segment_count; i++) {
        ThemeSegment *segment = theme->segments[i];
        if (!segment || !segment->enabled) continue;
        
        // Extract segment name from format
        char segment_name[64] = {0};
        
        if (strncmp(segment->format, "{directory}", 11) == 0) {
            strcpy(segment_name, "directory");
        } else if (strncmp(segment->format, "{git_branch}", 12) == 0) {
            strcpy(segment_name, "git_branch");
        } else {
            // Extract name between { and }
            const char *start = strchr(segment->format, '{');
            const char *end = strchr(segment->format, '}');
            if (start && end && end > start) {
                size_t name_len = end - start - 1;
                strncpy(segment_name, start + 1, name_len);
                segment_name[name_len] = '\0';
            }
        }
        
        // Check if this segment is referenced in the format
        char placeholder[128];
        snprintf(placeholder, sizeof(placeholder), "{%s}", segment_name);
        
        if (result && strstr(result, placeholder)) {
            // Use first command as default for backward compatibility
            char *segment_output = NULL;
            if (segment->commands && segment->commands[0]) {
                segment_output = get_segment_output(theme, segment->commands[0]->command);
            }
            
            if (segment_output && strlen(segment_output) > 0) {
                // Replace placeholders in segment output
                char *output = strdup(segment->format);
                temp = output;
                
                // Replace the segment value in the format
                output = str_replace(temp, placeholder, segment_output);
                free(temp);
                
                // Replace the placeholder in the result
                temp = result;
                result = str_replace(temp, placeholder, output);
                free(temp);
                free(output);
                free(segment_output);
            } else {
                // If no output, remove the placeholder
                temp = result;
                result = str_replace(temp, placeholder, "");
                free(temp);
            }
        }
    }
    
    return result;
}

// Theme command implementation
int theme_command(int argc, char **argv) {
    if (argc < 2) {
        // List available themes
        printf("Available themes:\n");
        
        // Check themes directory for available themes
        DIR *dir;
        struct dirent *ent;
        char themes_dir[256];
        
        // First check in user home
        char *home = getenv("HOME");
        if (home) {
            snprintf(themes_dir, sizeof(themes_dir), "%s/.nutshell/themes", home);
            dir = opendir(themes_dir);
            if (dir) {
                while ((ent = readdir(dir)) != NULL) {
                    // Skip . and .. entries
                    if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
                        continue;
                        
                    // Build the full path
                    char full_path[512];
                    struct stat st;
                    snprintf(full_path, sizeof(full_path), "%s/%s", themes_dir, ent->d_name);
                    
                    // Check if it's a regular file with .json extension
                    if (stat(full_path, &st) == 0 && S_ISREG(st.st_mode) && strstr(ent->d_name, ".json")) {
                        // Remove .json extension
                        char theme_name[128];
                        strncpy(theme_name, ent->d_name, sizeof(theme_name));
                        char *dot = strrchr(theme_name, '.');
                        if (dot) *dot = '\0';
                        
                        // Mark current theme
                        if (current_theme && strcmp(current_theme->name, theme_name) == 0) {
                            printf("* %s\n", theme_name);
                        } else {
                            printf("  %s\n", theme_name);
                        }
                    }
                }
                closedir(dir);
            }
        }
        
        // Then check system themes
        dir = opendir("./themes");
        if (dir) {
            while ((ent = readdir(dir)) != NULL) {
                // Skip . and .. entries
                if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
                    continue;
                    
                // Build the full path
                char full_path[512];
                struct stat st;
                snprintf(full_path, sizeof(full_path), "./themes/%s", ent->d_name);
                
                // Check if it's a regular file with .json extension
                if (stat(full_path, &st) == 0 && S_ISREG(st.st_mode) && strstr(ent->d_name, ".json")) {
                    // Remove .json extension
                    char theme_name[128];
                    strncpy(theme_name, ent->d_name, sizeof(theme_name));
                    char *dot = strrchr(theme_name, '.');
                    if (dot) *dot = '\0';
                    
                    // Mark current theme
                    if (current_theme && strcmp(current_theme->name, theme_name) == 0) {
                        printf("* %s\n", theme_name);
                    } else {
                        printf("  %s\n", theme_name);
                    }
                }
            }
            closedir(dir);
        }
        
        return 0;
    }
    
    // Change theme
    const char *theme_name = argv[1];
    
    // Clean up old theme
    if (current_theme) {
        free_theme(current_theme);
    }
    
    // Load new theme
    current_theme = load_theme(theme_name);
    if (!current_theme) {
        fprintf(stderr, "Failed to load theme: %s\n", theme_name);
        return 1;
    }
    
    // Save theme selection to configuration
    if (set_config_theme(theme_name)) {
        THEME_DEBUG("Theme selection saved to config");
    } else {
        THEME_DEBUG("Failed to save theme selection to config");
    }
    
    printf("Theme changed to: %s\n", theme_name);
    return 0;
}

// Free resources when program exits
void cleanup_theme_system() {
    if (current_theme) {
        free_theme(current_theme);
        current_theme = NULL;
    }
}

// Helper function to convert \u escape sequences to proper ANSI sequences
char *convert_color_escapes(const char *input) {
    if (!input) return NULL;
    
    char *result = strdup(input);
    char *temp;
    
    // Convert Unicode escape sequences to ASCII escape sequences
    if (strstr(result, "\\u001b")) {
        temp = result;
        result = str_replace(temp, "\\u001b", "\033");
        free(temp);
    }
    
    // Also handle simple ASCII escape notation
    if (strstr(result, "\\033")) {
        temp = result;
        result = str_replace(temp, "\\033", "\033");
        free(temp);
    }
    
    return result;
}