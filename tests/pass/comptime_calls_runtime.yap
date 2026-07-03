import io

// Regular runtime function — defined before the comptime function
i32 fn get_count() {
    ret 5;
}

// Comptime function that calls get_count() at compile time,
// then uses the result to build repeated additions via a loop
yExpr fn add_n(yExpr base) {
    _ n = get_count();
    _ result = base;
    for (i32 i = 0; i < n; i = i + 1) {
        _ one = yapi->int(1);
        result = yapi->bin_op(result, 43, one);
    }
    ret result;
}

i32 fn main() {
    // add_n#(10) calls get_count() which returns 5,
    // then builds: 10 + 1 + 1 + 1 + 1 + 1 = 15
    _ val = add_n:(#10);
    if (val == 15) io->print:(c"Comptime-calls-runtime OK\n");
    ret val - 15;
}
