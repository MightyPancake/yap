#ifndef YAP_IMPORTS_H
#define YAP_IMPORTS_H

void yap_expand_imports(yap_ctx* ctx);
void yap_resolve_module_decl(yap_ctx* ctx);
int yap_fetch_deps(yap_ctx* ctx, yap_args args);
int yap_install(yap_ctx* ctx, const char* where, bool global);

#endif //YAP_IMPORTS_H