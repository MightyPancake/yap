struct hello {
    byte@@ cheeky_double_pointer,
    u64 sz
}

enum my_enum {
    ea,
    ab = 2,
    ec
}

union pick_one {
    i32 num,
    byte const @ const str
}

struct giga_struct {
    union {
        i32 integer,
        f32 floating,
        struct {
            f32 x = 6,
            f32 y = 7
        } anon_struct_field
    } anon_u,
    struct {
        i32 joseph,
        i32 mamon
    } another_anon_struct_field,
    (fn i32, i32) some_callback_idk
}
