C Header Import for a C-Transpiling Language (Simple Spec)

Goal

Import C declarations from headers into your language so that:

functions are callable

types are usable

ABI compatibility is preserved

no manual duplication of C structs/typedefs is needed

You will use libclang as the frontend parser.

1. Input

You start from:

a header file (or set of headers)

include flags (-I, -D, sysroot, etc.)

You parse them using:

clang_parseTranslationUnit

This produces a fully preprocessed translation unit AST.

2. What you extract (roots)

From the AST, collect only exported symbols:

Keep:

FunctionDecl (non-static, external linkage)

StructDecl / UnionDecl / EnumDecl (named types)

TypedefDecl

optionally global variables

Ignore:

static functions

macros (initially)

builtins (_builtin*)

anonymous/internal compiler symbols

3. Core data structure

Maintain:

Type graph state

seen_types : set of canonical types worklist : queue of types to process 

4. Initial seeding

For every exported function:

Add to worklist:

return type

each parameter type

5. Type processing (core rule)

While worklist not empty:

Step A — normalize

get type from libclang

compute canonical type: 

clang_getCanonicalType

Step B — deduplicate

If canonical type already in seen_types → skip

Otherwise add it.

6. Expand type dependencies

For each new type:

Case 1: Primitive

int, float, char, void → STOP

Case 2: Pointer

T* → push T

(pointer shape is preserved, but dependency is T)

Case 3: Array

T[n] → push T

Case 4: Typedef

DO NOT recursively expand alias chains

just canonicalize and continue

Case 5: Struct / Union

If definition is available:

push all field types

recursively process them

If incomplete (forward declaration):

mark as opaque type

STOP expansion

Case 6: Enum

include fully

STOP expansion

Case 7: Function pointer

push return type

push parameter types

7. Typedef rule (important)

NEVER manually resolve typedef chains.

Instead:

Always rely on clang_getCanonicalType()

Typedef cycles are handled safely by Clang.

8. Cycle safety rules

You are safe if:

you only track canonical types in seen_types

you never expand typedef chains manually

you only DFS actual type dependencies (fields, params, returns)

This prevents:

typedef cycles

struct recursion loops

duplicate imports

9. Output model

You build a language-level type table:

struct → with known fields or opaque

enum → full definition

typedef → alias or resolved type

function → signature with resolved types

10. Key invariant

After import:

Every exported function’s full type signature must be representable using only imported types.

That is your correctness condition.

11. What Clang does vs what you do

Clang provides:

parsed AST

resolved types

canonical types

semantic structure

You provide:

filtering (what is public API)

dependency closure

deciding what becomes part of your language’s type system

12. One-sentence algorithm

Extract exported functions → collect their types → recursively close over all reachable canonical types via fields/params/returns until fixed point.

If you want next step, I can .

---

13. The "int problem" — preserving original C spelling

Your language uses fixed-width names like i32, f64, bool.
Their C emission names live in yap_prim_type.c_name:

  i32  → "int32_t"   (requires <stdint.h>)
  f32  → "float"
  bool → "bool"      (requires <stdbool.h>)

But imported C headers use native C names:

  int   (not int32_t)
  float (same)
  long  (not int64_t)
  char* (not byte@)

Emitting int32_t for an imported function like printf would produce
broken code — stdio.h did not declare printf with int32_t.

Two approaches exist.  We choose approach A (minimal, non-invasive).

A — extern function keep their original C spelling

Add a new decl kind that wraps a normal yap function signature
plus the original C-name strings for return and parameter types:

  yap_decl_extern_func {
    yap_func_decl yap_sig;          // yap-level signature (i32 fn byte@, ...)
    char *c_return_spelling;         // "int"
    darr(char *) c_param_spellings;  // ["const char*", "..."]
  }

Codegen rule: when kind == yap_decl_extern_func, emit
c_return_spelling and c_param_spellings verbatim instead of
looking up yap_prim_type.c_name.

The yap type system itself remains unchanged — i32 still means int32_t
internally and when emitting yap-native code.  Only the extern
declaration carries the original C spelling for ABI-correct output.

B — (future) named types get an extern_c_name field

If we later need to import structs, enums, and typedefs with their
original C spelling (e.g. size_t, FILE, struct timeval), add:

  yap_named_type.extern_c_name  →  "size_t" (imported), NULL (yap-native)

For now, approach A is sufficient and adds zero complexity to the
existing type system.