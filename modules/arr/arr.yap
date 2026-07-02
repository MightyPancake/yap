// Generic growable array. Built on top of the type-erased C backend in
// impl.c (arr_impl_init/push/get/len/free, realloc-based growth). The
// emitted struct always has the same four fields (data/count/cap/elem_size)
// but 'data' is genuinely T-typed, not void* -- so each T gets its own
// distinct emitted struct + methods (finish()'s field-layout hash naturally
// differs per T), while the C helpers still just treat it as a raw byte
// buffer sized by elem_size.
//
// push()/init()/free() mutate the instance in place, so they're built as
// pointer-receiver methods (new_ref_method()) -- the call site auto-takes
// the address of an lvalue receiver, so callers still just write
// 'a:push(x)', not '&a:push(x)'.
yType fn arr(yType T){
    _ ptr_t = yapi->ptr_of(T);
    _ st = yapi->struct_t();
    st:add_field(ptr_t, c"data");
    st:add_field(yapi->type(c"u32"), c"count");
    st:add_field(yapi->type(c"u32"), c"cap");
    st:add_field(yapi->type(c"u32"), c"elem_size");

    _ res = st:finish(c"arr");
    if (st:existed()) ret res;

    _ elem_sz = yapi->sizeof(T);

    // init(): wires elem_size, zeroes data/count/cap.
    _ init_t = res:new_ref_method();
    yExpr self1 = init_t:get_subject();
    init_t:set_body(yapi->expr_stmt(
        yapi->call2(yapi->var_value(c"arr_impl_init"), self1, elem_sz)));
    init_t:finish(c"init");

    // push(value): appends, growing the backing storage as needed.
    _ push_t = res:new_ref_method();
    _ val_p = push_t:add_param(T, c"value");
    yExpr self2 = push_t:get_subject();
    push_t:set_body(yapi->expr_stmt(
        yapi->call2(yapi->var_value(c"arr_impl_push"), self2, yapi->addr_of(val_p))));
    push_t:finish(c"push");

    // at(index): direct pointer indexing into data (real indexing, not a
    // C-helper round trip -- data is already correctly typed).
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

    // free(): releases the backing storage.
    _ free_t = res:new_ref_method();
    yExpr self5 = free_t:get_subject();
    free_t:set_body(yapi->expr_stmt(
        yapi->call1(yapi->var_value(c"arr_impl_free"), self5)));
    free_t:finish(c"free");

    ret res;
}
