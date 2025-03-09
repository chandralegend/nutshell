#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE

#include <nutshell/core.h>
#include <nutshell/pkg.h>
#include <nutshell/utils.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static const char* USER_PKG_DIR = "/.nutshell/packages";

// Built-in command to install packages
int install_pkg_command(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: install-pkg <package_name_or_path>\n");
        return 1;
    }
    
    const char *target = argv[1];
    struct stat st;
    
    // Check if the argument is a directory path or a package name
    if (stat(target, &st) == 0 && S_ISDIR(st.st_mode)) {
        // It's a directory, try to install from path
        if (install_package_from_path(target)) {
            printf("Package installed successfully from %s\n", target);
            return 0;
        } else {
            printf("Failed to install package from %s\n", target);
            return 1;
        }
    } else {
        // Assume it's a package name to download
        if (install_package_from_name(target)) {
            printf("Package %s installed successfully\n", target);
            return 0;
        } else {
            printf("Failed to install package %s\n", target);
            return 1;
        }
    }
}

// Install a package from a local directory
bool install_package_from_path(const char *path) {
    char *home = getenv("HOME");
    if (!home) {
        print_error("HOME environment variable not set");
        return false;
    }
    
    char pkg_name[128];
    char *last_slash = strrchr(path, '/');
    if (last_slash) {
        strcpy(pkg_name, last_slash + 1);
    } else {
        strcpy(pkg_name, path);
    }
    
    // Create user package directory if it doesn't exist
    char dest_dir[512];
    snprintf(dest_dir, sizeof(dest_dir), "%s%s", home, USER_PKG_DIR);
    mkdir(dest_dir, 0755);
    
    // Create destination package directory
    char pkg_dir[512];
    snprintf(pkg_dir, sizeof(pkg_dir), "%s/%s", dest_dir, pkg_name);
    mkdir(pkg_dir, 0755);
    
    // Copy package files
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "cp -r %s/* %s/", path, pkg_dir);
    if (system(cmd) != 0) {
        print_error("Failed to copy package files");
        return false;
    }
    
    // Make script executable
    snprintf(cmd, sizeof(cmd), "chmod +x %s/%s.sh", pkg_dir, pkg_name);
    if (system(cmd) != 0) {
        print_error("Failed to make script executable");
        return false;
    }
    
    // Register the new command - fix the function call
    if (!register_package_commands(dest_dir, pkg_name)) {
        print_error("Failed to register package command");
        return false;
    }
    
    return true;
}

// Install a package by name from the package registry
bool install_package_from_name(const char *name) {
    // For now, just use nutpkg_install
    PkgInstallResult result = nutpkg_install(name);
    return (result == PKG_INSTALL_SUCCESS);
}
