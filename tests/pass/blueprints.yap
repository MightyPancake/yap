import io

// --- arithmetic, fill/finish, stored/reused/chained (first cut) ---
yExpr fn bp_add1(yExpr a)     { ret $($x + 1):fill(c"x", a):finish(); }
yExpr fn manual_add1(yExpr a) { ret yapi->bin_op(a, 43, yapi->int(1)); } // '+' == 43
yExpr fn bp_stored(yExpr a)   { yExprBlueprint t = $($x + 1); ret t:fill(c"x", a):finish(); }
yExpr fn bp_square(yExpr a)   { ret $($x * $x):fill(c"x", a):finish(); }
yExpr fn bp_add(yExpr a, yExpr b) { ret $($x + $y):fill(c"x", a):fill(c"y", b):finish(); }

// --- comparisons (result type is bool) ---
yExpr fn bp_lt(yExpr a, yExpr b) { ret $($x < $y):fill(c"x", a):fill(c"y", b):finish(); }
yExpr fn bp_eq(yExpr a, yExpr b) { ret $($x == $y):fill(c"x", a):fill(c"y", b):finish(); }
yExpr fn bp_ge(yExpr a, yExpr b) { ret $($x >= $y):fill(c"x", a):fill(c"y", b):finish(); }

// --- unary minus ---
yExpr fn bp_neg(yExpr a) { ret $(-$x):fill(c"x", a):finish(); }

// --- ternary ---
yExpr fn bp_tern(yExpr cnd, yExpr a, yExpr b) {
    ret $($c ? $a else $b):fill(c"c", cnd):fill(c"a", a):fill(c"b", b):finish();
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

    if (b == 11)         { io->puts(c"arith fill/finish OK");  pass = pass + 1; }
    if (b == m)          { io->puts(c"blueprint == manual OK"); pass = pass + 1; }
    if (s == 42)         { io->puts(c"stored OK");             pass = pass + 1; }
    if (sq == 36)        { io->puts(c"hole reuse OK");         pass = pass + 1; }
    if (tw == 7)         { io->puts(c"chained fill OK");       pass = pass + 1; }
    if (ng == -5)        { io->puts(c"unary minus OK");        pass = pass + 1; }
    if (tn == 10)        { io->puts(c"ternary then OK");       pass = pass + 1; }
    if (tf == 20)        { io->puts(c"ternary else OK");       pass = pass + 1; }
    if (lt)              { io->puts(c"cmp < true OK");         pass = pass + 1; }
    if (nlt == false)    { io->puts(c"cmp < false OK");        pass = pass + 1; }
    if (eq)              { io->puts(c"cmp == OK");             pass = pass + 1; }
    if (ge)              { io->puts(c"cmp >= OK");             pass = pass + 1; }

    ret pass - 12;
}
