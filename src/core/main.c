#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE

#include <nutshell/core.h>
#include <nutshell/utils.h>
#include <nutshell/theme.h>
#include <nutshell/ai.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Define version information
#define NUTSHELL_VERSION "0.0.4"
#define NUTSHELL_RELEASE_DATE "March 2025"

// External declaration for AI shell initialization
extern void init_ai_shell();

// Display version information
void print_version() {
    printf("Nutshell Shell v%s (%s)\n", NUTSHELL_VERSION, NUTSHELL_RELEASE_DATE);
}

// Display help information
void print_usage() {
    printf("Usage: nutshell [OPTIONS]\n\n");
    printf("An enhanced Unix shell with simplified command language, package management, and AI assistance.\n\n");
    printf("Options:\n");
    printf("  --help        Display this help message and exit\n");
    printf("  --version     Display version information and exit\n");
    printf("  --test        Run in test mode (for internal testing)\n\n");
    printf("Environment variables:\n");
    printf("  NUT_DEBUG=1                 Enable general debug output\n");
    printf("  OPENAI_API_KEY=<your_key>   Set API key for AI features\n");
    printf("  NUT_DEBUG_THEME=1           Enable theme system debugging\n");
    printf("  NUT_DEBUG_CONFIG=1          Enable config system debugging\n\n");
    printf("Documentation: https://github.com/chandralegend/nutshell\n");
}

int main(int argc, char *argv[]) {
    // Check for command line arguments
    if (argc > 1) {
        if (strcmp(argv[1], "--version") == 0) {
            print_version();
            return 0;
        }
        else if (strcmp(argv[1], "--help") == 0) {
            print_usage();
            return 0;
        }
        else if (strcmp(argv[1], "--test") == 0) {
            printf("Running in test mode\n");
            // Continue with initialization but don't start shell loop
        }
        else {
            printf("Unknown option: %s\n", argv[1]);
            print_usage();
            return 1;
        }
    }

    // Initialize the command registry
    init_registry();
    
    // Initialize AI integration
    init_ai_shell();
    
    // Only start the shell loop in normal mode (not test mode)
    if (argc <= 1 || (argc > 1 && strcmp(argv[1], "--test") != 0)) {
        shell_loop();
    }
    
    // Free resources before exit
    free_registry();
    cleanup_ai_integration();
    
    return 0;
}
