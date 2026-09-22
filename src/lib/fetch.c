#include "yap/all.h"
#include <sys/stat.h>
#include <dirent.h>
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

/* Same no-shell rule as yap_exec, but the child's stdout is read back. */
static char* yap_exec_capture(char* const argv[]){
    int fds[2];
    if (pipe(fds) != 0) return NULL;

    pid_t pid = fork();
    if (pid < 0){ close(fds[0]); close(fds[1]); return NULL; }
    if (pid == 0){
        close(fds[0]);
        dup2(fds[1], STDOUT_FILENO);
        close(fds[1]);
        execvp(argv[0], argv);
        _exit(127);
    }
    close(fds[1]);

    char buf[256];
    ssize_t n = read(fds[0], buf, sizeof(buf) - 1);
    close(fds[0]);
    int status = 0;
    waitpid(pid, &status, 0);
    if (n <= 0 || !WIFEXITED(status) || WEXITSTATUS(status) != 0) return NULL;

    buf[n] = '\0';
    for (char* c = buf; *c; c++) if (*c == '\n' || *c == '\r'){ *c = '\0'; break; }
    return strus_copy(buf);
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
typedef struct { char* name; yap_version version; char* git; char* ref; char* rev; } yap_lock_entry;

static bool yap_fetch_git_dep(yap_ctx* ctx, yap_dep_node dep, const char* dest_root, char** out_dest, yap_lock_entry* out_lock){
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
    if (moved){
        printf("  fetched %s %u.%u.%u\n", dep.name, got.major, got.minor, got.patch);
        if (out_lock){
            char* rev_argv[] = { "git", "-C", dest, "rev-parse", "HEAD", NULL };
            *out_lock = (yap_lock_entry){
                .name = dep.name, .version = got, .git = dep.git,
                .ref = dep.tag ? dep.tag : (dep.branch ? dep.branch : dep.rev),
                .rev = yap_exec_capture(rev_argv)
            };
        }
        if (out_dest) *out_dest = dest; else free(dest);
    } else free(dest);
    free(staging);
    return moved;
}

/* A project names its manifest the same way a module does, so mod.yp wins; main.yp is
 * the conventional entry point for a program. Anything else is ambiguous and says so. */
static char* yap_find_project_manifest(yap_ctx* ctx, const char* dir){
    const char* candidates[] = { "mod.yp", "main.yp", NULL };
    for (const char** c = candidates; *c; c++){
        char* path = strus_newf("%s/%s", dir, *c);
        if (access(path, R_OK) == 0) return path;
        free(path);
    }

    /* Fall back to a lone .yp that declares a module. */
    char* found = NULL;
    unsigned matches = 0;
    DIR* d = opendir(dir);
    if (d){
        struct dirent* ent;
        while ((ent = readdir(d)) != NULL){
            size_t n = strlen(ent->d_name);
            if (n < 4 || strcmp(ent->d_name + n - 3, ".yp") != 0) continue;
            char* path = strus_newf("%s/%s", dir, ent->d_name);
            yap_module_decl_node m = {0};
            if (ctx->read_manifest && ctx->read_manifest(ctx, path, &m)){
                matches++;
                if (!found) found = path; else free(path);
            } else free(path);
        }
        closedir(d);
    }
    if (matches == 1) return found;
    free(found);
    return NULL;
}

/* The lock is a manifest with every constraint already resolved, so the compiler reads it
 * with the same machinery and nothing new has to parse it. */
/* A module fetched from git arrives as source, so its wrapper is compiled here the way
 * `make native_modules` does for the in-tree ones -- otherwise the final link has no
 * library to resolve the module's C bindings against. */
static bool yap_build_native_module(const char* dir, const char* name){
    char* wrapper = strus_newf("%s/wrapper.c", dir);
    if (access(wrapper, R_OK) != 0){ free(wrapper); return true; }

    char* cc = getenv("CC");
    if (!cc || !cc[0]) cc = "gcc";

    char* obj = strus_newf("%s/wrapper.o", dir);
    char* archive = strus_newf("%s/lib%s.a", dir, name);
    char* shared = strus_newf("%s/lib%s.so", dir, name);

    char* compile[] = { cc, "-fPIC", "-fvisibility=hidden", "-I", (char*)dir, "-c", wrapper, "-o", obj, NULL };
    char* ar_argv[] = { "ar", "rcs", archive, obj, NULL };
    char* link[]    = { cc, "-shared", "-o", shared, obj, NULL };

    bool ok = yap_exec(compile) && yap_exec(ar_argv) && yap_exec(link);
    if (ok) printf("  built lib%s for %s\n", name, name);
    else printf("  could not build lib%s; the module's C bindings will not link\n", name);

    free(wrapper); free(obj); free(archive); free(shared);
    return ok;
}

static void yap_write_lock(const char* dir, darr(yap_lock_entry) locked){
    char* path = strus_newf("%s/yap.lock", dir);
    FILE* f = fopen(path, "w");
    if (!f){ free(path); return; }

    fprintf(f, "// Generated by 'yap install'. Do not edit.\n");
    fprintf(f, "module yap_lock {\n    version: \"0.0.0\",\n    deps: [\n");
    for_darr(i, e, locked){
        fprintf(f, "        { name: \"%s\", version: \"%u.%u.%u\"", e.name, e.version.major, e.version.minor, e.version.patch);
        if (e.git) fprintf(f, ", git: \"%s\"", e.git);
        if (e.rev) fprintf(f, ", rev: \"%s\"", e.rev);
        fprintf(f, " },\n");
    }
    fprintf(f, "    ],\n}\n");
    fclose(f);
    printf("Wrote %s\n", path);
    free(path);
}

static bool yap_already_fetched(darr(char*) done, char* name){
    for_darr(i, n, done) if (strcmp(n, name) == 0) return true;
    return false;
}

/* Walks the graph rather than just the root manifest: a fetched module's own git deps are
 * queued as it lands, so one install brings in everything a build will look for. */
int yap_install(yap_ctx* ctx, const char* where){
    char* dir = yap_resolve_path(where && where[0] ? where : ".");
    if (!dir){
        printf("No such directory: %s\n", where ? where : ".");
        return 1;
    }

    char* manifest_path = yap_find_project_manifest(ctx, dir);
    if (!manifest_path){
        printf("No manifest found in %s (looked for mod.yp, main.yp, or a single .yp declaring a module)\n", dir);
        free(dir);
        return 1;
    }

    yap_module_decl_node manifest = {0};
    if (!ctx->read_manifest || !ctx->read_manifest(ctx, manifest_path, &manifest)){
        printf("%s has no module declaration\n", manifest_path);
        free(manifest_path); free(dir);
        return 1;
    }
    printf("Installing dependencies for %s\n", manifest_path);

    char* dest_root = strus_newf("%s/.yap/modules", dir);
    yap_mkdir_p(dest_root);

    darr(yap_dep_node) queue = darr_new(yap_dep_node);
    darr(char*) done = darr_new(char*);
    darr(yap_lock_entry) locked = darr_new(yap_lock_entry);
    if (manifest.deps) for_darr(i, d, manifest.deps) darr_push(queue, d);

    unsigned fetched = 0, failed = 0, skipped = 0;
    for (size_t qi = 0; qi < darr_len(queue); qi++){
        yap_dep_node dep = queue[qi];
        if (!dep.name || yap_already_fetched(done, dep.name)) continue;

        if (dep.registry || dep.path){
            printf("  skipping %s: only git sources are fetched for now\n", dep.name);
            skipped++;
            continue;
        }
        if (!dep.git){ skipped++; continue; }

        char* landed = NULL;
        yap_lock_entry entry = {0};
        if (!yap_fetch_git_dep(ctx, dep, dest_root, &landed, &entry)){ failed++; continue; }
        darr_push(locked, entry);
        fetched++;
        darr_push(done, dep.name);
        if (landed) yap_build_native_module(landed, dep.name);

        /* Whatever just landed may itself depend on something. */
        char* sub_manifest = landed ? yap_find_project_manifest(ctx, landed) : NULL;
        free(landed);
        if (!sub_manifest) continue;
        yap_module_decl_node sub_decl = {0};
        if (ctx->read_manifest(ctx, sub_manifest, &sub_decl) && sub_decl.deps)
            for_darr(k, d, sub_decl.deps) darr_push(queue, d);
        free(sub_manifest);
    }

    if (!failed) yap_write_lock(dir, locked);

    printf("%u fetched, %u failed, %u skipped\n", fetched, failed, skipped);
    for_darr(i, e, locked) free(e.rev);
    darr_free(locked);
    darr_free(queue); darr_free(done);
    free(dest_root); free(manifest_path); free(dir);
    return failed ? 1 : 0;
}

int yap_fetch_deps(yap_ctx* ctx, yap_args args){
    if (darr_len(args.extra) == 0) return yap_install(ctx, ".");
    char* dir = yap_get_parent_dir(darr_first(args.extra));
    int rc = yap_install(ctx, dir ? dir : ".");
    free(dir);
    return rc;
}
