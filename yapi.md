# YAPI
YAPI is a module that is implemented and given my the compiler itself to allow emission and return of types, expressions, statement, functions and methods.

All functions described below are to be part of the yapi module, meaning to call `foo`, you need to do `yapi->foo`.

## Calling macro (comptime-typed) functions
This applies to user-authored functions whose return type is a comptime type
(yExpr, yStatement, yType, yFunc, ...) -- e.g. `yType fn pair(yType t1, yType t2) {...}`,
`yType fn arr(yType T) {...}`. Not the yapi-> builtins themselves, which are
always plain calls.

Such a function is always executed at compile time, via TCC -- there is no
meaningful runtime version of it (the runtime build only has no-op stubs for
yapi-> calls, so actually running its body at program runtime would just
return garbage). There are two ways to call one:

- `func:(args)` -- executes at compile time, then splices/substitutes the
  result into the AST at the call site. This is what a human writes to
  invoke a macro directly, e.g. `pair:(i32, i32) p;`.
- `func(args)` -- executes at compile time, but binds the result as an
  ordinary value at that call site instead of splicing it. Used for
  composing macros: one macro-typed function calling another as a helper,
  keeping the returned yExpr/yType/yStatement to keep building with.

Both forms are valid anywhere a call is valid -- not just inside other
macro-typed function bodies -- and both accept the same argument syntax:
- ordinary expressions
- `#expr` -- passes a pre-built yExpr value as an already-built AST node
- `+ident` -- introduces a new hygienic identifier (checked against the
  caller's scope)
- raw type identifiers -- passed directly as yType args, e.g. `pair:(i32, i32)`

`:( )` and `( )` share one argument grammar; they differ only in what
happens to the result (splice vs. bind as a value).

## Expressions
Expressions are of type yExpr. The can be build via builder commands:
yExpr bin_op(yExpr, yBinOp, yExpr) //yBinOp is enum of '+', '-', '*' etc.
yExpr assign(yLval, yAssignOp, yExpr) //yAssignOp is enum of '=', '+=', '-=' etc.
//Note: yExpr is not read-only by nature -- like yap_expr, it can carry is_lvalue.
//member() and friends propagate is_lvalue from their receiver, so no separate
//yLval-returning variant is needed. assign() does not itself validate that its
//first arg is lvalue-tagged -- it's always safe / unchecked, on the caller.
yExpr member(yExpr, cstring) //c string is field name
...
You can build all expressions like this

### Variables
yExpr var_value(cstring) //This returns an expression of a given var . It has to be already in the scope! It can be only read from!
yLval new_var(yType, yIdent) //This creates a new variable that can be fully used. It requires yIdent (either `+ident` or uniq).
//Passing `lval=` makes an expression you can assign to, unlike passing normal `yExpr` which makes it readonly.

## Statements
yStatement expr_statement(yExpr) //Creates an expression statement
yStatement var_decl(yType, yIdent) //Generates a variable declaration statement
yStatement while_stmt(yExpr cond, yStatement body) //while loop; body is typically block(stmt_list)
...
You can build all statements like this

## Types
yType is used to represent types.

They should match yap_type_id.


To create a new type use:
yStructT struct_t()
yEnumT enum_t()
yUnionT union_t()

Note the `T` at the end of types; they stand for templates!

You can then fill those templates using builders on them

yStructT st = yapi->struct_t();
st:add_field(yType, "my_field_name");
...
etc.

To use the type, you *have* to call
yType st:finish("my_struct");

This:
1) Locks the type template so you cannot modify it no more
2) Calculates the hash name for the given type. The whole type declaration with the given name is hashed and the final type name is "my_struct" + "_" + calculated hash
3) Emits the type _if it was not done already_
4) Returns the usable type. It was either just emited or emitted before (check st:existed())

This makes it so you can use
yType st:type()
which also returns the emited type (it is inside `st` after `finish`)

Importantly, you can also check
bool st:existed()
To get if the template was newly emitted or if name lookup found the same type with the same name + hash combo.

Typically, you would:
yType my_cool_type(...){
    _ st = yapi->struct_t();
    ... //Fill the struct type
    _ res = st:finish("arr");
    if (st:existed()) ret res;
    //You can emit methods here (see, later)
    ...
    ret res;
}

This is important because you can speed up by returning the type if it already exists.
The name is always needed as the type emission takes the type structure, builds it with said name and the result (including the given name).

Returned yType becomes usable! You can also get a type from a passed arg or by using
yType type(cstring) //for example; yapi->type("i32");

Function types (for function-valued params, e.g. map/filter callbacks) are built with
yType func_type0(yType ret)
yType func_type1(yType ret, yType p1)
yType func_type2(yType ret, yType p1, yType p2)
yType func_type3(yType ret, yType p1, yType p2, yType p3)
//Fixed arities mirror call0..call3. The type is deduped via
//yap_ctx_insert_type_if_not_exists, so declaring a builder-made method param
//with func_typeN makes the normal call-site argument check reject mismatched
//function values ("Argument type mismatch") with no extra macro-side code.

## Functions
To emit a function, you need to build it like so.
yFuncT func_t()

Assuming:
_ ft = yapi->func_t();

You can then do:

ft:add_param(yType, cstring); //This can added multiple times ofc
//and...
ft:set_return_type(yType);
//then finally...
ft:set_body(yStatement);

Where statement has to be a block.
Finally, you can do. Finish, which works similarly to how it worked with out st.
_ f = ft:finish("my_cool_fn");

//This also exists, same behavior as with type
bool ft:existed()
yFunc ft:func();

This hashes the func code and appends it to the name.

## Methods
Methods follow a similar pattern to funcs:
yFuncT yType:new_method()
`yFuncT` contains a n `is_method` flag. Returned `yFuncT` already has first param set to given  `yType`.

The rest is the same, so you set return type, add body, and emit.

Running `finish("hello")` on a method results in a mangled name <mangled_subject_type> + "_" + "hello".

//OPEN QUESTION: new_method() auto-injects the subject as the first param, but
//it's not yet specified how the method body gets a yExpr referencing it while
//being built (e.g. to call member(subject_expr, c"data")). Does the injected
//param have a fixed name (so yapi->var_value(c"self") already works), or does
//new_method()/the returned yFuncT need its own accessor, e.g. get_t:subject()?


### Example: Creating an array type!
yType fn arr(yType T){
    _ st = yapi->struct_t();
    st:add_field(T, c"data");
    st:add_field(yapi->type("u32"), c"sz");

    _ res = st:finish("arr");
    if (st:existed()) ret res;

    //We can safely assume methods are ok to be added
    _ get_t = res:new_method();
    get_t:set_return_type(res);
    yExpr self = get_t:get_subject();
    yExpr p1 = get_t:add_param(yapi->type("u32"), "index");
    ... build body to `body`
    get_t:setBody(body);
    get_t:finish("at");

    ret res;

}

## Blueprints (quasi-quote sugar over the builders)

Blueprints let you write concrete syntax with `$` marks instead of building AST by
hand. `$` always means "reach into the comptime world"; the timing differs by form.
There are four keyword-tagged forms (the old `$(…)`/`${…}`/`$[…]`/`$<…>` brackets are
gone):

- `expr${ <expr with $holes> }` -> **yExprBlueprint** (lazy). `$name` is a named hole.
  Fill and close with methods: `expr${ $x + 1 }:fill_expr(c"x", a):finish()` -> yExpr.
  Supports literals, vars, holes, arithmetic/comparisons, ternary, assignment,
  member/index, deref, address-of, cast, and calls (<=3 args).

- `stmt${ <statements with $holes> }` -> **yStmtBlueprint** (lazy). Same
  fill_expr/finish protocol; `:finish()` yields a yStmt (a block if >1 statement).
  e.g. `stmt${ $l = $r; }:fill_expr(c"l", lhs):fill_expr(c"r", rhs):finish()`.

- `type${ struct/enum/union { … } }` -> a **yStructT / yEnumT / yUnionT** template
  (eager, Model A). `$T` in a field/variant type splices the in-scope comptime yType
  now; a named type -> `yapi->type(c"…")`. You `:finish("name")` it yourself, so
  naming/hash/dedup/`existed()`/methods all stay on the existing template API:
  `_ st = type${ struct { $T first, $T second } }; ret st:finish(c"pair");`

- `(RET fn$ params){ body }` -> a **yFnT** template (eager, Model A) — the anonymous
  func literal tagged `fn` -> `fn$`. `$T` in a param/return type splices eagerly; the
  body refs the fn's own params. You `:finish("name")` it:
  `_ ft = ($T fn$ $T a, $T b){ ret a + b; }; ret ft:finish(c"add");`

The two *lazy* forms (expr/stmt) produce hole-carrying blueprints you fill; the two
*eager* forms (type/fn) produce ready templates (no fill phase). `stmt${ }` also
supports **statement holes** — a bare `$body;` in statement position, filled with
`:fill_stmt(c"body", someStmt)` — so you can build control-flow templates:
`stmt${ if ($c) { $b; } }:fill_expr(c"c", cond):fill_stmt(c"b", body):finish()`.
(`fill_type` / `fill_ident` for holes in type/name positions are not yet supported —
those positions are builder arguments, not deferrable AST nodes.)