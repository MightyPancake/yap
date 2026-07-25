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

    if (first_decl){
        mod_name = first_decl->name.value ? first_decl->name.value : "main";
        if (first_decl->prefix){
            mod_prefix = first_decl->prefix;
        } else {
            mod_prefix = yap_ctx_strus_newf(ctx, "%s_", mod_name);
        }
        if (first_decl->version){
            yap_log("Module version: %s", first_decl->version);
        }
    } else {
        mod_name = "main";
        mod_prefix = "";
    }

    yap_log("Resolved module: name='%s' prefix='%s'", mod_name, mod_prefix);
    yap_ctx_create_new_module(ctx, mod_name, mod_prefix);
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

            if (!yap_ctx_get_module(ctx, imp_name)){
                yap_log("Registering imported module: name='%s' prefix='%s'", imp_name, imp_prefix);
                yap_ctx_create_new_module(ctx, imp_name, imp_prefix);

                yap_module* imp_mod = yap_ctx_get_module(ctx, imp_name);
                if (imp_mod) {
                    bool wasm_target = yap_target_is_wasm(ctx->args);
                    for_darr(pi, lookup_path, ctx->module_lookup_paths){
                        char* mod_dir = strus_newf("%s/%s", lookup_path, imp_name);
                        DIR* dir = opendir(mod_dir);
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
}
