import io

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

i32 fn main() {
    i32 x = 0;
    i32 y = 0;
    i32 z = 10;

    assign_it:(#x, #5);        // x = 5;
    add_into:(#y, #x, #3);     // y = x + 3;
    bump_twice:(#z);           // { z = z + 1; z = z + 1; }

    i32 ok = 0;
    if (x == 5)  { io->print:(c"stmt bp assign OK\n");      ok = ok + 1; }
    if (y == 8)  { io->print:(c"stmt bp add OK\n");         ok = ok + 1; }
    if (z == 12) { io->print:(c"stmt bp multi/block OK\n"); ok = ok + 1; }
    ret ok - 3;
}
