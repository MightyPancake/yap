import io

yExpr fn identity(yExpr a) {
    ret a;
}

yExpr fn plus_one(yExpr a) {
    _ one = yapi->int(1);
    // '+' == 43
    ret yapi->bin_op(a, 43, one);
}

yType fn pair(yType t1, yType t2){
    _ st = yapi->struct_t();
    st:add_field(t1, c"first");
    st:add_field(t2, c"second");
    ret st:finish(c"pair");
}

yExpr fn emit_and_add(yExpr x_val, yExpr y_val) {
    _ i32_id = yapi->type(c"i32");
    _ sb = yapi->struct_t();
    sb:add_field(i32_id, c"x");
    sb:add_field(i32_id, c"y");
    sb:finish(c"test_struct");
    // '+' == 43
    ret yapi->bin_op(x_val, 43, y_val);
}

yExpr fn add_via_uniq(yExpr a, yExpr b) {
    _ uname = yapi->uniq_name();
    ret yapi->bin_op(a, 43, b);
}

yExpr fn with_ident(yIdent name, yExpr val) {
    ret val;
}

yStmt fn declare_int(yIdent name) {
    _ tid = yapi->type(c"i32");
    ret yapi->var_decl(tid, name);
}

// Void comptime call for side effects
none fn ct_greet() {
    yapi->log(c"comptime greet!");
}

i32 fn main() {
    // #expr passes as yExpr AST node
    _ x = identity:(#42);
    _ y = plus_one:(#10);
    _ z = emit_and_add:(#3, #7);

    // Type args passed as raw identifiers
    pair:(i32, i32) p;
    p.first = 4;
    p.second = 2;
    if (p.first == 4) io->print:(c"Types OK\n");

    // #expr for AST, +ident for hygiene
    _ u = add_via_uniq:(#3, #4);
    _ w = with_ident:(+myvar, #99);

    // Statement macro: introduces the var, plain code initializes it
    declare_int:(+answer);
    answer = 100;

    // Void comptime call (raw args, side effect only)
    ct_greet:();

    // Comptime log with raw string arg
    yapi->log:(c"direct log from main");

    if (y == 11) io->print:(c"Builder OK\n");
    if (z == 10) io->print:(c"Emission OK\n");
    if (u == 7) io->print:(c"Uniq OK\n");
    if (w == 99) io->print:(c"Ident OK\n");
    if (answer == 100) io->print:(c"Statement OK\n");
    ret x + y + z + u + w + answer - 269;
}
