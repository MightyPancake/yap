#ifndef YAP_BINDGEN_H
#define YAP_BINDGEN_H

#include <clang-c/Index.h>  // libclang
#include "yap/types.h"

// A single imported C function, ready to be turned into a yap extern decl.
typedef struct yap_imported_func {
  char        *c_name;                // e.g. "printf"
  char        *c_return_spelling;     // e.g. "int"
  darr(char *) c_param_spellings;     // e.g. ["const char*", "..."]
  yap_func_decl yap_sig;             // yap-level representation
} yap_imported_func;

// Parse a C header file and return the collected exports.
void yap_bindgen_import(yap_ctx *ctx,
                        const char *header_path,
                        int clang_argc,
                        const char **clang_argv);

// --gen-c-bind CLI command: parse header, write binds.yap to outdir.
int yap_gen_c_bind(yap_args args);

#endif // YAP_BINDGEN_H
