import io

// type${ } eager blueprint -> yStructT template, :finish() as usual (Model A).
// $T splices the in-scope comptime yType param; a named type -> yapi->type(c"...").
yType fn pair(yType T) {
    _ st = type${
        struct {
            $T first,
            $T second
        }
    };
    ret st:finish(c"pair");
}

yType fn boxed() {
    _ st = type${
        struct {
            i32 value,
            i32 count
        }
    };
    ret st:finish(c"boxed");
}

// --- union: same add_field chain as struct, dispatched via union_t ---
yType fn number_box() {
    _ ut = type${
        union {
            i32 as_int,
            f32 as_float
        }
    };
    ret ut:finish(c"number_box");
}

// --- enum: add_variant chain via enum_t. Variant *names* aren't resolvable as
// values yet (that's a pre-existing gap in enum semantics generally, unrelated
// to blueprints -- nasty_types.yap only ever uses enums as anonymous field
// types, never constructs/reads a variant by name); a built enum type is
// otherwise a real, usable type via cast (its variants ARE real C enum
// constants, just not yet wired into yap's scope/type-checking).
yType fn signal() {
    _ et = type${
        enum {
            sig_none,
            sig_stop,
            sig_go
        }
    };
    ret et:finish(c"signal");
}

// --- field type extensions: pointer/slice fields (yapi->ptr_of/slice_of), and
// an anon nested struct field (recursively finish()ed under a compiler-
// generated name -- NOT the "__anon_*" convention normal anonymous nested
// types use, since that name means "inline me, don't emit a declaration" to
// codegen, which would break a field that references it by name).
yType fn ptr_box() {
    _ st = type${
        struct {
            i32@ p,
            i32 tag
        }
    };
    ret st:finish(c"ptr_box");
}

yType fn slice_box() {
    _ st = type${
        struct {
            (i32)[] items,
            i32 count
        }
    };
    ret st:finish(c"slice_box");
}

yType fn nested_box() {
    _ st = type${
        struct {
            struct {
                i32 x,
                i32 y
            } inner,
            i32 tag
        }
    };
    ret st:finish(c"nested_box");
}

i32 fn main() {
    pair:(i32) p;
    p.first = 10;
    p.second = 32;

    boxed:() b;
    b.value = 5;
    b.count = 7;

    number_box:() nb;
    nb.as_int = 7;

    // "signal" (the finished blueprint type) isn't nameable as an ordinary cast
    // target outside the macro that produced it, so go through a pointer cast
    // to i32 instead of casting an int TO signal.
    signal:() sg;
    i32@ sg_as_i32 = sg@.(i32@);
    sg_as_i32. = 2;

    i32 v = 9;
    ptr_box:() pb;
    pb.p = v@;
    pb.tag = 1;

    slice_box:() sb;
    sb.count = 3;

    nested_box:() nesb;
    nesb.inner.x = 4;
    nesb.inner.y = 5;

    i32 ok = 0;
    if (p.first + p.second == 42) { io->print:(c"type bp $T splice OK\n"); ok = ok + 1; }
    if (b.value + b.count == 12)  { io->print:(c"type bp named field OK\n"); ok = ok + 1; }
    if (nb.as_int == 7)           { io->print:(c"type bp union OK\n");       ok = ok + 1; }
    if (sg.(i32) == 2)            { io->print:(c"type bp enum OK\n");        ok = ok + 1; }
    if (pb.p. == 9)               { io->print:(c"type bp pointer field OK\n"); ok = ok + 1; }
    if (sb.count == 3)            { io->print:(c"type bp slice field OK\n");   ok = ok + 1; }
    if (nesb.inner.x + nesb.inner.y == 9) { io->print:(c"type bp nested anon field OK\n"); ok = ok + 1; }
    ret ok - 7;
}
