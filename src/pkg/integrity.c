#include <nutshell/pkg.h>
#include <openssl/evp.h>  // Use EVP interface instead of direct SHA256
#include <stdio.h>
#include <string.h>

// Define PKG_CACHE_DIR if not already defined
#ifndef PKG_CACHE_DIR
#define PKG_CACHE_DIR "/usr/local/nutshell/packages"
#endif

bool verify_package_integrity(const PackageManifest* manifest) {
    if(!manifest || !manifest->checksum) return false;

    char path[256];
    snprintf(path, sizeof(path), "%s/%s", PKG_CACHE_DIR, manifest->name);

    FILE *file = fopen(path, "rb");
    if(!file) return false;

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
    size_t bytesRead;
    while((bytesRead = fread(buffer, 1, sizeof(buffer), file))) {
        EVP_DigestUpdate(mdctx, buffer, bytesRead);
    }

    EVP_DigestFinal_ex(mdctx, hash, &hash_len);
    EVP_MD_CTX_free(mdctx);
    fclose(file);

    char hash_str[65];
    for(int i = 0; i < 32; i++) {
        // Replace sprintf with snprintf to avoid deprecation warning
        snprintf(hash_str + (i * 2), 3, "%02x", hash[i]);
    }
    hash_str[64] = '\0';

    return strcmp(hash_str, manifest->checksum) == 0;
}