#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE

#include <nutshell/utils.h>
#include <nutshell/pkg.h>
#include <stdio.h>
#include <string.h>
#include <openssl/evp.h>  // Use EVP interface instead of direct SHA256
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

// Define PKG_DIR if not already defined
#ifndef PKG_DIR
#define PKG_DIR "/usr/local/nutshell/packages"
#endif

void error(const char *msg) {
    fprintf(stderr, "Error: %s\n", msg);
    exit(EXIT_FAILURE);
}

bool verify_package_hash(const char *pkg_name) {
    char path[256];
    snprintf(path, sizeof(path), "%s/%s", PKG_DIR, pkg_name);

    FILE *file = fopen(path, "rb");
    if (!file) return false;

    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hash_len;
    
    // Use the EVP interface (modern approach)
    EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
    if (!mdctx) {
        fclose(file);
        return false;
    }

    EVP_DigestInit_ex(mdctx, EVP_sha256(), NULL);

    unsigned char buffer[4096];
    size_t bytes_read;
    while((bytes_read = fread(buffer, 1, sizeof(buffer), file))) {
        EVP_DigestUpdate(mdctx, buffer, bytes_read);
    }

    EVP_DigestFinal_ex(mdctx, hash, &hash_len);
    EVP_MD_CTX_free(mdctx);
    fclose(file);

    // Read expected hash
    unsigned char expected_hash[EVP_MAX_MD_SIZE];
    char hash_path[256];
    snprintf(hash_path, sizeof(hash_path), "%s/%s.sha256", PKG_DIR, pkg_name);

    FILE *hash_file = fopen(hash_path, "rb");
    if (!hash_file) {
        DEBUG_LOG("No hash file found for package: %s", pkg_name);
        return false;
    }

    if (fread(expected_hash, 1, hash_len, hash_file) != hash_len) {
        DEBUG_LOG("Invalid hash file for package: %s", pkg_name);
        fclose(hash_file);
        return false;
    }
    fclose(hash_file);

    // Compare hashes
    if(memcmp(expected_hash, hash, hash_len) != 0) {
        DEBUG_LOG("Hash verification failed for package: %s", pkg_name);
        return false;
    }

    return true;
}