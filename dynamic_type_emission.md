# Dynamic Type Emission Strategy

## Problem

Metaprograms (compile-time functions) can emit C type declarations (structs, enums, unions) that feed into libtcc. Two challenges:

1. **Uniqueness**: `vec2(i32)` and `vec2(f32)` must produce distinct C types, even though both are called `vec2` at the yap level.
2. **Dedup**: Calling `darr(i32)` twice should not re-emit the same C type or re-compile it in TCC.

## Solution: Output Hashing with Hash-Suffixed C Names

A single mechanism handles both problems. After the metaprogram produces its C string:

1. Hash the C string using `hashmap_murmur` (already available in `hashmap.h`).
2. Look up the hash in a global `emitted_hashes` set.
3. **Found** → the type already exists. Return the existing `yap_type_id`.
4. **Not found** → first time seeing this C definition.
   - Append the hash to the C type name (e.g., `vec2` → `vec2_A3F2B1C9`).
   - Feed the modified C string to TCC via `yap_c_feed_c`.
   - Register the type in the yap type system (get a `yap_type_id`).
   - Record hash in `emitted_hashes`.
   - Return the new `yap_type_id`.

The metaprogram runs on every call. This is correct because metaprograms can depend on mutable state (globals, I/O, random seeds) not captured in their argument list. Generating a C typedef string is cheap enough that caching the metaprogram body is unnecessary — the output hash alone prevents the expensive operation (duplicate TCC compilation).

### Why hash the output, not the inputs?

Metaprograms can branch on runtime values and read mutable state:

```
foo(i32 a) {
    if a == 1 { emit struct_A }
    else      { emit struct_B }
}
```

You cannot predict the output from the argument signature alone. Hashing the output captures what was actually generated and is always correct.

### Why the C name includes the hash?

The C-level name must be unique per distinct struct body. Since `vec2(i32)` and `vec2(f32)` both want the yap-level name `vec2`, the C name gets a hash suffix to disambiguate. The yap user never sees the C name.

## Example Flow

```
Source:  darr(i32) a = new_darr(i32);

// First darr(i32) — type annotation
run darr(i32) → "typedef struct darr_i32 { i32* data; i32 len; } darr_i32;"
hash("typedef struct darr_i32 ...") → 0xABC123
emitted_hashes.lookup(0xABC123) → miss
  rename to "darr_i32_ABC123"
  feed to TCC
  register in yap type system → type_id = 42
  emitted_hashes.insert(0xABC123)
return type_id=42

// Second darr(i32) — constructor call
run darr(i32) → "typedef struct darr_i32 { i32* data; i32 len; } darr_i32;"
hash("typedef struct darr_i32 ...") → 0xABC123
emitted_hashes.lookup(0xABC123) → found!
return existing type_id=42  // TCC not invoked again
```

## Hashing Function

Use `hashmap_murmur` from the existing hashmap library:

```c
uint64_t hash = hashmap_murmur(c_string, strlen(c_string), 0, 0);
```

## Data Structures (to add to yap_ctx or build_state)

```c
// Output hash set: has this C definition been emitted?
map emitted_hashes;  // uint64_t → dummy (just a set)
```

## Notes

- All named types must be emitted with `typedef` so the C name (with hash suffix) is bound to both the struct tag and the typedef alias. This matches existing codegen behavior.
- Module prefixes add another layer of namespace isolation at the C level.
- The hash suffix only needs to be a few hex digits (e.g., 8) — collisions are astronomically unlikely with MurmurHash3 over structured C code.
