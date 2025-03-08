#define _GNU_SOURCE
#define RL_READLINE_VERSION 0x0603
#include <nutshell/core.h>
#include <nutshell/utils.h>
#include <nutshell/theme.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>

// Function prototypes
char *expand_path(const char *path);
char *get_current_dir(void);

volatile sig_atomic_t sigint_received = 0;

void handle_sigint(int sig) {
    // Unused parameter
    (void)sig;
    
    // Simpler approach without using rl_replace_line
    sigint_received = 1;
    printf("\n");
    rl_on_new_line();
    rl_redisplay();
}

extern int install_pkg_command(int argc, char **argv);
extern int theme_command(int argc, char **argv);

void shell_loop() {
    char *input;
    struct sigaction sa;
    
    // Initialize the theme system
    init_theme_system();
    
    sa.sa_handler = handle_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    while (1) {
        sigint_received = 0;
        char *prompt = get_prompt();
        input = readline(prompt);
        free(prompt);

        if (!input) break;  // EOF
        
        if (strlen(input) > 0) {
            add_history(input);
            ParsedCommand *cmd = parse_command(input);
            if (cmd) {
                // Special handling for theme command
                if (cmd->args[0] && strcmp(cmd->args[0], "theme") == 0) {
                    // Count arguments
                    int argc = 0;
                    while (cmd->args[argc]) argc++;
                    theme_command(argc, cmd->args);
                } else {
                    execute_command(cmd);
                }
                free_parsed_command(cmd);
            }
        }
        
        free(input);
    }
    
    // Clean up theme system
    cleanup_theme_system();
}

char *get_prompt() {
    // Use the theme system if available
    if (current_theme) {
        return get_theme_prompt(current_theme);
    }
    
    // Fall back to default prompt
    static char prompt[256];
    snprintf(prompt, sizeof(prompt), 
            "\001\033[1;32m\002🥜 %s \001\033[0m\002➜ ",
            get_current_dir());
    return strdup(prompt);
}