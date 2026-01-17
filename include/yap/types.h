#ifndef YAP_TYPES_H
#define YAP_TYPES_H

//Hashmap
#include "hashmap.h"
typedef struct hashmap* map;

#include "config.h"
#include "utils/utils.h"

//Helper macros
#define base_type(PT) __typeof__(* ( (PT) 0 ))
#define ptr_type(T) __typeof__(T)*

typedef struct yap_args{
  char* output_file;
  darr extra;
  bool show_modules_path;
  char* command;
}yap_args;

//Types
typedef struct yap_source{
  void* parent;
  char* path;
  size_t sz;
  char* content;
}yap_source;

kenobi_new_struct_free(yap_code_pos,
  int line;
  int column;
  int offset; //bytes
);

kenobi_new_struct_free(yap_code_range,
  yap_code_pos start;
  yap_code_pos end;
);

kenobi_new_struct_free(yap_error,
  enum {
    yap_error_no_pos,
    yap_error_pos,
    yap_error_calc_offset
  } kind;
  yap_source* src;
  yap_code_range range;
  char* msg;
);


typedef enum yap_type_kind{
  yap_type_primitive,
  yap_type_ptr,
  yap_type_fn
}yap_type_kind;

typedef struct yap_prim_type{
  char* name;
  size_t sz;
}yap_prim_type;

typedef struct yap_type{
  yap_type_kind kind;
  union {
    yap_prim_type primitive;
    void* subtype;
  };
}yap_type;

typedef struct yap_var{
  char* name;
  yap_type type;
}yap_var;

kenobi_new_struct_free(yap_blob,
  //TODO: Finish blobs
  unsigned int field_count;
);

kenobi_new_struct_free(yap_literal,
  enum {
    yap_literal_error,
    yap_literal_numerical,
    yap_literal_blob,
  } kind;
  union {
    yap_blob blob;
    char* text;
    
  };
);

kenobi_new_struct_free(yap_bin_expr,
  enum {
    yap_bin_expr_error = 0,
    yap_bin_expr_add = '+',
    yap_bin_expr_sub = '-',
    yap_bin_expr_mul = '*',
    yap_bin_expr_div = '/',
    yap_bin_expr_mod = '%'
  } kind;
  union {
    struct {
      void* right; //yap_expr*
      void* left;  //yap_expr*
    };
    yap_error err;
  };
);

kenobi_new_struct_free(yap_lvalue,
  enum {
    yap_lvalue_error,
    yap_lvalue_var
  } kind;
  union {
    yap_error err;
    struct {
      yap_type type;
      yap_var var;
    };
  };
);

kenobi_new_struct_free(yap_assignment,
  enum {
    yap_assignment_error,
    yap_assignment_valid
  } kind;
  union {
    yap_error err;
    struct {
      void* left; //yap_expr*
      void* right; //yap_expr*
      char op; //'=', '+', '-' etc.
    };
  };
);


kenobi_new_struct_free(yap_expr,
  enum {
    yap_expr_error,
    yap_expr_literal,
    yap_expr_var,
    yap_expr_bin,
    yap_expr_assignment,
  } kind;
  union {
    yap_error err;
    yap_literal literal;
    yap_bin_expr bin_expr;
    yap_assignment assignment;
  };
  yap_type type;
  bool is_lvalue;
  bool is_comptime;
);


kenobi_new_struct_free(yap_statement,
  enum {
    yap_statement_error,
    yap_statement_empty,
    yap_statement_expr,
  } kind;
  union {
    yap_error err;
    yap_expr expr;
  };
);

typedef struct yap_block{
  enum {
    yap_block_error,
    yap_block_valid
  } kind;
  union {
    darr statements;
    yap_error err;
  };
}yap_block;

typedef struct yap_func_def{
  darr args;
  yap_type ret_typ;
  yap_block body;
}yap_func_def;

typedef struct yap_scope{
  void* parent;
  map variables;
}yap_scope;
void yap_scope_free();

typedef struct yap_def{
  enum {
    yap_def_error,
    yap_def_null,
    yap_def_func
  } kind;
  union{
    yap_func_def func_def;
    yap_error err;
  };
}yap_def;
void yap_def_free(yap_def def);

kenobi_new_struct_free(yap_ctx,
  darr sources;
  darr source_codes;
  yap_scope* scope;
);
yap_ctx* yap_ctx_new();

typedef struct yap_source_code{
  darr definitions;
}yap_source_code;
void yap_source_code_free(yap_source_code src_code);

//Hashmap functions for types
#define new_map(typ, hsh, comp) ((map)(hashmap_new(sizeof(typ), 0, 0, 0, hsh, comp, NULL, NULL)))
#define define_map_for(T) \
uint64_t map_hash_##T##_f(const void *item, uint64_t seed0, uint64_t seed1); \
int map_cmp_##T##_f(const void* a, const void* b, void *udata); \
map new_##T##_map();

#define declare_map_for(T) \
uint64_t map_hash_##T##_f(const void *item, uint64_t seed0, uint64_t seed1) {return hashmap_murmur(((ptr_type(yap_##T))item)->name, strlen(((ptr_type(yap_##T))item)->name), seed0, seed1);} \
int map_cmp_##T##_f(const void* a, const void* b, void *udata){return strcmp(((ptr_type(yap_##T))(a))->name, ((ptr_type(yap_##T))(b))->name); } \
map new_##T##_map(){return new_map(yap_##T, map_hash_##T##_f, map_cmp_##T##_f);}

#endif //YAP_TYPES_H
