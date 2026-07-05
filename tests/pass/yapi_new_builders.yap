import io

i32 fn assert_eq(i32 a, i32 b, i32 test_num) {
    if (a != b) {
        io->putchar(70); // 'F'
        io->putchar(test_num + 48);
        io->putchar(10);
        ret 1;
    }
    ret 0;
}

// --- for_stmt / break_stmt / continue_stmt ---
// Builds: i32 sum = 0; for (i32 i = 0; i < 10; ++i) { if (i == skip) continue; if (i == stop) break; sum += i; }
// i_name/sum_name must be FRESH hygienic idents (+ident), not pre-declared by
// the caller -- this macro declares both itself and returns one top-level
// block, which a bare macro-call statement flattens into the caller's own
// scope (same mechanism stmt${ }'s var_decl support relies on), so sum stays
// visible to the caller's code after the splice while i stays loop-local.
// Limit is baked in (not a param) to stay within the macro-call arg cap (4).
yStmt fn build_counting_loop(yIdent i_name, yIdent sum_name, yExpr skip_at, yExpr stop_at) {
    _ i32_t = yapi->type(c"i32");

    _ decl_sum = yapi->var_decl(i32_t, sum_name);
    _ init_sum = yapi->expr_stmt(yapi->assign(yapi->new_var(i32_t, sum_name), 61, yapi->int(0))); // sum = 0;  ('=' == 61)
    _ decl_i   = yapi->var_decl(i32_t, i_name);

    _ init   = yapi->expr_stmt(yapi->assign(yapi->new_var(i32_t, i_name), 61, yapi->int(0))); // i = 0;
    _ cond   = yapi->bin_op(yapi->new_var(i32_t, i_name), 130, yapi->int(10));                 // i < 10  (130 == yap_bin_expr_lt)
    _ update = yapi->increment(yapi->new_var(i32_t, i_name), 1);                               // ++i

    _ skip_check = yapi->if_stmt(yapi->bin_op(yapi->new_var(i32_t, i_name), 128, skip_at), yapi->continue_stmt()); // 128 == yap_bin_expr_eq
    _ stop_check = yapi->if_stmt(yapi->bin_op(yapi->new_var(i32_t, i_name), 128, stop_at), yapi->break_stmt());
    _ accumulate = yapi->expr_stmt(yapi->assign(yapi->new_var(i32_t, sum_name), 43, yapi->new_var(i32_t, i_name))); // sum += i;  ('+' == 43)

    _ body_list = yapi->stmt_list_new();
    body_list = yapi->stmt_list_push(body_list, skip_check);
    body_list = yapi->stmt_list_push(body_list, stop_check);
    body_list = yapi->stmt_list_push(body_list, accumulate);
    _ body = yapi->block(body_list);

    _ for_loop = yapi->for_stmt(init, cond, update, body);

    _ top = yapi->stmt_list_new();
    top = yapi->stmt_list_push(top, decl_sum);
    top = yapi->stmt_list_push(top, init_sum);
    top = yapi->stmt_list_push(top, decl_i);
    top = yapi->stmt_list_push(top, for_loop);
    ret yapi->block(top);
}

// --- increment/decrement builders, used standalone (not just as a for-step) ---
yExpr fn build_preinc(yExpr lval)  { ret yapi->increment(lval, 1); } // ++lval
yExpr fn build_postdec(yExpr lval) { ret yapi->decrement(lval, 0); } // lval--

// --- not/bnot builders: mirror '!expr'/'~expr' surface syntax (see operators.yap) ---
yExpr fn build_not(yExpr a)  { ret yapi->not(a); }
yExpr fn build_bnot(yExpr a) { ret yapi->bnot(a); }

// --- opt_member: mirrors 'obj?.field' surface syntax (see optional_chaining.yap) ---
struct Box { i32 val, }

yExpr fn read_opt(yExpr ptr_expr) {
    ret yapi->opt_member(ptr_expr, c"val");
}

// --- array_of: yapi->array_of(elem_type, size) mirrors 'Type[N]' surface syntax ---
yStmt fn declare_i32_array(yIdent name) {
    _ i32_t = yapi->type(c"i32");
    _ arr_t = yapi->array_of(i32_t, 5);
    ret yapi->var_decl(arr_t, name);
}

// --- enum variant with an explicit value ---
// Variant *names* aren't resolvable as values at the yap level yet (a
// pre-existing gap unrelated to this builder -- see type_blueprints.yap's
// signal() comment), so this is a build/round-trip smoke test, same shape as
// that existing enum test: declare, poke a value in via a pointer cast, read
// it back through the emitted type.
yType fn traffic_light() {
    _ et = yapi->enum_t();
    et:add_variant(c"red");
    et:add_variant_value(c"go", yapi->int(10));
    et:add_variant(c"yellow");
    _ res = et:finish(c"traffic_light");
    if (et:existed()) ret res;
    ret res;
}

i32 fn main() {
    i32 fail = 0;

    // for_stmt / break_stmt / continue_stmt (i/sum declared by the macro itself)
    build_counting_loop:(+i, +sum, #3, #7);
    // i: 0,1,2,3(skip),4,5,6,7(break) -> sum = 0+1+2+4+5+6 = 18
    fail = fail + assert_eq(sum, 18, 0);

    // increment/decrement builders
    i32 p = 5;
    _ v1 = build_preinc:(#p);
    fail = fail + assert_eq(v1, 6, 1);
    fail = fail + assert_eq(p, 6, 2);

    i32 q = 5;
    _ v2 = build_postdec:(#q);
    fail = fail + assert_eq(v2, 5, 3);
    fail = fail + assert_eq(q, 4, 4);

    // not/bnot builders
    if (build_not:(#0) == true) fail = fail + 0; else fail = fail + 1;
    fail = fail + assert_eq(build_bnot:(#5), -6, 5);

    // opt_member
    Box b = [42];
    Box@ bptr = b@;
    _ v3 = read_opt:(#bptr);
    fail = fail + assert_eq(v3, 42, 6);

    Box@ null_ptr = null;
    _ v4 = read_opt:(#null_ptr);
    fail = fail + assert_eq(v4, 0, 7);

    // array_of
    declare_i32_array:(+nums);
    nums:[0] = 10;
    nums:[1] = 20;
    nums:[2] = nums:[0] + nums:[1];
    fail = fail + assert_eq(nums:[2], 30, 8);

    // enum variant with an explicit value (build/round-trip smoke test)
    traffic_light:() tl;
    i32@ tl_as_i32 = tl@.(i32@);
    tl_as_i32. = 10;
    fail = fail + assert_eq(tl.(i32), 10, 9);

    ret fail;
}
