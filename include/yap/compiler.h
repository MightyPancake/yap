#ifndef YAP_COMPILER_H
#define YAP_COMPILER_H

typedef yap_ctx* (*yap_parse_fn)(yap_ctx*, yap_args);
typedef void (*yap_print_error_fn)(yap_error);
typedef yap_macro_val (*yap_macro_eval_fn)(yap_ctx* ctx, yap_macro_val* args, uint32_t arg_count);

typedef struct yap_compiler_front_module{
  yap_parse_fn parse;
  yap_print_error_fn print_error;
}yap_compiler_front_module;

typedef struct yap_compiler_macro_evaluation_module{
  yap_macro_eval_fn macro_eval;
} yap_compiler_macro_evaluation_module;

typedef struct yap_compiler{
  //Handles
  void* front_handle;
  void* back_handle;
  void* macro_eval_handle;
  //Modules
  yap_compiler_front_module front_module;
  yap_compiler_macro_evaluation_module macro_eval_module;
}yap_compiler;

char* yap_get_yap_home_path();
void yap_compiler_load_macro_eval_module(yap_compiler* compiler, const char* path, const char* name);
void yap_compiler_load_back_module(yap_compiler* compiler, const char* path, const char* name);
void yap_compiler_load_front_module(yap_compiler* compiler, const char* path, const char* name);

#endif //YAP_COMPILER_H
