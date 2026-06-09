#include "yap/all.h"

char* yap_resolve_path_relative_to(const char* base_path, const char* relative_path){
    char* original_cwd = yap_cwd();
    if (!original_cwd) return NULL;
    yap_cd(base_path);
    char* resolved = yap_resolve_path(relative_path);
    yap_cd(original_cwd);
    free(original_cwd);
    return resolved;
}