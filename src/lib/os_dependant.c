#include "yap/all.h"

#ifdef __linux__
char* yap_resolve_path(char* path){
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
  #error 'yap_resolve_path' not yet implemented for other platforms!
#endif

