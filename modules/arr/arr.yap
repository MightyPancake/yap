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

    // init(): zeroes data/count/cap. Written as a stmt${ } template (holes
    // filled with self1 and the pointer type) instead of hand-assembled
    // yapi-> calls -- reads like the C it generates.
    _ init_t = res:new_ref_method();
    yExpr self1 = yapi->deref(init_t:get_subject());
    init_t:set_body(
        stmt${
            $self.data = 0.($PtrT);
            $self.count = 0;
            $self.cap = 0;
        }:fill_expr(c"self", self1):fill_type(c"PtrT", ptr_t):finish()
    );
    init_t:finish(c"init");

    // push(value): grows the backing storage (doubling, min capacity 8) via
    // stdlib->realloc when count catches up to cap, then appends.
    // Doubling avoids an if/else (not supported in stmt${ }): cap = cap*2
    // maps 0 -> 0 and n -> 2n, then the single-branch `if (cap == 0)` supplies
    // the minimum capacity 8. $realloc/$elemsize are comptime values spliced
    // in (function ref / sizeof(T)); $NonePtr/$PtrT are the cast types.
    _ push_t = res:new_ref_method();
    _ val_p = push_t:add_param(T, c"value");
    yExpr self2 = yapi->deref(push_t:get_subject());
    push_t:set_body(
        stmt${
            if ($self.count >= $self.cap) {
                $self.cap = $self.cap * 2;
                if ($self.cap == 0) { $self.cap = 8; }
                $self.data = $realloc($self.data.($NonePtr), $self.cap * $elemsize).($PtrT);
            }
            $self.data:[$self.count] = $value;
            $self.count = $self.count + 1;
        }
        :fill_type(c"NonePtr", none_ptr_t)
        :fill_type(c"PtrT", ptr_t)
        :fill_expr(c"self", self2)
        :fill_expr(c"value", val_p)
        :fill_expr(c"realloc", yapi->var_value(c"stdlib_realloc"))
        :fill_expr(c"elemsize", yapi->sizeof(T)):finish());
    push_t:finish(c"push");

    // at(index): direct pointer indexing into data (data is already
    // correctly typed, so this is real indexing, not a C-helper round trip).
    // Index access inside a blueprint must use ':[' (the index_access rule);
    // bare '[' is ambiguous with array_type's size-bracket, since a
    // blueprint_hole is valid in both _expr and typ positions.
    _ at_t = res:new_method();
    _ idx_p = at_t:add_param(yapi->type(c"i32"), c"index");
    yExpr self3 = at_t:get_subject();
    at_t:set_return_type(T);
    at_t:set_body(
        stmt${ ret $self.data:[$idx]; }:fill_expr(c"self", self3):fill_expr(c"idx", idx_p):finish());
    at_t:finish(c"at");

    // len(): element count.
    _ len_t = res:new_method();
    yExpr self4 = len_t:get_subject();
    len_t:set_return_type(yapi->type(c"u32"));
    len_t:set_body(stmt${ ret $self.count; }:fill_expr(c"self", self4):finish());
    len_t:finish(c"len");

    // free(): releases the backing storage via stdlib->free.
    _ free_t = res:new_ref_method();
    yExpr self5 = yapi->deref(free_t:get_subject());
    free_t:set_body(
        stmt${ $free($self.data.($NonePtr)); }
            :fill_expr(c"free", yapi->var_value(c"stdlib_free"))
            :fill_expr(c"self", self5)
            :fill_type(c"NonePtr", none_ptr_t):finish());
    free_t:finish(c"free");

    // Higher-order methods. Each callback param is declared with its exact
    // function type (yapi->func_typeN), so the ordinary call-site argument
    // check rejects a mismatched function ("Argument type mismatch: expected
    // '(T fn(T))', got ...") -- no ad-hoc checking needed here.
    _ u32_t = yapi->type(c"u32");

    // map(f): new arr(T) with f (T fn T) applied to every element. The result
    // is exactly-sized (cap = count) and independently owned -- free() both.
    // $ResT is a type hole (the arr(T) instantiation isn't nameable as a
    // literal in the template); $elemsize/$malloc are comptime values spliced
    // in ($elemsize = sizeof(T), since there's no sizeof surface syntax).
    _ map_t = res:new_method();
    _ map_f = map_t:add_param(yapi->fn_type1(T, T), c"f");
    yExpr self6 = map_t:get_subject();
    map_t:set_return_type(res);
    map_t:set_body(
        stmt${
            $ResT $out;
            $out.data = $malloc($self.count * $elemsize).($PtrT);
            $out.count = $self.count;
            $out.cap = $self.count;
            u32 $i;
            $i = 0;
            while ($i < $self.count) {
                $out.data:[$i] = $f($self.data:[$i]);
                $i = $i + 1;
            }
            ret $out;
        }
        :fill_type(c"ResT", res)
        :fill_type(c"PtrT", ptr_t)
        :fill_var(c"out", res, yapi->uniq_name())
        :fill_var(c"i", u32_t, yapi->uniq_name())
        :fill_expr(c"self", self6)
        :fill_expr(c"f", map_f)
        :fill_expr(c"malloc", yapi->var_value(c"stdlib_malloc"))
        :fill_expr(c"elemsize", yapi->sizeof(T)):finish());
    map_t:finish(c"map");

    // filter(keep): new arr(T) with the elements keep (bool fn T) approved.
    // Over-allocates to the source count (cap = source count, count = kept).
    _ filt_t = res:new_method();
    _ filt_f = filt_t:add_param(yapi->fn_type1(yapi->type(c"bool"), T), c"keep");
    yExpr self7 = filt_t:get_subject();
    filt_t:set_return_type(res);
    filt_t:set_body(
        stmt${
            $ResT $out;
            $out.data = $malloc($self.count * $elemsize).($PtrT);
            $out.cap = $self.count;
            $out.count = 0;
            u32 $i;
            $i = 0;
            while ($i < $self.count) {
                if ($keep($self.data:[$i])) {
                    $out.data:[$out.count] = $self.data:[$i];
                    $out.count = $out.count + 1;
                }
                $i = $i + 1;
            }
            ret $out;
        }
        :fill_type(c"ResT", res)
        :fill_type(c"PtrT", ptr_t)
        :fill_var(c"out", res, yapi->uniq_name())
        :fill_var(c"i", u32_t, yapi->uniq_name())
        :fill_expr(c"self", self7)
        :fill_expr(c"keep", filt_f)
        :fill_expr(c"malloc", yapi->var_value(c"stdlib_malloc"))
        :fill_expr(c"elemsize", yapi->sizeof(T)):finish());
    filt_t:finish(c"filter");

    // fold(f, acc): reduce with f (T fn T acc, T elem), starting from acc.
    _ fold_t = res:new_method();
    _ fold_f = fold_t:add_param(yapi->fn_type2(T, T, T), c"f");
    _ fold_acc = fold_t:add_param(T, c"acc");
    yExpr self8 = fold_t:get_subject();
    fold_t:set_return_type(T);
    fold_t:set_body(
        stmt${
            u32 $i;
            $i = 0;
            while ($i < $self.count) {
                $acc = $f($acc, $self.data:[$i]);
                $i = $i + 1;
            }
            ret $acc;
        }
        :fill_var(c"i", u32_t, yapi->uniq_name())
        :fill_expr(c"self", self8)
        :fill_expr(c"acc", fold_acc)
        :fill_expr(c"f", fold_f):finish());
    fold_t:finish(c"fold");

    // foreach(cb): calls cb(index, element) once per element, in order --
    // pure iteration side effect (unlike map/filter/fold, doesn't build a
    // new arr(T)), e.g. `nums:foreach((none fn u32 i, i32 v) { io->print:(c"%u: %d\n", [i, v]); });`.
    // Named distinctly from the receiver-dispatched macro `for` below (not
    // "for" itself) -- same word for two different mechanisms reads as one
    // ambiguous API, even with them living in genuinely separate tables
    // (see the register_macro_method call at the end of this function).
    _ foreach_t = res:new_method();
    _ foreach_cb = foreach_t:add_param(yapi->fn_type2(yapi->type(c"none"), u32_t, T), c"cb");
    yExpr self9 = foreach_t:get_subject();
    foreach_t:set_body(
        stmt${
            u32 $i;
            $i = 0;
            while ($i < $self.count) {
                $cb($i, $self.data:[$i]);
                $i = $i + 1;
            }
        }
        :fill_var(c"i", u32_t, yapi->uniq_name())
        :fill_expr(c"self", self9)
        :fill_expr(c"cb", foreach_cb):finish());
    foreach_t:finish(c"foreach");

    // Claim "for" (defined below, an ordinary ungeneric top-level macro) as
    // THIS concrete instantiation's macro method, dynamically, right where
    // res is known -- see yapi->register_macro_method's own comment for why
    // this makes a macro method and a same-named real method structurally
    // incapable of colliding (separate table entirely), not just
    // disambiguated by a shape check.
    yapi->register_macro_method(res, c"for", c"for");

    ret res;
}

// new(T): value-yielding convenience constructor. Builds a zero-initialized
// arr(T) (the same fields init() sets) and yields it as a single expression
// via yapi->block_expr's GNU statement-expression, so callers can write
// `_ a = arr->new:(i32);` -- type inferred from the initializer -- instead of
// a separate var decl followed by an init() call. arr(T) is fetched via a
// plain (non-splicing) call through the module, `arr->arr(T)`, same as any
// yapi-> builder call -- a *bare* `arr(T)` call resolves at the source level
// but the comptime C translation unit only ever defines/links the type's
// methods (and itself) under the module-prefixed name ("arr_arr"), so a bare
// reference emits an unresolvable symbol; going through the module gives the
// correctly-prefixed name, same as every other cross-function reference here.
yExpr fn new(yType T) {
    _ arr_t = arr->arr(T);
    _ ptr_t = yapi->ptr_of(T);
    _ name = yapi->uniq_name();
    _ out = yapi->new_var(arr_t, name);

    _ stmts = yapi->stmt_list_new();
    stmts = yapi->stmt_list_push(stmts, yapi->var_decl(arr_t, name));
    stmts = yapi->stmt_list_push(stmts, yapi->expr_stmt(
        yapi->assign(yapi->member(out, c"data"), 61, yapi->cast(yapi->int(0), ptr_t))));
    stmts = yapi->stmt_list_push(stmts, yapi->expr_stmt(
        yapi->assign(yapi->member(out, c"count"), 61, yapi->int(0))));
    stmts = yapi->stmt_list_push(stmts, yapi->expr_stmt(
        yapi->assign(yapi->member(out, c"cap"), 61, yapi->int(0))));
    stmts = yapi->stmt_list_push(stmts, yapi->expr_stmt(out));

    ret yapi->block_expr(stmts);
}

// for(self, i, v, body): receiver-dispatched macro-method form of iteration,
// e.g. `gnums:for:(+i, +v, { io->print:(c"%u: %d\n", [i, v]); });` -- +i/+v
// are hygienic idents the macro itself declares (u32 index, T-typed element)
// and the block body can reference them directly, unlike the typed-callback
// method `foreach(cb)` above. This is an ordinary, ungeneric top-level
// macro -- it's arr(T) (above) that dynamically claims it as each concrete
// instantiation's own "for" via yapi->register_macro_method, so 'gnums:for:(...)'
// dispatch is a direct (receiver's exact type, "for") lookup, not a guess
// by name or shape. The hygiene itself works because 'body' arrives as an
// unbuilt fragment (yap_macro_param_statement in yap_exec_macro_call) that
// gets built later, once $i/$v actually exist in scope, by
// yap_resolve_deferred_fragments (yap-semantic/build.c) walking the spliced
// tree with a real, nested scope. Element type isn't an explicit argument
// here (dispatched by receiver, not `arr->for:(T, ...)`), so it's derived
// structurally from self's own 'data' field -- via yapi->field_type
// directly on self's type, not yapi->member(self,...) + type_of, since a
// yapi->member(...)-built expr's .type is only resolved once the *spliced*
// result later undergoes real semantic building, not during this macro's
// own comptime execution.
yStmt fn for(yExpr self, yIdent i, yIdent v, yStmt body) {
    _ ptr_t = yapi->field_type(yapi->type_of(self), c"data");
    _ elem_t = yapi->pointee_type(ptr_t);
    _ u32_t = yapi->type(c"u32");
    ret stmt${
        u32 $i;
        $i = 0;
        while ($i < $self.count) {
            $ElemT $v;
            $v = $self.data:[$i];
            $body;
            $i = $i + 1;
        }
    }
    :fill_var(c"i", u32_t, i)
    :fill_type(c"ElemT", elem_t)
    :fill_var(c"v", elem_t, v)
    :fill_expr(c"self", self)
    :fill_stmt(c"body", body):finish();
}
