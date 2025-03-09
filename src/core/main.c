#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE

#include <nutshell/core.h>
#include <nutshell/utils.h>
#include <nutshell/theme.h>
#include <nutshell/ai.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// External declaration for AI shell initialization
extern void init_ai_shell();

int main(int argc, char *argv[]) {
    // Initialize the command registry
    init_registry();
    
    // Initialize AI integration
    init_ai_shell();
    
    // Check if we're running in test mode
    if (argc > 1 && strcmp(argv[1], "--test") == 0) {
        printf("Running in test mode\n");
    } else {
        // Start the shell loop only in normal mode
        shell_loop();
    }
    
    // Free resources before exit
    free_registry();
    cleanup_ai_integration();
    
    return 0;
}
