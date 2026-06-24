#include "yap/all.h"

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
            }
        }
    }
}
