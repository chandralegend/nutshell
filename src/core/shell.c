#define _GNU_SOURCE
#define RL_READLINE_VERSION 0x0603
#include <nutshell/core.h>
#include <nutshell/utils.h>
#include <nutshell/theme.h>
#include <nutshell/ai.h>  // Add this include to access AI functions
#include <nutshell/config.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>

// Function prototypes
char *expand_path(const char *path);
char *get_current_dir(void);
extern bool is_terminal_control_command(const char *cmd);

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

// Initialize command history
CommandHistory cmd_history = {NULL, NULL, 0, false};

// Function to store command output
void capture_command_output(const char *command, int exit_status, const char *output) {
    // Free previous entries
    free(cmd_history.last_command);
    free(cmd_history.last_output);
    
    // Store new entries
    cmd_history.last_command = strdup(command);
    cmd_history.last_output = output ? strdup(output) : NULL;
    cmd_history.exit_status = exit_status;
    cmd_history.has_error = (exit_status != 0);
    
    if (getenv("NUT_DEBUG")) {
        DEBUG_LOG("Stored command: %s", cmd_history.last_command);
        DEBUG_LOG("Exit status: %d", cmd_history.exit_status);
        DEBUG_LOG("Output: %.40s%s", cmd_history.last_output ? cmd_history.last_output : "(none)",
                  cmd_history.last_output && strlen(cmd_history.last_output) > 40 ? "..." : "");
    }
}

void shell_loop() {
    char *input;
    struct sigaction sa;
    
    // Initialize the configuration system first
    if (getenv("NUT_DEBUG")) {
        DEBUG_LOG("Initializing configuration system");
    }
    init_config_system();
    
    // Initialize the theme system
    if (getenv("NUT_DEBUG")) {
        DEBUG_LOG("Initializing theme system");
    }
    init_theme_system();
    
    // Load saved theme from config if available
    const char *saved_theme = get_config_theme();
    if (saved_theme && current_theme && strcmp(current_theme->name, saved_theme) != 0) {
        if (getenv("NUT_DEBUG")) {
            DEBUG_LOG("Loading saved theme from config: %s", saved_theme);
        }
        Theme *theme = load_theme(saved_theme);
        if (theme) {
            if (current_theme) {
                free_theme(current_theme);
            }
            current_theme = theme;
            if (getenv("NUT_DEBUG")) {
                DEBUG_LOG("Successfully loaded saved theme: %s", theme->name);
            }
        } else if (getenv("NUT_DEBUG")) {
            DEBUG_LOG("Failed to load saved theme: %s", saved_theme);
        }
    }
    
    // Initialize the AI shell integration
    init_ai_shell();
    
    sa.sa_handler = handle_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    printf("Nutshell initialized. Type commands or 'exit' to quit.\n");

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
                // Save the original command string for history regardless of how we process it
                char full_cmd[1024] = {0};
                for (int i = 0; cmd->args[i]; i++) {
                    if (i > 0) strcat(full_cmd, " ");
                    strcat(full_cmd, cmd->args[i]);
                }
                
                // Special handling for theme command
                if (cmd->args[0] && strcmp(cmd->args[0], "theme") == 0) {
                    // Count arguments
                    int argc = 0;
                    while (cmd->args[argc]) argc++;
                    
                    // Capture theme command output
                    char output_file[] = "/tmp/nutshell_output_XXXXXX";
                    int output_fd = mkstemp(output_file);
                    if (output_fd != -1) {
                        int stdout_bak = dup(STDOUT_FILENO);
                        int stderr_bak = dup(STDERR_FILENO);
                        
                        // Redirect stdout and stderr to temp file
                        dup2(output_fd, STDOUT_FILENO);
                        dup2(output_fd, STDERR_FILENO);
                        
                        int status = theme_command(argc, cmd->args);
                        
                        // Restore stdout and stderr
                        fflush(stdout);
                        fflush(stderr);
                        dup2(stdout_bak, STDOUT_FILENO);
                        dup2(stderr_bak, STDERR_FILENO);
                        close(stdout_bak);
                        close(stderr_bak);
                        
                        // Read the captured output
                        lseek(output_fd, 0, SEEK_SET);
                        char output_buf[4096] = {0};
                        read(output_fd, output_buf, sizeof(output_buf) - 1);
                        close(output_fd);
                        unlink(output_file);
                        
                        // Display the output
                        printf("%s", output_buf);
                        
                        // Store command history
                        capture_command_output(full_cmd, status, output_buf);
                    } else {
                        int status = theme_command(argc, cmd->args);
                        capture_command_output(full_cmd, status, NULL);
                    }
                } else if (handle_ai_command(cmd)) {
                    // For AI commands, just store the command without output capture
                    capture_command_output(full_cmd, 0, NULL);
                    
                    // We can still capture stderr if needed in the future
                    // by uncommenting and implementing this code:
                    /*
                    char output_file[] = "/tmp/nutshell_output_XXXXXX";
                    int output_fd = mkstemp(output_file);
                    if (output_fd != -1) {
                        // Implementation would go here
                        close(output_fd);
                        unlink(output_file);
                    }
                    */
                } else if (cmd->args[0] && is_terminal_control_command(cmd->args[0])) {
                    // For terminal control commands, execute directly but capture output where possible
                    FILE *fp = NULL;
                    
                    // Create the full command with arguments
                    char full_terminal_cmd[1024] = {0};
                    for (int i = 0; cmd->args[i]; i++) {
                        if (i > 0) strcat(full_terminal_cmd, " ");
                        strcat(full_terminal_cmd, cmd->args[i]);
                    }
                    
                    // Try to capture any error output
                    fp = popen(full_terminal_cmd, "r");
                    if (fp) {
                        // Execute the command directly for terminal interaction
                        system(full_terminal_cmd);
                        
                        // Read any output that might have been captured
                        char output_buf[4096] = {0};
                        if (fread(output_buf, 1, sizeof(output_buf) - 1, fp) > 0) {
                            capture_command_output(full_cmd, 0, output_buf);
                        } else {
                            capture_command_output(full_cmd, 0, NULL);
                        }
                        int status = pclose(fp);
                        if (status != 0) {
                            capture_command_output(full_cmd, WEXITSTATUS(status), NULL);
                        }
                    } else {
                        // Fall back to direct execution
                        execute_command(cmd);
                        capture_command_output(full_cmd, 0, NULL);
                    }
                } else {
                    // For regular commands, track execution and capture output
                    
                    // Capture stdout and stderr for error tracking
                    char output_file[] = "/tmp/nutshell_output_XXXXXX";
                    int output_fd = mkstemp(output_file);
                    if (output_fd != -1) {
                        int stdout_bak = dup(STDOUT_FILENO);
                        int stderr_bak = dup(STDERR_FILENO);
                        
                        // Redirect stdout and stderr to temp file
                        dup2(output_fd, STDOUT_FILENO);
                        dup2(output_fd, STDERR_FILENO);
                        
                        // Execute the command
                        execute_command(cmd);
                        int exit_status = WEXITSTATUS(0); // Get last command status
                        
                        // Restore stdout and stderr
                        fflush(stdout);
                        fflush(stderr);
                        dup2(stdout_bak, STDOUT_FILENO);
                        dup2(stderr_bak, STDERR_FILENO);
                        close(stdout_bak);
                        close(stderr_bak);
                        
                        // Read the captured output
                        lseek(output_fd, 0, SEEK_SET);
                        char output_buf[4096] = {0};
                        read(output_fd, output_buf, sizeof(output_buf) - 1);
                        close(output_fd);
                        unlink(output_file);
                        
                        // IMPORTANT: Display the output to the user
                        printf("%s", output_buf);
                        
                        // Store command history
                        capture_command_output(full_cmd, exit_status, output_buf);
                    } else {
                        // Couldn't create temp file, execute normally
                        execute_command(cmd);
                        
                        // Still store command but without output
                        capture_command_output(full_cmd, 0, NULL);
                    }
                }
                free_parsed_command(cmd);
            }
        }
        
        free(input);
    }
    
    // Clean up command history
    free(cmd_history.last_command);
    free(cmd_history.last_output);
    
    // Clean up theme system
    cleanup_theme_system();
    
    // Clean up configuration system
    cleanup_config_system();
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