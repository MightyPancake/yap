struct Color {
    i32 r = 0,
    i32 g = 0,
    i32 b = 0,
}

i32 fn add_with_offset(i32 a, i32 b, i32 offset = 0) {
    ret a + b + offset;
}
