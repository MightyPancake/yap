# Module Versioning and Dependencies

Motivated by the case where two modules disagree about a third: a program
imports `Z` at 2.0 directly, and also imports `Y`, which was written against
`Z` 1.0. That question could not even be asked at the outset -- module lookup
built `<lookup_path>/<name>/mod.yp`, one path per name, so both imports
silently resolved to whatever single copy was on disk.

Running both versions at once was the goal, and it works ; section 10 describes
how. Section 11 covers `bind`, which separates a type C owns from a type a
module owns -- the distinction the whole thing turns on once two versions are
loaded at once.

## 0. Where it stands today

`version:` is parsed off the module declaration into
`yap_module_decl_node.version`, then logged and discarded
(`src/lib/imports.c:69-71`). `yap_module` has no version field, and
`yap_ctx_create_new_module(ctx, name, prefix)` (`src/lib/ctx.c:290`) has
nowhere to put one.

Three layers key on the bare name and would break under two versions:

- `ctx->modules` hashes on `->name` (`declare_map_for(module)`,
  `src/lib/ctx.c:4`) ; a second registration errors with
  "Module 'X' already exists".
- `src->from_module_import` is a bare name string, and module scope lookup
  goes through it.
- The C symbol prefix comes from the declared `prefix:` ; two versions both
  emitting `z_init` collide at link, or worse, collapse silently under
  `c_name` deduplication.

One layer is already correct and needs no change: the parser's source-node
cache is keyed by absolute path (`components/yap-ts/src/parser.c:84`), so two
versions living at two paths are two distinct nodes and are both built. The
build walk's dedup is per-node and therefore version-agnostic.

## 1. Version as a value, not a string

`yap_version { u16 major, minor, patch }`, parsed at phase 0 and compared
numerically.

The subset is exactly `major.minor.patch`, integers only. No prerelease tags,
no build metadata ; anything else is a parse error naming what was found.
Full semver drags in prerelease ordering rules that are fiddly and that
nothing here needs. A deliberately restricted parser is ~30 lines and never
surprises anyone ; a half-implemented one surprises everyone.

String comparison must not appear anywhere on the resolution path:
`"0.10.0" < "0.9.0"` under `strcmp`.

Partial versions complete with zeros: `1.4` is `1.4.0`, in a declaration and
in a constraint alike. A bare number never silently means a range — if any
patch will do, `^1.4.0` says so. Bare is exact, ranges carry a sigil.

## 2. Module identity is (name, version)

- `yap_module` gains `version`, plus a precomputed `key` (`"io@0.0.1"`).
- `ctx->modules` stays keyed by `name` while the single-version policy holds,
  because that policy is exactly what makes the name unique. User code writes
  bare names — `io->puts` reaches `yap_ctx_get_module` with `"io"`
  (`components/yap-semantic/src/build.c:2768`) — so a name-to-module path is
  load-bearing and cannot be replaced by a key lookup. Rekeying the map to
  `key` belongs with coexistence, alongside the name resolution step that
  would then have to choose between candidates.
- `src->from_module_import` likewise stays a name until then.
- The prefix stays `<name>_` while the single-version policy holds. It only
  needs to incorporate the version under coexistence (section 10).

Registration is where the policy is enforced: a second module of the same
name reports the two versions by number rather than a bare "already exists".

Every module under `modules/` already declares both `version` and `prefix`
explicitly, so nothing about emitted symbol names changes.

## 3. The manifest stays in the module block

```
module io {
    version: "0.0.1",
    prefix: "io_",
    deps: [
        "stdlib@^0.0.1",
        "math@latest",
    ],
}
```

Two rules, one of which is a fix: **unknown key is an error, missing key takes
a default.** Today it is the other way round for the first half ;
`components/yap-ts/src/parse.c:317` silently skips any key that is not
`prefix` or `version`, so a typo'd `verison:` yields no version and no
diagnostic.

`deps:` and `import X` intentionally both name X. The manifest is
authoritative for *which version*, the import for *what is in scope*.
Importing a module absent from `deps` is an error.

That rule is scoped by the presence of the `module { }` block, not by the
presence of `deps:`. A file that declares no module has no manifest and gets
no enforcement, which is what keeps tests, scripts and one-off programs
working untouched. Declaring a module opts into its deps being authoritative.
Only one test declares one today (`tests/pass/statements.yp`, `module hello {}`)
and it imports nothing, so the rule costs no churn outside `modules/`.

### Why not a metadata function

A `fn module()` returning a struct was considered and rejected. It is circular
on `prefix`, which is needed to mangle the module's own declarations before
any of them can be built and run. It makes deps unreadable until the module
successfully compiles, so a type error anywhere in a module renders its
*version* unknowable. And it yields no static dependency graph, since
executing code to discover edges produces them one at a time. This is the same
ground Python covered moving from `setup.py` to `pyproject.toml`.

### Why not a const struct, or a separate package.json

A `const ModuleInfo module = [...]` blob literal reuses the language's own
literal syntax, but reading it requires yap's full expression grammar, which
makes third-party tooling harder rather than easier. The module block is a
**data** language ; blob literals are an **expression** language, and phase 0
runs before the type system exists.

A separate `package.json` splits a yap project across two languages and puts a
JSON parser in the compiler's critical path. The lockfile is a different
matter — see section 9.

## 4. Dep specs: a string is sugar for a struct

The value slot accepts either form, from the start:

```
"stdlib@^0.0.1"
{ name: "stdlib", version: "^0.0.1" }
```

The string is defined as exactly equivalent to the object with `name` and
`version` filled in. Fixing this now means later additions are purely
additive, with no manifest migration.

`module_info` is already `comma_sep(key: value)`
(`components/yap-ts/grammar/grammar.js:118-128`), so the grammar change is to
make the *value* slot recursive rather than to add a new construct:

```js
value: choice($.string_literal, $.num_literal, $.bool_literal,
              $.module_info_list, $.module_info_object)
module_info_object: $ => seq('{', optional($.module_info), '}')
module_info_list:   $ => seq('[', optional(comma_sep($._module_info_value)), ']')
```

That yields a JSON-equivalent data language: literals, lists and objects, with
no calls, arithmetic or identifiers-as-values. The restriction is the point.

Version specs, in the string form after `@`:

| Spec      | Resolves to                          | Reproducible        |
|-----------|--------------------------------------|---------------------|
| `0.0.1`   | itself                               | yes                 |
| `^0.1.0`  | solved against what is installed     | yes, given a lock   |
| `latest`  | newest installed, at build time      | no                  |
| `local`   | the local tree's declared version    | no, machine-varying |

A bare `import Z` with no `deps` entry is sugar for `Z@latest`.

Note that `^` follows semver, where the caret pins the minor below 1.0:
`^0.1.0` means `>=0.1.0 <0.2.0`, and `^0.0.1` means exactly `0.0.1`. Every
module in `modules/` is currently `0.0.1`, so caret is a no-op across the
whole tree and will look unimplemented. Either accept that and document it, or
define a different rule — but do not quietly deviate while using semver's
syntax.

## 5. Sources answer "where", versions answer "which"

```
{ name: "foo", version: "^1.2.0",
  git: "https://github.com/x/foo", tag: "v1.2.3" }
```

Source fields (`git:`, `path:`, `registry:`) are orthogonal to `version:`.
A dep from any source still resolves to a concrete version by reading that
module's own declared `version:` after fetching, and that version must satisfy
`version:` if one was given. This is the same rule that makes `local`
participate in conflict resolution instead of being a hole in it — `local` is
itself just `path:` pointing into the lookup directory.

Two different sources for the same name are a conflict even when the versions
agree. Under the single-version policy there is no way to have `foo` from
GitHub and `foo` from GitLab in one graph.

Stage this by freezing the format first and implementing the fetcher later: a
git source should parse cleanly and fail resolution with "source not yet
supported". The format is expensive to change afterwards ; the fetcher is not.

## 6. Resolution becomes two passes

Imports are currently followed during parsing, the moment each is seen
(`components/yap-ts/src/parse.c:310`). That cannot work with constraints,
because a version for `Z` would be chosen before discovering that `Y` also
constrains `Z`.

1. **Manifest scan.** For each dep, enumerate `<lookup_path>/<name>/` for
   available versions, read each candidate's `module { }` block, and follow its
   own `deps:` — building the whole graph without compiling anything.
2. **Solve**, then parse the chosen files normally.

Pass 1 needs a manifest-only parse mode that reads a file and ignores its
import declarations. This is cheap because deps now come from `deps:` rather
than from following imports, and it is the entire reason the manifest must be
readable without executing or type-checking anything.

## 7. Conflict policy

Collect every constraint on a name, intersect them, take the newest version
satisfying all of them. "Latest" is a tiebreaker among *satisfying* versions,
never a licence to ignore a constraint.

The motivating case — a program pinning `Z@2.0` against a `Y` needing
`Z@^1.0` — has an empty intersection and is an error that names both
constraints and who imposed them. It is not a silent upgrade of `Y`. This is
the single most valuable behaviour in the design, and it lands long before
coexistence does.

## 8. `__import` selects among declared deps

The dynamic import work (`import collections:(["arr","map"])`, dispatched
through a module's `__import` macro during the semantic build) discovers
imports while building, which is in tension with a graph solved before
parsing.

The rule that keeps both: **`__import` may only import modules already listed
in `deps:`.** It selects among declared dependencies and never introduces new
ones. `collections` declares `arr` and `map` as its own deps, and the macro
chooses from them ; the graph stays static and solvable, and the feature keeps
its power.

Settle this before building the `import foo:(args)` grammar. Retrofitting the
restriction later breaks macros that have already been written against it.

## 9. Lockfile

A generated data file, holding exact versions only, plus source and revision
for non-registry deps. Locking is precisely the act of collapsing `latest`,
`^` and friends into fixed versions.

It is machine-written and never hand-edited, so it does not belong in yap
source: nobody wants a generated `.yp` file with two hundred entries. `local`
is not lockable, and should be refused or recorded as an override rather than
silently frozen.

The format needs two slots per entry, not one: what was *asked for* and what
it *resolved to*.

```
{ name: "experiment", git: "...", branch: "main",
  resolved: { rev: "a1b2c3d", version: "0.3.1" } }
```

The manifest says "track main" ; the lock records that main was `a1b2c3d` at
lock time, and that the module there declared `0.3.1`. Builds read the
resolved rev and stay reproducible although the source moves. Re-resolving is
then an explicit update command that rewrites the lock, never a side effect of
building.

Every non-exact spec has this shape — `^0.1.0` resolves to `0.1.7`, `latest`
resolves to something concrete. `branch:` does not create the requirement, it
only makes it impossible to overlook, which is why it stays in the schema:
dropping it would save no format work.

Not needed until modules are actually fetched from somewhere.

## 10. Coexistence

Two versions of one module in one binary is what this is all for, and it works.

Types live in a single global map keyed by the bare name
(`yap_ctx_push_named_type`, `src/lib/ctx.c`), and modules have their own
`yap_scope` for symbols but no equivalent for types. Symbols, types and
mangling therefore all needed the same question answered: given this source,
which module does a bare name refer to? That answer is now per-importer --
`z->foo` written inside `Y` means Y's `Z`, not the program's.

Four parts, all in place:

1. Each module carries a `name -> resolved module` map for its own deps, built
   from the import records the parse phase stamps with a resolved key.
2. Module access and type lookup route through the importing source's module
   rather than the global table, via `yap_source_owning_module`. A module's own
   types are recorded in `yap_module.own_types` and consulted first.
3. A module's types are emitted under its prefix, so two versions of a module
   may each declare `struct Thing` without colliding.
4. A name loaded at more than one version folds the version into its prefix
   (`sp_` becomes `sp_0_1_0_` and `sp_0_2_0_`). A name with one version keeps
   exactly what it declared, which is every module in the tree today.

Under it all sits one rule: **an emitted C name is a function of the thing's
identity**, never of build order. Functions are identified by module, version
and name ; a module's own types the same way ; a bound type by its C name and
layout. Three failure modes came from breaking that rule in three different
places, and they are covered by `module_coexistence`,
`module_type_coexistence` and `version_prefix_mangling`.

## 11. `bind` marks a C type

A type declared in C is not owned by the module that describes it. `stdlib` and
`time` both describe `struct timespec`, and they must land on one type or a
`timespec` could not cross between them. A module's own type is the opposite:
two versions of it must stay distinct.

The two cannot be told apart syntactically, so bound types are marked:

```
bind type _IO_marker
bind struct timespec {
    i64 tv_sec,
    i64 tv_nsec,
}
```

bindgen emits the marker, and `modules/*/binds.yp` is generated, so nobody
writes it by hand. The rule it selects:

- **Identity is the C name plus the layout.** Same name and layout is one type,
  shared and emitted once. Same name, different layout -- glibc's `FILE` beside
  musl's -- are distinct types, and mixing them is a type error rather than
  silent corruption.
- **The C name always carries the layout hash** (`timespec__b477af42`), never
  conditionally. Deciding it per-conflict made the emitted name depend on which
  module was built first, so the same layout could be emitted under different
  names depending on import order.
- **The yap-level name stays as written**, which is what diagnostics and
  in-module lookups use. This is the same `name`/`c_name` split that function
  prefixing already relies on.

The one thing this does not constrain is linkage. yap re-declares bound types
in its own `types.h` rather than including the C header, and C does not encode
type names in symbols, so a bound type's C name never reaches the linker --
only its layout has to match. Sharing is the reason for the rule, not naming.

Marked types skip module prefixing ; unmarked ones get it (section 10, part 3).

## Build order

1. Parse the version into `yap_version` and store it on `yap_module`, enforcing
   single-version at registration with both versions named on a clash. *(done,
   since superseded by coexistence)*
2. `deps:` in the grammar, with the recursive value slot and the unknown-key
   error. *(done)*
3. Normalize the in-tree manifests: the nine `0.0.1` modules bumped to `0.1.0`
   so `^` is meaningful, and a `deps:` list on the modules that need one.
   `raylib` was later moved out of the tree entirely, to its own repository at
   github.com/MightyPancake/raylib-yap, and is consumed as a git dependency.
   *(done)*
4. Version directories `<name>/<version>/mod.yp`, falling back to
   `<name>/mod.yp`. *(done)*
5. Manifest scan and solver, producing each module's per-importer dep map.
   *(done)*
6. Per-importer resolution for symbols and types. *(done)*
7. Module-prefixed type names. *(done)*
8. Conditional version mangling. *(done)*
9. Lockfile, once modules are fetched from somewhere. *(open)*

`git:` and `path:` sources are fetched ; `path:` is linked rather than copied,
so a module edited alongside its consumer -- a repo's own examples, say -- is
picked up without reinstalling. `registry:` still parses and is refused, since
there is no registry to fetch from. Smaller debts: the importer's
manifest is re-read on every module import while walking the parent chain ; an
import cycle carrying a real back-reference still fails, needing a pass 1
hoisted across all sources ; hard links to one file still read as two.

## Appendix: a manifest using every idiom

```
module webthing {
    // --- identity: what the compiler needs before it can build anything ---
    version: "1.4.2",
    prefix:  "wt_",

    // --- description: the compiler never reads these ---
    author:      "MightyPancake",
    webpage:     "https://nullptr.free/webthing",
    license:     "MIT",
    description: "Does web things",

    deps: [
        // 1. bare name -- unconstrained, equivalent to @latest
        "hashmap",

        // 2. exact pin
        "stdlib@0.0.1",

        // 3. caret range: >=0.1.0 <0.2.0
        "arr@^0.1.0",

        // 4. explicit latest
        "io@latest",

        // 5. local -- whatever is in the lookup dir, version read from it
        "laydoh@local",

        // 6. object form; identical to "math@^0.2.0"
        { name: "math", version: "^0.2.0" },

        // 7. git source + tag, with a constraint the fetched module must satisfy
        { name:    "clay",
          version: "^2.0.0",
          git:     "https://github.com/nicbarker/clay",
          tag:     "v2.0.1" },

        // 8. git + exact commit, no constraint -- version comes from the module
        { name: "raylib",
          git:  "https://gitlab.com/mirror/raylib",
          rev:  "9f4c2e1" },

        // 9. git + branch -- resolves, but moves; see section 9 on locking it
        { name:   "experiment",
          git:    "https://github.com/x/experiment",
          branch: "main" },

        // 10. path source -- the long form of @local
        { name: "sibling", path: "../sibling" },

        // 11. explicit registry
        { name: "json", version: "^1.0.0", registry: "https://yap.pkg/x" },
    ],
}
```

Trailing commas are already permitted everywhere by `comma_sep`
(`components/yap-ts/grammar/grammar.js:17-30`), so the layout above needs no
grammar accommodation.

A program declaring its own deps looks identical ; `module main { ... }` in
the root file, which phase 0 already handles as its first pass.

### Desugarings

| Short form        | Means                                              |
|-------------------|----------------------------------------------------|
| `"hashmap"`       | `{ name: "hashmap", version: "latest" }`           |
| `"stdlib@0.0.1"`  | `{ name: "stdlib", version: "0.0.1" }`             |
| `"laydoh@local"`  | `{ name: "laydoh", path: "<lookup_path>/laydoh" }` |

### What is not valid

The data language is open ; the schema is closed. Every key below parses and
is then rejected.

```
verison: "0.0.1"                         // unknown key -- not silently skipped
version: "1.0.0-rc.1"                    // prerelease tags are out of subset
"foo@>=1.0"                              // only exact / ^ / latest / local
{ version: "^1.0" }                      // name is required in the object form
{ name: "foo", git: "...", path: "..." } // two sources for one dep
{ name: "foo", prefix: "f_" }            // prefix belongs to the module itself
deps: ["arr@" + version]                 // no expressions, no identifiers
```

Note what is *not* on that list: `version: "1.4"` and `"foo@^0.1"` are both
valid and complete with zeros, per section 1.

Two of the entries are deliberate choices rather than oversights.

Two sources for one dep is an error rather than a precedence rule. "Git wins
over path" is exactly the sort of silent behaviour that costs an afternoon.

The last line is the entire point of the data-versus-expression split. If it
ever needs to work, phase 0 is executing code again, and every argument in
section 3 applies in reverse.
