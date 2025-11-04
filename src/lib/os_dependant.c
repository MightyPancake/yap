#include "yap/all.h"

#ifdef __linux__
  #include <libgen.h>
  #include <unistd.h>
#endif

#ifdef __linux__
char* yap_resolve_path(const char* path){
  if (path == NULL) {
        printf("ERROR: path is NULL\n");
        return NULL;
    }
    char* ret = (char*)malloc(YAP_PATH_MAX);
    if (ret == NULL) {
        printf("ERROR: malloc failed\n");
        return NULL;
    }
    char* res = realpath(path, ret);
    if (res == NULL) {
        printf("Couldn't get realpath for %s!", path);
        free(ret);
        return NULL;
    }
    return ret;
}
#else
  #error 'yap_resolve_path' not yet implemented for platforms other than linux!
#endif

#ifdef __linux__
char* yap_get_parent_dir(const char *full_path) {
    if (!full_path) return NULL;

    // Copy full_path because dirname may modify it
    char temp[YAP_PATH_MAX];
    strncpy(temp, full_path, sizeof(temp) - 1);
    temp[sizeof(temp) - 1] = '\0';

    char *dir = dirname(temp);

    // Allocate buffer for the result
    char *result = malloc(strlen(dir) + 1);
    if (!result) return NULL;

    strcpy(result, dir);
    return result;
}
#else
  #error 'yap_get_parent_dir' not yet implemented for platforms other than linux!
#endif

#ifdef __linux__
char* yap_get_self_path() {
    // Allocate buffer
    char *buf = malloc(YAP_PATH_MAX);
    if (!buf) {
        perror("malloc failed");
        return NULL;
    }

    // Read the symlink
    ssize_t bytes = readlink("/proc/self/exe", buf, YAP_PATH_MAX - 1);
    if (bytes >= 0) {
        buf[bytes] = '\0';  // null-terminate
        return buf;
    } else {
        perror("Error getting executable path");
        free(buf);
        return NULL;
    }
}
#else
  #error 'yap_get_self_path' not yet implemented for platforms other than linux!
#endif
