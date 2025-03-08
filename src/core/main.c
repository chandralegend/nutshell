#include <nutshell/core.h>
#include <nutshell/utils.h>
#include <nutshell/theme.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
    // Initialize the command registry
    init_registry();
    
    // Check if we're running in test mode
    if (argc > 1 && strcmp(argv[1], "--test") == 0) {
        printf("Running in test mode\n");
    } else {
        // Start the shell loop only in normal mode
        shell_loop();
    }
    
    // Free resources before exit
    free_registry();
    
    return 0;
}
