#include "yap/all.h"
#include <sys/stat.h>

/* Fetching is a separate command rather than part of a build: the compiler stays offline
 * and a build either finds what it needs or names the command that would get it. */

static bool yap_run_git(const char* fmt, ...){
    va_list ap;
    va_start(ap, fmt);
    char* cmd = NULL;
    int n = vasprintf(&cmd, fmt, ap);
    va_end(ap);
    if (n < 0 || !cmd) return false;

    yap_log("fetch: %s", cmd);
    int rc = system(cmd);
    free(cmd);
    return rc == 0;
}

/* Clones into a staging directory, reads the version the module declares for itself, then
 * moves it to <dest>/<name>/<version>. The version cannot be known before the clone, so
 * the manifest's constraint is verified afterwards rather than driving the fetch. */
static bool yap_fetch_git_dep(yap_ctx* ctx, yap_dep_node dep, const char* dest_root){
    char* staging = strus_newf("%s/.staging-%s", dest_root, dep.name);
    yap_run_git("rm -rf '%s'", staging);

    bool ok;
    if (dep.tag || dep.branch){
        ok = yap_run_git("git clone --quiet --depth 1 --branch '%s' '%s' '%s'",
                         dep.tag ? dep.tag : dep.branch, dep.git, staging);
    } else {
        ok = yap_run_git("git clone --quiet '%s' '%s'", dep.git, staging);
    }
    if (ok && dep.rev)
        ok = yap_run_git("git -C '%s' fetch --quiet --depth 1 origin '%s' && git -C '%s' checkout --quiet '%s'",
                         staging, dep.rev, staging, dep.rev);

    if (!ok){
        yap_ctx_push_error(ctx, (yap_error){
            .kind = yap_error_no_pos,
            .msg  = strus_newf("Could not clone '%s' from %s", dep.name, dep.git)
        });
        yap_run_git("rm -rf '%s'", staging);
        free(staging);
        return false;
    }

    char* manifest_path = strus_newf("%s/mod.yp", staging);
    yap_module_decl_node manifest = {0};
    yap_version got = {0};
    bool have_manifest = ctx->read_manifest && ctx->read_manifest(ctx, manifest_path, &manifest);
    if (have_manifest && manifest.version) yap_version_parse(manifest.version, &got);
    free(manifest_path);

    if (!have_manifest){
        yap_ctx_push_error(ctx, (yap_error){
            .kind = yap_error_no_pos,
            .msg  = strus_newf("'%s' has no mod.yp with a module declaration at its root", dep.name)
        });
        yap_run_git("rm -rf '%s'", staging);
        free(staging);
        return false;
    }

    if (!yap_dep_satisfied_by(dep, got)){
        yap_ctx_push_error(ctx, (yap_error){
            .kind = yap_error_no_pos,
            .msg  = strus_newf("'%s' declares version %u.%u.%u, which does not satisfy '%s'",
                               dep.name, got.major, got.minor, got.patch, yap_dep_spec_string(ctx, dep))
        });
        yap_run_git("rm -rf '%s'", staging);
        free(staging);
        return false;
    }

    char* dest = strus_newf("%s/%s/%u.%u.%u", dest_root, dep.name, got.major, got.minor, got.patch);
    yap_run_git("rm -rf '%s'", dest);
    yap_run_git("mkdir -p '%s/%s'", dest_root, dep.name);
    bool moved = yap_run_git("mv '%s' '%s'", staging, dest);
    if (moved)
        printf("Fetched %s %u.%u.%u\n", dep.name, got.major, got.minor, got.patch);
    free(dest);
    free(staging);
    return moved;
}

int yap_fetch_deps(yap_ctx* ctx, yap_args args){
    if (darr_len(args.extra) == 0){
        printf("No source file given; --fetch reads its manifest.\n");
        return 1;
    }

    char* source = darr_first(args.extra);
    char* resolved = yap_resolve_path(source);
    if (!resolved){
        printf("Source file '%s' not found\n", source);
        return 1;
    }

    yap_module_decl_node manifest = {0};
    bool have = ctx->read_manifest && ctx->read_manifest(ctx, resolved, &manifest);
    if (!have || !manifest.deps){
        printf("Nothing to fetch: '%s' declares no deps.\n", source);
        free(resolved);
        return 0;
    }

    char* src_dir = yap_get_parent_dir(resolved);
    char* dest_root = strus_newf("%s/.yap/modules", src_dir);
    yap_run_git("mkdir -p '%s'", dest_root);

    unsigned fetched = 0, failed = 0, skipped = 0;
    for_darr(i, dep, manifest.deps){
        if (!dep.name) continue;
        if (dep.registry || dep.path){
            printf("Skipping '%s': only git sources are fetched for now\n", dep.name);
            skipped++;
            continue;
        }
        if (!dep.git){ skipped++; continue; }

        if (yap_fetch_git_dep(ctx, dep, dest_root)) fetched++;
        else failed++;
    }

    printf("Fetched %u, failed %u, skipped %u\n", fetched, failed, skipped);
    free(dest_root);
    free(src_dir);
    free(resolved);
    return failed ? 1 : 0;
}
