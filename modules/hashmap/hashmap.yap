// Generic, type-safe wrapper around the raw hashmap module (binds.yap),
// mirroring modules/arr's approach: hashmap(K, V) builds one concrete
// struct type per (K, V) instantiation (hash-deduplicated by yapi's
// struct_t:finish, same as arr(T)), with real K/V-typed methods layered
// over the raw none@ item-pointer surface underneath.
//
// Key support is intentionally narrow: K must be a 4-byte or 8-byte scalar
// (i32/u32/f32/i64/u64/f64), or byte@ treated as a null-terminated C string
// (hashed/compared by contents, not pointer identity -- see wrapper.c's
// hm_hash_cstr/hm_compare_cstr). No arbitrary struct keys.

// Picks wrapper.c's hash_id/cmp_id (0 = 4-byte key, 1 = 8-byte key, 2 =
// byte@ C-string key) for a given key type. Early-returns per branch rather
// than declaring then reassigning a local (yap requires initialization at
// declaration). -1 means "unsupported"; callers check for that.
i32 fn key_size_id(yType K) {
    if (K == yapi->type(c"i32") || K == yapi->type(c"u32") || K == yapi->type(c"f32")) {
        ret 0;
    }
    if (K == yapi->type(c"i64") || K == yapi->type(c"u64") || K == yapi->type(c"f64")) {
        ret 1;
    }
    if (K == yapi->ptr_of(yapi->type(c"byte"))) {
        ret 2;
    }
    ret -1;
}

yType fn hashmap(yType K, yType V) {
    _ none_t = yapi->type(c"none");
    if (K == none_t) {
        yapi->error(c"hashmap:(K, V): key type cannot be 'none'");
        ret K;
    }
    if (V == none_t) {
        yapi->error(c"hashmap:(K, V): value type cannot be 'none'");
        ret K;
    }
    _ size_id = hashmap->key_size_id(K);
    if (size_id < 0) {
        yapi->error(c"hashmap:(K, V): unsupported key type -- K must be a 4- or 8-byte scalar (i32/u32/f32/i64/u64/f64) or byte@ (C string)");
        ret K;
    }

    // entry_t = { K key, V value }: gives hashmap_make's elsize via
    // yapi->sizeof, and (being K/V-shaped itself) is what forces the outer
    // wrapper struct below to hash differently per (K, V) instantiation --
    // same trick arr(T) gets for free from its own T-typed 'data' field.
    _ entry_st = yapi->struct_t();
    entry_st:add_field(K, c"key");
    entry_st:add_field(V, c"value");
    _ entry_t = entry_st:finish(c"hashmap_entry");

    _ none_ptr_t = yapi->ptr_of(none_t);
    _ entry_ptr_t = yapi->ptr_of(entry_t);
    _ raw_map_ptr_t = yapi->ptr_of(yapi->type(c"hashmap"));

    _ st = yapi->struct_t();
    st:add_field(raw_map_ptr_t, c"map");
    st:add_field(entry_ptr_t, c"_phantom"); // never read/written; forces per-(K,V) struct identity
    _ res = st:finish(c"hashmap");
    if (st:existed()) ret res;

    // init(): allocates the real underlying map via hashmap_make. Pointer
    // receiver since it assigns self's own fields (same reasoning as
    // arr(T)'s init()).
    _ init_t = res:new_ref_method();
    yExpr self1 = yapi->deref(init_t:get_subject());
    init_t:set_body(
        stmt${
            $self.map = $make($elsize, $hashid, $cmpid);
            $self._phantom = 0.($EntryPtr);
        }
        :fill_expr(c"self", self1)
        :fill_expr(c"make", yapi->var_value(c"yhm_make"))
        :fill_expr(c"elsize", yapi->sizeof(entry_t))
        :fill_expr(c"hashid", yapi->int(size_id))
        :fill_expr(c"cmpid", yapi->int(size_id))
        :fill_type(c"EntryPtr", entry_ptr_t)
        :finish()
    );
    init_t:finish(c"init");

    // put(key, value): builds a { key, value } entry on the stack and hands
    // its address to hashmap_set, which copies it in (replacing any
    // existing entry for the same key). For a byte@ key this only copies
    // the *pointer*, not the string contents (the map borrows it) -- the
    // caller must keep the buffer alive for as long as it's used as a key.
    _ put_t = res:new_method();
    _ pkey_p = put_t:add_param(K, c"key");
    _ pval_p = put_t:add_param(V, c"value");
    yExpr self2 = put_t:get_subject();
    put_t:set_body(
        stmt${
            $EntryT $e;
            $e.key = $k;
            $e.value = $v;
            $set($self.map, $e@.($NoneP));
        }
        :fill_type(c"EntryT", entry_t)
        :fill_var(c"e", entry_t, yapi->uniq_name())
        :fill_expr(c"k", pkey_p)
        :fill_expr(c"v", pval_p)
        :fill_expr(c"self", self2)
        :fill_expr(c"set", yapi->var_value(c"yhm_set"))
        :fill_type(c"NoneP", none_ptr_t)
        :finish()
    );
    put_t:finish(c"put");

    // has(key): true iff a lookup finds an entry (no value copy).
    _ has_t = res:new_method();
    _ hkey_p = has_t:add_param(K, c"key");
    yExpr self3 = has_t:get_subject();
    has_t:set_return_type(yapi->type(c"bool"));
    has_t:set_body(
        stmt${
            $EntryT $e;
            $e.key = $k;
            ret $get($self.map, $e@.($NoneP)) != 0.($NoneP);
        }
        :fill_type(c"EntryT", entry_t)
        :fill_var(c"e", entry_t, yapi->uniq_name())
        :fill_expr(c"k", hkey_p)
        :fill_expr(c"self", self3)
        :fill_expr(c"get", yapi->var_value(c"yhm_get"))
        :fill_type(c"NoneP", none_ptr_t)
        :finish()
    );
    has_t:finish(c"has");

    // get(key): returns the stored value, or V's zero value on a miss (a
    // deliberate default -- simpler than threading a found/not-found out
    // param through every call site; use has(key) first if that
    // distinction matters, or try_get(key, out) below to do both in one
    // lookup). The zero value is built via yhm_zero (memset) rather than
    // casting 0 to V, since that cast only works for scalar V -- casting an
    // int to a struct type isn't valid, same as in C, and V may now be a
    // struct (e.g. hashmap(byte@, test_struct)).
    _ get_t = res:new_method();
    _ gkey_p = get_t:add_param(K, c"key");
    yExpr self4 = get_t:get_subject();
    get_t:set_return_type(V);
    {
        _ probe_name = yapi->uniq_name();
        yExpr probe_var = yapi->new_var(entry_t, probe_name);
        _ found_name = yapi->uniq_name();
        yExpr found_var = yapi->new_var(none_ptr_t, found_name);
        _ zero_name = yapi->uniq_name();
        yExpr zero_var = yapi->new_var(V, zero_name);

        _ stmts = yapi->stmt_list_new();
        stmts = yapi->stmt_list_push(stmts, yapi->var_decl(entry_t, probe_name));
        stmts = yapi->stmt_list_push(stmts, yapi->expr_stmt(
            yapi->assign(yapi->member(probe_var, c"key"), 61, gkey_p)));
        stmts = yapi->stmt_list_push(stmts, yapi->var_decl(none_ptr_t, found_name));
        stmts = yapi->stmt_list_push(stmts, yapi->expr_stmt(
            yapi->assign(found_var, 61,
                yapi->call2(yapi->var_value(c"yhm_get"),
                    yapi->member(self4, c"map"),
                    yapi->cast(yapi->addr_of(probe_var), none_ptr_t)))));
        stmts = yapi->stmt_list_push(stmts, yapi->var_decl(V, zero_name));
        stmts = yapi->stmt_list_push(stmts, yapi->expr_stmt(
            yapi->call2(yapi->var_value(c"yhm_zero"),
                yapi->cast(yapi->addr_of(zero_var), none_ptr_t),
                yapi->sizeof(V))));

        yExpr is_miss = yapi->bin_op(found_var, 128, yapi->cast(yapi->int(0), none_ptr_t));
        yStmt miss_ret = yapi->return_stmt(zero_var);
        yStmt hit_ret = yapi->return_stmt(
            yapi->member(yapi->deref(yapi->cast(found_var, entry_ptr_t)), c"value"));
        stmts = yapi->stmt_list_push(stmts, yapi->if_else_stmt(is_miss, miss_ret, hit_ret));

        get_t:set_body(yapi->block(stmts));
    }
    get_t:finish(c"get");

    // try_get(key, out): single-lookup found/not-found check that also
    // writes the value through `out` on a hit -- unlike has(key) followed
    // by get(key), which would probe the map twice. Leaves *out untouched
    // on a miss.
    _ tryget_t = res:new_method();
    _ tgkey_p = tryget_t:add_param(K, c"key");
    _ tgout_p = tryget_t:add_param(yapi->ptr_of(V), c"out");
    yExpr self4b = tryget_t:get_subject();
    tryget_t:set_return_type(yapi->type(c"bool"));
    {
        _ probe_name2 = yapi->uniq_name();
        yExpr probe_var2 = yapi->new_var(entry_t, probe_name2);
        _ found_name2 = yapi->uniq_name();
        yExpr found_var2 = yapi->new_var(none_ptr_t, found_name2);

        _ stmts2 = yapi->stmt_list_new();
        stmts2 = yapi->stmt_list_push(stmts2, yapi->var_decl(entry_t, probe_name2));
        stmts2 = yapi->stmt_list_push(stmts2, yapi->expr_stmt(
            yapi->assign(yapi->member(probe_var2, c"key"), 61, tgkey_p)));
        stmts2 = yapi->stmt_list_push(stmts2, yapi->var_decl(none_ptr_t, found_name2));
        stmts2 = yapi->stmt_list_push(stmts2, yapi->expr_stmt(
            yapi->assign(found_var2, 61,
                yapi->call2(yapi->var_value(c"yhm_get"),
                    yapi->member(self4b, c"map"),
                    yapi->cast(yapi->addr_of(probe_var2), none_ptr_t)))));

        yExpr is_miss2 = yapi->bin_op(found_var2, 128, yapi->cast(yapi->int(0), none_ptr_t));
        yStmt miss_ret2 = yapi->return_stmt(yapi->bool(false));

        _ hit_stmts = yapi->stmt_list_new();
        hit_stmts = yapi->stmt_list_push(hit_stmts, yapi->expr_stmt(
            yapi->assign(yapi->deref(tgout_p), 61,
                yapi->member(yapi->deref(yapi->cast(found_var2, entry_ptr_t)), c"value"))));
        hit_stmts = yapi->stmt_list_push(hit_stmts, yapi->return_stmt(yapi->bool(true)));
        yStmt hit_ret2 = yapi->block(hit_stmts);

        stmts2 = yapi->stmt_list_push(stmts2, yapi->if_else_stmt(is_miss2, miss_ret2, hit_ret2));

        tryget_t:set_body(yapi->block(stmts2));
    }
    tryget_t:finish(c"try_get");

    // delete(key): true iff an entry was found and removed.
    _ del_t = res:new_method();
    _ dkey_p = del_t:add_param(K, c"key");
    yExpr self5 = del_t:get_subject();
    del_t:set_return_type(yapi->type(c"bool"));
    del_t:set_body(
        stmt${
            $EntryT $e;
            $e.key = $k;
            ret $del($self.map, $e@.($NoneP)) != 0.($NoneP);
        }
        :fill_type(c"EntryT", entry_t)
        :fill_var(c"e", entry_t, yapi->uniq_name())
        :fill_expr(c"k", dkey_p)
        :fill_expr(c"self", self5)
        :fill_expr(c"del", yapi->var_value(c"yhm_delete"))
        :fill_type(c"NoneP", none_ptr_t)
        :finish()
    );
    del_t:finish(c"delete");

    // len(): live entry count.
    _ len_t = res:new_method();
    yExpr self6 = len_t:get_subject();
    len_t:set_return_type(yapi->type(c"i64"));
    len_t:set_body(
        stmt${ ret $count($self.map); }
        :fill_expr(c"self", self6)
        :fill_expr(c"count", yapi->var_value(c"yhm_count"))
        :finish()
    );
    len_t:finish(c"len");

    // free(): releases the underlying map.
    _ free_t = res:new_ref_method();
    yExpr self7 = yapi->deref(free_t:get_subject());
    free_t:set_body(
        stmt${ $free($self.map); }
        :fill_expr(c"self", self7)
        :fill_expr(c"free", yapi->var_value(c"yhm_free"))
        :finish()
    );
    free_t:finish(c"free");

    // foreach(cb): calls cb(key, value) once per entry, in map order (no
    // particular guarantee beyond "every live entry exactly once").
    _ foreach_t = res:new_method();
    _ cb_p = foreach_t:add_param(yapi->fn_type2(none_t, K, V), c"cb");
    yExpr self8 = foreach_t:get_subject();
    foreach_t:set_body(
        stmt${
            i64 $cursor;
            $cursor = 0;
            $NoneP $item;
            while ($iter($self.map, $cursor@.($I64P), $item@.($NonePP))) {
                $cb($item.($EntryPtr):[0].key, $item.($EntryPtr):[0].value);
            }
        }
        :fill_var(c"cursor", yapi->type(c"i64"), yapi->uniq_name())
        :fill_type(c"NoneP", none_ptr_t)
        :fill_var(c"item", none_ptr_t, yapi->uniq_name())
        :fill_expr(c"self", self8)
        :fill_expr(c"iter", yapi->var_value(c"yhm_iter"))
        :fill_type(c"I64P", yapi->ptr_of(yapi->type(c"i64")))
        :fill_type(c"NonePP", yapi->ptr_of(none_ptr_t))
        :fill_type(c"EntryPtr", entry_ptr_t)
        :fill_expr(c"cb", cb_p)
        :finish()
    );
    foreach_t:finish(c"foreach");

    // Claim "for" (defined below) as THIS instantiation's receiver-dispatched
    // macro method -- same mechanism arr(T) uses, see its own comment on why
    // a same-named real method could never collide with this.
    yapi->register_macro_method(res, c"for", c"for");

    ret res;
}

// new(K, V): value-yielding convenience constructor, e.g.
// `_ h = hashmap->new:(i32, i32);` instead of a separate declare + init().
yExpr fn new(yType K, yType V) {
    _ hm_t = hashmap->hashmap(K, V);
    _ entry_ptr_t = yapi->field_type(hm_t, c"_phantom");
    _ entry_t = yapi->pointee_type(entry_ptr_t);
    _ size_id = hashmap->key_size_id(K);

    _ name = yapi->uniq_name();
    _ out = yapi->new_var(hm_t, name);

    _ stmts = yapi->stmt_list_new();
    stmts = yapi->stmt_list_push(stmts, yapi->var_decl(hm_t, name));
    stmts = yapi->stmt_list_push(stmts, yapi->expr_stmt(
        yapi->assign(yapi->member(out, c"map"), 61,
            yapi->call3(yapi->var_value(c"yhm_make"), yapi->sizeof(entry_t),
                yapi->int(size_id), yapi->int(size_id)))));
    stmts = yapi->stmt_list_push(stmts, yapi->expr_stmt(
        yapi->assign(yapi->member(out, c"_phantom"), 61, yapi->cast(yapi->int(0), entry_ptr_t))));
    stmts = yapi->stmt_list_push(stmts, yapi->expr_stmt(out));

    ret yapi->block_expr(stmts);
}

// new_dict(V): value-yielding constructor for a byte@-keyed (C string)
// hashmap -- i.e. a "dictionary" in the Python/JS sense, string keys to any
// V. `hashmap->new:(byte@, V)` works directly too (a macro call's
// type-argument slot used to only resolve a bare identifier like `i32`,
// never a compound type expression such as `byte@` -- fixed in
// yap_resolve_macro_type_arg, components/yap-semantic/src/build.c); this
// is purely ergonomic sugar for the common string-keyed case, sparing the
// caller from spelling `byte@` out at every call site:
// `hashmap->new_dict:(test_struct)`.
yExpr fn new_dict(yType V) {
    ret hashmap->new(yapi->ptr_of(yapi->type(c"byte")), V);
}

// for(self, k, v, body): receiver-dispatched macro-method form of
// iteration, e.g. `h:for:(+k, +v, { io->print:(c"%d\n", [v]); });` -- +k/+v
// are hygienic idents the macro itself declares (K-typed key, V-typed
// value) and the block body can reference them directly, and (unlike the
// typed-callback foreach(cb) above) can also close over an outer local,
// same as arr(T)'s own 'for'.
yStmt fn for(yExpr self, yIdent k, yIdent v, yStmt body) {
    _ entry_ptr_t = yapi->field_type(yapi->type_of(self), c"_phantom");
    _ entry_t = yapi->pointee_type(entry_ptr_t);
    _ k_t = yapi->field_type(entry_t, c"key");
    _ v_t = yapi->field_type(entry_t, c"value");
    _ none_ptr_t = yapi->ptr_of(yapi->type(c"none"));
    _ i64_t = yapi->type(c"i64");
    ret stmt${
        i64 $cursor;
        $cursor = 0;
        $NoneP $item;
        while ($iter($self.map, $cursor@.($I64P), $item@.($NonePP))) {
            $KT $k;
            $k = $item.($EntryPtr):[0].key;
            $VT $v;
            $v = $item.($EntryPtr):[0].value;
            $body;
        }
    }
    :fill_var(c"cursor", i64_t, yapi->uniq_name())
    :fill_type(c"NoneP", none_ptr_t)
    :fill_var(c"item", none_ptr_t, yapi->uniq_name())
    :fill_expr(c"self", self)
    :fill_expr(c"iter", yapi->var_value(c"yhm_iter"))
    :fill_type(c"I64P", yapi->ptr_of(i64_t))
    :fill_type(c"NonePP", yapi->ptr_of(none_ptr_t))
    :fill_type(c"KT", k_t)
    :fill_var(c"k", k_t, k)
    :fill_type(c"EntryPtr", entry_ptr_t)
    :fill_type(c"VT", v_t)
    :fill_var(c"v", v_t, v)
    :fill_stmt(c"body", body)
    :finish();
}
