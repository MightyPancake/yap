// Raw C bindings for the vendored Robin Hood hashmap in wrapper.c (originally
// src/lib/hashmap.c, already used internally by the compiler itself -- see
// include/yap/hashmap.h). Hand-written rather than run through
// `--gen-c-bind`: bindgen silently skips wrapper generation for any function
// with a function-pointer parameter (src/lib/bindgen.c), which is exactly
// hashmap_new's shape, so the automatic pipeline can't produce this module.
// Types follow the same C-type-to-yap mapping bindgen itself uses (size_t /
// uint64_t -> i64, int -> i32, const dropped) for consistency with the
// generated modules.
//
// Bare names below ("make", "free", ...) resolve via mod.yap's "yhm_"
// module prefix to wrapper.c's yhm_make/yhm_free/... -- NOT hashmap_make/
// hashmap_free/etc, even though that would be the usual math_/stdlib_-style
// convention. The compiler's own binary already links in a separate copy of
// this same vendored file (for its own internal symbol tables), so anything
// literally named hashmap_* here would collide at link time in a compiled
// yap program that imports this module; wrapper.c marks every original
// tidwall function `static` and exports only the yhm_-prefixed forwarders
// declared here.
//
// `struct hashmap` stays fully opaque here -- callers never touch its
// fields, only the pointer. The generic, type-safe wrapper (hashmap(K, V):
// init/put/get/delete/len/free/for) lives in hashmap.yap, built on top of
// this raw surface.
type hashmap

// hash_id/cmp_id select between three hash/compare pairs baked into
// wrapper.c's yhm_make (see its own comment): 0 for a 4-byte scalar key
// (i32/u32/f32), 1 for an 8-byte one (i64/u64/f64), 2 for a null-terminated
// C string key (byte@, hashed/compared by contents).
hashmap@ fn make(i64 elsize, i32 hash_id, i32 cmp_id);
none fn free(hashmap@ map);
none fn clear(hashmap@ map, bool update_cap);
i64 fn count(hashmap@ map);
bool fn oom(hashmap@ map);

// get/set/delete take/return a pointer to a full { key, value } entry (see
// hashmap.yap's entry_t) -- set/delete return the replaced/removed entry
// (none@ / null if there wasn't one), matching hashmap.c's own contract.
none@ fn get(hashmap@ map, none@ item);
none@ fn set(hashmap@ map, none@ item);
none@ fn delete(hashmap@ map, none@ item);

// Cursor-based iteration: seed `i` at 0, call repeatedly; returns false once
// exhausted. `item` is written with a pointer to the current entry.
bool fn iter(hashmap@ map, i64@ i, none@@ item);

// Zero-fills n bytes at ptr -- used by hashmap.yap's get() to build a zero
// value of V that works whether V is a scalar or a struct.
none fn zero(none@ ptr, i64 n);
