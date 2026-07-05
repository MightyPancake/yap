import io

// stmt${ }'s var_decl support: `$T $name = $init;` -- a genuine lazy type-hole
// (:fill_type, closed via yapi->type_hole) and ident-hole (:fill_ident, closed
// via yapi->ident_hole), composing with the pre-existing :fill_expr for the
// initializer. This is the canonical example from the original design doc.
yStmt fn declare_var(yType t, yIdent name, yExpr init_val) {
    ret stmt${ $T $name = $init; }
        :fill_type(c"T", t)
        :fill_ident(c"name", name)
        :fill_expr(c"init", init_val)
        :finish();
}

// :fill_var(name, type, ident) combinator: declares $out once, then
// references it as a plain value TWICE more ($out = $init; $out = $out +
// $out;). A declaration's name-hole and a later plain reference to the same
// name are two different hole kinds (ident-hole vs expr-hole) that happen to
// share a spelling -- fill_var closes both with a single call instead of
// requiring a second hole name and a manually-built yapi->new_var reference.
yStmt fn build_double(yType t, yExpr init_val, yExpr result_slot) {
    ret stmt${
        $T $out;
        $out = $init;
        $out = $out + $out;
        $result = $out;
    }
        :fill_type(c"T", t)
        :fill_var(c"out", t, yapi->uniq_name())
        :fill_expr(c"init", init_val)
        :fill_expr(c"result", result_slot)
        :finish();
}

i32 fn main() {
    i32 ok = 0;

    declare_var:(i32, +x, #42);
    x = x + 1;
    if (x == 43) { io->print:(c"canonical var_decl blueprint fill OK\n"); ok = ok + 1; }

    i32 r = 0;
    build_double:(i32, #5, #r);
    if (r == 10) { io->print:(c"fill_var declare+reference OK\n"); ok = ok + 1; }

    ret ok - 2;
}
