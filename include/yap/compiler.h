#ifndef BOB_COMPILER_H
#define BOB_COMPILER_H

typedef yap_ctx* (*yap_parse_fn)(yap_args);
typedef void (*yap_print_error_fn)(yap_error);

typedef struct yap_compiler_front{
  yap_parse_fn parse;
  yap_print_error_fn print_error;
}yap_compiler_front;

typedef struct yap_compiler{
  yap_compiler_front front;
}yap_compiler;

char* yap_get_yap_home_path();

#endif //BOB_COMPILER_H
