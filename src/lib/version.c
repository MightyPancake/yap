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

int yap_version_cmp(yap_version a, yap_version b){
    if (a.major != b.major) return a.major < b.major ? -1 : 1;
    if (a.minor != b.minor) return a.minor < b.minor ? -1 : 1;
    if (a.patch != b.patch) return a.patch < b.patch ? -1 : 1;
    return 0;
}

/* Semver caret: the leftmost non-zero component is pinned, so ^0.1.0 allows 0.1.x
 * but not 0.2.0, and ^0.0.1 allows nothing but itself. */
bool yap_version_satisfies_caret(yap_version req, yap_version have){
    if (yap_version_cmp(have, req) < 0) return false;
    if (req.major > 0) return have.major == req.major;
    if (req.minor > 0) return have.major == 0 && have.minor == req.minor;
    return have.major == 0 && have.minor == 0 && have.patch == req.patch;
}

bool yap_dep_satisfied_by(yap_dep_node dep, yap_version have){
    switch (dep.kind){
        case yap_dep_exact: return yap_version_cmp(have, dep.version) == 0;
        case yap_dep_caret: return yap_version_satisfies_caret(dep.version, have);
        case yap_dep_latest:
        case yap_dep_local:
        default: return true;
    }
}

char* yap_dep_spec_string(yap_ctx* ctx, yap_dep_node dep){
    switch (dep.kind){
        case yap_dep_exact: return yap_ctx_strus_newf(ctx, "%u.%u.%u", dep.version.major, dep.version.minor, dep.version.patch);
        case yap_dep_caret: return yap_ctx_strus_newf(ctx, "^%u.%u.%u", dep.version.major, dep.version.minor, dep.version.patch);
        case yap_dep_local: return "local";
        case yap_dep_latest:
        default: return "latest";
    }
}
