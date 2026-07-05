import io

// --- arithmetic, fill/finish, stored/reused/chained (first cut) ---
yExpr fn bp_add1(yExpr a)     { ret expr${ $x + 1 }:fill_expr(c"x", a):finish(); }
yExpr fn manual_add1(yExpr a) { ret yapi->bin_op(a, 43, yapi->int(1)); } // '+' == 43
yExpr fn bp_stored(yExpr a)   { yExprBlueprint t = expr${ $x + 1 }; ret t:fill_expr(c"x", a):finish(); }
yExpr fn bp_square(yExpr a)   { ret expr${ $x * $x }:fill_expr(c"x", a):finish(); }
yExpr fn bp_add(yExpr a, yExpr b) { ret expr${ $x + $y }:fill_expr(c"x", a):fill_expr(c"y", b):finish(); }

// --- comparisons (result type is bool) ---
yExpr fn bp_lt(yExpr a, yExpr b) { ret expr${ $x < $y }:fill_expr(c"x", a):fill_expr(c"y", b):finish(); }
yExpr fn bp_eq(yExpr a, yExpr b) { ret expr${ $x == $y }:fill_expr(c"x", a):fill_expr(c"y", b):finish(); }
yExpr fn bp_ge(yExpr a, yExpr b) { ret expr${ $x >= $y }:fill_expr(c"x", a):fill_expr(c"y", b):finish(); }

// --- unary minus ---
yExpr fn bp_neg(yExpr a) { ret expr${ -$x }:fill_expr(c"x", a):finish(); }

// --- ternary ---
yExpr fn bp_tern(yExpr cnd, yExpr a, yExpr b) {
    ret expr${ $c ? $a else $b }:fill_expr(c"c", cnd):fill_expr(c"a", a):fill_expr(c"b", b):finish();
}

// --- holes inside call args (regression: ct_clone_expr must deep-clone func_call args, not shallow-share them) ---
i32 fn bp_helper_add(i32 a, i32 b) { ret a + b; }
yExpr fn bp_call1(yExpr a) { ret expr${ bp_helper_add($x, 1) }:fill_expr(c"x", a):finish(); }
yExpr fn bp_call2(yExpr a, yExpr b) { ret expr${ bp_helper_add($x, $y) }:fill_expr(c"x", a):fill_expr(c"y", b):finish(); }

// --- lazy $T inside a cast: a cast's type used to always eagerly splice
// regardless of the enclosing template's own laziness (a shortcut from before
// type holes existed); now $T is a genuine lazy hole here too, closed via
// :fill_type (yapi->type_hole under the hood, same as stmt${ }'s var_decl). ---
yExpr fn bp_cast(yType t, yExpr a) { ret expr${ $x.($T) }:fill_expr(c"x", a):fill_type(c"T", t):finish(); }

i32 fn main() {
    i32 pass = 0;

    _ b  = bp_add1:(#10);          // 11
    _ m  = manual_add1:(#10);      // 11
    _ s  = bp_stored:(#41);        // 42
    _ sq = bp_square:(#6);         // 36
    _ tw = bp_add:(#3, #4);        // 7
    _ ng = bp_neg:(#5);            // -5
    _ tn = bp_tern:(#1, #10, #20); // 10
    _ tf = bp_tern:(#0, #10, #20); // 20

    _ lt  = bp_lt:(#3, #4);        // true
    _ nlt = bp_lt:(#5, #4);        // false
    _ eq  = bp_eq:(#7, #7);        // true
    _ ge  = bp_ge:(#9, #9);        // true

    _ c1 = bp_call1:(#10);         // bp_helper_add(10, 1) = 11
    _ c2 = bp_call2:(#3, #4);      // bp_helper_add(3, 4) = 7

    _ ct = bp_cast:(i64, #7);      // (i64)7

    if (b == 11)         { io->print:(c"arith fill/finish OK\n");  pass = pass + 1; }
    if (b == m)          { io->print:(c"blueprint == manual OK\n"); pass = pass + 1; }
    if (s == 42)         { io->print:(c"stored OK\n");             pass = pass + 1; }
    if (sq == 36)        { io->print:(c"hole reuse OK\n");         pass = pass + 1; }
    if (tw == 7)         { io->print:(c"chained fill OK\n");       pass = pass + 1; }
    if (ng == -5)        { io->print:(c"unary minus OK\n");        pass = pass + 1; }
    if (tn == 10)        { io->print:(c"ternary then OK\n");       pass = pass + 1; }
    if (tf == 20)        { io->print:(c"ternary else OK\n");       pass = pass + 1; }
    if (lt)              { io->print:(c"cmp < true OK\n");         pass = pass + 1; }
    if (nlt == false)    { io->print:(c"cmp < false OK\n");        pass = pass + 1; }
    if (eq)              { io->print:(c"cmp == OK\n");             pass = pass + 1; }
    if (ge)              { io->print:(c"cmp >= OK\n");             pass = pass + 1; }

    if (c1 == 11)        { io->print:(c"call-arg hole fill OK\n"); pass = pass + 1; }
    if (c2 == 7)         { io->print:(c"call-arg multi-hole fill OK\n"); pass = pass + 1; }

    if (ct == 7)         { io->print:(c"lazy cast type-hole fill OK\n"); pass = pass + 1; }

    ret pass - 15;
}
