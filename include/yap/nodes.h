#ifndef YAP_NODES_H
#define YAP_NODES_H

//Forward declarations
typedef struct yap_decl_node yap_decl_node;
typedef struct yap_expr_node yap_expr_node;
typedef struct yap_statement_node yap_statement_node;
typedef struct yap_var_decl_node yap_var_decl_node;
typedef struct yap_named_type_decl_node yap_named_type_decl_node;
typedef struct yap_block_node yap_block_node;

//Misc
kenobi_new_struct_free(yap_identifier_node,
    char* value;
    yap_loc loc;
);

kenobi_new_struct_free(yap_block_node,
    darr(yap_statement_node) statements;
    yap_loc loc;
);

kenobi_new_struct_free(yap_string_literal_node,
    char prefix[4]; // e.g. L for wide string literals, currently unused
    char* value;
    yap_loc loc;
);

kenobi_new_struct_free(yap_literal_node,
    yap_literal_kind kind;
    union {
        yap_error err;
        char* numerical;
        yap_string_literal_node string;
        char* blob;
    };
    yap_loc loc;
);

//Expressions
kenobi_new_struct_free(yap_bin_op_node,
    char op;
    yap_expr_node* left;
    yap_expr_node* right;
    yap_loc loc;
);

kenobi_new_struct_free(yap_assignment_node,
    char op[3];
    yap_expr_node* left;
    yap_expr_node* right;
    yap_loc loc;
);

kenobi_new_struct_free(yap_func_call_node,
    yap_expr_node* func;
    darr(yap_expr_node) args;
    yap_loc loc;
);

kenobi_new_struct_free(yap_cast_node,
    yap_identifier_node type_name;
    yap_expr_node* expr;
    yap_loc loc;
);

kenobi_new_struct_free(yap_at_op_node,
    yap_identifier_node name;
    yap_expr_node* expr;
    yap_loc loc;
);

kenobi_new_struct_free(yap_paren_node,
    yap_expr_node* expr;
    yap_loc loc;
);

kenobi_new_struct_free(yap_increment_node,
    yap_expr_node* expr;
    yap_loc loc;
);

kenobi_new_struct_free(yap_decrement_node,
    yap_expr_node* expr;
    yap_loc loc;
);

kenobi_new_struct_free(yap_ternary_node,
    yap_expr_node* condition;
    yap_expr_node* then_expr;
    yap_expr_node* else_expr;
    yap_loc loc;
);

kenobi_new_struct_free(yap_member_access_node,
    yap_expr_node* object;
    yap_identifier_node member;
    yap_loc loc;
);

kenobi_new_struct_free(yap_expr_node,
    yap_expr_kind kind;
    union {
        yap_error err;
        yap_literal_node literal;
        yap_identifier_node var;
        yap_bin_op_node bin;
        yap_assignment_node assignment;
        yap_func_call_node func_call;
        yap_cast_node cast;
        yap_at_op_node at_op;
        yap_paren_node paren;
        yap_increment_node increment;
        yap_decrement_node decrement;
        yap_ternary_node ternary;
        yap_member_access_node member_access;
    };
    yap_loc loc;
);

//Statements
kenobi_new_struct_free(yap_var_decl_node,
    yap_identifier_node name;
    bool has_type;
    yap_identifier_node type_name;
    bool has_init;
    yap_expr_node init;
    yap_loc loc;
);

kenobi_new_struct_free(yap_return_statement_node,
    bool has_value;
    yap_expr_node value;
    yap_loc loc;
);

kenobi_new_struct_free(yap_if_node,
    yap_expr_node condition;
    yap_statement_node* then_branch;
    yap_loc loc;
);

kenobi_new_struct_free(yap_if_else_node,
    yap_expr_node condition;
    yap_statement_node* then_branch;
    yap_statement_node* else_branch;
    yap_loc loc;
);

kenobi_new_struct_free(yap_while_node,
    yap_expr_node condition;
    yap_statement_node* body;
    yap_loc loc;
);

kenobi_new_struct_free(yap_for_node,
    yap_statement_node* init;
    yap_expr_node condition;
    yap_expr_node update;
    yap_statement_node* body;
    yap_loc loc;
);

kenobi_new_struct_free(yap_statement_node,
    yap_statement_kind kind;
    union {
        yap_error err;
        yap_expr_node expr;
        yap_var_decl_node var_decl;
        yap_return_statement_node return_stmt;
        yap_if_node if_stmt;
        yap_if_else_node if_else_stmt;
        yap_while_node while_stmt;
        yap_for_node for_stmt;
        yap_block_node block;
    };
    yap_loc loc;
);

//Declarations
kenobi_new_struct_free(yap_func_arg_node,
    union {
        struct {
            yap_identifier_node name;
            bool has_type;
            yap_identifier_node type_name;
            bool has_default;
            yap_expr_node default_value;
        };
        yap_error err;
    };
    bool is_valid;
    yap_loc loc;
);

kenobi_new_struct_free(yap_enum_variant_node,
    yap_identifier_node name;
    bool has_value;
    yap_expr_node value;
    yap_loc loc;
);

kenobi_new_struct_free(yap_named_type_decl_node,
    yap_named_type_decl_kind kind;
    yap_identifier_node name;
    union {
        struct {
            darr(yap_var_decl_node) fields; // struct fields
        } as_struct;
        struct {
            darr(yap_enum_variant_node) variants; // enum variants
        } as_enum;
        struct {
            darr(yap_var_decl_node) variants; // union variants (as var_decl-like nodes)
        } as_union;
        yap_error err;
    };
    yap_loc loc;
);

kenobi_new_struct_free(yap_func_decl_node,
    yap_identifier_node name;
    darr(yap_func_arg_node) args;
    bool has_return_type;
    yap_identifier_node return_type;
    yap_block_node body;
    yap_loc loc;
);

kenobi_new_struct_free(yap_file_import_node,
    yap_string_literal_node path;
    yap_loc loc;
);

kenobi_new_struct_free(yap_module_import_node,
    yap_identifier_node module_name;
    yap_loc loc;
);

kenobi_new_struct_free(yap_module_decl_node,
    yap_identifier_node name;
    yap_loc loc;
    //TODO: Add module info
);

kenobi_new_struct_free(yap_decl_node,
    yap_decl_kind kind;
    union {
        yap_func_decl_node func_decl;
        yap_named_type_decl_node named_type_decl;
        yap_module_import_node module_import;
        yap_file_import_node file_import;
        yap_module_decl_node module_decl;
        yap_error err;
    };
    yap_loc loc;
);

kenobi_new_struct_free(yap_module_node,
    darr(yap_decl_node) declarations;
    yap_loc loc;
);

#endif //YAP_NODES_H
