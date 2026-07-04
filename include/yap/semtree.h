#ifndef YAP_SEMTREE_H
#define YAP_SEMTREE_H

//Types

typedef struct yap_prim_type{
  size_t bytes;
  bool is_signed;
  bool is_float;
  char* name;
  char* c_name;
  char* mangled_name;
}yap_prim_type;

typedef struct yap_block yap_block; //forward declared: yap_expr holds a block* (block_expr), yap_block itself is defined after yap_expr (it holds statements, which hold exprs)

kenobi_new_struct_free(yap_struct_field,
  enum {
    yap_struct_field_error,
    yap_struct_field_valid
  } kind;
  char* name;
  yap_type_id type;
  yap_expr* default_value;
);

kenobi_new_struct_free(yap_struct_type,
  char* name;
  char* c_name;
  darr(yap_struct_field) fields; //pointers to yap_var
);

kenobi_new_struct_free(yap_union_type,
  char* name;
  char* c_name;
  darr(yap_struct_field) variants;
);

kenobi_new_struct_free(yap_enum_variant,
  char* name;
  yap_expr* value;
);

kenobi_new_struct_free(yap_enum_type,
  char* name;
  char* c_name;
  darr(yap_enum_variant) variants;
);

kenobi_new_struct_free(yap_fn_type,
  darr(yap_type_id) args; //pointers to yap_type
  // darr(char*) arg_names; //optional parameter names (NULL if positional-only, otherwise parallel to args)
  yap_type_id return_type; //pointer to return type
);

kenobi_new_struct_free(yap_blob,
  darr(yap_expr) elements;
  darr(char*) names;
  unsigned int field_count;
);

typedef struct yap_array_type {
  yap_type_id element_type;
  size_t size;
} yap_array_type;

typedef struct yap_slice_type {
  yap_type_id element_type;
} yap_slice_type;

kenobi_new_struct_free(yap_type,
  yap_type_kind kind;
  union {
    yap_type_id untyped_default; //for pointers, points to the type it points to
    yap_prim_type primitive;
    yap_type_id pointer_type;
    yap_fn_type func;
    yap_struct_type structure;
    yap_union_type uni;
    yap_enum_type enumeration;
    yap_blob blob;
    yap_array_type array;
    yap_slice_type slice;
    char* hole_name; //yap_type_hole: name of the unfilled $name blueprint type-hole
    yap_error err;
  };
  bool is_const;
);

#define yap_type_qualifier_string_len 16
kenobi_new_struct_free(yap_type_qualifier_strings,
  char restrict_str[yap_type_qualifier_string_len];
  char volatile_str[yap_type_qualifier_string_len];
  char const_str[yap_type_qualifier_string_len];
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

kenobi_new_struct_free(yap_var_declarator,
  char* name;
  bool is_const;
);

kenobi_new_struct_free(yap_literal,
  yap_literal_kind kind;
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
    yap_bin_expr_mod = '%',
    yap_bin_expr_eq = 128,
    yap_bin_expr_neq,
    yap_bin_expr_lt,
    yap_bin_expr_gt,
    yap_bin_expr_le,
    yap_bin_expr_ge
  } op;
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
      yap_expr* left;
      yap_expr* right;
      char op[3]; //'=', '+=', '-=' etc. stored as fixed-width bytes
    };
  };
);

kenobi_new_struct_free(yap_func_call,
  yap_expr* func_expr;
  darr(yap_expr) params;
);

kenobi_new_struct_free(yap_ternary_expr,
  yap_expr* condition;
  yap_expr* then_expr;
  yap_expr* else_expr;
);

kenobi_new_struct_free(yap_member_access,
  yap_expr* object;
  char* member;
);

kenobi_new_struct_free(yap_index_access,
  yap_expr* object;
  yap_expr* index;
);

kenobi_new_struct_free(yap_expr,
  yap_expr_kind kind;
  union {
    yap_error err;
    yap_literal literal;
    yap_bin_expr bin_expr;
    yap_assignment assignment;
    yap_func_call func_call;
    yap_ternary_expr ternary;
    char* var_name;
    yap_expr* subexpr;
    yap_member_access member_access;
    yap_index_access index_access;
    yap_block* block;
  };
  yap_type_id type;
  bool is_lvalue;
  bool is_comptime;
  yap_loc loc;
  yap_code_range range;
);

kenobi_new_struct_free(yap_var_decl,
  enum {
    yap_var_decl_error,
    yap_var_decl_valid
  } kind;
  yap_var var;
  bool has_init;
  yap_expr init;
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
  yap_expr update;
  yap_statement* body;
);

kenobi_new_struct_free(yap_return_statement,
  yap_expr value;
);

kenobi_new_struct_free(yap_block,
  enum {
    yap_block_error,
    yap_block_valid,
    yap_block_none
  } kind;
  union {
    darr(yap_statement) statements;
    yap_error err;
  };
);

kenobi_new_struct_free(yap_statement,
  yap_statement_kind kind;
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
  yap_loc loc;
  yap_code_range range;
);

/* Growable comptime handle lists — backing storage for yExprList/yStmtList,
 * the macro-side vehicle for passing/building a variable number of yExpr or
 * yStmt values through a single fixed-arity comptime call argument. */
typedef struct {
  yap_expr* items;
  unsigned int count;
  unsigned int cap;
} yap_expr_list;

typedef struct {
  yap_statement* items;
  unsigned int count;
  unsigned int cap;
} yap_stmt_list;

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
  char* name;
  darr(yap_func_arg) args;
  yap_type_id ret_typ;
  yap_block body;
);

kenobi_new_struct_free(yap_scope,
  void* parent;
  map variables;
  bool is_loop; //Whether this scope is a loop scope, used for break/continue statements
);

kenobi_new_struct_free(yap_named_type_decl,
  char* name;
  char* c_name;
  yap_named_type_decl_kind kind;
  yap_type_id type_id;
);

kenobi_new_struct_free(yap_decl,
  yap_decl_kind kind;
  union{
    yap_func_decl func_decl;
    yap_named_type_decl named_type_decl;
    yap_error err;
  };
  yap_loc loc;
  yap_code_range range;
  char* module_prefix;
);

kenobi_new_struct_free(yap_incremental_val,
  enum {
    yap_incremental_val_error,
    yap_incremental_val_expr,
    yap_incremental_val_decl,
    yap_incremental_val_stmt,
  } kind;
  union {
    yap_error err;
    yap_expr expr;
    yap_decl decl;
    yap_statement stmt;
  };
);

#endif //YAP_SEMTREE_H