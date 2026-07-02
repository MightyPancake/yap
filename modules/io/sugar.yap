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
// 'args' is yExprList@ (pointer to a real slice of yExpr), not a bare
// yExprList value — see project memory on why: the macro-call blob-literal
// marshalling that populates it can only pass one pointer-sized value per
// argument slot, and a genuine by-value slice is two words. 'args.:[ai]'
// dereferences the pointer (a plain 'ptr.'), then indexes the resulting
// slice ('expr:[idx]') — both native slice operations, no yapi->list_* or
// yExprList methods needed anymore.
yStatement fn print(byte@ fmt, yExprList@ args) {
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
            _ call_expr = yapi->call1(yapi->var_value(c"io_print_char"), yapi->int(decoded));
            stmts = yapi->stmt_list_push(stmts, yapi->expr_stmt(call_expr));
            i = i + 2;
        } else if (ch == 37) { // '%'
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
