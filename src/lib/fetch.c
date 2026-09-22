#include "yap/all.h"
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

/* Fetching is a separate command rather than part of a build: the compiler stays offline
 * and a build either finds what it needs or names the command that would get it. */

/* Arguments are handed to exec directly and never to a shell, because every one of them
 * comes out of a dependency's manifest -- a URL containing a quote would otherwise run
 * whatever followed it. */
static bool yap_exec(char* const argv[]){
    if (!argv || !argv[0]) return false;

    for (char* const* a = argv; *a; a++) yap_log("fetch arg: %s", *a);

    pid_t pid = fork();
    if (pid < 0) return false;
    if (pid == 0){
        execvp(argv[0], argv);
        _exit(127);
    }
    int status = 0;
    if (waitpid(pid, &status, 0) < 0) return false;
    return WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

static bool yap_rm_rf(char* path){
    char* argv[] = { "rm", "-rf", "--", path, NULL };
    return yap_exec(argv);
}

static bool yap_mkdir_p(char* path){
    char* argv[] = { "mkdir", "-p", "--", path, NULL };
    return yap_exec(argv);
}

/* Clones into a staging directory, reads the version the module declares for itself, then
 * moves it to <dest>/<name>/<version>. The version cannot be known before the clone, so
 * the manifest's constraint is verified afterwards rather than driving the fetch. */
static bool yap_fetch_git_dep(yap_ctx* ctx, yap_dep_node dep, const char* dest_root){
    char* staging = strus_newf("%s/.staging-%s", dest_root, dep.name);
    yap_rm_rf(staging);

    bool ok;
    if (dep.tag || dep.branch){
        char* ref = dep.tag ? dep.tag : dep.branch;
        char* argv[] = { "git", "clone", "--quiet", "--depth", "1", "--branch", ref, "--", dep.git, staging, NULL };
        ok = yap_exec(argv);
    } else {
        char* argv[] = { "git", "clone", "--quiet", "--", dep.git, staging, NULL };
        ok = yap_exec(argv);
    }
    if (ok && dep.rev){
        char* fetch_argv[]    = { "git", "-C", staging, "fetch", "--quiet", "--depth", "1", "origin", dep.rev, NULL };
        char* checkout_argv[] = { "git", "-C", staging, "checkout", "--quiet", dep.rev, NULL };
        ok = yap_exec(fetch_argv) && yap_exec(checkout_argv);
    }

    if (!ok){
        yap_ctx_push_error(ctx, (yap_error){
            .kind = yap_error_no_pos,
            .msg  = strus_newf("Could not clone '%s' from %s", dep.name, dep.git)
        });
        yap_rm_rf(staging);
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
        yap_rm_rf(staging);
        free(staging);
        return false;
    }

    if (!yap_dep_satisfied_by(dep, got)){
        yap_ctx_push_error(ctx, (yap_error){
            .kind = yap_error_no_pos,
            .msg  = strus_newf("'%s' declares version %u.%u.%u, which does not satisfy '%s'",
                               dep.name, got.major, got.minor, got.patch, yap_dep_spec_string(ctx, dep))
        });
        yap_rm_rf(staging);
        free(staging);
        return false;
    }

    char* dest = strus_newf("%s/%s/%u.%u.%u", dest_root, dep.name, got.major, got.minor, got.patch);
    char* dest_parent = strus_newf("%s/%s", dest_root, dep.name);
    yap_rm_rf(dest);
    yap_mkdir_p(dest_parent);
    free(dest_parent);
    /* Staging sits inside the destination tree, so a rename never crosses a filesystem. */
    bool moved = rename(staging, dest) == 0;
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
    yap_mkdir_p(dest_root);

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
