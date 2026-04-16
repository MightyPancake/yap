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
  darr(char*) extra;
  bool show_modules_path;
  char* command;
}yap_args;

//Types
typedef struct yap_source{
  void* parent;
  char* path;
  size_t sz;
  char* content;
  void* ctx;
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
    yap_error_no_pos, //Errors without position (ie. no source)
    yap_error_pos, //Errors with position
    yap_error_calc_offset //Not used yet
  } kind;
  yap_source* src;
  yap_code_range range;
  char* msg;
);

typedef uint32_t yap_type_id;

typedef enum yap_type_kind{
  yap_type_untyped, //Default type for literals and variables before type inference
  yap_type_primitive, //Primitive types like int, float, byte etc.
  yap_type_ptr, //Pointer to another type, subtype is the type it points to
  yap_type_func,
  yap_type_struct,
  yap_type_blob,
  yap_type_error,
}yap_type_kind;

typedef struct yap_prim_type{
  size_t bytes;
  bool is_signed;
  bool is_float;
  char* name;
  char* mangled_name;
}yap_prim_type;

kenobi_new_struct_free(yap_struct_type,
  char* name;
  char* c_name;
  darr(yap_type_id) fields; //pointers to yap_var
);

kenobi_new_struct_free(yap_fn_type,
  darr(yap_type_id) args; //pointers to yap_type
  // darr(char*) arg_names; //optional parameter names (NULL if positional-only, otherwise parallel to args)
  yap_type_id return_type; //pointer to return type
);

kenobi_new_struct_free(yap_blob,
  //TODO: Finish blobs aka literals for structs and arrays
  unsigned int field_count;
);

kenobi_new_struct_free(yap_type,
  yap_type_kind kind;
  union {
    yap_type_id untyped_default; //for pointers, points to the type it points to
    yap_prim_type primitive;
    yap_type_id pointer_type;
    yap_fn_type func;
    yap_struct_type structure;
    yap_blob blob;
    yap_error err;
  };
);

kenobi_new_struct_free(yap_named_type,
  yap_type_id id;
  char* name;
  char* c_name;
);

kenobi_new_struct_free(yap_var,
  char* name;
  yap_type_id type;
);

typedef struct yap_expr yap_expr;

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
      yap_expr* right; //yap_expr*
      yap_expr* left;  //yap_expr*
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
      yap_type_id type;
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
      yap_expr* left; //yap_expr*
      yap_expr* right; //yap_expr*
      char op; //'=', '+', '-' etc.
    };
  };
);

kenobi_new_struct_free(yap_func_call,
  yap_expr* func_expr;
  darr(yap_expr) params;
);

kenobi_new_struct_free(yap_expr,
  enum {
    yap_expr_error,
    yap_expr_literal,
    yap_expr_var,
    yap_expr_bin,
    yap_expr_assignment,
    yap_expr_func_call
  } kind;
  union {
    yap_error err;
    yap_literal literal;
    yap_bin_expr bin_expr;
    yap_assignment assignment;
    yap_func_call func_call;
  };
  yap_type_id type;
  bool is_lvalue;
  bool is_comptime;
);

typedef struct yap_statement yap_statement;

kenobi_new_struct_free(yap_var_decl,
  enum {
    yap_var_decl_error,
    yap_var_decl_valid
  } kind;
  yap_var var;
  yap_expr expr;
);

kenobi_new_struct_free(yap_if,
  yap_expr condition;
  yap_statement* then_branch;
);

kenobi_new_struct_free(yap_if_else,
  yap_expr condition;
  yap_statement* then_branch;
  yap_statement* else_branch;
);

kenobi_new_struct_free(yap_while,
  yap_expr condition;
  yap_statement* body;
);

kenobi_new_struct_free(yap_for,
  yap_statement* init;
  yap_expr condition;
  yap_expr* update;
  yap_statement* body;
);

kenobi_new_struct_free(yap_return_statement,
  yap_expr value;
);

kenobi_new_struct_free(yap_block,
  enum {
    yap_block_error,
    yap_block_valid
  } kind;
  union {
    darr(yap_statement) statements;
    yap_error err;
  };
);

kenobi_new_struct_free(yap_statement,
  enum {
    yap_statement_error,
    yap_statement_empty,
    yap_statement_expr,
    yap_statement_var_decl,
    yap_statement_return,
    yap_statement_if,
    yap_statement_if_else,
    yap_statement_while,
    yap_statement_for,
    yap_statement_break,
    yap_statement_continue,
    yap_statement_block
  } kind;
  union {
    yap_error err;
    yap_expr expr;
    yap_var_decl var_decl;
    yap_return_statement return_stmt;
    yap_if if_stmt;
    yap_if_else if_else_stmt;
    yap_while while_stmt;
    yap_for for_stmt;
    yap_block block;
  };
);

kenobi_new_struct_free(yap_func_arg,
  enum {
    yap_func_arg_error,
    yap_func_arg_valid
  } kind;
  union {
    yap_error err;
    struct {
      char* name;
      yap_type_id type;
      yap_expr default_value; //optional default value for the argument, if kind is yap_func_arg_error then this is not used
    };
  };
);

kenobi_new_struct_free(yap_func_decl,
  darr(yap_func_arg) args;
  yap_type_id ret_typ;
  yap_block body;
);

kenobi_new_struct_free(yap_scope,
  void* parent;
  map variables;
  bool is_loop; //Whether this scope is a loop scope, used for break/continue statements
);

kenobi_new_struct_free(yap_module,
  char* name;
  yap_scope* scope;
);

kenobi_new_struct_free(yap_decl,
  enum {
    yap_decl_error,
    yap_decl_null,
    yap_decl_func
  } kind;
  union{
    yap_func_decl func_decl;
    yap_error err;
  };
);

kenobi_new_struct_free(yap_macro_val,
  enum {
    yap_macro_val_error,
    yap_macro_val_expr,
    yap_macro_val_decl,
    yap_macro_val_stmt,
  } kind;
  union {
    yap_error err;
    yap_expr expr;
    yap_decl decl;
    yap_statement stmt;
  };
);

typedef struct yap_source_code yap_source_code;

kenobi_new_struct_free(yap_ctx,
  //Arena
  quake arena; //Memory arena for all allocations in the compiler, freed at the end of compilation. Speeds up allocation and deallocation significantly.
  darr(yap_source) sources; //darr of yap_source, represents the source files being compiled.
  darr(yap_source_code) source_codes; //darr of yap_source_code
  darr(yap_error) errors; //darr of yap_error
  //TODO: Do we make scopes dynamic and lose them after parsing or introduce a new 'scope'
  darr(yap_scope*) scopes; //Array holding all scopes
  darr(yap_scope*) current_scopes; //stack of scopes for codegen. Top is current, bottom is global.
  yap_scope* global_scope;

  //Modules
  map modules; //map of named modules
  yap_module* current_module;

  //Types
  darr(yap_type) types; //yap_type_id points to types in this array
  map named_types; //map of named types
  //Cached type ids for primitives and untyped literals for fast access during parsing and type inference
  yap_type_id void_type_id; //cached type_id for void
  yap_type_id int_type_id;  //cached type_id for i32
  yap_type_id bool_type_id; //cached type_id for bool
  yap_type_id float_type_id; //cached type_id for f32
  yap_type_id untyped_int_type_id;  //cached type_id for untyped integer literals
  yap_type_id untyped_float_type_id; //cached type_id for untyped float literals
  yap_type_id untyped_byte_type_id;  //cached type_id for untyped byte literals
);
yap_ctx* yap_ctx_new();

typedef struct yap_source_code{
  darr(yap_decl) declarations;
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
int map_cmp_##T##_f(const void* a, const void* b, void *udata){(void)udata; return strcmp(((ptr_type(yap_##T))(a))->name, ((ptr_type(yap_##T))(b))->name); } \
map new_##T##_map(){return new_map(yap_##T, map_hash_##T##_f, map_cmp_##T##_f);}

#endif //YAP_TYPES_H
