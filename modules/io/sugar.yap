// Hand-written sugar on top of the auto-generated stdio bindings in
// binds.yap. NOT touched by `make bindings` (that target only regenerates
// binds.yap/wrapper.c/libio.*), so it's safe to extend here directly.

none fn print_char(i32 c) {
    io->putchar(c);
}

none fn print_str(byte@ s) {
    i32 i = 0;
    while (s:[i] != 0) {
        io->print_char(s:[i].(i32));
        i = i + 1;
    }
}

none fn print_i32(i32 v) {
    if (v < 0) {
        io->print_char(45); // '-'
        v = 0 - v;
    }
    if (v == 0) {
        io->print_char(48); // '0'
        ret;
    }
    i32[12] digits;
    i32 n = 0;
    while (v > 0) {
        digits:[n] = (v % 10) + 48;
        v = v / 10;
        n = n + 1;
    }
    i32 i = n - 1;
    while (i >= 0) {
        io->print_char(digits:[i]);
        i = i - 1;
    }
}

// printf-style macro: compile-time walks `fmt`, dispatching one runtime call
// per %-specifier and chaining them into a single block. `args` is
// yExprList@ (a pointer) rather than a bare value since macro-call blob
// marshalling only passes one pointer-sized value per slot. `fmt` bytes are
// already escape-decoded, so this only needs to look for '%' specifiers.
yStmt fn print(byte@ fmt, yExpr[]@ args) {
    _ stmts = yapi->stmt_list_new();
    i32 ai = 0;
    i32 i = 0;
    while (fmt:[i] != 0) {
        i32 ch = fmt:[i].(i32);
        if (ch == 37) { // '%'
            i32 spec = fmt:[i + 1].(i32);
            if (spec == 100) { // %d
                _ call_expr = yapi->call1(yapi->var_value(c"io_print_i32"), args.:[ai]);
                stmts = yapi->stmt_list_push(stmts, yapi->expr_stmt(call_expr));
                ai = ai + 1;
                i = i + 2;
            } else if (spec == 115) { // %s
                _ call_expr = yapi->call1(yapi->var_value(c"io_print_str"), args.:[ai]);
                stmts = yapi->stmt_list_push(stmts, yapi->expr_stmt(call_expr));
                ai = ai + 1;
                i = i + 2;
            } else if (spec == 99) { // %c
                _ call_expr = yapi->call1(yapi->var_value(c"io_print_char"), args.:[ai]);
                stmts = yapi->stmt_list_push(stmts, yapi->expr_stmt(call_expr));
                ai = ai + 1;
                i = i + 2;
            } else { // %% or unknown -> print literally
                _ call_expr = yapi->call1(yapi->var_value(c"io_print_char"), yapi->int(spec));
                stmts = yapi->stmt_list_push(stmts, yapi->expr_stmt(call_expr));
                i = i + 2;
            }
        } else {
            _ call_expr = yapi->call1(yapi->var_value(c"io_print_char"), yapi->int(ch));
            stmts = yapi->stmt_list_push(stmts, yapi->expr_stmt(call_expr));
            i = i + 1;
        }
    }
    ret yapi->block(stmts);
}
