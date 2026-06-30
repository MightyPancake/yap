import io

yExpr fn identity(yExpr a) {
    ret a;
}

yExpr fn plus_one(yExpr a) {
    _ one = yapi->int(1);
    // '+' == 43
    ret yapi->bin(a, 43, one);
}

yType fn pair(yType t1, yType t2){
    _ res = yapi->struct_new(c"pair");
    yapi->struct_field(res, c"first", t1);
    yapi->struct_field(res, c"second", t2);
    _ emission = yapi->emit_type(res);
    ret emission.type;
}

yExpr fn emit_and_add(yExpr x_val, yExpr y_val) {
    _ i32_id = yapi->type_id(c"i32");
    _ sb = yapi->struct_new(c"test_struct");
    yapi->struct_field(sb, c"x", i32_id);
    yapi->struct_field(sb, c"y", i32_id);
    yapi->emit_type(sb);
    // '+' == 43
    ret yapi->bin(x_val, 43, y_val);
}

yExpr fn add_via_uniq(yExpr a, yExpr b) {
    _ uname = yapi->uniq_name();
    ret yapi->bin(a, 43, b);
}

yExpr fn with_ident(yIdent name, yExpr val) {
    ret val;
}

yStatement fn declare_int(yIdent name, yExpr value) {
    _ tid = yapi->type_id(c"i32");
    ret yapi->var_decl(name, tid, value);
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
    if (p.first == 4) io->puts(c"Types OK");

    // #expr for AST, +ident for hygiene
    _ u = add_via_uniq:(#3, #4);
    _ w = with_ident:(+myvar, #99);

    // Statement macro
    declare_int:(+answer, #100);

    // Void comptime call (raw args, side effect only)
    ct_greet:();

    // Comptime log with raw string arg
    yapi->log:(c"direct log from main");

    if (y == 11) io->puts(c"Builder OK");
    if (z == 10) io->puts(c"Emission OK");
    if (u == 7) io->puts(c"Uniq OK");
    if (w == 99) io->puts(c"Ident OK");
    if (answer == 100) io->puts(c"Statement OK");
    ret x + y + z + u + w + answer - 269;
}
