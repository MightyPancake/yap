// Generic growable array, implemented entirely in yap via the yapi builder
// API -- no C backend. Callers must `import stdlib` themselves before
// `import arr` (nested module-to-module imports mis-prefix declarations).
//
// Each arr:(T) instantiation embeds T's real size (yapi->sizeof(T)) directly
// into its own push() body, so every field is genuinely T-typed, known at
// comptime per instantiation -- no type-erased elem_size field.
//
// yBinOp/yAssignOp codes (no char-literal syntax in yap; passed as raw
// ASCII/enum ints, same convention as tests/macros.yap):
//   '+' = 43, '*' = 42, '=' = 61
//   yap_bin_expr_eq = 128, yap_bin_expr_lt = 130, yap_bin_expr_ge = 133  (include/yap/semtree.h)
yType fn arr(yType T){
    if (T == yapi->type(c"none")) {
        yapi->error(c"arr:(T): element type cannot be 'none'");
        ret T;
    }
    _ ptr_t = yapi->ptr_of(T);
    _ none_ptr_t = yapi->ptr_of(yapi->type(c"none"));
    _ st = yapi->struct_t();
    st:add_field(ptr_t, c"data");
    st:add_field(yapi->type(c"u32"), c"count");
    st:add_field(yapi->type(c"u32"), c"cap");

    _ res = st:finish(c"arr");
    if (st:existed()) ret res;

    // init(): zeroes data/count/cap.
    _ init_t = res:new_ref_method();
    yExpr self1 = yapi->deref(init_t:get_subject());
    _ init_body = yapi->stmt_list_new();
    init_body = yapi->stmt_list_push(init_body, yapi->expr_stmt(
        yapi->assign(yapi->member(self1, c"data"), 61, yapi->cast(yapi->int(0), ptr_t))));
    init_body = yapi->stmt_list_push(init_body, yapi->expr_stmt(
        yapi->assign(yapi->member(self1, c"count"), 61, yapi->int(0))));
    init_body = yapi->stmt_list_push(init_body, yapi->expr_stmt(
        yapi->assign(yapi->member(self1, c"cap"), 61, yapi->int(0))));
    init_t:set_body(yapi->block(init_body));
    init_t:finish(c"init");

    // push(value): grows the backing storage (doubling, min capacity 8) via
    // stdlib->realloc when count catches up to cap, then appends.
    _ push_t = res:new_ref_method();
    _ val_p = push_t:add_param(T, c"value");
    yExpr self2 = yapi->deref(push_t:get_subject());

    _ grow_body = yapi->stmt_list_new();
    grow_body = yapi->stmt_list_push(grow_body, yapi->if_else_stmt(
        yapi->bin_op(yapi->member(self2, c"cap"), 128, yapi->int(0)), // cap == 0
        yapi->expr_stmt(yapi->assign(yapi->member(self2, c"cap"), 61, yapi->int(8))),
        yapi->expr_stmt(yapi->assign(yapi->member(self2, c"cap"), 42, yapi->int(2))))); // cap *= 2
    grow_body = yapi->stmt_list_push(grow_body, yapi->expr_stmt(
        yapi->assign(yapi->member(self2, c"data"), 61,
            yapi->cast(
                yapi->call2(yapi->var_value(c"stdlib_realloc"),
                    yapi->cast(yapi->member(self2, c"data"), none_ptr_t),
                    yapi->bin_op(yapi->member(self2, c"cap"), 42, yapi->sizeof(T))),
                ptr_t))));

    _ push_body = yapi->stmt_list_new();
    push_body = yapi->stmt_list_push(push_body, yapi->if_stmt(
        yapi->bin_op(yapi->member(self2, c"count"), 133, yapi->member(self2, c"cap")), // count >= cap
        yapi->block(grow_body)));
    push_body = yapi->stmt_list_push(push_body, yapi->expr_stmt(
        yapi->assign(yapi->index(yapi->member(self2, c"data"), yapi->member(self2, c"count")), 61, val_p)));
    push_body = yapi->stmt_list_push(push_body, yapi->expr_stmt(
        yapi->assign(yapi->member(self2, c"count"), 43, yapi->int(1)))); // count += 1
    push_t:set_body(yapi->block(push_body));
    push_t:finish(c"push");

    // at(index): direct pointer indexing into data (data is already
    // correctly typed, so this is real indexing, not a C-helper round trip).
    _ at_t = res:new_method();
    _ idx_p = at_t:add_param(yapi->type(c"i32"), c"index");
    yExpr self3 = at_t:get_subject();
    at_t:set_return_type(T);
    at_t:set_body(yapi->return_stmt(
        yapi->index(yapi->member(self3, c"data"), idx_p)));
    at_t:finish(c"at");

    // len(): element count.
    _ len_t = res:new_method();
    yExpr self4 = len_t:get_subject();
    len_t:set_return_type(yapi->type(c"u32"));
    len_t:set_body(yapi->return_stmt(yapi->member(self4, c"count")));
    len_t:finish(c"len");

    // free(): releases the backing storage via stdlib->free.
    _ free_t = res:new_ref_method();
    yExpr self5 = yapi->deref(free_t:get_subject());
    free_t:set_body(yapi->expr_stmt(
        yapi->call1(yapi->var_value(c"stdlib_free"),
            yapi->cast(yapi->member(self5, c"data"), none_ptr_t))));
    free_t:finish(c"free");

    // Higher-order methods. Each callback param is declared with its exact
    // function type (yapi->func_typeN), so the ordinary call-site argument
    // check rejects a mismatched function ("Argument type mismatch: expected
    // '(T fn(T))', got ...") -- no ad-hoc checking needed here.
    _ u32_t = yapi->type(c"u32");

    // map(f): new arr(T) with f (T fn T) applied to every element. The result
    // is exactly-sized (cap = count) and independently owned -- free() both.
    _ map_t = res:new_method();
    _ map_f = map_t:add_param(yapi->func_type1(T, T), c"f");
    yExpr self6 = map_t:get_subject();
    map_t:set_return_type(res);
    _ out6_n = yapi->uniq_name();
    _ i6_n = yapi->uniq_name();
    _ out6 = yapi->new_var(res, out6_n);
    _ i6 = yapi->new_var(u32_t, i6_n);

    _ map_loop = yapi->stmt_list_new();
    map_loop = yapi->stmt_list_push(map_loop, yapi->expr_stmt(
        yapi->assign(yapi->index(yapi->member(out6, c"data"), i6), 61,
            yapi->call1(map_f, yapi->index(yapi->member(self6, c"data"), i6)))));
    map_loop = yapi->stmt_list_push(map_loop, yapi->expr_stmt(
        yapi->assign(i6, 43, yapi->int(1)))); // i += 1

    _ map_body = yapi->stmt_list_new();
    map_body = yapi->stmt_list_push(map_body, yapi->var_decl(res, out6_n));
    map_body = yapi->stmt_list_push(map_body, yapi->expr_stmt(
        yapi->assign(yapi->member(out6, c"data"), 61,
            yapi->cast(yapi->call1(yapi->var_value(c"stdlib_malloc"),
                yapi->bin_op(yapi->member(self6, c"count"), 42, yapi->sizeof(T))), ptr_t))));
    map_body = yapi->stmt_list_push(map_body, yapi->expr_stmt(
        yapi->assign(yapi->member(out6, c"count"), 61, yapi->member(self6, c"count"))));
    map_body = yapi->stmt_list_push(map_body, yapi->expr_stmt(
        yapi->assign(yapi->member(out6, c"cap"), 61, yapi->member(self6, c"count"))));
    map_body = yapi->stmt_list_push(map_body, yapi->var_decl(u32_t, i6_n));
    map_body = yapi->stmt_list_push(map_body, yapi->expr_stmt(
        yapi->assign(i6, 61, yapi->int(0))));
    map_body = yapi->stmt_list_push(map_body, yapi->while_stmt(
        yapi->bin_op(i6, 130, yapi->member(self6, c"count")), // i < count
        yapi->block(map_loop)));
    map_body = yapi->stmt_list_push(map_body, yapi->return_stmt(out6));
    map_t:set_body(yapi->block(map_body));
    map_t:finish(c"map");

    // filter(keep): new arr(T) with the elements keep (bool fn T) approved.
    // Over-allocates to the source count (cap = source count, count = kept).
    _ filt_t = res:new_method();
    _ filt_f = filt_t:add_param(yapi->func_type1(yapi->type(c"bool"), T), c"keep");
    yExpr self7 = filt_t:get_subject();
    filt_t:set_return_type(res);
    _ out7_n = yapi->uniq_name();
    _ i7_n = yapi->uniq_name();
    _ out7 = yapi->new_var(res, out7_n);
    _ i7 = yapi->new_var(u32_t, i7_n);

    _ keep_body = yapi->stmt_list_new();
    keep_body = yapi->stmt_list_push(keep_body, yapi->expr_stmt(
        yapi->assign(yapi->index(yapi->member(out7, c"data"), yapi->member(out7, c"count")), 61,
            yapi->index(yapi->member(self7, c"data"), i7))));
    keep_body = yapi->stmt_list_push(keep_body, yapi->expr_stmt(
        yapi->assign(yapi->member(out7, c"count"), 43, yapi->int(1)))); // count += 1

    _ filt_loop = yapi->stmt_list_new();
    filt_loop = yapi->stmt_list_push(filt_loop, yapi->if_stmt(
        yapi->call1(filt_f, yapi->index(yapi->member(self7, c"data"), i7)),
        yapi->block(keep_body)));
    filt_loop = yapi->stmt_list_push(filt_loop, yapi->expr_stmt(
        yapi->assign(i7, 43, yapi->int(1)))); // i += 1

    _ filt_body = yapi->stmt_list_new();
    filt_body = yapi->stmt_list_push(filt_body, yapi->var_decl(res, out7_n));
    filt_body = yapi->stmt_list_push(filt_body, yapi->expr_stmt(
        yapi->assign(yapi->member(out7, c"data"), 61,
            yapi->cast(yapi->call1(yapi->var_value(c"stdlib_malloc"),
                yapi->bin_op(yapi->member(self7, c"count"), 42, yapi->sizeof(T))), ptr_t))));
    filt_body = yapi->stmt_list_push(filt_body, yapi->expr_stmt(
        yapi->assign(yapi->member(out7, c"cap"), 61, yapi->member(self7, c"count"))));
    filt_body = yapi->stmt_list_push(filt_body, yapi->expr_stmt(
        yapi->assign(yapi->member(out7, c"count"), 61, yapi->int(0))));
    filt_body = yapi->stmt_list_push(filt_body, yapi->var_decl(u32_t, i7_n));
    filt_body = yapi->stmt_list_push(filt_body, yapi->expr_stmt(
        yapi->assign(i7, 61, yapi->int(0))));
    filt_body = yapi->stmt_list_push(filt_body, yapi->while_stmt(
        yapi->bin_op(i7, 130, yapi->member(self7, c"count")), // i < count
        yapi->block(filt_loop)));
    filt_body = yapi->stmt_list_push(filt_body, yapi->return_stmt(out7));
    filt_t:set_body(yapi->block(filt_body));
    filt_t:finish(c"filter");

    // fold(f, acc): reduce with f (T fn T acc, T elem), starting from acc.
    _ fold_t = res:new_method();
    _ fold_f = fold_t:add_param(yapi->func_type2(T, T, T), c"f");
    _ fold_acc = fold_t:add_param(T, c"acc");
    yExpr self8 = fold_t:get_subject();
    fold_t:set_return_type(T);
    _ i8_n = yapi->uniq_name();
    _ i8 = yapi->new_var(u32_t, i8_n);

    _ fold_loop = yapi->stmt_list_new();
    fold_loop = yapi->stmt_list_push(fold_loop, yapi->expr_stmt(
        yapi->assign(fold_acc, 61,
            yapi->call2(fold_f, fold_acc, yapi->index(yapi->member(self8, c"data"), i8)))));
    fold_loop = yapi->stmt_list_push(fold_loop, yapi->expr_stmt(
        yapi->assign(i8, 43, yapi->int(1)))); // i += 1

    _ fold_body = yapi->stmt_list_new();
    fold_body = yapi->stmt_list_push(fold_body, yapi->var_decl(u32_t, i8_n));
    fold_body = yapi->stmt_list_push(fold_body, yapi->expr_stmt(
        yapi->assign(i8, 61, yapi->int(0))));
    fold_body = yapi->stmt_list_push(fold_body, yapi->while_stmt(
        yapi->bin_op(i8, 130, yapi->member(self8, c"count")), // i < count
        yapi->block(fold_loop)));
    fold_body = yapi->stmt_list_push(fold_body, yapi->return_stmt(fold_acc));
    fold_t:set_body(yapi->block(fold_body));
    fold_t:finish(c"fold");

    ret res;
}
