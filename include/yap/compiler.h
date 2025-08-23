#ifndef BOB_COMPILER_H
#define BOB_COMPILER_H

typedef yap_state* (*yap_parse_fn)(yap_args);

typedef struct yap_compiler_front{
  yap_parse_fn parse;
}yap_compiler_front;

typedef struct yap_compiler{
  yap_compiler_front front;
}yap_compiler;

#endif //BOB_COMPILER_H
