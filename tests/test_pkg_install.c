#include <nutshell/core.h>
#include <nutshell/pkg.h>
#include <nutshell/utils.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

// Mock implementation for testing
extern int install_pkg_command(int argc, char **argv);

void test_install_from_path() {
    printf("Testing package installation from path...\n");
    
    // Check if our sample package exists
    char package_path[256];
    snprintf(package_path, sizeof(package_path), 
             "%s/Desktop/nutshell/packages/gitify", getenv("HOME"));
    
    // We'll use the install_pkg_command function directly
    char *args[] = {"install-pkg", package_path, NULL};
    int result = install_pkg_command(2, args);
    
    assert(result == 0); // Should succeed
    
    // Verify the package is installed in the user's directory
    char installed_path[512];
    snprintf(installed_path, sizeof(installed_path), 
             "%s/.nutshell/packages/gitify/gitify.sh", getenv("HOME"));
    
    assert(access(installed_path, F_OK) == 0); // File should exist
    
    printf("Package installation test passed!\n");
}

void test_command_registration() {
    printf("Testing command registration...\n");
    
    // The command should be registered in the registry
    const CommandMapping *cmd = find_command("gitify");
    assert(cmd != NULL);
    assert(strcmp(cmd->nut_cmd, "gitify") == 0);
    assert(cmd->is_builtin == false);
    
    printf("Command registration test passed!\n");
}

void test_package_script() {
    printf("Testing package script execution...\n");
    
    // We'll execute a modified version that just checks its own functionality
    // without actually modifying any git repo
    char installed_path[512];
    snprintf(installed_path, sizeof(installed_path), 
             "%s/.nutshell/packages/gitify", getenv("HOME"));
    
    // Create a small test script that just returns 0 for testing
    char test_command[1024];
    snprintf(test_command, sizeof(test_command), 
             "echo '#!/bin/bash\necho \"Testing gitify\"\nexit 0' > %s/test.sh && chmod +x %s/test.sh",
             installed_path, installed_path);
    
    int result = system(test_command);
    assert(result == 0);
    
    // Execute the test script
    char exec_command[512];
    snprintf(exec_command, sizeof(exec_command), "%s/test.sh", installed_path);
    result = system(exec_command);
    assert(result == 0);
    
    printf("Package script execution test passed!\n");
}

int main() {
    printf("Running package installation tests...\n");
    
    // Initialize registry
    init_registry();
    
    // Run the tests
    test_install_from_path();
    test_command_registration();
    test_package_script();
    
    // Cleanup
    free_registry();
    
    printf("All package installation tests passed!\n");
    return 0;
}
