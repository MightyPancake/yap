import io

none fn emit_char(i32 c) {
    io->putchar(c);
}

// Variable number of values through one fixed-arity macro argument, via a
// blob literal that gets unpacked into a yExprList (a real slice of yExpr).
// 'extra' is yExprList@ (pointer), not a bare yExprList value — see project
// memory for why. Reading it is 'extra..len' (deref via a plain 'ptr.', then
// the slice's native '.len' field) and 'extra.:[i]' (deref, then native
// ':[idx]' index access) — no yapi->list_* or yExprList methods needed.
yExpr fn add_const(yExpr base, yExprList@ extra) {
    _ n = extra..len.(i32);
    _ acc = base;
    i32 i = 0;
    while (i < n) {
        acc = yapi->bin_op(acc, 43, extra.:[i]); // '+' == 43
        i = i + 1;
    }
    ret acc;
}

// Statement macro returning a block of calls — exercises yExprList's native
// .len/:[i] through a deref'd pointer, yapi->call1, yapi->stmt_list_new/push
// and yapi->block end to end.
yStatement fn emit_chars(yExprList@ vals) {
    _ stmts = yapi->stmt_list_new();
    _ n = vals..len.(i32);
    i32 i = 0;
    while (i < n) {
        _ call_expr = yapi->call1(yapi->var_value(c"emit_char"), vals.:[i]);
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
