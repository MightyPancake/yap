#include "yap/all.h"

#ifdef __linux__
  #include <libgen.h>
  #include <unistd.h>
  #include <limits.h> //PATH_MAX
#endif

#ifdef __linux__
char* yap_resolve_path(const char* path){
  if (path == NULL) {
        printf("ERROR: path is NULL\n");
        return NULL;
    }
    char* res = realpath(path, NULL);
    if (res == NULL) {
        printf("Couldn't get realpath for %s!", path);
        return NULL;
    }
    return res;
}
#else
  #error 'yap_resolve_path' not yet implemented for platforms other than linux!
#endif

#ifdef __linux__
char* yap_get_parent_dir(const char *full_path) {
    if (!full_path) return NULL;

    // Copy full_path because dirname may modify it
    char temp[PATH_MAX];
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
    char *buf = malloc(PATH_MAX);
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

#ifdef __linux__
char* yap_cwd() {
    char *buf = malloc(YAP_PATH_MAX);
    if (!buf) {
        perror("malloc failed");
        return NULL;
    }

    if (getcwd(buf, YAP_PATH_MAX) != NULL) {
        return buf;
    } else {
        perror("Error getting current working directory");
        free(buf);
        return NULL;
    }
}
#else
  #error 'yap_cwd' not yet implemented for platforms other than linux!
#endif

#ifdef __linux__
void yap_cd(const char* path) {
    if (path == NULL) {
        fprintf(stderr, "ERROR: yap_cd: path is NULL\n");
        return;
    }
    if (chdir(path) != 0) {
        perror("Error changing directory");
    }
}
#else
  #error 'yap_cd' not yet implemented for platforms other than linux!
#endif

#ifdef __linux__
#include <dirent.h>
#include <sys/stat.h>

void yap_rmdir_recursive(const char* path){
    if (!path || !path[0]) return;
    DIR* d = opendir(path);
    if (!d){ remove(path); return; }
    struct dirent* entry;
    char child[PATH_MAX];
    while ((entry = readdir(d)) != NULL){
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
        snprintf(child, sizeof(child), "%s/%s", path, entry->d_name);
        struct stat st;
        if (stat(child, &st) == 0 && S_ISDIR(st.st_mode))
            yap_rmdir_recursive(child);
        else
            remove(child);
    }
    closedir(d);
    rmdir(path);
}

int yap_mkdir(const char* path){
    if (!path) return -1;
    return mkdir(path, 0755);
}

char* yap_make_temp_dir(void) {
    char template[] = "/tmp/yap_build_XXXXXX";
    char* dir = mkdtemp(template);
    if (!dir) {
        perror("Failed to create temp directory");
        return NULL;
    }
    return strdup(dir);
}

int yap_copy_dir_recursive(const char* src, const char* dst){
    if (!src || !dst) return -1;

    struct stat dst_st;
    if (stat(dst, &dst_st) == 0 && S_ISDIR(dst_st.st_mode))
        yap_rmdir_recursive(dst);
    if (yap_mkdir(dst) != 0) return -1;

    DIR* d = opendir(src);
    if (!d) return -1;

    int result = 0;
    struct dirent* entry;
    char src_child[PATH_MAX];
    char dst_child[PATH_MAX];
    while ((entry = readdir(d)) != NULL){
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
        snprintf(src_child, sizeof(src_child), "%s/%s", src, entry->d_name);
        snprintf(dst_child, sizeof(dst_child), "%s/%s", dst, entry->d_name);

        struct stat child_st;
        if (stat(src_child, &child_st) != 0){ result = -1; continue; }

        if (S_ISDIR(child_st.st_mode)){
            if (yap_copy_dir_recursive(src_child, dst_child) != 0) result = -1;
            continue;
        }

        FILE* in = fopen(src_child, "rb");
        if (!in){ result = -1; continue; }
        FILE* out = fopen(dst_child, "wb");
        if (!out){ fclose(in); result = -1; continue; }
        char buf[8192];
        size_t n;
        while ((n = fread(buf, 1, sizeof(buf), in)) > 0)
            fwrite(buf, 1, n, out);
        fclose(in);
        fclose(out);
    }
    closedir(d);
    return result;
}
#else
  #error 'yap_make_temp_dir' not yet implemented for platforms other than linux!
#endif
