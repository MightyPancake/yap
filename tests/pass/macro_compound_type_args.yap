import io

// Extends macros.yap's pair(T1, T2) macro to cover compound type spellings
// as macro-call type arguments -- previously only bare identifiers and
// pointer types (e.g. i32@) could be passed here. array_type/slice_type/
// function_type/const_type failed with a syntax error (no _expr equivalent
// existed for these shapes); paren-wrapped types and nested macro-calls
// already parsed fine (paren_expr/macro_expr are valid _expr) but
// yap_resolve_macro_type_arg didn't know how to unwrap/execute them.
yType fn pair(yType t1, yType t2){
    _ st = yapi->struct_t();
    st:add_field(t1, c"first");
    st:add_field(t2, c"second");
    ret st:finish(c"pair");
}

yType fn same(yType T) {
    ret T;
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

    pair:(i32 const, i32) p4;
    p4.second = 7;
    if (p4.second == 7) { io->print:(c"const type arg OK\n"); } else { code = code + 1; }

    pair:((i32), i32) p5;
    p5.first = 8;
    p5.second = 9;
    if (p5.first == 8 && p5.second == 9) { io->print:(c"paren type arg OK\n"); } else { code = code + 1; }

    pair:(same:(i32), i32) p6;
    p6.first = 10;
    p6.second = 11;
    if (p6.first == 10 && p6.second == 11) { io->print:(c"nested macro type arg OK\n"); } else { code = code + 1; }

    ret code;
}
