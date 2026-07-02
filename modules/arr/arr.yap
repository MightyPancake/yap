// Generic growable array, implemented entirely in yap via the yapi builder
// API -- no C backend. Callers must `import stdlib` themselves before
// `import arr` (arr's own mod.yap can't import stdlib on arr's behalf --
// nested module-to-module imports mis-prefix the inner module's
// declarations with the outer module's prefix; see the CI gotcha note in
// this project's arr-module memory).
//
// Each arr:(T) instantiation embeds T's real size
// (yapi->sizeof(T)) directly into its own push() body, so unlike the earlier
// C-backend version there's no type-erased elem_size field: every field is
// genuinely T-typed / u32-typed, known at comptime per instantiation.
//
// yBinOp/yAssignOp codes (no char-literal syntax in yap; passed as raw
// ASCII/enum ints, same convention as tests/macros.yap):
//   '+' = 43, '*' = 42, '=' = 61
//   yap_bin_expr_eq = 128, yap_bin_expr_ge = 133  (include/yap/semtree.h)
yType fn arr(yType T){
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

    ret res;
}
