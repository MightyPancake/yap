#ifndef YAP_NODE_KINDS_H
#define YAP_NODE_KINDS_H

//Declarations
typedef enum {
    yap_decl_error,
    yap_decl_func,
    yap_decl_named_type,
    //TODO: Add missing declaration kinds
} yap_decl_kind;

typedef enum {
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
} yap_statement_kind;

typedef enum {
    yap_expr_error,
    yap_expr_literal,
    yap_expr_var,
    yap_expr_bin,
    yap_expr_assignment,
    yap_expr_func_call,
    yap_expr_cast,
    yap_expr_at_op,
    yap_expr_paren,
    yap_expr_increment,
    yap_expr_decrement,
    yap_expr_ternary,
    yap_expr_member_access,
} yap_expr_kind;

typedef enum {
    yap_literal_error,
    yap_literal_numerical,
    yap_literal_blob,
} yap_literal_kind;

typedef enum {
    yap_named_type_decl_error,
    yap_named_type_decl_alias,
    yap_named_type_decl_struct,
    yap_named_type_decl_enum,
    yap_named_type_decl_union,
} yap_named_type_decl_kind;

typedef enum {
  yap_type_untyped, //Default type for literals and variables before type inference
  yap_type_primitive, //Primitive types like int, float, byte etc.
  yap_type_ptr, //Pointer to another type, subtype is the type it points to
  yap_type_func,
  yap_type_struct,
  yap_type_union,
  yap_type_enum,
  yap_type_blob,
  yap_type_error,
}yap_type_kind;

#endif //YAP_NODE_KINDS_H