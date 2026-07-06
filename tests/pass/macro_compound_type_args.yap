import io

// Extends macros.yap's pair(T1, T2) macro to cover compound type spellings
// (array/slice/function types) as macro-call type arguments -- previously
// only bare identifiers and pointer types (e.g. i32@) could be passed here;
// arrays/slices/function types failed with a syntax error.
yType fn pair(yType t1, yType t2){
    _ st = yapi->struct_t();
    st:add_field(t1, c"first");
    st:add_field(t2, c"second");
    ret st:finish(c"pair");
}

i32 fn main() {
    i32 code = 0;

    pair:(i32[4], i32) p1;
    p1.first:[0] = 9;
    p1.second = 4;
    if (p1.first:[0] == 9 && p1.second == 4) { io->print:(c"array type arg OK\n"); } else { code = code + 1; }

    pair:(i32[], i32) p2;
    p2.second = 5;
    if (p2.second == 5) { io->print:(c"slice type arg OK\n"); } else { code = code + 1; }

    pair:((i32 fn i32), i32) p3;
    p3.second = 6;
    if (p3.second == 6) { io->print:(c"function type arg OK\n"); } else { code = code + 1; }

    ret code;
}
