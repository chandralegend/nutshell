#ifndef NUTSHELL_UTILS_H
#define NUTSHELL_UTILS_H

#include <sys/stat.h>
#include "core.h"

// Error handling
void print_error(const char *msg);
void print_success(const char *msg);

// File utilities
bool file_exists(const char *path);
char *expand_path(const char *path);

// String utilities
char **split_string(const char *input, const char *delim, int *count);
char *trim_whitespace(char *str);
char *str_replace(const char *str, const char *find, const char *replace);

// Security utilities
bool sanitize_command(const char *cmd);
bool is_safe_path(const char *path);

// Configuration utilities
void load_config(const char *path);
void reload_config();

// Helper macros
#define DEBUG_LOG(fmt, ...) \
    do { if (getenv("NUT_DEBUG")) fprintf(stderr, "DEBUG: " fmt "\n", ##__VA_ARGS__); } while(0)

#define NUT_ASSERT(expr) \
    do { \
        if (!(expr)) { \
            fprintf(stderr, "Assertion failed: %s (%s:%d)\n", #expr, __FILE__, __LINE__); \
            abort(); \
        } \
    } while(0)

#endif // NUTSHELL_UTILS_H