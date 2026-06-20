#ifndef YAP_OS_DEPENDANT_H
#define YAP_OS_DEPENDANT_H

#define YAP_PATH_MAX 1024

char* yap_resolve_path(const char* path);
char* yap_get_parent_dir(const char *full_path);
char* yap_get_self_path();
char* yap_cwd();
void yap_cd(const char* path);

// Creates a temp directory and returns its path (caller frees).
// On Linux uses mkdtemp with a template in /tmp.
char* yap_make_temp_dir(void);

#endif //YAP_OS_DEPENDANT_H
