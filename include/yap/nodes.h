#ifndef YAP_NODES_H
#define YAP_NODES_H

//Forward declarations
typedef struct yap_decl_node yap_decl_node;
typedef struct yap_expr_node yap_expr_node;
typedef struct yap_statement_node yap_statement_node;
typedef struct yap_var_decl_node yap_var_decl_node;
typedef struct yap_named_type_decl_node yap_named_type_decl_node;
typedef struct yap_block_node yap_block_node;
typedef struct yap_type_node yap_type_node;
typedef struct yap_func_arg_node yap_func_arg_node;

//Misc
kenobi_new_struct_free(yap_identifier_node,
    char* value;
    bool is_hole; // true if this came from a $name blueprint_hole token (name in .value, stripped of '$')
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

kenobi_new_struct_free(yap_blob_element_node,
    bool is_named;
    yap_identifier_node name;
    yap_expr_node* value;
    yap_loc loc;
);

// One argument at a call site: 'expr' (positional) or '.name = expr' (named,
// matched against the callee's declared parameter names in build.c).
kenobi_new_struct_free(yap_call_arg_node,
    bool is_named;
    yap_identifier_node name;
    yap_expr_node* value;
    yap_loc loc;
);

/* Anonymous function literal: (<optional return_type> fn(<params>) { <body> }).
 * Same shape as yap_func_decl_node minus name/subject; params reuse
 * yap_func_arg_node (has_default is always false ; literal params are just
 * type+name). */
kenobi_new_struct_free(yap_func_literal_node,
    darr(yap_func_arg_node) args;
    bool has_return_type;
    yap_type_node* return_type_node;
    yap_block_node body;
    yap_loc loc;
);

kenobi_new_struct_free(yap_literal_node,
    yap_literal_kind kind;
    union {
        yap_error err;
        char* numerical;
        yap_string_literal_node string;
        darr(yap_blob_element_node) blob_elements;
        yap_func_literal_node func_literal;
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
    darr(yap_call_arg_node) args;
    yap_loc loc;
);

kenobi_new_struct_free(yap_cast_node,
    yap_type_node* type_node;
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

// Prefix unary minus ('-expr'). Only operator supported today; same shape
// would carry '!'/'~' too if those get added later, but nothing builds that
// path yet.
kenobi_new_struct_free(yap_unary_op_node,
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

kenobi_new_struct_free(yap_method_access_node,
    yap_expr_node* caller;
    yap_identifier_node name;
    yap_loc loc;
);

kenobi_new_struct_free(yap_deref_node,
    yap_expr_node* expr;
    yap_loc loc;
);

kenobi_new_struct_free(yap_index_access_node,
    yap_expr_node* object;
    yap_expr_node* index;
    yap_loc loc;
);

kenobi_new_struct_free(yap_module_access_node,
    yap_identifier_node module;
    yap_identifier_node field;
    yap_loc loc;
);

// $( template ) expression blueprint. Parse-AST only: build.c desugars the
// template into yapi-> builder calls, so this never reaches codegen/semtree.
kenobi_new_struct_free(yap_blueprint_node,
    yap_expr_node* template;
    yap_loc loc;
);

// $name placeholder inside a blueprint template.
kenobi_new_struct_free(yap_blueprint_hole_node,
    yap_identifier_node name;
    yap_loc loc;
);

// type${ struct{...} } eager type blueprint. Parse-AST only: build.c desugars
// `body` (an anon struct/enum/union type node) into a chained
// yapi->struct_t()/enum_t()/union_t() + add_field/add_variant call, yielding a
// yStructT/yEnumT/yUnionT template. $T in a field/variant type is an eager splice.
kenobi_new_struct_free(yap_type_blueprint_node,
    yap_type_node* body;
    yap_loc loc;
);

// stmt${ ...statements... } lazy statement blueprint. Parse-AST only: build.c
// desugars the statement sequence into yapi->expr_stmt/return_stmt/... + block
// builder calls (holes -> yapi->hole), yielding a yStmtBlueprint.
kenobi_new_struct_free(yap_stmt_blueprint_node,
    darr(yap_statement_node) body;
    yap_loc loc;
);

typedef enum {
    yap_macro_param_unnamed,
    yap_macro_param_named,
    yap_macro_param_statement,
    yap_macro_param_ident_add,
    yap_macro_param_mut,
    yap_macro_param_ast,
} yap_macro_param_kind;

typedef struct yap_macro_param_node yap_macro_param_node;
kenobi_new_struct_free(yap_macro_param_node,
    yap_macro_param_kind kind;
    union {
        yap_expr_node* expr;
        struct { yap_identifier_node name; yap_expr_node* value; } named;
        yap_statement_node* statement;
        yap_identifier_node ident_add;
        yap_expr_node* mut_expr;
    };
    yap_loc loc;
);

kenobi_new_struct_free(yap_macro_call_node,
    yap_expr_node* caller;
    darr(yap_macro_param_node) params;
    yap_loc loc;
);

kenobi_new_struct_free(yap_expr_node,
    yap_expr_kind kind;
    union {
        yap_error err;
        yap_literal_node literal;
        yap_identifier_node var;
        yap_bin_op_node bin;
        yap_unary_op_node unary;
        yap_assignment_node assignment;
        yap_func_call_node func_call;
        yap_cast_node cast;
        yap_at_op_node at_op;
        yap_paren_node paren;
        yap_increment_node increment;
        yap_decrement_node decrement;
        yap_ternary_node ternary;
        yap_member_access_node member_access;
        yap_method_access_node method_access;
        yap_deref_node deref;
        yap_index_access_node index_access;
        yap_block_node block;
        yap_module_access_node module_access;
        yap_macro_call_node macro_call;
        yap_blueprint_node blueprint;
        yap_blueprint_hole_node blueprint_hole;
        yap_type_blueprint_node type_blueprint;
        yap_stmt_blueprint_node stmt_blueprint;
        yap_func_literal_node fn_blueprint; // (RET fn$ params){body}: reuses the func-literal shape
    };
    yap_loc loc;
);

//Statements
kenobi_new_struct_free(yap_var_decl_node,
    yap_identifier_node name;
    bool has_type;
    yap_type_node* type_node;  // NULL if no type (inferred)
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
        yap_macro_call_node macro_call;
    };
    yap_loc loc;
);

//Declarations
kenobi_new_struct_free(yap_func_arg_node,
    union {
        struct {
            yap_identifier_node name;
            bool has_type;
            yap_type_node* type_node;
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
    yap_type_node* return_type_node;
    bool has_subject;
    yap_type_node* subject_type_node;
    yap_identifier_node subject_name;
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
    char* prefix;
    char* version;
    yap_loc loc;
);

kenobi_new_struct_free(yap_decl_node,
    yap_decl_kind kind;
    union {
        yap_func_decl_node func_decl;
        yap_named_type_decl_node named_type_decl;
        yap_module_import_node module_import;
        yap_file_import_node file_import;
        yap_module_decl_node module_decl;
        yap_macro_call_node macro_call;
        yap_error err;
    };
    yap_loc loc;
);

kenobi_new_struct_free(yap_source_node,
    darr(yap_decl_node) declarations;
    yap_loc loc;
    bool was_built;
);

typedef enum {
    yap_type_node_error,
    yap_type_node_identifier,
    yap_type_node_pointer,
    yap_type_node_const,
    yap_type_node_paren,
    yap_type_node_function,
    yap_type_node_anon_struct,
    yap_type_node_anon_enum,
    yap_type_node_anon_union,
    yap_type_node_array,
    yap_type_node_slice,
    yap_type_node_macro,
    yap_type_node_blueprint_hole,  // $name in type position (eager splice / type-hole); reuses .identifier for the name
} yap_type_node_kind;

kenobi_new_struct_free(yap_type_node,
    yap_type_node_kind kind;
    union {
        yap_error             err;
        yap_identifier_node   identifier;
        yap_type_node*        pointer_subtype;  // for pointer_type:  subtype @
        yap_type_node*        const_subtype;    // for const_type:    subtype const
        yap_type_node*        paren_subtype;    // for paren_type:    (subtype)
        struct {
            yap_type_node*    return_type;      // optional, NULL for void
            darr(yap_type_node) params;         // func_type_params
        } func_type;
        struct {
            darr(yap_var_decl_node) fields;     // anon struct fields
        } anon_struct;
        struct {
            darr(yap_enum_variant_node) variants; // anon enum variants
        } anon_enum;
        struct {
            darr(yap_var_decl_node) variants;   // anon union variants
        } anon_union;
        struct {
            yap_type_node*    element_type;
            yap_expr_node*    size_expr;         // compile-time size expression
        } array_type;
        yap_type_node*        slice_subtype;     // element type for slices
        yap_macro_call_node   macro_call;        // macro returning a type
    };
    yap_loc loc;
);

#endif //YAP_NODES_H
