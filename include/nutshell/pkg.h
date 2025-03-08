#ifndef NUTSHELL_PKG_H
#define NUTSHELL_PKG_H

#include <stdbool.h>

#define NUTPKG_REGISTRY "https://registry.nutshell.sh/v1"
#define MAX_DEPENDENCIES 32

typedef struct {
    char* name;
    char* version;
    char* description;
    char* dependencies[MAX_DEPENDENCIES];
    char* author;
    char* checksum;
} PackageManifest;

typedef enum {
    PKG_INSTALL_SUCCESS,
    PKG_INSTALL_FAILED,
    PKG_DEPENDENCY_FAILED,
    PKG_INTEGRITY_FAILED
} PkgInstallResult;

// Package management functions
PkgInstallResult nutpkg_install(const char* pkg_name);
bool nutpkg_uninstall(const char* pkg_name);
PackageManifest* nutpkg_search(const char* query);
void nutpkg_cleanup(PackageManifest* manifest);

// Dependency resolution
bool resolve_dependencies(const char* pkg_name);

// Integrity verification
bool verify_package_integrity(const PackageManifest* manifest);

#endif // NUTSHELL_PKG_H