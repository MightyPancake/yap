# Module Versioning and Dependencies

Motivated by the case where two modules disagree about a third: a program
imports `Z` at 2.0 directly, and also imports `Y`, which was written against
`Z` 1.0. Today that question cannot even be asked ; module lookup builds
`<lookup_path>/<name>/mod.yp` (`components/yap-ts/src/parse.c:310`), one path
per name, so both imports silently resolve to whatever single copy is on disk.

The design below makes module identity version-aware everywhere, while
deliberately keeping a single-version policy on top of it. See section 10 for
why coexistence is deferred rather than built.

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
- `ctx->modules` hashes and compares on `key` rather than `name`. `name` and
  `version` stay as separate fields for diagnostics.
- `src->from_module_import` carries the key. It is already an opaque string
  fed straight to `yap_ctx_get_module`, so this is mechanical.
- The prefix stays `<name>_` while the single-version policy holds. It only
  needs to incorporate the version under coexistence (section 10).

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

## 10. Coexistence is deferred, not designed away

Allowing two versions of one module in a single binary requires type
namespacing. The `c_name` plumbing exists on types but is intentionally not
applied, so `Z.Thing` from 1.0 and from 2.0 resolve to the same nominal type
and unify silently.

That halfway state is worse than no support: two `z_init` functions operating
on what the compiler believes is one type. So the policy stays **one version
per module name**, enforced, with a clear error.

Keying by `(name, version)` from the start is what makes this a later switch
rather than a rewrite. Turning coexistence on means relaxing the
single-version check, deriving the prefix from the version
(`z_1_0_` / `z_2_0_`), and enabling type prefixing — not re-plumbing the
registry, scopes, `from_module_import` and mangling.

## Build order

1. Parse the version into `yap_version`, store it on `yap_module`. No
   behavioural change.
2. Key the registry by `(name, version)`, single-version enforced. No
   behavioural change, since only one version exists today.
3. `deps:` in the grammar, with the recursive value slot and the unknown-key
   error.
4. Normalize the in-tree manifests: bump the nine `0.0.1` modules to `0.1.0`
   so that `^` is meaningful (section 4), and add a `deps:` list to each of the
   ten `modules/*/mod.yp`, which is what declaring a module now opts into.
   `raylib` stays at `6.0.0` on the convention that a binding tracks its
   upstream's version while a native module versions itself.
5. Version directories `<name>/<version>/mod.yp`, falling back to
   `<name>/mod.yp` for backwards compatibility.
6. Manifest scan and solver.
7. Lockfile, once fetching exists.
8. Coexistence and type namespacing, if ever needed.

Steps 1 and 2 are invisible plumbing that make everything after them
incremental.

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
