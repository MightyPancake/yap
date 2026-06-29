import io

// Phase 3: identity macro — passes through the argument unchanged
yExpr fn identity(yExpr a) {
    ret a;
}

// Phase 4: builder macro — constructs (a + 1) using comptime builders
yExpr fn plus_one(yExpr a) {
    _ one = yapi->int(1);
    // '+' == 43
    ret yapi->bin(a, 43, one);
}

i32 fn main() {
    // Test identity macro
    _ x = identity#(42);

    // Test builder macro
    _ y = plus_one#(10);

    // y should be 10 + 1 = 11
    // x should be 42
    // x + y = 53
    if y == 11 io->puts(c"Correct!");
    ret x + y - 53;
}
