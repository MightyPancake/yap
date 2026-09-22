#include "yap/all.h"
#include <ctype.h>

/* Prerelease tags and build metadata are rejected, not ignored; absent components are zero. */
bool yap_version_parse(const char* str, yap_version* out){
    if (!str || !*str || !out) return false;

    uint16_t parts[3] = {0, 0, 0};
    size_t i = 0;
    for (size_t p = 0; p < 3; p++){
        if (!isdigit((unsigned char)str[i])) return false;
        unsigned long v = 0;
        while (isdigit((unsigned char)str[i])){
            v = v * 10 + (unsigned long)(str[i++] - '0');
            if (v > UINT16_MAX) return false;
        }
        parts[p] = (uint16_t)v;
        if (!str[i]){
            *out = (yap_version){ .major = parts[0], .minor = parts[1], .patch = parts[2] };
            return true;
        }
        if (str[i] != '.') return false;
        i++;
    }
    return false;
}
