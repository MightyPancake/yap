import io

none fn emit_char(i32 c) {
    io->putchar(c);
}

// Variable number of values through one fixed-arity macro argument, via a
// blob literal that gets unpacked into a yExprList.
yExpr fn add_const(yExpr base, yExprList extra) {
    _ n = yapi->list_len(extra);
    _ acc = base;
    i32 i = 0;
    while (i < n) {
        acc = yapi->bin(acc, 43, yapi->list_get(extra, i)); // '+' == 43
        i = i + 1;
    }
    ret acc;
}

// Statement macro returning a block of calls — exercises yapi->call,
// yapi->stmt_list_new/push and yapi->block end to end.
yStatement fn emit_chars(yExprList vals) {
    _ stmts = yapi->stmt_list_new();
    _ n = yapi->list_len(vals);
    i32 i = 0;
    while (i < n) {
        _ args = yapi->list_new();
        args = yapi->list_push(args, yapi->list_get(vals, i));
        _ call_expr = yapi->call(yapi->var(c"emit_char"), args);
        stmts = yapi->stmt_list_push(stmts, yapi->expr_stmt(call_expr));
        i = i + 1;
    }
    ret yapi->block(stmts);
}

i32 fn main() {
    _ a = add_const:(#10, [1, 2, 3]); // 10 + 1 + 2 + 3 = 16
    _ b = add_const:(#100);           // omitted last arg defaults to empty yExprList

    emit_chars:([72, 105, 10]); // prints "Hi\n"

    if (a == 16) io->puts(c"List sum OK");
    if (b == 100) io->puts(c"Default empty list OK");

    ret a + b - 116;
}
