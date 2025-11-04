#ifndef YAP_OS_DEPENDANT_H
#define YAP_OS_DEPENDANT_H

#define YAP_PATH_MAX 1024

char* yap_resolve_path(const char* path);
char* yap_get_parent_dir(const char *full_path);
char* yap_get_self_path();

#endif //YAP_OS_DEPENDANT_H
