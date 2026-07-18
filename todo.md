# YAP Compiler ; Next Steps

## 1. Variable declaration inference (`_ name = expr`)

`_ f = node.kind;` ; the `_` means "infer the type." Build the RHS expression, grab its type, and declare `f` with that type. Small, self-contained, immediately testable.

## 2. Type coercion for literals

`42` can't be assigned to `u64`, `f32` fields. Fix untyped→primitive coercion so real assignments work in tests.

## 3. Cross-module function resolution

`import.yp` calls `foo()` from an imported file. The build phase needs to look up symbols across source boundaries.

## 4. `(fn ...)` function types as field/param types

`callback_hell` declares function-pointer fields like `(fn i32, i32) simple_cb` ; grammar parses them, but the build phase can't resolve anonymous function types yet.

## 5. Block expressions `({ ... })`

The grammar supports `({ ... })` (GNU C statement expressions), but the parse doesn't produce usable nodes yet. Enables things like:

```yap
i32 x = ({ i32 tmp = compute(); tmp + 1; });
```
