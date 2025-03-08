#include <nutshell/utils.h>
#include <nutshell/core.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <pwd.h>
#include <unistd.h>

// String utilities
char *trim_whitespace(char *str) {
    if (!str) return NULL;
    
    // Trim leading space
    while (isspace((unsigned char)*str)) str++;
    
    if (*str == 0) return str;  // All spaces
    
    // Trim trailing space
    char *end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    
    // Write new null terminator
    *(end + 1) = '\0';
    
    return str;
}

char **split_string(const char *input, const char *delim, int *count) {
    if (!input || !delim || !count) return NULL;
    
    *count = 0;
    char *copy = strdup(input);
    if (!copy) return NULL;
    
    // First pass: count tokens
    char *token, *saveptr;
    token = strtok_r(copy, delim, &saveptr);
    while (token) {
        (*count)++;
        token = strtok_r(NULL, delim, &saveptr);
    }
    
    // Allocate result array
    char **result = malloc((*count + 1) * sizeof(char *));
    if (!result) {
        free(copy);
        return NULL;
    }
    
    // Second pass: store tokens
    free(copy);
    copy = strdup(input);
    if (!copy) {
        free(result);
        return NULL;
    }
    
    token = strtok_r(copy, delim, &saveptr);
    for (int i = 0; i < *count; i++) {
        result[i] = strdup(token);
        token = strtok_r(NULL, delim, &saveptr);
    }
    result[*count] = NULL;
    
    free(copy);
    return result;
}

// File utilities
bool file_exists(const char *path) {
    struct stat buffer;
    return (stat(path, &buffer) == 0);
}

char *expand_path(const char *path) {
    if (!path) return NULL;
    
    static char result[1024];
    
    if (path[0] == '~') {
        struct passwd *pw = getpwuid(getuid());
        if (pw) {
            snprintf(result, sizeof(result), "%s%s", pw->pw_dir, path + 1);
            return result;
        }
    }
    
    // Handle relative paths if needed
    if (path[0] != '/') {
        char cwd[1024];
        if (getcwd(cwd, sizeof(cwd))) {
            snprintf(result, sizeof(result), "%s/%s", cwd, path);
            return result;
        }
    }
    
    return strdup(path);
}

// Error handling
void print_error(const char *msg) {
    fprintf(stderr, "\033[1;31mError: %s\033[0m\n", msg);
}

void print_success(const char *msg) {
    fprintf(stdout, "\033[1;32m%s\033[0m\n", msg);
}

// For shell.c
char *get_current_dir() {
    static char cwd[1024];
    if (!getcwd(cwd, sizeof(cwd))) {
        strcpy(cwd, "unknown");
    }
    return cwd;
}

bool sanitize_command(const char *cmd) {
    if (!cmd) return false;
    
    // List of disallowed commands for security
    const char *blocked_commands[] = {
        "rm -rf /", "rm -rf /*", ":(){ :|:& };:", "dd", 
        NULL
    };
    
    // Check against blocked commands
    for (int i = 0; blocked_commands[i]; i++) {
        if (strcmp(cmd, blocked_commands[i]) == 0) {
            return false;
        }
    }
    
    return true;
}
