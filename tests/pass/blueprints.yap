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

// stmt${ } lazy blueprint -> yStmtBlueprint; fill_expr(...) + finish() -> yStmt.
// A statement macro's yStmt result is spliced at the call site.
yStmt fn assign_it(yExpr lhs, yExpr rhs) {
    ret stmt${ $l = $r; }:fill_expr(c"l", lhs):fill_expr(c"r", rhs):finish();
}

yStmt fn add_into(yExpr dst, yExpr a, yExpr b) {
    ret stmt${ $d = $a + $b; }:fill_expr(c"d", dst):fill_expr(c"a", a):fill_expr(c"b", b):finish();
}

// multi-statement blueprint -> block
yStmt fn bump_twice(yExpr v) {
    ret stmt${
        $x = $x + 1;
        $x = $x + 1;
    }:fill_expr(c"x", v):finish();
}

// stmt${ } with a *statement* hole ($b), filled via fill_stmt.
// Builds a body statement, then splices it into a control-flow template.
yStmt fn guard_inc(yExpr flag, yExpr counter) {
    _ body = stmt${ $x = $x + 1; }:fill_expr(c"x", counter):finish();
    ret stmt${ if ($c) { $b; } }:fill_expr(c"c", flag):fill_stmt(c"b", body):finish();
}

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

    // --- stmt${ } basics: assignment, arithmetic, multi-statement block ---
    i32 x = 0;
    i32 y = 0;
    i32 z = 10;

    assign_it:(#x, #5);        // x = 5;
    add_into:(#y, #x, #3);     // y = x + 3;
    bump_twice:(#z);           // { z = z + 1; z = z + 1; }

    if (x == 5)  { io->print:(c"stmt bp assign OK\n");      pass = pass + 1; }
    if (y == 8)  { io->print:(c"stmt bp add OK\n");         pass = pass + 1; }
    if (z == 12) { io->print:(c"stmt bp multi/block OK\n"); pass = pass + 1; }

    // --- stmt${ } with a statement hole (:fill_stmt) ---
    i32 c = 0;
    guard_inc:(#true, #c);    // if (true) { c = c + 1; }
    i32 d = 0;
    guard_inc:(#false, #d);   // if (false) { d = d + 1; }

    if (c == 1) { io->print:(c"fill_stmt then-taken OK\n");    pass = pass + 1; }
    if (d == 0) { io->print:(c"fill_stmt then-skipped OK\n");  pass = pass + 1; }

    // --- stmt${ }'s var_decl support (:fill_type + :fill_ident + :fill_var) ---
    declare_var:(i32, +vx, #42);
    vx = vx + 1;
    if (vx == 43) { io->print:(c"canonical var_decl blueprint fill OK\n"); pass = pass + 1; }

    i32 r = 0;
    build_double:(i32, #5, #r);
    if (r == 10) { io->print:(c"fill_var declare+reference OK\n"); pass = pass + 1; }

    ret pass - 22;
}
