yExpr fn bp_cast_unfilled(yExpr a) {
    ret expr${ $x.($T) }:fill_expr(c"x", a):finish();
}

i32 fn main() {
    _ v = bp_cast_unfilled:(#5);
    ret 0;
}
