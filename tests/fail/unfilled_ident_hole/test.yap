yStmt fn declare_var_unfilled(yType t, yExpr init_val) {
    ret stmt${ $T $name = $init; }
        :fill_type(c"T", t)
        :fill_expr(c"init", init_val)
        :finish();
}

i32 fn main() {
    declare_var_unfilled:(i32, #42);
    ret 0;
}
