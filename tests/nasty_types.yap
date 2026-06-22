// Pointer madness — chains of pointers and const qualifiers
struct pointer_party {
    i32@ single_ptr,
    i32@@ double_ptr,
    i32@@@@ quad_ptr,
    byte const single_const,
    byte const @ const_ptr_to_const,
    byte const @ const @ const_ptr_to_const_ptr,
    byte @ const const_ptr,
    byte@@ const const_double_ptr,
    i32 const @ const int_const_ptr,
    byte const @ const @ const ptr_to_const_ptr_to_const,
    u64 const @ const @ const @ const u64_triple_const_ptr
}

// Deeply nested anonymous embedded types — Inception style
struct deep_nesting {
    // Level 1: anonymous struct
    struct {
        i32 sx,
        f32 sy,
        // Level 2: anonymous union inside anonymous struct
        union {
            i32 z_int,
            f64 z_double,
            // Level 3: anonymous struct inside anonymous union
            struct {
                byte flag,
                i32 value
            }
        },
        // Another at level 2: named embedded
        struct {
            byte a, byte b, byte c, byte d
        } rgba
    },
    // Level 1: anonymous union next to the struct
    union {
        i32 simple_int,
        f64 simple_double,
        // Level 2 in union: anonymous struct
        struct {
            i32 ux,
            i32 uy,
            i32 uz
        }
    },
    // Regular named field
    u64 id
}

// Function pointer type combos
struct callback_hell {
    (fn i32, i32) simple_cb,
    (fn i32, i32)@ ptr_to_cb,
    (fn) void_cb,
    (fn i32)@ const const_ptr_to_ret_i32_cb,
    (i32 fn f64) float_to_int_cb,
    (fn i32@, i32@) ptr_args_cb,
    (fn i32@)@ const const_ptr_to_ptr_returning_cb,
    (fn)@ const const_ptr_to_void_cb,
    (i32@ fn i32@) ptr_return_and_arg_cb,
    (i32@ const fn f64 const @ const) full_const_madness
}

// Union with mixed named + anonymous + function types
union schizophrenic_union {
    i32 just_an_int,
    f64 just_a_double,
    struct {
        byte data,
        i32 len
    },
    // Anonymous union inside named union
    union {
        i32 a,
        f32 b
    },
    (fn i32, i32) callback,
    byte const @ str_ptr
}

// Real-world-ish: AST node
struct ast_node_data {
    enum {
        kind_none,
        kind_literal,
        kind_binary,
        kind_call,
        kind_var
    } kind,
    union {
        struct {
            i32 value
        } literal_data,
        struct {
            i32@ left,
            byte op,
            i32@ right
        } binary_data,
        struct {
            i32@ callee,
            i32@ args
        } call_data,
        struct {
            byte@ name
        } var_data
    }
}

fn main() {
    pointer_party pp;
    pp.single_ptr = null;
    pp.const_ptr_to_const = null;

    deep_nesting dn;
    dn.sx = 42;
    dn.simple_int = 100;

    callback_hell ch;

    ast_node_data node;
    
    //This should produce an error
    _ f = node.kind;
}
