#include "yap/all.h"
#include <dirent.h>

// Whether the selected backend compiler targets wasm (emcc), not native ELF. A native .so
// can never link into a wasm build, so module library scanning needs to know which flavor
// of artifact to look for. Re-derives the '-bcc=' parsing yap-c's own resolver does, since
// core can't reach into a dlopen'd backend component to ask it directly.
static bool yap_target_is_wasm(yap_args* args){
    if (!args) return false;
    for_darr(i, flag, args->backend_flags){
        if (flag && strncmp(flag, "cc=", 3) == 0 && strstr(flag + 3, "emcc"))
            return true;
    }
    return false;
}

static void yap_resolve_module_version(yap_ctx* ctx, yap_source* src, yap_module_decl_node* mdecl, yap_version* out){
    if (!mdecl->version) return;
    if (yap_version_parse(mdecl->version, out)) return;

    yap_ctx_push_error(ctx, (yap_error){
        .kind  = yap_error_pos,
        .src   = src,
        .range = mdecl->loc.range,
        .loc   = mdecl->loc,
        .msg   = strus_newf("Invalid version '%s' in module '%s'; expected major.minor.patch",
                            mdecl->version, mdecl->name.value ? mdecl->name.value : "(anon)")
    });
}

/* A module that declares itself opts into its deps being authoritative; a plain
 * program or script declares none and is left alone. */
static void yap_check_module_imports(yap_ctx* ctx){
    for_darr(si, src, ctx->sources){
        if (!src || !src->source_node) continue;

        yap_module* owner = src->from_module_import
            ? yap_ctx_get_module(ctx, src->from_module_import)
            : yap_ctx_current_module(ctx);
        if (!owner || !owner->declared) continue;

        for_darr(ii, imp, src->imports){
            if (imp.kind != yap_import_module || !imp.module_name) continue;

            bool listed = false;
            if (owner->deps){
                for_darr(di, dep, owner->deps){
                    if (dep.name && strcmp(dep.name, imp.module_name) == 0){ listed = true; break; }
                }
            }
            if (listed) continue;

            yap_ctx_push_error(ctx, (yap_error){
                .kind  = yap_error_pos,
                .src   = src,
                .range = imp.loc.range,
                .loc   = imp.loc,
                .msg   = strus_newf("Module '%s' imports '%s', which is not listed in its deps",
                                    owner->name, imp.module_name)
            });
        }
    }
}

void yap_resolve_module_decl(yap_ctx* ctx){
    yap_log("\n\nPhase 0: Module declaration resolution\n");

    // Pass 1: Resolve the user's own module (skip sources from module imports)
    yap_module_decl_node* first_decl = NULL;
    yap_source* first_src = NULL;

    for_darr(si, src, ctx->sources){
        if (!src || !src->source_node) continue;
        if (src->from_module_import) continue;
        yap_source_node* snode = src->source_node;

        for_darr(di, dnode, snode->declarations){
            if (dnode.kind != yap_decl_module_decl) continue;
            yap_module_decl_node* mdecl = &snode->declarations[di].module_decl;

            if (!first_decl){
                first_decl = mdecl;
                first_src = src;
                yap_log("Found module declaration '%s' in %s",
                    mdecl->name.value ? mdecl->name.value : "(anon)",
                    src->label ? src->label : "(unknown)");
                continue;
            }

            char* msg = strus_newf(
                "Duplicate module declaration '%s' (first declared as '%s' at %s:%d:%d)",
                mdecl->name.value ? mdecl->name.value : "(anon)",
                first_decl->name.value ? first_decl->name.value : "(anon)",
                first_src->label ? first_src->label : "(unknown)",
                first_decl->loc.range.start.line + 1,
                first_decl->loc.range.start.column + 1);
            yap_ctx_push_error(ctx, (yap_error){
                .kind = yap_error_pos,
                .src = src,
                .range = mdecl->loc.range,
                .loc = mdecl->loc,
                .msg = msg
            });
        }
    }

    char* mod_name;
    char* mod_prefix;
    yap_version mod_version = {0};

    if (first_decl){
        mod_name = first_decl->name.value ? first_decl->name.value : "main";
        if (first_decl->prefix){
            mod_prefix = first_decl->prefix;
        } else {
            mod_prefix = yap_ctx_strus_newf(ctx, "%s_", mod_name);
        }
        yap_resolve_module_version(ctx, first_src, first_decl, &mod_version);
    } else {
        mod_name = "main";
        mod_prefix = "";
    }

    yap_log("Resolved module: name='%s' prefix='%s' version=%u.%u.%u",
        mod_name, mod_prefix, mod_version.major, mod_version.minor, mod_version.patch);
    yap_module* root_mod = yap_ctx_create_new_module(ctx, mod_name, mod_prefix, mod_version);
    if (root_mod && first_decl){
        root_mod->declared = true;
        root_mod->deps = first_decl->deps;
    }
    yap_ctx_switch_module(ctx, mod_name);

    // Pass 2: Register imported modules from module-imported sources
    for_darr(si, src, ctx->sources){
        if (!src || !src->source_node) continue;
        if (!src->from_module_import) continue;
        yap_source_node* snode = src->source_node;

        for_darr(di, dnode, snode->declarations){
            if (dnode.kind != yap_decl_module_decl) continue;
            yap_module_decl_node* mdecl = &snode->declarations[di].module_decl;

            char* imp_name = mdecl->name.value ? mdecl->name.value : src->from_module_import;
            char* imp_prefix = mdecl->prefix ? mdecl->prefix : yap_ctx_strus_newf(ctx, "%s_", imp_name);
            yap_version imp_version = {0};
            yap_resolve_module_version(ctx, src, mdecl, &imp_version);

            if (!yap_ctx_get_module(ctx, imp_name)){
                yap_log("Registering imported module: name='%s' prefix='%s'", imp_name, imp_prefix);
                yap_ctx_create_new_module(ctx, imp_name, imp_prefix, imp_version);

                yap_module* imp_mod = yap_ctx_get_module(ctx, imp_name);
                if (imp_mod) {
                    imp_mod->declared = true;
                    imp_mod->deps = mdecl->deps;
                    bool wasm_target = yap_target_is_wasm(ctx->args);
                    /* Taken from the mod.yp actually loaded, not rebuilt from the lookup path,
                     * so a module under <name>/<version>/ finds the libraries beside it. */
                    char* mod_dir = yap_get_parent_dir(src->origin);
                    {
                        DIR* dir = mod_dir ? opendir(mod_dir) : NULL;
                        if (dir) {
                            struct dirent* ent;
                            while ((ent = readdir(dir)) != NULL) {
                                size_t nlen = strlen(ent->d_name);
                                bool has_lib_prefix = nlen > 3
                                    && ent->d_name[0] == 'l' && ent->d_name[1] == 'i' && ent->d_name[2] == 'b';
                                bool is_wasm_a = has_lib_prefix && nlen > 7
                                    && strcmp(ent->d_name + nlen - 7, "_wasm.a") == 0;
                                bool is_a = has_lib_prefix && nlen > 2
                                    && strcmp(ent->d_name + nlen - 2, ".a") == 0 && !is_wasm_a;
                                bool is_so = has_lib_prefix && nlen > 3
                                    && strcmp(ent->d_name + nlen - 3, ".so") == 0;
                                // A native .so can never link into a wasm build, and a wasm-flavored
                                // .a should never be picked up by a native build even if both sit in
                                // the same module directory -- only one flavor is ever collected here.
                                bool is_lib = wasm_target ? is_wasm_a : (is_a || is_so);
                                if (is_lib) {
                                    char* lib_path = strus_newf("%s/%s", mod_dir, ent->d_name);
                                    darr_push(imp_mod->lib_paths, lib_path);
                                    yap_log("Module '%s': found library '%s'", imp_name, lib_path);
                                }
                                // native_lib_paths is collected independent of wasm_target: macro-typed
                                // function bodies run through an embedded host-native TCC regardless of
                                // the selected backend, and TCC can never load wasm object code -- it
                                // always needs the native .a/.so flavor to resolve symbols, even when
                                // the final link (lib_paths above) targets wasm.
                                if (is_a || is_so) {
                                    char* native_lib_path = strus_newf("%s/%s", mod_dir, ent->d_name);
                                    darr_push(imp_mod->native_lib_paths, native_lib_path);
                                    yap_log("Module '%s': found native library '%s'", imp_name, native_lib_path);
                                }
                            }
                            closedir(dir);
                        }
                        free(mod_dir);
                    }
                }
            }
        }
    }

    yap_check_module_imports(ctx);
}
