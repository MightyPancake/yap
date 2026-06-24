#ifndef YAP_COMPILER_H
#define YAP_COMPILER_H

#include "yap/bindgen.h"

//Parsing
typedef yap_ctx* (*yap_parse_fn)(yap_ctx*, yap_args);
//Error printing
typedef void (*yap_print_error_fn)(yap_error);

typedef yap_ctx* (*yap_build_fn)(yap_ctx*, yap_args);
typedef yap_ctx* (*yap_emit_fn)(yap_ctx* ctx);
typedef void (*yap_backend_init_fn)(yap_ctx* ctx);
typedef void (*yap_backend_free_fn)(yap_ctx* ctx);

typedef struct yap_compiler_front_component{
  yap_parse_fn parse;
  yap_print_error_fn print_error;
}yap_compiler_front_component;

typedef struct yap_compiler_semantic_component{
  yap_build_fn build;
}yap_compiler_semantic_component;

typedef struct yap_compiler_backend_component{
  yap_backend_init_fn init;
  yap_backend_free_fn free;
  yap_gen_decl_fn gen_decl;
  yap_emit_fn emit;
}yap_compiler_backend_component;

typedef struct yap_compiler{
  //Handles
  void* frontend_handle;
  void* backend_handle;
  void* semantic_handle;
  //Components
  yap_compiler_front_component frontend;
  yap_compiler_semantic_component semantic;
  yap_compiler_backend_component backend;
  //State
  yap_args* args; //Owned args, freed by yap_free_compiler
}yap_compiler;

char* yap_get_yap_home_path();
void yap_compiler_load_frontend_component(yap_compiler* compiler, const char* path, const char* name);
void yap_compiler_load_backend_component(yap_compiler* compiler, const char* path, const char* name);
void yap_compiler_load_semantic_component(yap_compiler* compiler, const char* path, const char* name);

void yap_free_compiler(yap_compiler compiler);
int yap_early_compile_error_return(yap_compiler compiler, yap_ctx* ctx, int error_code);

#endif //YAP_COMPILER_H
