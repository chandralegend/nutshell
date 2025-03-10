#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE

#include <nutshell/ai.h>
#include <nutshell/utils.h>
#include <curl/curl.h>
#include <jansson.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

// Define debug macro for AI specific logging
#define AI_DEBUG(fmt, ...) \
    do { if (getenv("NUT_DEBUG_AI")) fprintf(stderr, "AI: " fmt "\n", ##__VA_ARGS__); } while(0)

// Define API endpoint
#define OPENAI_API_URL "https://api.openai.com/v1/chat/completions"
#define OPENAI_MODEL "gpt-3.5-turbo"

// API Key storage
static char *api_key = NULL;

// Memory structure for CURL responses
struct MemoryStruct {
    char *memory;
    size_t size;
};

// Callback for CURL to write responses to memory
static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t real_size = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;
    
    char *ptr = realloc(mem->memory, mem->size + real_size + 1);
    if (!ptr) {
        print_error("Not enough memory for AI response");
        return 0;
    }
    
    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, real_size);
    mem->size += real_size;
    mem->memory[mem->size] = 0;
    
    return real_size;
}

// Check if API key exists
bool has_api_key() {
    // When in test mode, don't check environment variables or file
    if (getenv("NUTSHELL_TESTING")) {
        AI_DEBUG("Testing mode - only checking in-memory key");
        return api_key != NULL && strlen(api_key) > 0;
    }
    
    // First check if we already have a key in memory
    if (api_key != NULL && strlen(api_key) > 0) {
        AI_DEBUG("Using API key from memory");
        return true;
    }
    
    // Then check environment variable
    char *env_key = getenv("OPENAI_API_KEY");
    if (env_key != NULL && strlen(env_key) > 0) {
        AI_DEBUG("Found API key in environment variable");
        set_api_key(env_key);
        return true;
    }
    
    // Finally check if we have a key file
    char key_path[512];
    char *home = getenv("HOME");
    if (home) {
        snprintf(key_path, sizeof(key_path), "%s/.nutshell/openai_key", home);
        FILE *key_file = fopen(key_path, "r");
        if (key_file) {
            AI_DEBUG("Found API key file at %s", key_path);
            char buffer[256] = {0};
            if (fgets(buffer, sizeof(buffer), key_file)) {
                // Remove trailing newline
                buffer[strcspn(buffer, "\n")] = 0;
                if (strlen(buffer) > 0) {
                    set_api_key(buffer);
                    fclose(key_file);
                    return true;
                }
            }
            fclose(key_file);
        } else {
            AI_DEBUG("No API key file found at %s", key_path);
        }
    }
    
    AI_DEBUG("No API key found");
    return false;
}

// Set API key
void set_api_key(const char *key) {
    if (api_key) {
        free(api_key);
    }
    api_key = strdup(key);
    AI_DEBUG("API key set");
    
    // Save key to file for persistence
    char key_path[512];
    char *home = getenv("HOME");
    if (home) {
        snprintf(key_path, sizeof(key_path), "%s/.nutshell", home);
        mkdir(key_path, 0700); // Create directory if it doesn't exist
        
        snprintf(key_path, sizeof(key_path), "%s/.nutshell/openai_key", home);
        FILE *key_file = fopen(key_path, "w");
        if (key_file) {
            fprintf(key_file, "%s\n", key);
            fclose(key_file);
            // Set restrictive permissions
            chmod(key_path, 0600);
            AI_DEBUG("API key saved to %s", key_path);
        } else {
            AI_DEBUG("Failed to save API key to %s", key_path);
        }
    }
}

// Initialize AI integration
bool init_ai_integration() {
    AI_DEBUG("Initializing AI integration");
    
    // Initialize CURL
    curl_global_init(CURL_GLOBAL_DEFAULT);
    
    // Check if we have API key
    if (!has_api_key()) {
        AI_DEBUG("OpenAI API key not found. Please set OPENAI_API_KEY environment variable");
        DEBUG_LOG("or run: set-api-key YOUR_API_KEY");
        return false;
    }
    
    AI_DEBUG("AI integration initialized successfully");
    return true;
}

// Clean up AI resources
void cleanup_ai_integration() {
    AI_DEBUG("Cleaning up AI resources");
    
    if (api_key) {
        free(api_key);
        api_key = NULL;
    }
    curl_global_cleanup();
    
    AI_DEBUG("AI resources cleaned up");
}

// Make a request to OpenAI API with given prompt
static char *request_completion(const char *system_prompt, const char *user_prompt) {
    if (!has_api_key()) {
        print_error("OpenAI API key not set");
        return NULL;
    }
    
    AI_DEBUG("Making OpenAI API request");
    AI_DEBUG("System prompt: %.40s...", system_prompt);
    AI_DEBUG("User prompt: %.40s...", user_prompt);
    
    CURL *curl = curl_easy_init();
    if (!curl) {
        print_error("Failed to initialize CURL");
        return NULL;
    }
    
    // Create memory structure for response
    struct MemoryStruct response;
    response.memory = malloc(1);
    response.size = 0;
    
    // Create JSON request
    json_t *root = json_object();
    json_object_set_new(root, "model", json_string(OPENAI_MODEL));
    
    // Create messages array
    json_t *messages = json_array();
    
    // Add system message
    json_t *system_msg = json_object();
    json_object_set_new(system_msg, "role", json_string("system"));
    json_object_set_new(system_msg, "content", json_string(system_prompt));
    json_array_append_new(messages, system_msg);
    
    // Add user message
    json_t *user_msg = json_object();
    json_object_set_new(user_msg, "role", json_string("user"));
    json_object_set_new(user_msg, "content", json_string(user_prompt));
    json_array_append_new(messages, user_msg);
    
    json_object_set_new(root, "messages", messages);
    json_object_set_new(root, "temperature", json_real(0.2)); // Low temperature for deterministic responses
    
    // Convert JSON to string
    char *json_str = json_dumps(root, JSON_COMPACT);
    json_decref(root);
    
    // Set up CURL
    curl_easy_setopt(curl, CURLOPT_URL, OPENAI_API_URL);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_str);
    
    // Set headers
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    
    char auth_header[256];
    snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s", api_key);
    headers = curl_slist_append(headers, auth_header);
    
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    
    // Perform request
    CURLcode res = curl_easy_perform(curl);
    
    // Clean up
    free(json_str);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    
    // Check for errors
    if (res != CURLE_OK) {
        print_error("CURL request failed");
        AI_DEBUG("CURL error: %s", curl_easy_strerror(res));
        free(response.memory);
        return NULL;
    }
    
    AI_DEBUG("API request successful");
    
    // Parse response JSON to extract the completion
    json_error_t error;
    json_t *response_json = json_loads(response.memory, 0, &error);
    
    // Always print the first part of the response in debug mode
    AI_DEBUG("Response first 80 chars: %.80s", response.memory);
    
    // When in verbose debug mode, print the entire response
    if (getenv("NUT_DEBUG_AI_VERBOSE")) {
        AI_DEBUG("API full response: %s", response.memory);
    }
    
    free(response.memory);
    
    if (!response_json) {
        print_error("Failed to parse JSON response");
        AI_DEBUG("JSON parse error: %s at line %d, column %d, position %d", 
               error.text, error.line, error.column, error.position);
        return NULL;
    }
    
    // Check for error in the response
    json_t *error_obj = json_object_get(response_json, "error");
    if (error_obj) {
        json_t *error_message = json_object_get(error_obj, "message");
        if (json_is_string(error_message)) {
            // Create a formatted string first, then pass it to print_error
            char error_buffer[512];
            snprintf(error_buffer, sizeof(error_buffer), "API error: %s", json_string_value(error_message));
            print_error(error_buffer);
            AI_DEBUG("API returned an error: %s", json_string_value(error_message));
        } else {
            print_error("API returned an error");
            AI_DEBUG("API returned an unknown error");
        }
        json_decref(response_json);
        return NULL;
    }
    
    // Extract completion text
    char *result = NULL;
    json_t *choices = json_object_get(response_json, "choices");
    
    if (!choices || !json_is_array(choices)) {
        AI_DEBUG("Response missing 'choices' array");
        json_decref(response_json);
        return NULL;
    }
    
    if (json_array_size(choices) == 0) {
        AI_DEBUG("'choices' array is empty");
        json_decref(response_json);
        return NULL;
    }
    
    json_t *choice = json_array_get(choices, 0);
    if (!choice || !json_is_object(choice)) {
        AI_DEBUG("First choice is not an object");
        json_decref(response_json);
        return NULL;
    }
    
    json_t *message = json_object_get(choice, "message");
    if (!message || !json_is_object(message)) {
        AI_DEBUG("Choice missing 'message' object");
        json_decref(response_json);
        return NULL;
    }
    
    json_t *content = json_object_get(message, "content");
    if (!content || !json_is_string(content)) {
        AI_DEBUG("Message missing 'content' string");
        json_decref(response_json);
        return NULL;
    }
    
    result = strdup(json_string_value(content));
    if (result) {
        AI_DEBUG("Successfully extracted completion: %.40s...", result);
    } else {
        AI_DEBUG("Failed to allocate memory for completion");
    }
    
    json_decref(response_json);
    return result;
}

// Add these declarations at the top of the file, after the includes
// Function pointers for testing
typedef char* (*NlToCommandFunc)(const char*);
typedef char* (*ExplainCommandFunc)(const char*);
typedef char* (*SuggestFixFunc)(const char*, const char*, int);

// Default to the real implementations
static NlToCommandFunc nl_to_command_impl = NULL;
static ExplainCommandFunc explain_command_impl = NULL;
// Remove static to make it accessible to tests
SuggestFixFunc suggest_fix_impl = NULL;

// The actual implementation functions
char* real_nl_to_command(const char* natural_language_query) {
    // System prompt for command conversion - very simple and direct
    const char *system_prompt = 
        "You are a CLI command generation assistant. Convert the user's natural language query into a single shell command.\n"
        "Rules:\n"
        "1. Respond ONLY with the command, no explanations or additional text\n"
        "2. Use Nutshell commands where appropriate (peekaboo for ls, hop for cd, roast for exit)\n"
        "3. If uncertain, provide the most likely command but start with a comment # Not sure, try:\n"
        "4. Keep it simple and straightforward";
    
    return request_completion(system_prompt, natural_language_query);
}

char* real_explain_command_ai(const char* command) {
    // System prompt for command explanation
    const char *system_prompt = 
        "You are a shell command explanation assistant. Explain what the given command does in simple terms.\n"
        "Rules:\n"
        "1. Provide a clear, concise explanation of what the command does\n"
        "2. Explain any flags or options used\n"
        "3. Highlight any potential risks or dangerous operations if present\n"
        "4. Keep the explanation user-friendly and educational";
    
    return request_completion(system_prompt, command);
}

// Define this function early so it can be referenced by set_ai_mock_functions
char *real_suggest_fix(const char *command, const char *error, int exit_status) {
    // System prompt for fix suggestions
    const char *system_prompt = 
        "You are a shell command assistant helping fix errors. The user has run a command "
        "that produced an error. Please suggest a fix or alternative command.\n"
        "Rules:\n"
        "1. Analyze why the command failed and provide a brief explanation\n"
        "2. Suggest a corrected command that will likely work\n"
        "3. If there are multiple possible fixes, list the most likely one first\n"
        "4. Use Nutshell commands where appropriate (peekaboo for ls, hop for cd, etc.)\n"
        "5. Format your response with sections: 'Explanation:', 'Suggested Fix:', followed by 'Corrected command:' on a new line\n"
        "6. Be helpful and educational in your response";
    
    // Create user prompt with context
    char user_prompt[4096];
    snprintf(user_prompt, sizeof(user_prompt),
             "Command: %s\nOutput/Error: %s\nExit status: %d\n"
             "Please suggest how to fix this command. If the command has a typo or common error, "
             "show the corrected version. Format your response with 'Explanation:' followed by "
             "'Corrected command:' sections.",
             command, error && strlen(error) > 0 ? error : "No error output available", exit_status);
    
    return request_completion(system_prompt, user_prompt);
}

// Public API functions that use the function pointers
char* nl_to_command(const char* natural_language_query) {
    if (nl_to_command_impl == NULL) {
        nl_to_command_impl = real_nl_to_command;
    }
    return nl_to_command_impl(natural_language_query);
}

char* explain_command_ai(const char* command) {
    if (explain_command_impl == NULL) {
        explain_command_impl = real_explain_command_ai;
    }
    return explain_command_impl(command);
}

// Function to set mock implementations for testing - modified to match header
void set_ai_mock_functions(NlToCommandFunc nl_func, ExplainCommandFunc explain_func) {
    nl_to_command_impl = nl_func ? nl_func : real_nl_to_command;
    explain_command_impl = explain_func ? explain_func : real_explain_command_ai;
    // Remove the third parameter handling
}

// Public wrapper function
char *suggest_fix(const char *command, const char *error, int exit_status) {
    if (suggest_fix_impl == NULL) {
        suggest_fix_impl = real_suggest_fix;
    }
    return suggest_fix_impl(command, error, exit_status);
}

// Suggest commands based on context
char *suggest_commands(const char *context) {
    // System prompt for command suggestions
    const char *system_prompt = 
        "You are a shell command suggestion assistant. Based on the user's context, suggest helpful commands.\n"
        "Rules:\n"
        "1. Suggest 3-5 useful commands that might help with the described task\n"
        "2. Format each suggestion on a new line with a brief explanation\n"
        "3. Use Nutshell commands where appropriate (peekaboo for ls, hop for cd, roast for exit)\n"
        "4. Focus on being practical and helpful for the specific context";
    
    return request_completion(system_prompt, context);
}

// Built-in command to set API key
int set_api_key_command(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: set-api-key YOUR_API_KEY\n");
        return 1;
    }
    
    set_api_key(argv[1]);
    printf("API key set successfully.\n");
    return 0;
}

// Built-in command to ask AI for a command
int ask_ai_command(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: ask 'your question in natural language'\n");
        printf("Example: ask 'find all PDF files modified in the last week'\n");
        return 1;
    }
    
    // Combine all arguments into a single query
    char query[1024] = {0};
    for (int i = 1; i < argc; i++) {
        if (i > 1) strcat(query, " ");
        strncat(query, argv[i], sizeof(query) - strlen(query) - 1);
    }
    
    if (!has_api_key()) {
        printf("No OpenAI API key found. Please set one with: set-api-key YOUR_API_KEY\n");
        return 1;
    }
    
    printf("Asking AI: \"%s\"\n", query);
    char *result = nl_to_command(query);
    
    if (result) {
        printf("\n%s\n\n", result);
        
        // Skip interactive prompt in testing mode
        if (getenv("NUTSHELL_TESTING")) {
            free(result);
            return 0;
        }
        
        // Ask if the user wants to execute the command
        printf("Execute this command? (y/n): ");
        char response[10] = {0};
        if (fgets(response, sizeof(response), stdin)) {
            if (response[0] == 'y' || response[0] == 'Y') {
                // Create a temporary script and execute it
                char temp_script[1024];
                snprintf(temp_script, sizeof(temp_script), "%s/.nutshell/temp_cmd.sh", getenv("HOME"));
                FILE *fp = fopen(temp_script, "w");
                if (fp) {
                    fprintf(fp, "#!/bin/bash\n%s\n", result);
                    fclose(fp);
                    chmod(temp_script, 0700);
                    system(temp_script);
                    // Clean up
                    unlink(temp_script);
                }
            }
        }
        free(result);
    } else {
        printf("Failed to get a response from the AI.\n");
    }
    
    return 0;
}

// Built-in command to explain a command
int explain_command(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: explain 'command to explain'\n");
        printf("Example: explain 'find . -name \"*.txt\" -mtime -7'\n");
        return 1;
    }
    
    // Combine all arguments into a single command
    char command[1024] = {0};
    for (int i = 1; i < argc; i++) {
        if (i > 1) strcat(command, " ");
        strncat(command, argv[i], sizeof(command) - strlen(command) - 1);
    }
    
    if (!has_api_key()) {
        printf("No OpenAI API key found. Please set one with: set-api-key YOUR_API_KEY\n");
        return 1;
    }
    
    printf("Explaining: %s\n", command);
    AI_DEBUG("Requesting explanation for: %s", command);
    
    char *result = explain_command_ai(command);
    
    if (result) {
        printf("\n%s\n", result);
        free(result);
    } else {
        printf("Failed to get an explanation. Check if your API key is valid.\n");
        printf("You can set a new key with: set-api-key YOUR_API_KEY\n");
        
        // Check if debug is enabled
        if (!getenv("NUT_DEBUG_AI")) {
            printf("For more error details, run with: NUT_DEBUG_AI=1 NUT_DEBUG_AI_VERBOSE=1 explain '%s'\n", command);
        }
    }
    
    return 0;
}

// Add this function for testing purposes, allowing tests to reset the API key
void reset_api_key_for_testing() {
    if (api_key) {
        free(api_key);
        api_key = NULL;
    }
    AI_DEBUG("API key reset for testing");
}

// Built-in command to get fix suggestions for the last command
int fix_command(int argc, char **argv) {
    // Suppress unused parameter warnings
    (void)argc;
    (void)argv;
    
    // Import command history
    extern CommandHistory cmd_history;
    
    // Check if we have a last command
    if (!cmd_history.last_command || strlen(cmd_history.last_command) == 0) {
        printf("No previous command to fix.\n");
        return 1;
    }
    
    // Check if we have API key
    if (!has_api_key()) {
        printf("No OpenAI API key found. Please set one with: set-api-key YOUR_API_KEY\n");
        return 1;
    }
    
    printf("Analyzing: %s\n", cmd_history.last_command);
    if (cmd_history.has_error) {
        printf("Exit status: %d\n", cmd_history.exit_status);
    }
    
    // If there's no error output but we have a non-zero exit status, use a generic message
    const char *error_text = cmd_history.last_output && strlen(cmd_history.last_output) > 0 ? 
                             cmd_history.last_output : "Command failed with no output";
    
    // Get fix suggestion
    char *result = suggest_fix(cmd_history.last_command, error_text, cmd_history.exit_status);
    
    if (result) {
        printf("\n%s\n\n", result);
        
        // Skip interactive prompt in testing mode
        if (getenv("NUTSHELL_TESTING")) {
            free(result);
            return 0;
        }
        
        // Create a copy of the result for parsing
        char *result_copy = strdup(result);
        if (!result_copy) {
            free(result);
            return 1;
        }
        
        // Extract command from the suggestion
        char *suggested_cmd = NULL;
        char *lines[100] = {0};  // Increased max lines to 100
        int line_count = 0;
        
        // Split the result into lines
        char *line = strtok(result_copy, "\n");
        while (line && line_count < 100) {
            lines[line_count++] = line;
            line = strtok(NULL, "\n");
        }
        
        // Try to find a code block with a command
        bool in_code_block = false;
        char *code_block_content = NULL;
        
        AI_DEBUG("Searching for command in AI response with %d lines", line_count);
        
        for (int i = 0; i < line_count; i++) {
            AI_DEBUG("Line %d: %s", i, lines[i]);
            
            // Check for code block markers (with or without language specifier)
            if (strncmp(lines[i], "```", 3) == 0) {
                if (!in_code_block) {
                    // Start of code block - skip this line
                    in_code_block = true;
                    AI_DEBUG("Code block starts at line %d", i);
                } else {
                    // End of code block
                    in_code_block = false;
                    AI_DEBUG("Code block ends at line %d", i);
                }
                continue;
            }
            
            // If we're in a code block, check for actual command content
            if (in_code_block) {
                // Skip empty lines or comments
                if (strlen(lines[i]) == 0 || lines[i][0] == '#') {
                    continue;
                }
                
                // Found code within a code block
                code_block_content = lines[i];
                AI_DEBUG("Found command in code block: %s", code_block_content);
                break;
            }
            
            // Look for lines after "Corrected command:" pattern
            if (strstr(lines[i], "Corrected command:") != NULL && i + 1 < line_count) {
                // The next line is likely our command
                // Skip to the next line that's not empty and doesn't start with ```
                for (int j = i + 1; j < line_count; j++) {
                    if (strlen(lines[j]) == 0 || strncmp(lines[j], "```", 3) == 0) {
                        continue;
                    }
                    suggested_cmd = lines[j];
                    AI_DEBUG("Found command after 'Corrected command:' pattern: %s", suggested_cmd);
                    break;
                }
                if (suggested_cmd) {
                    break;
                }
            }
        }
        
        // If we found a likely command, ask if user wants to execute it
        if (code_block_content) {
            suggested_cmd = code_block_content;
        }
        
        if (suggested_cmd) {
            printf("Would you like to execute: %s ? (y/n): ", suggested_cmd);
            char response[10] = {0};
            if (fgets(response, sizeof(response), stdin)) {
                if (response[0] == 'y' || response[0] == 'Y') {
                    // Create a temporary script and execute it
                    char temp_script[1024];
                    snprintf(temp_script, sizeof(temp_script), "%s/.nutshell/temp_fix.sh", getenv("HOME"));
                    FILE *fp = fopen(temp_script, "w");
                    if (fp) {
                        fprintf(fp, "#!/bin/bash\n%s\n", suggested_cmd);
                        fclose(fp);
                        chmod(temp_script, 0700);
                        system(temp_script);
                        // Clean up
                        unlink(temp_script);
                    }
                }
            }
        }
        
        free(result_copy);
        free(result);
    } else {
        printf("Failed to get a fix suggestion.\n");
    }
    
    return 0;
}
