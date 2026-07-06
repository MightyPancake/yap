// Generic hashmap(K, V) module: type-safe wrapper (init/put/get/has/
// try_get/delete/len/free) around the vendored Robin Hood hashmap
// (modules/hashmap), plus its sugar: the value-yielding
// `hashmap->new:(K, V)` constructor, the receiver-dispatched `for` macro
// (hygienic +k/+v idents, body built inline in the caller's own scope --
// can close over an outer local, same as arr(T)'s own `for`), and the
// typed-callback `foreach(cb)` HOF (capture-free func_literal, same as
// arr(T)'s `foreach`). Key support: 4-/8-byte scalars, and byte@ treated as
// a C string (hashed/compared by contents).
import io
import hashmap

struct test_struct {
    i32 a,
    i32 b,
}

i32 fn main() {
    _ h = hashmap->new:(i32, i32);
    h:put(1, 100);
    h:put(2, 200);
    h:put(3, 300);

    if (h:len() == 3) io->print:(c"len OK\n");
    if (h:has(2)) io->print:(c"has OK\n");
    if (!h:has(99)) io->print:(c"has-miss OK\n");
    if (h:get(2) == 200) io->print:(c"get OK\n");
    if (h:get(99) == 0) io->print:(c"get-miss-zero OK\n");

    // try_get: single-lookup found+value, distinguishes a genuine stored 0
    // from a miss (get() alone can't -- both return 0).
    h:put(4, 0);
    i32 got = -1;
    if (h:try_get(4, got@) && got == 0) io->print:(c"try_get zero-value OK\n");
    got = -1;
    if (!h:try_get(999, got@) && got == -1) io->print:(c"try_get miss OK\n");
    h:delete(4);

    // put() on an existing key replaces the value rather than duplicating.
    h:put(2, 999);
    if (h:get(2) == 999 && h:len() == 3) io->print:(c"put-replace OK\n");

    if (!h:delete(999)) io->print:(c"delete-miss OK\n");
    if (h:delete(2)) io->print:(c"delete OK\n");
    if (!h:has(2)) io->print:(c"delete-effect OK\n");
    if (h:len() == 2) io->print:(c"len-after-delete OK\n");

    // for (macro): +k/+v are hygienic idents the macro itself declares; the
    // body is built inline in this scope, so it can accumulate into an
    // outer local (unlike foreach(cb) below).
    i32 sum = 0;
    h:for:(+k, +v, {
        sum = sum + v;
    });
    if (sum == 400) io->print:(c"for macro OK\n"); // 100 + 300 (2 was deleted)

    // A second instantiation with a different (8-byte) key/value type --
    // confirms hashmap(K, V) is genuinely generic, not just working for i32.
    _ h2 = hashmap->new:(i64, i64);
    h2:put(10, 1000);
    h2:put(20, 2000);
    h2:foreach((none fn i64 k, i64 v) { io->print:(c"foreach: %d\n", [v.(i32)]); });
    if (h2:len() == 2) io->print:(c"foreach HOF OK\n");
    h2:free();

    // String-keyed, struct-valued instantiation: byte@ key (C string,
    // hashed/compared by contents) and a real struct V -- exercises get()'s
    // memset-based zero value (a struct can't be produced by casting 0).
    // Both the direct spelling and the new_dict(V) sugar work now that a
    // compound type can be resolved as a macro type argument.
    _ h3direct = hashmap->new:(byte@, test_struct);
    h3direct:free();
    _ h3 = hashmap->new_dict:(test_struct);
    h3:put(c"alice", [1, 2].(test_struct));
    h3:put(c"bob", [3, 4].(test_struct));
    if (h3:len() == 2) io->print:(c"string-key len OK\n");
    if (h3:has(c"alice")) io->print:(c"string-key has OK\n");
    if (!h3:has(c"carol")) io->print:(c"string-key has-miss OK\n");

    _ av = h3:get(c"alice");
    if (av.a == 1 && av.b == 2) io->print:(c"string-key get OK\n");

    _ mv = h3:get(c"carol");
    if (mv.a == 0 && mv.b == 0) io->print:(c"string-key get-miss-zero-struct OK\n");

    test_struct out = [0, 0].(test_struct);
    if (h3:try_get(c"bob", out@) && out.a == 3 && out.b == 4) io->print:(c"string-key try_get OK\n");

    if (h3:delete(c"alice") && !h3:has(c"alice")) io->print:(c"string-key delete OK\n");
    h3:free();

    h:free();
    ret 0;
}
