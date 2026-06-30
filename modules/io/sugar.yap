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

// printf-style macro: compile-time walks `fmt`, dispatching one runtime
// call per %-specifier (or literal character) and chaining them into a
// single returned block. `args` may be omitted entirely at the call site
// (defaults to an empty yExprList) when the format string has no
// specifiers, e.g. io->print:(c"hello\n");
yStatement fn print(byte@ fmt, yExprList args) {
    _ stmts = yapi->stmt_list_new();
    i32 ai = 0;
    i32 i = 0;
    while (fmt:[i] != 0) {
        i32 ch = fmt:[i].(i32);
        if (ch == 92) { // '\' — the lexer keeps escapes un-decoded in the
                         // raw text the macro walks, so decode them here
            i32 esc = fmt:[i + 1].(i32);
            i32 decoded = esc;
            if (esc == 110) decoded = 10;      // \n
            else if (esc == 116) decoded = 9;  // \t
            else if (esc == 114) decoded = 13; // \r
            _ a = yapi->list_new();
            a = yapi->list_push(a, yapi->int(decoded));
            _ call_expr = yapi->call(yapi->var(c"io_print_char"), a);
            stmts = yapi->stmt_list_push(stmts, yapi->expr_stmt(call_expr));
            i = i + 2;
        } else if (ch == 37) { // '%'
            i32 spec = fmt:[i + 1].(i32);
            if (spec == 100) { // %d
                _ a = yapi->list_new();
                a = yapi->list_push(a, yapi->list_get(args, ai));
                _ call_expr = yapi->call(yapi->var(c"io_print_i32"), a);
                stmts = yapi->stmt_list_push(stmts, yapi->expr_stmt(call_expr));
                ai = ai + 1;
                i = i + 2;
            } else if (spec == 115) { // %s
                _ a = yapi->list_new();
                a = yapi->list_push(a, yapi->list_get(args, ai));
                _ call_expr = yapi->call(yapi->var(c"io_print_str"), a);
                stmts = yapi->stmt_list_push(stmts, yapi->expr_stmt(call_expr));
                ai = ai + 1;
                i = i + 2;
            } else if (spec == 99) { // %c
                _ a = yapi->list_new();
                a = yapi->list_push(a, yapi->list_get(args, ai));
                _ call_expr = yapi->call(yapi->var(c"io_print_char"), a);
                stmts = yapi->stmt_list_push(stmts, yapi->expr_stmt(call_expr));
                ai = ai + 1;
                i = i + 2;
            } else { // %% or unknown -> print literally
                _ a = yapi->list_new();
                a = yapi->list_push(a, yapi->int(spec));
                _ call_expr = yapi->call(yapi->var(c"io_print_char"), a);
                stmts = yapi->stmt_list_push(stmts, yapi->expr_stmt(call_expr));
                i = i + 2;
            }
        } else {
            _ a = yapi->list_new();
            a = yapi->list_push(a, yapi->int(ch));
            _ call_expr = yapi->call(yapi->var(c"io_print_char"), a);
            stmts = yapi->stmt_list_push(stmts, yapi->expr_stmt(call_expr));
            i = i + 1;
        }
    }
    ret yapi->block(stmts);
}
