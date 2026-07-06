import io
import arity_test

// Verifies yapi->call_args_new/call_args_push + yapi->call: the restored
// general-arity call-expression builder for >3 args (yapi->call0..call3 top
// out at 3). Builds a call to arity_test->sum5, a real externally-bound C
// function (modules/arity_test/wrapper.c's arity_test_sum5) taking 5 plain
// args -- the motivating case (an external C function like hashmap_new that
// needs more args than call0..call3 can build).
//
// `vals` is passed as ONE yExprList@ macro argument (same pattern as
// expr_list.yap's add_const/emit_chars) rather than 5 separate ones: macro
// EXECUTION dispatch (yap_exec_macro_call) has its own, unrelated 4-arg cap,
// not touched by this change -- only call-expression BUILDING (yapi->call0..
// call3) was capped at 3 and is what's being fixed here.
//
// cast() pins the result to i32: yapi->var_value(c"arity_test_sum5")'s scope
// lookup only ever resolves names registered in ctx->global_scope itself, so
// it can't see a name registered inside another module's own scope (same
// reason modules/hashmap/hashmap.yap never relies on the un-cast type of its
// own yapi->var_value(c"yhm_make") call -- it only ever appears in assign()/
// expr_stmt position there). This is a pre-existing, unrelated limitation;
// cast() is the same explicit-pin escape hatch used throughout this codebase.
yExpr fn call_sum5(yExprList@ vals) {
    _ args = yapi->call_args_new();
    _ n = vals..len.(i32);
    i32 i = 0;
    while (i < n) {
        args = yapi->call_args_push(args, vals.:[i]);
        i = i + 1;
    }
    _ call_expr = yapi->call(yapi->var_value(c"arity_test_sum5"), args);
    ret yapi->cast(call_expr, yapi->type(c"i32"));
}

i32 fn main() {
    _ result = call_sum5:([1, 2, 3, 4, 5]); // 1+2+3+4+5 = 15

    if (result == 15) { io->print:(c"5-arg call_args builder OK\n"); }

    ret result - 15;
}
