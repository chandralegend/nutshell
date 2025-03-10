#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE

#include <nutshell/core.h>
#include <nutshell/ai.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Define debug macro for AI shell integration
#define AI_SHELL_DEBUG(fmt, ...) \
    do { if (getenv("NUT_DEBUG_AI_SHELL")) fprintf(stderr, "AI_SHELL: " fmt "\n", ##__VA_ARGS__); } while(0)

// External declarations for AI commands
extern int set_api_key_command(int argc, char **argv);
extern int ask_ai_command(int argc, char **argv);
extern int explain_command(int argc, char **argv);
extern int fix_command(int argc, char **argv);  // Add the new command

// Register AI commands with the shell
void register_ai_commands() {
    AI_SHELL_DEBUG("Registering AI commands");
    
    // Register the AI commands
    register_command("set-api-key", "set-api-key", true);
    register_command("ask", "ask", true);
    register_command("explain", "explain", true);
    register_command("fix", "fix", true);  // Register the new command
    
    AI_SHELL_DEBUG("AI commands registered successfully");
}

// Initialize AI integration for the shell
void init_ai_shell() {
    AI_SHELL_DEBUG("Initializing AI shell integration");
    
    // Register commands
    register_ai_commands();
    
    // Initialize AI systems
    bool success = init_ai_integration();
    
    if (success) {
        AI_SHELL_DEBUG("AI shell integration initialized successfully");
    } else {
        AI_SHELL_DEBUG("AI shell integration initialization failed - continuing without AI features");
    }
}

// Update the shell loop function to handle AI commands
bool handle_ai_command(ParsedCommand *cmd) {
    if (!cmd || !cmd->args[0]) return false;
    
    AI_SHELL_DEBUG("Checking if '%s' is an AI command", cmd->args[0]);
    
    if (strcmp(cmd->args[0], "set-api-key") == 0) {
        AI_SHELL_DEBUG("Handling set-api-key command");
        // Count arguments
        int argc = 0;
        while (cmd->args[argc]) argc++;
        bool result = set_api_key_command(argc, cmd->args) == 0;
        AI_SHELL_DEBUG("set-api-key command %s", result ? "succeeded" : "failed");
        return result;
    }
    else if (strcmp(cmd->args[0], "ask") == 0) {
        AI_SHELL_DEBUG("Handling ask command");
        // Count arguments
        int argc = 0;
        while (cmd->args[argc]) argc++;
        bool result = ask_ai_command(argc, cmd->args) == 0;
        AI_SHELL_DEBUG("ask command %s", result ? "succeeded" : "failed");
        return result;
    }
    else if (strcmp(cmd->args[0], "explain") == 0) {
        AI_SHELL_DEBUG("Handling explain command");
        // Count arguments
        int argc = 0;
        while (cmd->args[argc]) argc++;
        bool result = explain_command(argc, cmd->args) == 0;
        AI_SHELL_DEBUG("explain command %s", result ? "succeeded" : "failed");
        return result;
    }
    else if (strcmp(cmd->args[0], "fix") == 0) {  // Add handling for the new command
        AI_SHELL_DEBUG("Handling fix command");
        // Count arguments
        int argc = 0;
        while (cmd->args[argc]) argc++;
        bool result = fix_command(argc, cmd->args) == 0;
        AI_SHELL_DEBUG("fix command %s", result ? "succeeded" : "failed");
        return result;
    }
    
    AI_SHELL_DEBUG("'%s' is not an AI command", cmd->args[0]);
    return false;
}
