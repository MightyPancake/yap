import io

// stmt${ } with a *statement* hole ($b), filled via fill_stmt.
// Builds a body statement, then splices it into a control-flow template.
yStmt fn guard_inc(yExpr flag, yExpr counter) {
    _ body = stmt${ $x = $x + 1; }:fill_expr(c"x", counter):finish();
    ret stmt${ if ($c) { $b; } }:fill_expr(c"c", flag):fill_stmt(c"b", body):finish();
}

i32 fn main() {
    i32 c = 0;
    guard_inc:(#true, #c);    // if (true) { c = c + 1; }
    i32 d = 0;
    guard_inc:(#false, #d);   // if (false) { d = d + 1; }

    i32 ok = 0;
    if (c == 1) { io->print:(c"fill_stmt then-taken OK\n");    ok = ok + 1; }
    if (d == 0) { io->print:(c"fill_stmt then-skipped OK\n");  ok = ok + 1; }
    ret ok - 2;
}
