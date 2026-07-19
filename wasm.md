# WASM / Emscripten Backend Support

Motivated by wanting `laydoh` (yap bindings for clay.h, in the sibling `laydoh`
repo) to run as a live, interactive browser demo the way clay.h's own web
demos and this environment's `nullptr` project do ; compile to WASM, drive
layout every frame from JS ; instead of the current native-binary +
static-JSON-snapshot workaround (`laydoh/examples/web/`).

Worth doing because `-bcc=<compiler>` in yap-c has no allowlist
(`yap_c_resolve_compiler` in `components/yap-c/src/codegen.c`) and just shells
out to whatever binary is named there ; `-bcc=emcc` is architecturally almost
a no-op already. The real gaps are below, roughly in dependency order.

## 1. Static-only linking for non-native targets

`src/lib/imports.c` (~line 87-107) collects every `.a` and `.so` it finds
under a module's directory and pushes all of them into the final link line
unconditionally. A `.so` is always native ELF and can never link into a wasm
build. When the selected compiler targets something non-native, only `.a`
files should be considered ; `.so` should be skipped outright.

## 2. A wasm-targeted build of module libraries

Module wrapper libraries (e.g. `modules/laydoh/libclay.a`) are built
exclusively with a hardcoded `gcc`, both in the Makefile's `native_modules`
target and in `bindgen.c`'s auto-wrapper compile step (`gcc -fPIC
-fvisibility=hidden -c wrapper.c ...`). Object code built that way is x86-64
native and cannot link into an emcc/wasm32 output. Need a second build path ;
an emcc-compiled variant of each module's library ; selected whenever the
final target is wasm. This is the item that would silently produce broken
output today rather than just being an inconvenience ; should be first thing
actually implemented once the design lands.

## 3. `-bcc=emcc` plumbing (mostly already works)

Confirm `-fno-semantic-interposition` doesn't choke emcc (should be harmless,
it's clang-based). Confirm `-o app.js` / `-o app.wasm` pass straight through
as the output name (they already do ; yap's `-o` is just forwarded verbatim).
No real code change expected here, just verification.

## 4. `export fn` modifier for JS-callable functions

Decided against doc comments (giving comments compiler semantics fights yap's
own "no magic annotations, real macros/attributes instead" stance) and
against flags-only (a hand-maintained `-s EXPORTED_FUNCTIONS=[...]` string
silently drifts from the real function names with no compiler diagnostic).
Instead: a real `export` modifier on the function declaration itself, e.g.

```yap
export fn app_init(i32 width, i32 height) { ... }
```

yap's own codegen (which already assembles the final compiler invocation, see
`codegen.c`) should walk declarations, collect every `export`-marked one, and
synthesize `-s EXPORTED_FUNCTIONS=[...]` itself ; removing the manual
duplication and giving a place to catch mistakes at compile time instead of
at runtime in the browser.

Needs: grammar support for the modifier, semantic-pass bookkeeping of which
functions are marked, and codegen wiring to turn that list into the right
emcc flag (and to just ignore the modifier harmlessly for non-wasm targets).

## 5. Optional `main()` for library-style wasm output

yap currently assumes exactly one `i32 fn main()` entrypoint per program. A
live demo (an `app_init`/`app_update`/`app_get_draw_commands`-style API, as in
`nullptr`'s `main.c`) shouldn't auto-run anything ; it should sit idle until
JS calls into specific exports. `main()` needs to become optional when
building this way, or emcc's default `INVOKE_RUN` behavior will try to
execute it as a one-shot CLI program on load.

## 6. Type-width correctness under wasm32

yap's primitive type table hardcodes C spellings tuned for x86-64 Linux
(`i64` -> `long`, assumed 8 bytes there). Emscripten's ABI may size some of
these differently. This is exactly the same class of bug just chased down in
`laydoh`'s Clay bindings (packed enums and `uint16_t` fields silently widened
to 4 bytes) ; needs verifying against emscripten's actual type widths before
trusting any struct passed across the wasm boundary, and likely needs
per-target primitive C-name overrides if there's a mismatch.

## 7. End-to-end validation

A trivial smoke test first: one `export`-marked function, called from a JS
harness, checked for a correct return value. Then a full round-trip: adapt
`laydoh/examples/demo.yp` into an `app_init`/`app_update`-shaped module and
confirm it behaves like `nullptr`'s live demo (real interactivity, not the
static JSON snapshot currently in `laydoh/examples/web/`).

---

Items 1-3 are small and mostly mechanical. 4-6 are the real design/
implementation work. 7 is where 1-6 actually get proven right or wrong.
