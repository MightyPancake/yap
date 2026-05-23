#ifndef YAP_NODE_KINDS_H
#define YAP_NODE_KINDS_H

//Node kinds
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

#endif //YAP_NODE_KINDS_H