#ifndef YAP_COMPILER_H
#define YAP_COMPILER_H

//Parsing
typedef yap_ctx* (*yap_parse_fn)(yap_ctx*, yap_args);
//Error printing
typedef void (*yap_print_error_fn)(yap_error);

//Macros
typedef yap_ctx* (*yap_register_macros_fn)(yap_ctx* ctx);
typedef yap_macro_val (*yap_macro_eval_fn)(yap_ctx* ctx, yap_macro_val* args, uint32_t arg_count);

typedef yap_ctx* (*yap_codegen_fn)(yap_ctx* ctx);
typedef yap_ctx* (*yap_build_fn)(yap_ctx*, yap_args);

typedef struct yap_compiler_front_component{
  yap_parse_fn parse;
  yap_print_error_fn print_error;
}yap_compiler_front_component;

typedef struct yap_compiler_macro_component{
  yap_macro_eval_fn macro_eval;
  yap_register_macros_fn register_macros;
} yap_compiler_macro_component;

typedef struct yap_compiler_backend_component{
  yap_codegen_fn codegen;
}yap_compiler_backend_component;

typedef struct yap_compiler_internal_component{
  yap_build_fn build;
}yap_compiler_internal_component;

typedef struct yap_compiler{
  //Handles
  void* frontend_handle;
  void* backend_handle;
  void* internal_handle;
  void* macro_handle;
  //Components
  yap_compiler_front_component frontend;
  yap_compiler_macro_component macro;
  yap_compiler_backend_component backend;
  yap_compiler_internal_component internal;
}yap_compiler;

char* yap_get_yap_home_path();
void yap_compiler_load_macro_component(yap_compiler* compiler, const char* path, const char* name);
void yap_compiler_load_backend_component(yap_compiler* compiler, const char* path, const char* name);
void yap_compiler_load_internal_component(yap_compiler* compiler, const char* path, const char* name);
void yap_compiler_load_frontend_component(yap_compiler* compiler, const char* path, const char* name);

void yap_free_compiler(yap_compiler compiler);
int yap_early_compile_error_return(yap_compiler compiler, yap_ctx* ctx, int error_code);

#endif //YAP_COMPILER_H
