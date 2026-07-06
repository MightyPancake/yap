# YAPI
YAPI is a module that is implemented and given my the compiler itself to allow emission and return of types, expressions, statement, functions and methods.

All functions described below are to be part of the yapi module, meaning to call `foo`, you need to do `yapi->foo`.

## Calling macro (comptime-typed) functions
This applies to user-authored functions whose return type is a comptime type
(yExpr, yStmt, yType, yFn, ...) -- e.g. `yType fn pair(yType t1, yType t2) {...}`,
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
  keeping the returned yExpr/yType/yStmt to keep building with.

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
yExpr opt_member(yExpr, cstring) //optional chaining ('obj?.field'); requires a pointer-to-struct/union operand, never an lvalue -- falls back to the zero value at runtime when the pointer is null
yExpr neg(yExpr) //prefix unary minus ('-expr')
yExpr not(yExpr) //prefix logical not ('!expr'); truthy-checks any scalar (primitive or pointer), always yields bool
yExpr bnot(yExpr) //prefix bitwise not ('~expr'); numeric operand, keeps the operand's own type
yExpr increment(yExpr, bool prefix) //'++expr' (prefix=true) / 'expr++' (prefix=false); operand must be an lvalue (unchecked, like assign())
yExpr decrement(yExpr, bool prefix) //'--expr' / 'expr--'; same rules as increment
yExpr call0(yExpr func)
yExpr call1(yExpr func, yExpr a)
yExpr call2(yExpr func, yExpr a, yExpr b)
yExpr call3(yExpr func, yExpr a, yExpr b, yExpr c)
//call0..call3 cover the common small-arity case directly (no list needed).
//For an arbitrary number of arguments, build a yCallArgs list first, then call():
yCallArgs call_args_new()                    //new, empty growable arg list
yCallArgs call_args_push(yCallArgs, yExpr)   //returns the same list, grown by one
yExpr call(yExpr func, yCallArgs args)       //calls func with however many args the list holds
...
You can build all expressions like this

### Variables
yExpr var_value(cstring) //This returns an expression of a given var . It has to be already in the scope! It can be only read from!
yLval new_var(yType, yIdent) //This creates a new variable that can be fully used. It requires yIdent (either `+ident` or uniq).
//Passing `lval=` makes an expression you can assign to, unlike passing normal `yExpr` which makes it readonly.

## Statements
yStmt expr_stmt(yExpr) //Creates an expression statement
yStmt var_decl(yType, yIdent) //Generates a variable declaration statement (declare only -- no inline initializer; follow with an assign()-wrapped expr_stmt to initialize)
yStmt while_stmt(yExpr cond, yStmt body) //while loop; body is typically block(stmt_list)
yStmt for_stmt(yStmt init, yExpr cond, yExpr update, yStmt body) //'for (init; cond; update) body'
yStmt break_stmt() //'break;' -- no payload; unchecked whether the splice site is actually inside a loop (same trust-the-caller stance as assign())
yStmt continue_stmt() //'continue;' -- same caveat as break_stmt
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

//yEnumT has its own variant builders (name-only, auto-incrementing value, or
//an explicit value):
yEnumT et = yapi->enum_t();
et:add_variant("first");                    //implicit value
et:add_variant_value("ten", yapi->int(10));  //explicit value ('ten = 10' in the emitted C enum)

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
yType fn_type0(yType ret)
yType fn_type1(yType ret, yType p1)
yType fn_type2(yType ret, yType p1, yType p2)
yType fn_type3(yType ret, yType p1, yType p2, yType p3)
//Fixed arities only, by deliberate design -- unlike call0..call3 below, there
//is no general variable-arity fn_type builder (no real call site has ever
//needed a function-valued param with more than 3 args). The type is deduped
//via yap_ctx_insert_type_if_not_exists, so declaring a builder-made method
//param with fn_typeN makes the normal call-site argument check reject
//mismatched function values ("Argument type mismatch") with no extra
//macro-side code.

Fixed-size array types ('Type[N]' in surface syntax) are built with
yType array_of(yType elem, int size)
//Mirrors ptr_of/slice_of; deduped the same way.

## Functions
To emit a function, you need to build it like so.
yFnT fn_t()

Assuming:
_ ft = yapi->fn_t();

You can then do:

ft:add_param(yType, cstring); //This can added multiple times ofc
//and...
ft:set_return_type(yType);
//then finally...
ft:set_body(yStmt);

Where statement has to be a block.
Finally, you can do. Finish, which works similarly to how it worked with out st.
_ f = ft:finish("my_cool_fn");

//This also exists, same behavior as with type
bool ft:existed()
yFn ft:func();

This hashes the func code and appends it to the name.

To get a callable yExpr referencing an already-finished yFn by its real
(possibly hashed/mangled) emitted name -- e.g. to pass it as a value, or
splice a call to it -- use:
yExpr f:ref()

## Methods
Methods follow a similar pattern to funcs:
yFnT yType:new_method()
yFnT yType:new_ref_method() //pointer-receiver variant: the subject param is a pointer to yType instead of yType itself, auto-address-of'd at the call site
`yFnT` contains an `is_method` flag. Returned `yFnT` already has first param set to given `yType` (or a pointer to it, for `new_ref_method`).

The rest is the same, so you set return type, add body, and emit.

Running `finish("hello")` on a method results in a mangled name <mangled_subject_type> + "_" + "hello".

To reference the injected subject param while building the method's body
(e.g. to call member(subject_expr, c"data")), call:
yExpr ft:get_subject() //returns a yExpr referencing the method's own first (subject) param


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
    get_t:set_body(body);
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
  Supports literals, vars, holes, arithmetic/comparisons, unary (`-`/`!`/`~`),
  ternary, assignment, member/index, deref, address-of, cast, and calls (any
  number of args -- desugars to call0..call3 for <=3 args, or a folded
  call_args_new/call_args_push chain into `call()` beyond that).
  A cast's type (`$x.($T)`) is a lazy hole too, closed with `:fill_type(c"T", ty)`.

- `stmt${ <statements with $holes> }` -> **yStmtBlueprint** (lazy). Same
  fill_expr/finish protocol; `:finish()` yields a yStmt (a block if >1 statement).
  e.g. `stmt${ $l = $r; }:fill_expr(c"l", lhs):fill_expr(c"r", rhs):finish()`.

- `type${ struct/enum/union { … } }` -> a **yStructT / yEnumT / yUnionT** template
  (eager, Model A). `$T` in a field/variant type splices the in-scope comptime yType
  now; a named type -> `yapi->type(c"…")`. You `:finish("name")` it yourself, so
  naming/hash/dedup/`existed()`/methods all stay on the existing template API:
  `_ st = type${ struct { $T first, $T second } }; ret st:finish(c"pair");`

- `(RET fn$ params){ body }` -> a **yFnT** template (eager, Model A) ; the anonymous
  func literal tagged `fn` -> `fn$`. `$T` in a param/return type splices eagerly; the
  body refs the fn's own params. You `:finish("name")` it:
  `_ ft = ($T fn$ $T a, $T b){ ret a + b; }; ret ft:finish(c"add");`

The two *lazy* forms (expr/stmt) produce hole-carrying blueprints you fill; the two
*eager* forms (type/fn) produce ready templates (no fill phase). `stmt${ }` also
supports **statement holes** ; a bare `$body;` in statement position, filled with
`:fill_stmt(c"body", someStmt)` ; so you can build control-flow templates:
`stmt${ if ($c) { $b; } }:fill_expr(c"c", cond):fill_stmt(c"b", body):finish()`.
`stmt${ }` also has real lazy type-holes and ident-holes (closed via `:fill_type`/
`:fill_ident`), which is what makes a `var_decl` with a `$`-named type and a
`$`-named var possible: `stmt${ $T $name = $init; }:fill_type(c"T", ty)
:fill_ident(c"name", +n):fill_expr(c"init", seed):finish()`. `fill_ident` only
closes the *declaration's* name; if the template also references that same
name again later (e.g. a loop counter used in its own condition/body), use
`fill_var(name, type, ident)` instead -- it closes the declaration's
ident-hole AND every later plain `$name` expr-hole reference in one call.
(`type${ }`/`fn$` stay eager and unchanged -- no fill phase, `$T` splices
immediately -- so `fill_type`/`fill_ident` are only meaningful for `stmt${ }`.)