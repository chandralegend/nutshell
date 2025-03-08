#include <nutshell/pkg.h>
#include <curl/curl.h>
// Try to find jansson.h in various locations
#if __has_include(<jansson.h>)
  #include <jansson.h>
  #define JANSSON_AVAILABLE 1
#elif __has_include("jansson.h")
  #include "jansson.h"
  #define JANSSON_AVAILABLE 1
#else
  #warning "jansson.h not found - some functionality will be limited"
  #define JANSSON_AVAILABLE 0
  // Minimal stubs for jansson types
  typedef void* json_t;
  typedef struct { char text[256]; } json_error_t;
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

// Fix function declaration to match implementation
bool load_manifest(const char* pkg_name, PackageManifest* manifest);
char* download_to_string(const char *url);

// Add missing struct and callback at the top of the file
struct MemoryStruct {
    char *memory;
    size_t size;
};

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t real_size = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;
    
    char *ptr = realloc(mem->memory, mem->size + real_size + 1);
    if (!ptr) {
        fprintf(stderr, "Not enough memory (realloc returned NULL)\n");
        return 0;
    }
    
    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, real_size);
    mem->size += real_size;
    mem->memory[mem->size] = 0;
    
    return real_size;
}

static const char* PKG_CACHE_DIR = "/var/cache/nutshell/packages";

static size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    return fwrite(ptr, size, nmemb, stream);
}

PkgInstallResult nutpkg_install(const char* pkg_name) {
    char url[256];
    snprintf(url, sizeof(url), "%s/packages/%s", NUTPKG_REGISTRY, pkg_name);

    // Create cache directory if it doesn't exist
    mkdir(PKG_CACHE_DIR, 0755);

    char tmpfile[256];
    snprintf(tmpfile, sizeof(tmpfile), "%s/%s.tmp", PKG_CACHE_DIR, pkg_name);

    CURL *curl = curl_easy_init();
    if(!curl) return PKG_INSTALL_FAILED;

    FILE *fp = fopen(tmpfile, "wb");
    if(!fp) return PKG_INSTALL_FAILED;

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    CURLcode res = curl_easy_perform(curl);
    fclose(fp);
    curl_easy_cleanup(curl);

    if(res != CURLE_OK) {
        remove(tmpfile);
        return PKG_INSTALL_FAILED;
    }

    // Verify package integrity
    PackageManifest manifest;
    if(!load_manifest(pkg_name, &manifest)) {
        remove(tmpfile);
        return PKG_INTEGRITY_FAILED;
    }

    if(!verify_package_integrity(&manifest)) {
        remove(tmpfile);
        return PKG_INTEGRITY_FAILED;
    }

    // Install dependencies
    for(int i = 0; manifest.dependencies[i]; i++) {
        if(nutpkg_install(manifest.dependencies[i]) != PKG_INSTALL_SUCCESS) {
            remove(tmpfile);
            return PKG_DEPENDENCY_FAILED;
        }
    }

    // Final installation
    char target[256];
    snprintf(target, sizeof(target), "/usr/local/nutshell/packages/%s", pkg_name);
    rename(tmpfile, target);

    return PKG_INSTALL_SUCCESS;
}

// Fix load_manifest function to handle missing jansson library
bool load_manifest(const char* pkg_name, PackageManifest* manifest) {
    if (!pkg_name || !manifest) return false;
    
#if !JANSSON_AVAILABLE
    fprintf(stderr, "ERROR: Jansson library not available. Cannot load package manifest.\n");
    fprintf(stderr, "Please install Jansson with: brew install jansson\n");
    return false;
#else
    char manifest_url[256];
    snprintf(manifest_url, sizeof(manifest_url), 
            "%s/packages/%s/manifest", NUTPKG_REGISTRY, pkg_name);

    char* json_str = download_to_string(manifest_url);
    if(!json_str) return false;

    json_error_t error;
    json_t *root = json_loads(json_str, 0, &error);
    free(json_str);
    
    if(!root) return false;

    // Parse JSON manifest
    manifest->name = strdup(json_string_value(json_object_get(root, "name")));
    manifest->version = strdup(json_string_value(json_object_get(root, "version")));
    manifest->description = strdup(json_string_value(json_object_get(root, "description")));
    manifest->checksum = strdup(json_string_value(json_object_get(root, "sha256")));

    json_t *deps = json_object_get(root, "dependencies");
    if(deps) {
        size_t index;
        json_t *value;
        json_array_foreach(deps, index, value) {
            if(index < MAX_DEPENDENCIES) {
                manifest->dependencies[index] = strdup(json_string_value(value));
            }
        }
    }

    json_decref(root);
    return true;
#endif
}

char* download_to_string(const char *url) {
    CURL *curl = curl_easy_init();
    if(!curl) return NULL;

    struct MemoryStruct chunk;
    chunk.memory = malloc(1);
    chunk.size = 0;

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "nutshell-pkg/1.0");

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if(res != CURLE_OK) {
        free(chunk.memory);
        return NULL;
    }

    return chunk.memory;
}