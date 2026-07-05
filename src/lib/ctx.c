#include "yap/all.h"

declare_map_for(named_type);
declare_map_for(module);

yap_ctx* yap_ctx_new(){
    yap_log("Creating new ctx");
    yap_ctx* ctx = mem_one_cpy(((yap_ctx){
      .arena = quake_new(),
      .sources=darr_new(yap_source*),
      .source_stack=darr_new(yap_source*),
      .scopes=darr_new(yap_scope*),
      .current_scopes=darr_new(yap_scope*),
      .errors=darr_new(yap_error),
      .modules=new_module_map(),
      .current_module_name=NULL,
      .module_lookup_paths=darr_new(char*),
      .semantic_decls=darr_new(yap_decl),
      .types=darr_new(yap_type), //yap_type_id points to types in this array
      .named_types=new_named_type_map(),
    }));
    yap_ctx_init_root_source(ctx);
    yap_ctx_push_new_scope(ctx); //Push global scope
    ctx->global_scope = yap_ctx_current_scope(ctx);
    yap_ctx_create_new_module(ctx, "global", ""); //The global module lacks prefix for mangling since it's the root module
    yap_ctx_switch_module(ctx, "global");
    //Default types (requires <stdint.h> for fixed width integer types and <stdbool.h> for bool)
    ctx->internal_error_type_id = yap_ctx_push_new_primitive_type(ctx, 0, false, false, "internal_error_t", "ie_t", "__yap_internal_error_t"); //This is a dummy type used for invalid/empty types. Basically, we can return 0 for error in this case
    ctx->void_type_id = yap_ctx_push_new_primitive_type(ctx, 0, false, false, "none", "v", "void");
    ctx->bool_type_id = yap_ctx_push_new_primitive_type(ctx, 1, false, false, "bool", "b", "bool");
    yap_ctx_push_new_primitive_type(ctx, 1, false, false, "byte", "c", "char");
    ctx->int_type_id = yap_ctx_push_new_primitive_type(ctx, 4, true, false, "i32", "i32", "int");
    yap_ctx_push_new_primitive_type(ctx, 4, false, false, "u32", "u32", "unsigned int");
    yap_ctx_push_new_primitive_type(ctx, 8, true, false, "i64", "i64", "long");
    yap_ctx_push_new_primitive_type(ctx, 8, false, false, "u64", "u64", "unsigned long");
    ctx->float_type_id = yap_ctx_push_new_primitive_type(ctx, 4, true, true, "f32", "f32", "float");
    yap_ctx_push_new_primitive_type(ctx, 8, true, true, "f64", "f64", "double");

    //Comptime types (opaque pointer handles for metaprogramming)
    ctx->yexpr_type_id      = yap_ctx_push_new_primitive_type(ctx, 8, false, false, "yExpr",      "yExpr",      "void*");
    ctx->ytype_type_id      = yap_ctx_push_new_primitive_type(ctx, 8, false, false, "yType",      "yType",      "void*");
    ctx->ystmt_type_id = yap_ctx_push_new_primitive_type(ctx, 8, false, false, "yStmt", "yStmt", "void*");
    ctx->yfn_type_id      = yap_ctx_push_new_primitive_type(ctx, 8, false, false, "yFn",      "yFn",      "void*");
    ctx->yident_type_id     = yap_ctx_push_new_primitive_type(ctx, 8, false, false, "yIdent",     "yIdent",     "const char*");
    //yExprBlueprint: a yExpr template that may contain named holes ($name). Same
    //C representation as yExpr (a yap_expr*), but a distinct front-end type so a
    //stored blueprint must be :fill()'d and :finish()'d before use as a yExpr.
    //(Named for the expression form specifically; a yStmtBlueprint etc. can
    //follow when ${}/$[] land.)
    ctx->yexprblueprint_type_id = yap_ctx_push_new_primitive_type(ctx, 8, false, false, "yExprBlueprint", "yExprBlueprint", "void*");
    //yStmtBlueprint: the stmt${ } analogue -- a yStmt template with named holes,
    //filled via :fill_expr(...)/:finish() (same C repr as yStmt, a yap_statement*).
    ctx->ystmtblueprint_type_id = yap_ctx_push_new_primitive_type(ctx, 8, false, false, "yStmtBlueprint", "yStmtBlueprint", "void*");
    //yExprList is a real slice of yExpr (not an opaque handle) so it gets native
    //'.len' and ':[i]' -- see yap_build_member_access_expr's slice case and
    //yap_exec_macro_call's blob-literal marshalling (both in build.c) for the
    //two places that had to learn about slices specifically because of this.
    {
        yap_type yexprlist_slice = { .kind = yap_type_slice, .slice = { .element_type = ctx->yexpr_type_id }, .is_const = false };
        ctx->yexprlist_type_id = yap_ctx_push_named_type(ctx, "yExprList", NULL, yexprlist_slice);
    }
    ctx->ystmtlist_type_id  = yap_ctx_push_new_primitive_type(ctx, 8, false, false, "yStmtList",  "yStmtList",  "void*");

    //Comptime builder templates (yapi.md): opaque handles for the incremental
    //struct/enum/union/func builders, distinct from the finished yType/yFn they emit.
    ctx->ystructt_type_id = yap_ctx_push_new_primitive_type(ctx, 8, false, false, "yStructT", "yStructT", "void*");
    ctx->yenumt_type_id   = yap_ctx_push_new_primitive_type(ctx, 8, false, false, "yEnumT",   "yEnumT",   "void*");
    ctx->yuniont_type_id  = yap_ctx_push_new_primitive_type(ctx, 8, false, false, "yUnionT",  "yUnionT",  "void*");
    ctx->yfnt_type_id   = yap_ctx_push_new_primitive_type(ctx, 8, false, false, "yFnT",   "yFnT",   "void*");

    //Comptime builder module: yapi
    {
        yap_module* yapi = yap_ctx_create_new_module(ctx, "yapi", "yapi_");

        yap_type_id ye = ctx->yexpr_type_id;
        yap_type_id i  = ctx->int_type_id;
        yap_type_id f  = yap_ctx_get_type_id_by_name(ctx, "f64");
        yap_type_id b  = ctx->bool_type_id;
        yap_type_id bp = yap_ctx_get_pointer_of_type_id(ctx, yap_ctx_get_type_id_by_name(ctx, "byte"));
        yap_type_id yt = ctx->ytype_type_id;
        yap_type_id yi = ctx->yident_type_id;
        yap_type_id ys = ctx->ystmt_type_id;
        yap_type_id v  = ctx->void_type_id;
        yap_type_id ysl = ctx->ystmtlist_type_id;
        yap_type_id yst = ctx->ystructt_type_id;
        yap_type_id yen = ctx->yenumt_type_id;
        yap_type_id yun = ctx->yuniont_type_id;
        yap_type_id yft = ctx->yfnt_type_id;

        struct { const char* name; yap_type_id ret; yap_type_id args[4]; int argc; } builtins[] = {
            { "int",           ye,      {i},          1 },
            { "float",         ye,      {f},          1 },
            { "string",        ye,      {bp},         1 },
            { "bool",          ye,      {b},          1 },
            { "var_value",     ye,      {bp},         1 },
            { "new_var",       ye,      {yt, yi},     2 },
            { "bin_op",        ye,      {ye, i, ye},  3 },
            { "neg",           ye,      {ye},         1 },
            { "ternary",       ye,      {ye, ye, ye}, 3 },
            { "assign",        ye,      {ye, i, ye},  3 },
            { "member",        ye,      {ye, bp},     2 },
            { "index",         ye,      {ye, ye},     2 },
            { "cast",          ye,      {ye, yt},     2 },
            { "deref",         ye,      {ye},         1 },
            { "addr_of",       ye,      {ye},         1 },
            { "ptr_of",        yt,      {yt},         1 },
            { "slice_of",      yt,      {yt},         1 },
            { "sizeof",        ye,      {yt},         1 },
            { "call0",         ye,      {ye},             1 },
            { "call1",         ye,      {ye, ye},         2 },
            { "call2",         ye,      {ye, ye, ye},     3 },
            { "call3",         ye,      {ye, ye, ye, ye}, 4 },
            { "kind",          i,       {ye},         1 },
            { "is_comptime",   i,       {ye},         1 },
            { "var_decl",      ys,      {yt, yi},     2 },
            { "expr_stmt",     ys,      {ye},         1 },
            { "return_stmt",   ys,      {ye},         1 },
            { "if_stmt",       ys,      {ye, ys},     2 },
            { "if_else_stmt",  ys,      {ye, ys, ys}, 3 },
            { "while_stmt",    ys,      {ye, ys},     2 },
            { "block",         ys,      {ysl},        1 },
            { "block_expr",    ye,      {ysl},        1 },
            { "uniq",          ye,      {ye},         0 },
            { "uniq_name",     yi,      {ye},         0 },
            { "stmt_list_new",  ysl,    {ys},         0 },
            { "stmt_list_push", ysl,    {ysl, ys},    2 },
            { "struct_t",      yst,     {i},          0 },
            { "enum_t",        yen,     {i},          0 },
            { "union_t",       yun,     {i},          0 },
            { "fn_t",        yft,     {i},          0 },
            { "type",          yt,      {bp},         1 },
            { "fn_type0",    yt,      {yt},             1 },
            { "fn_type1",    yt,      {yt, yt},         2 },
            { "fn_type2",    yt,      {yt, yt, yt},     3 },
            { "fn_type3",    yt,      {yt, yt, yt, yt}, 4 },
            { "type_exists",   b,       {bp},         1 },
            { "func_exists",   b,       {bp},         1 },
            { "log",           v,       {bp},         1 },
            { "error",         v,       {bp},         1 },
            { "warn",          v,       {bp},         1 },
            //Blueprint support: build.c desugars a blueprint literal into these hole
            //placeholders (fill/finish are methods on yExprBlueprint/yStmtBlueprint, below).
            { "hole",          ye,      {bp},         1 },
            { "hole_stmt",     ys,      {bp},         1 },
            { "type_hole",     yt,      {bp},         1 },
            { "ident_hole",    yi,      {bp},         1 },
        };
        int n = sizeof(builtins) / sizeof(builtins[0]);
        for (int bi = 0; bi < n; bi++){
            darr(yap_type_id) arg_ids = yap_ctx_darr_new(ctx, yap_type_id,
                .cap = builtins[bi].argc, .len = 0);
            for (int ai = 0; ai < builtins[bi].argc; ai++)
                darr_push(arg_ids, builtins[bi].args[ai]);
            yap_type ft = {
                .kind = yap_type_func,
                .func = { .args = arg_ids, .return_type = builtins[bi].ret },
            };
            yap_type_id ftid = yap_ctx_insert_type_if_not_exists(ctx, ft);
            yap_scope_set_var(yapi->scope, (yap_var){ .name = (char*)builtins[bi].name, .type = ftid });
        }
    }

    //Builtin methods on comptime builder templates (yapi.md): registered directly into
    //global scope (not a module) since method dispatch (yap_build_method_callee) does a
    //plain recursive scope lookup on "OwnerName_methodname" -- exactly like user-declared
    //type methods -- rather than a module-prefixed lookup.
    {
        yap_type_id ye  = ctx->yexpr_type_id;
        yap_type_id yt  = ctx->ytype_type_id;
        yap_type_id ys  = ctx->ystmt_type_id;
        yap_type_id yfn = ctx->yfn_type_id;
        yap_type_id yst = ctx->ystructt_type_id;
        yap_type_id yen = ctx->yenumt_type_id;
        yap_type_id yun = ctx->yuniont_type_id;
        yap_type_id yft = ctx->yfnt_type_id;
        yap_type_id b   = ctx->bool_type_id;
        yap_type_id v   = ctx->void_type_id;
        yap_type_id bp  = yap_ctx_get_pointer_of_type_id(ctx, yap_ctx_get_type_id_by_name(ctx, "byte"));
        yap_type_id yi  = ctx->yident_type_id;
        yap_type_id ybp = ctx->yexprblueprint_type_id;
        yap_type_id ysbp = ctx->ystmtblueprint_type_id;

        struct { const char* name; yap_type_id ret; yap_type_id args[4]; int argc; } methods[] = {
            { "yStructT_add_field", yst,   {yst, yt, bp}, 3 },
            { "yStructT_finish",    yt,  {yst, bp},     2 },
            { "yStructT_existed",   b,   {yst},         1 },
            { "yStructT_type",      yt,  {yst},         1 },

            { "yEnumT_add_variant", yen,   {yen, bp},     2 },
            { "yEnumT_finish",      yt,  {yen, bp},     2 },
            { "yEnumT_existed",     b,   {yen},         1 },
            { "yEnumT_type",        yt,  {yen},         1 },

            { "yUnionT_add_field",  yun,   {yun, yt, bp}, 3 },
            { "yUnionT_finish",     yt,  {yun, bp},     2 },
            { "yUnionT_existed",    b,   {yun},         1 },
            { "yUnionT_type",       yt,  {yun},         1 },

            { "yFnT_add_param",       ye,  {yft, yt, bp}, 3 },
            { "yFnT_set_return_type", v,   {yft, yt},     2 },
            { "yFnT_set_body",        v,   {yft, ys},     2 },
            { "yFnT_finish",          yfn, {yft, bp},     2 },
            { "yFnT_existed",         b,   {yft},         1 },
            { "yFnT_func",            yfn, {yft},         1 },
            { "yFnT_get_subject",     ye,  {yft},         1 },

            // yFn:ref() -> a callable yExpr referencing the finished function by
            // its real (possibly hashed/mangled) emitted name, so target code can
            // actually call a fn$/fn_t()-built function instead of only verifying
            // it was emitted.
            { "yFn_ref",              ye,  {yfn},         1 },

            { "yType_new_method",     yft, {yt},          1 },
            { "yType_new_ref_method", yft, {yt},          1 },

            //yExprBlueprint (the $(...) quasi-quote): fill one named hole at a
            //time (chainable, returns the blueprint), then finish to a yExpr.
            //fill_type closes a cast's lazy $T type-hole nested inside the expr.
            { "yExprBlueprint_fill_expr",   ybp, {ybp, bp, ye}, 3 },
            { "yExprBlueprint_fill_type",   ybp, {ybp, bp, yt}, 3 },
            { "yExprBlueprint_finish", ye,  {ybp},         1 },

            //yStmtBlueprint (the stmt${ } quasi-quote): fill_expr for expr holes,
            //fill_stmt for statement holes ($body in statement position),
            //fill_type/fill_ident for a var_decl's type/name holes. fill_var is
            //a combinator: closes BOTH a var_decl's name-hole (the declaration)
            //AND any later plain reference to that same name (a separate
            //expr-hole) in one call, so a template can write $out once for a
            //declared variable and use it again later without a second hole name.
            { "yStmtBlueprint_fill_expr",  ysbp, {ysbp, bp, ye},     3 },
            { "yStmtBlueprint_fill_stmt",  ysbp, {ysbp, bp, ys},     3 },
            { "yStmtBlueprint_fill_type",  ysbp, {ysbp, bp, yt},     3 },
            { "yStmtBlueprint_fill_ident", ysbp, {ysbp, bp, yi},     3 },
            { "yStmtBlueprint_fill_var",   ysbp, {ysbp, bp, yt, yi}, 4 },
            { "yStmtBlueprint_finish",     ys,   {ysbp},             1 },
        };
        int n = sizeof(methods) / sizeof(methods[0]);
        for (int mi = 0; mi < n; mi++){
            darr(yap_type_id) arg_ids = yap_ctx_darr_new(ctx, yap_type_id,
                .cap = methods[mi].argc, .len = 0);
            for (int ai = 0; ai < methods[mi].argc; ai++)
                darr_push(arg_ids, methods[mi].args[ai]);
            yap_type ft = {
                .kind = yap_type_func,
                .func = { .args = arg_ids, .return_type = methods[mi].ret },
            };
            yap_type_id ftid = yap_ctx_insert_type_if_not_exists(ctx, ft);
            yap_scope_set_var(ctx->global_scope, (yap_var){ .name = (char*)methods[mi].name, .type = ftid });
        }
    }

    //Untyped default types for literal coercion
    ctx->untyped_int_type_id = yap_ctx_push_type(ctx, yap_untyped_type(ctx->int_type_id));
    ctx->untyped_float_type_id = yap_ctx_push_type(ctx, yap_untyped_type(ctx->float_type_id));
    yap_type_id byte_id = yap_ctx_get_type_id_by_name(ctx, "byte");
    ctx->untyped_byte_type_id = yap_ctx_push_type(ctx, yap_untyped_type(byte_id));
    
    return ctx;
}

void yap_ctx_init_root_source(yap_ctx* ctx){
    if (!ctx) return;
    yap_source root = (yap_source){
      .kind=yap_source_root,
      .identity="<root>",
      .parent=NULL,
      .label="<root>",
      .origin="<root>",
      .content=NULL,
      .sz=0,
      .source_node=NULL,
      .ctx=ctx,
      .imports=darr_new(yap_import)
    };
    ctx->root_source = yap_ctx_one_cpy(ctx, root);
    darr_push(ctx->sources, ctx->root_source);
    yap_ctx_push_source(ctx, ctx->root_source);
}

yap_module* yap_ctx_get_module(yap_ctx* ctx, char* name){
  if (!ctx || !name) return NULL;
  const yap_module dummy = {.name = name};
  return (yap_module*)hashmap_get(ctx->modules, &dummy);
}

yap_module* yap_ctx_create_new_module(yap_ctx* ctx, char* name, char* prefix){
  if (!ctx || !name || !ctx->global_scope) return NULL;
  yap_module* module = yap_ctx_get_module(ctx, name);
  if (module){
    char* msg = strus_newf("Module '%s' already exists", name);
    yap_ctx_push_error(ctx, (yap_error){
      .kind = yap_error_no_pos,
      .src = NULL,
      .msg = msg
    });
    return NULL;
  }

  /* Parented to global_scope (not NULL) so that code living inside a module's
   * own source (e.g. modules/io/sugar.yap, modules/arr/arr.yap) can reach
   * builtins registered directly into global_scope -- notably the yapi.md
   * builder methods (yStructT_add_field, yExprList_at, ...), which are
   * looked up via a plain recursive scope walk from whatever the "current
   * scope" is at the call site, not a module-prefixed lookup. Purely
   * additive: only adds a fallback for names that don't already resolve
   * within the module's own scope. */
  yap_module new_module = {
    .name = yap_ctx_strus_cpy(ctx, name),
    .prefix = yap_ctx_strus_cpy(ctx, prefix),
    .decls = darr_new(yap_decl_node),
    .module_ctx = NULL,
    .scope = yap_ctx_new_scope(ctx, ctx->global_scope),
    .lib_paths = darr_new(char*)
  };
  hashmap_set(ctx->modules, &new_module);
  return yap_ctx_get_module(ctx, name);
}

yap_module* yap_ctx_switch_module(yap_ctx* ctx, char* name){
  if (!ctx || !name) return NULL;
  yap_module* module = yap_ctx_get_module(ctx, name);
  if (!module){
    char* msg = strus_newf("Module '%s' does not exist", name);
    yap_ctx_push_error(ctx, (yap_error){
      .kind = yap_error_no_pos,
      .src = NULL,
      .msg = msg ? msg : strus_copy("Module does not exist")
    });
    return NULL;
  }
  ctx->current_module_name = module->name;
  return module;
}

yap_module* yap_ctx_current_module(yap_ctx* ctx){
  if (!ctx || !ctx->current_module_name) return NULL;
  return yap_ctx_get_module(ctx, ctx->current_module_name);
}

void yap_ctx_push_decl_node(yap_ctx* ctx, yap_decl_node decl){
  yap_module* module = yap_ctx_current_module(ctx);
  if (!module) return;
  darr_push(module->decls, decl);
}

void* yap_ctx_malloc(yap_ctx* ctx, size_t bytes){
    if (!ctx || bytes == 0) return NULL;
    return quake_alloc(&ctx->arena, bytes);
}

void* yap_ctx_one_raw(yap_ctx* ctx, size_t bytes){
  return yap_ctx_malloc(ctx, bytes);
}

void* yap_ctx_one_cpy_raw(yap_ctx* ctx, const void* src, size_t bytes){
  if (!src || bytes == 0) return NULL;
  void* out = yap_ctx_one_raw(ctx, bytes);
  if (!out) return NULL;
  memcpy(out, src, bytes);
  return out;
}

char* yap_ctx_strus_newf(yap_ctx* ctx, const char* fmt, ...){
  if (!ctx || !fmt) return NULL;

  char* out = NULL;
  va_list ap;
  va_start(ap, fmt);
  int result = quake_vasprintf(&ctx->arena, &out, fmt, ap);
  va_end(ap);

  if (result < 0) return NULL;
  return out;
}

char* yap_ctx_strus_cpy(yap_ctx* ctx, char* src){
  if (!ctx || !src) return NULL;
  size_t len = strlen(src);
  return yap_ctx_one_cpy_raw(ctx, src, len + 1);
}

yap_scope* yap_ctx_new_scope(yap_ctx* ctx, yap_scope* parent){
    if (!ctx) return NULL;
    yap_scope* new_scope = yap_ctx_one_cpy(ctx, yap_new_scope(parent));
    darr_push(ctx->scopes, new_scope);
    return new_scope;
}

yap_scope* yap_ctx_push_new_scope(yap_ctx* ctx){
    if (!ctx) return NULL;
    yap_scope* new_scope = yap_ctx_new_scope(ctx, yap_ctx_current_scope(ctx));
    darr_push(ctx->current_scopes, new_scope);
    return new_scope;
}

yap_scope* yap_ctx_push_new_loop_scope(yap_ctx* ctx){
    if (!ctx) return NULL;
    yap_scope* new_scope = yap_ctx_new_scope(ctx, yap_ctx_current_scope(ctx));
    new_scope->is_loop = true;
    darr_push(ctx->current_scopes, new_scope);
    return new_scope;
}

yap_scope* yap_ctx_pop_scope(yap_ctx* ctx){
    if (!ctx || darr_len(ctx->current_scopes) == 0) return NULL;
    return darr_pop(ctx->current_scopes);
}

yap_scope* yap_ctx_current_scope(yap_ctx* ctx){
    if (darr_len(ctx->current_scopes) == 0) return NULL;
    return darr_last(ctx->current_scopes);
}

void yap_ctx_push_var(yap_ctx* ctx, yap_var var){
    if (!ctx) return;
    yap_scope* current_scope = yap_ctx_current_scope(ctx);
    if (!current_scope){
        yap_log("No scope to push variable to");
        return;
    }
    yap_scope_set_var(current_scope, var);

}

yap_type yap_primitive_type(size_t bytes, bool is_signed, bool is_float, char* name, char* mangled_name, char* c_name){
  return (yap_type){
    .kind = yap_type_primitive,
    .primitive = (yap_prim_type){
      .bytes = bytes,
      .is_signed = is_signed,
      .is_float = is_float,
      .name = name,
      .mangled_name = mangled_name,
      .c_name = c_name
    }
  };
  
}

yap_source* yap_add_source(yap_ctx* ctx, yap_source src){
  yap_source* src_p = yap_ctx_one_cpy(ctx, src);
  darr_push(ctx->sources, src_p);
  return src_p;
}

void yap_ctx_push_source(yap_ctx* ctx, yap_source* src){
  darr_push(ctx->source_stack, src);
}

yap_source* yap_ctx_pop_source(yap_ctx* ctx){
  return darr_pop(ctx->source_stack);
}

yap_source* yap_ctx_top_source(yap_ctx* ctx){
  if (darr_len(ctx->source_stack) == 0) return NULL;
  return darr_last(ctx->source_stack);
}

yap_source* yap_ctx_new_file_source(yap_ctx* ctx, yap_source* parent, char* label, char* absolute_path){
    char *content = NULL;
    char* identity = yap_ctx_strus_newf(ctx, "%s@%s", label, parent ? parent->origin : "<unknown>");
    char* abs_path = yap_ctx_strus_cpy(ctx, absolute_path);
    size_t size = yap_read_file_to_string(absolute_path, &content);
    if (size == 0 || !content){
      yap_log("Failed to read file '%s'", absolute_path);
      return NULL;
    }
    if (parent){
      yap_import parent_import = {
        .kind = yap_import_file,
        .identity = identity
      };
      darr_push(parent->imports, parent_import);
    }
    yap_source* src = yap_add_source(ctx, (yap_source){
      .kind = yap_source_file,
      .identity = identity,
      .parent = parent,
      .label = label,
      .origin=abs_path,
      .content=content,
      .sz=size,
      .ctx=ctx,
      .source_node=NULL,
      .imports=darr_new(yap_import),
      .from_module_import = parent ? parent->from_module_import : NULL
    });
    return src;
}

void yap_ctx_push_error(yap_ctx* ctx, yap_error err){
  if (!ctx) return;
  darr_push(ctx->errors, err);
}

bool yap_ctx_dispatch_errors(yap_ctx* ctx){
  if (!ctx) return false;
  if (darr_len(ctx->errors) == 0) return false;
  for_darr(i, err, ctx->errors){
    ctx->print_error(err);
  }
  for_darr(i, err, ctx->errors){
    yap_error_free(err);
  }
  darr_free(ctx->errors);
  ctx->errors = darr_new(yap_error);
  return true;
}

yap_type_id yap_ctx_push_type(yap_ctx* ctx, yap_type typ){
  darr_push(ctx->types, typ);
  return darr_len(ctx->types) - 1;
}

yap_type* yap_ctx_get_type(yap_ctx* ctx, yap_type_id id){
  if (id >= darr_len(ctx->types)) return NULL;
  return &ctx->types[id];
}

yap_type* yap_ctx_get_type_by_name(yap_ctx* ctx, char* name){
  yap_named_type dummy = {.name = name};
  const yap_named_type* named = hashmap_get(ctx->named_types, &dummy);
  if (!named) return NULL;
  return yap_ctx_get_type(ctx, named->id);
}

yap_type_id yap_ctx_get_type_id_by_name(yap_ctx* ctx, char* name){
  const yap_named_type dummy = {.name = name};
  const yap_named_type* named = hashmap_get(ctx->named_types, &dummy);
  if (!named) return 0;
  return named->id;
}

yap_type yap_untyped_type(yap_type_id default_id){
  return (yap_type){
    .kind = yap_type_untyped,
    .untyped_default = default_id
  };
}

yap_type_id yap_ctx_push_new_primitive_type(yap_ctx* ctx, size_t bytes, bool is_signed, bool is_float, char* name, char* mangled_name, char* c_name){
  return yap_ctx_push_named_type(ctx, name, c_name, yap_primitive_type(bytes, is_signed, is_float, name, mangled_name, c_name));
}

yap_type_id yap_ctx_push_named_type(yap_ctx* ctx, char* name_p, char* c_name_p, yap_type typ){
  yap_type t = typ;
  t.is_const = false;
  char* name = yap_ctx_strus_cpy(ctx, name_p);
  char* c_name = c_name_p ? yap_ctx_strus_cpy(ctx, c_name_p) : NULL;
  yap_type_id id = yap_ctx_push_type(ctx, t);
  yap_log("Pushing named type '%s' with C name '%s', got id %u", name_p, c_name_p ? c_name_p : "NULL", id);
  yap_named_type named = {
    .id = id,
    .name = name,
    .c_name = c_name
  };
  hashmap_set(ctx->named_types, &named);
  return id;
}

yap_type_id yap_ctx_coerce_type_id_to_id(yap_ctx* ctx, yap_type_id src_id){
  yap_type* src = yap_ctx_get_type(ctx, src_id);
  if (!src) return -1;
  if (src->kind == yap_type_untyped) return src->untyped_default;
  return src_id;
}

yap_type yap_ctx_coerce_type(yap_ctx* ctx, yap_type src){
  if (src.kind == yap_type_untyped) return *yap_ctx_get_type(ctx, src.untyped_default);
  return src;
}

bool yap_ctx_type_ids_eq(yap_ctx* ctx, yap_type_id left_id, yap_type_id right_id){
  if (!ctx) return false;
  left_id = yap_ctx_coerce_type_id_to_id(ctx, left_id);
  right_id = yap_ctx_coerce_type_id_to_id(ctx, right_id);
  if (left_id == right_id) return true;

  yap_type* left = yap_ctx_get_type(ctx, left_id);
  yap_type* right = yap_ctx_get_type(ctx, right_id);
  if (!left || !right) return false;
  return yap_ctx_types_eq(ctx, *left, *right);
}

bool yap_ctx_type_id_compatible(yap_ctx* ctx, yap_type_id id1, yap_type_id id2){
  if (!ctx) return false;
  yap_type* type1 = yap_ctx_get_type(ctx, id1);
  yap_type* type2 = yap_ctx_get_type(ctx, id2);
  if (!type1 || !type2) return false;
  return yap_ctx_type_compatible(ctx, *type1, *type2);
}

yap_type_id yap_ctx_find_common_type(yap_ctx* ctx, yap_type_id id1, yap_type_id id2){
  if (!ctx) return 0;
  yap_type* type1 = yap_ctx_get_type(ctx, id1);
  yap_type* type2 = yap_ctx_get_type(ctx, id2);
  if (!type1 || !type2) return 0;
  if (yap_ctx_type_compatible(ctx, *type1, *type2)) return yap_ctx_coerce_type_id_to_id(ctx, id1);
  if (yap_ctx_type_compatible(ctx, *type2, *type1)) return yap_ctx_coerce_type_id_to_id(ctx, id2);
  return ctx->internal_error_type_id;
}

//Check if type1 can be used in a context that expects type2.
//This should be treated one way only! Mainly used for possible coercions.
bool yap_ctx_type_compatible(yap_ctx* ctx, yap_type type1, yap_type type2){
  if (yap_ctx_types_eq(ctx, type1, type2)) return true;
  // Make sure at least one of the types is untype to proceed
  if (!(type1.kind == yap_type_untyped || type2.kind == yap_type_untyped)) return false;
  yap_type t1 = type2;
  yap_type t2 = type1;
  if (type1.kind == yap_type_untyped){
    t1 = type1;
    t2 = type2;
  }
  // t1 has to be untyped, t2 *can* be untyped
  yap_type coerced_t1 = yap_ctx_coerce_type(ctx, t1);
  // All untyped primitives match
  // TODO: This is clearly not the case, so it needs a fix!
  if (coerced_t1.kind == yap_type_primitive && t2.kind == yap_type_primitive) return true;
  return false;
}

//Check if a value of type 'rhs_id' can be assigned to a location of type 'lhs_id'.
//Untyped types are coerced to their defaults before comparison.
//This is stricter than type_compatible: untyped_int is NOT assignable to f32.
bool yap_ctx_type_id_assignable(yap_ctx* ctx, yap_type_id lhs_id, yap_type_id rhs_id){
  if (!ctx) return false;
  // null (void) is assignable to any pointer type
  if (rhs_id == ctx->void_type_id) {
    yap_type* lhs_typ = yap_ctx_get_type(ctx, lhs_id);
    if (lhs_typ && lhs_typ->kind == yap_type_ptr) return true;
  }
  // Coerce both sides (untyped types become their defaults)
  lhs_id = yap_ctx_coerce_type_id_to_id(ctx, lhs_id);
  rhs_id = yap_ctx_coerce_type_id_to_id(ctx, rhs_id);
  return yap_ctx_type_ids_eq(ctx, lhs_id, rhs_id);
}

char* yap_ctx_type_id_to_string(yap_ctx* ctx, yap_type_id id){
  yap_type* typ = yap_ctx_get_type(ctx, id);
  if (!typ) return "(invalid type id)";
  return yap_ctx_type_to_string(ctx, *typ);
}

//Needs to be freed after use!
char* yap_ctx_type_to_string(yap_ctx* ctx, yap_type typ){
  char* res = NULL;
  switch (typ.kind){
    case yap_type_primitive:
      return strus_copy(typ.primitive.name);
    case yap_type_ptr:
      yap_type* sub_type = yap_ctx_get_type(ctx, typ.pointer_type);
      if (!sub_type) return strus_copy("(error getting type)");
      char* sub = yap_ctx_type_to_string(ctx, *sub_type);
      if (!sub) return strus_copy("(error getting type)");
      res = strus_newf("%s@", sub);
      free(sub);
      break;
    case yap_type_func:
      yap_fn_type func = typ.func;
      char* ret_type_str = yap_ctx_type_to_string(ctx, *yap_ctx_get_type(ctx, func.return_type));
      if (!ret_type_str) return strus_copy("(error getting type)");
      res = strus_newf("(%s fn", ret_type_str);
      free(ret_type_str);
      for_darr(i, arg_type_id, typ.func.args){
        yap_type* arg_type = yap_ctx_get_type(ctx, arg_type_id);
        if (!arg_type) {
          free(res);
          return strus_copy("(error getting type)");
        }
        char* arg_type_str = yap_ctx_type_to_string(ctx, *arg_type);
        if (!arg_type_str) {
          free(res);
          return strus_copy("(error getting type)");
        }
        strus_cat(res, " ");
        strus_cat(res, arg_type_str);
        free(arg_type_str);
        if (i != darr_len(typ.func.args) - 1) strus_cat(res, ", ");
      }
      strus_cat(res, ")");
      break;
    case yap_type_struct:
      return strus_newf("struct %s", typ.structure.name ? typ.structure.name : "(anon)");
    case yap_type_union:
      return strus_newf("union %s", typ.uni.name ? typ.uni.name : "(anon)");
    case yap_type_enum:
      return strus_newf("enum %s", typ.enumeration.name ? typ.enumeration.name : "(anon)");
    case yap_type_array: {
      char* elem = yap_ctx_type_to_string(ctx, *yap_ctx_get_type(ctx, typ.array.element_type));
      res = strus_newf("%s[%zu]", elem, typ.array.size);
      free(elem);
      break;
    }
    case yap_type_slice: {
      char* elem = yap_ctx_type_to_string(ctx, *yap_ctx_get_type(ctx, typ.slice.element_type));
      res = strus_newf("%s[]", elem);
      free(elem);
      break;
    }
    case yap_type_blob:
      return strus_newf("blob(%u)", typ.blob.field_count);
    default:
      return strus_copy("(unimplemented type to string)");
  }
  return res;
}

bool yap_ctx_types_eq(yap_ctx* ctx, yap_type left, yap_type right){
  if (!ctx) return false;
  left = yap_ctx_coerce_type(ctx, left);
  right = yap_ctx_coerce_type(ctx, right);
  if (left.kind != right.kind) return false;

  switch(left.kind){
    case yap_type_primitive:
      return left.primitive.bytes == right.primitive.bytes &&
             left.primitive.is_signed == right.primitive.is_signed &&
             left.primitive.is_float == right.primitive.is_float;
    case yap_type_ptr:
      return yap_ctx_type_ids_eq(ctx, left.pointer_type, right.pointer_type);
    case yap_type_func:
      if (darr_len(left.func.args) != darr_len(right.func.args)) return false;
      for (size_t i = 0; i < darr_len(left.func.args); i++){
        if (!yap_ctx_type_ids_eq(ctx, left.func.args[i], right.func.args[i])) return false;
      }
      return yap_ctx_type_ids_eq(ctx, left.func.return_type, right.func.return_type);
    case yap_type_struct:
      /* Named structs are nominally typed: a forward-declaration placeholder
       * (fields not yet built) and the fully-built struct of the same name
       * both get registered as separate type ids, so structural comparison
       * would either dereference a NULL fields array or wrongly call them
       * distinct types. Compare by name when both are named. */
      if (left.structure.name && right.structure.name)
        return strus_eq(left.structure.name, right.structure.name);
      if (darr_len(left.structure.fields) != darr_len(right.structure.fields)) return false;
      for (size_t i = 0; i < darr_len(left.structure.fields); i++){
        if (!yap_ctx_type_ids_eq(ctx, left.structure.fields[i].type, right.structure.fields[i].type)) return false;
      }
      return true;
    case yap_type_union:
      if (left.uni.name && right.uni.name)
        return strus_eq(left.uni.name, right.uni.name);
      if (darr_len(left.uni.variants) != darr_len(right.uni.variants)) return false;
      for (size_t i = 0; i < darr_len(left.uni.variants); i++){
        if (!yap_ctx_type_ids_eq(ctx, left.uni.variants[i].type, right.uni.variants[i].type)) return false;
      }
      return true;
    case yap_type_enum:
      if (left.enumeration.name && right.enumeration.name)
        return strus_eq(left.enumeration.name, right.enumeration.name);
      if (darr_len(left.enumeration.variants) != darr_len(right.enumeration.variants)) return false;
      for (size_t i = 0; i < darr_len(left.enumeration.variants); i++){
        if (!strus_eq(left.enumeration.variants[i].name, right.enumeration.variants[i].name)) return false;
      }
      return true;
    case yap_type_blob:
      return left.blob.field_count == right.blob.field_count;
    case yap_type_untyped:
      return yap_ctx_type_ids_eq(ctx, left.untyped_default, right.untyped_default);
    case yap_type_hole:
      // Two holes are the same type iff same hole_name, so repeated $T in one
      // template dedupes to a single type_id (one :fill_type() closes all of them).
      return strus_eq(left.hole_name, right.hole_name);
    default:
      return false;
  }
}

char* yap_ctx_type_to_mangle_string(yap_ctx* ctx, yap_type typ){
  return yap_ctx_mangle_type(ctx, typ, (yap_type_qualifier_strings){
    .restrict_str = "r",
    .volatile_str = "V",
    .const_str = "K",
  });
}

char* yap_ctx_mangle_type(yap_ctx* ctx, yap_type typ, yap_type_qualifier_strings qs){
  const char* const_str = (typ.is_const ? qs.const_str : "");
  char* res;
  switch(typ.kind){
    case yap_type_primitive:
      yap_log("Getting mangle string for primitive type with mangled name '%s'", typ.primitive.mangled_name);
      return strus_newf("%s%s", const_str, typ.primitive.mangled_name);
    case yap_type_ptr: {
      yap_log("Getting mangle string for pointer type");
      yap_type* sub_type = yap_ctx_get_type(ctx, typ.pointer_type);
      if (!sub_type) return NULL;
      char* sub = yap_ctx_type_to_mangle_string(ctx, *sub_type);
      if (!sub) return NULL;
      res = strus_newf("%sP%s", const_str, sub);
      free(sub);
      return res;
    }
    case yap_type_func: {
      yap_log("Getting mangle string for function type");
      res = strus_newf("%sPF", const_str); //Function Pointer
      //Append mangle string of return type
      yap_type* return_type = yap_ctx_get_type(ctx, typ.func.return_type);
      if (!return_type) {
        free(res);
        return NULL;
      }
      char* return_type_str = yap_ctx_type_to_mangle_string(ctx, *return_type);
      if (!return_type_str) {
        free(res);
        return NULL;
      }
      strus_cat(res, return_type_str);
      free(return_type_str);
      //Append mangle string of parameter types
      for_darr(i, param_type_id, typ.func.args){
        yap_type* arg_type = yap_ctx_get_type(ctx, param_type_id);
        if (!arg_type) {
          free(res);
          return NULL;
        }
        char* arg_type_str = yap_ctx_type_to_mangle_string(ctx, *arg_type);
        if (!arg_type_str) {
          free(res);
          return NULL;
        }
        strus_cat(res, arg_type_str);
        free(arg_type_str);
      }
      strus_cat(res, "E"); //End of function type
      return res;
    }
    case yap_type_struct:
      yap_log("Getting mangle string for struct type");
      //TODO: Change to correct pattern: NXStructNameE where X is the length of the struct name. For now we just use the struct name as the mangle string, which is simpler but can cause conflicts and doesn't allow for anonymous structs.
      char* struct_name = typ.structure.name ? typ.structure.name : typ.structure.c_name;
      return strus_newf("%s%d%s", const_str, (int)strlen(struct_name), struct_name);
    case yap_type_union:
      yap_log("Getting mangle string for union type");
      char* union_name = typ.uni.name ? typ.uni.name : typ.uni.c_name;
      return strus_newf("%sU%d%s", const_str, (int)strlen(union_name), union_name);
    case yap_type_enum:
      yap_log("Getting mangle string for enum type");
      char* enum_name = typ.enumeration.name ? typ.enumeration.name : typ.enumeration.c_name;
      return strus_newf("%sN%d%s", const_str, (int)strlen(enum_name), enum_name);
    case yap_type_untyped:
      yap_log("Getting mangle string for untyped type");
      return yap_ctx_type_to_mangle_string(ctx, yap_ctx_coerce_type(ctx, typ));
    case yap_type_array: {
      yap_type* elem = yap_ctx_get_type(ctx, typ.array.element_type);
      if (!elem) return NULL;
      char* elem_str = yap_ctx_type_to_mangle_string(ctx, *elem);
      if (!elem_str) return NULL;
      res = strus_newf("%sA%zu_%s", const_str, typ.array.size, elem_str);
      free(elem_str);
      return res;
    }
    case yap_type_slice: {
      yap_type* elem = yap_ctx_get_type(ctx, typ.slice.element_type);
      if (!elem) return NULL;
      char* elem_str = yap_ctx_type_to_mangle_string(ctx, *elem);
      if (!elem_str) return NULL;
      res = strus_newf("%sS%s", const_str, elem_str);
      free(elem_str);
      return res;
    }
    case yap_type_blob:
      return strus_newf("%sB%u", const_str, typ.blob.field_count);
    case yap_type_hole:
      // Not real codegen-able C -- only used as a yap_ctx_insert_type_if_not_exists
      // interning key, so repeated $T in one template naturally dedupes to the
      // same type_id (name lookup hits before a second type gets pushed).
      return strus_newf("%sH%s", const_str, typ.hole_name);
    default:
      return NULL;
  }
}

yap_type_id yap_ctx_insert_type_if_not_exists(yap_ctx* ctx, yap_type typ){
  char* name = NULL;
  switch(typ.kind){
    default:
      char* temp_name = yap_ctx_type_to_mangle_string(ctx, typ);
      name = yap_ctx_strus_cpy(ctx, temp_name);
      free(temp_name);
      break;
  }
  if (!name) return 0;
  yap_log("Inserting type '%s' if it does not exist", name);
  yap_type_id existing_id = yap_ctx_get_type_id_by_name(ctx, name);
  if (existing_id){
    yap_log("Type '%s' already exists with id %u", name, existing_id);
    return existing_id;
  }
  yap_log("Type '%s' does not exist", name);
  yap_type_id id = yap_ctx_push_type(ctx, typ);
  yap_named_type named = {
    .id = id,
    .name = name,
    .c_name = NULL,
  };
  hashmap_set(ctx->named_types, &named);
  return id;
}

yap_type yap_empty_type(yap_type_kind kind){
    return (yap_type){
        .kind=kind,
        .is_const=false
    };
}

yap_type_id yap_ctx_get_pointer_of_type_id(yap_ctx* ctx, yap_type_id id){
  yap_type ptr_type = (yap_type){
    .kind=yap_type_ptr,
    .pointer_type=id
  };
  return yap_ctx_insert_type_if_not_exists(ctx, ptr_type);
}

yap_type_id yap_ctx_get_slice_of_type_id(yap_ctx* ctx, yap_type_id id){
  yap_type slice_type = (yap_type){
    .kind=yap_type_slice,
    .slice={.element_type=id}
  };
  return yap_ctx_insert_type_if_not_exists(ctx, slice_type);
}

yap_type_id yap_ctx_find_member_type(yap_ctx* ctx, yap_type_id object_type_id, const char* member_name){
  yap_type* object_type = yap_ctx_get_type(ctx, object_type_id);
  if (!object_type) return 0;
  switch(object_type->kind){
    case yap_type_struct:
      for_darr(i, field, object_type->structure.fields){
        // Anonymous embedded struct/union — flatten fields recursively
        if (!field.name || field.name[0] == '\0'){
          yap_type_id nested = yap_ctx_find_member_type(ctx, field.type, member_name);
          if (nested != ctx->internal_error_type_id) return nested;
          continue;
        }
        // Direct field match
        if (strus_eq(field.name, member_name)){
          return field.type;
        }
      }
      break;
    case yap_type_union:
      for_darr(i, variant, object_type->uni.variants){
        // Anonymous embedded — flatten
        if (!variant.name || variant.name[0] == '\0'){
          yap_type_id nested = yap_ctx_find_member_type(ctx, variant.type, member_name);
          if (nested != ctx->internal_error_type_id) return nested;
          continue;
        }
        // Direct variant match
        if (strus_eq(variant.name, member_name)){
          return variant.type;
        }
      }
      break;
    default:
      return ctx->internal_error_type_id;
  }
  return ctx->internal_error_type_id;
}

yap_type_id yap_push_blob_type(yap_ctx* ctx, size_t field_count){
  yap_type blob_type = (yap_type){
    .kind = yap_type_blob,
    .blob = {
      .field_count = field_count
    }
  };
  return yap_ctx_insert_type_if_not_exists(ctx, blob_type);
}

char* yap_ctx_get_anon_name(yap_ctx* ctx, const char* t_name, yap_anon_id anon_id){
  return yap_ctx_strus_newf(ctx, "__anon_%s_%lu", t_name, anon_id);
}

yap_source* find_source_by_identity(yap_ctx* ctx, const char* identity){
    if (!ctx || !identity) return NULL;
    for (size_t i = 0; i < darr_len(ctx->sources); i++){
        yap_source* s = ctx->sources[i];
        if (s && s->identity && strcmp(s->identity, identity) == 0)
            return s;
    }
    return NULL;
}

static void print_source_subtree(yap_ctx* ctx, yap_source* src, const char* prefix){
    size_t n = darr_len(src->imports);
    for (size_t i = 0; i < n; i++){
        bool is_last = (i == n - 1);
        yap_import imp = src->imports[i];

        printf("%s%s── ", prefix, is_last ? "└" : "├");

        if (imp.kind == yap_import_module){
            printf("[module] %s\n", imp.module_name ? imp.module_name : "(unknown)");
        } else {
            // file import
            yap_source* child = find_source_by_identity(ctx, imp.identity);
            if (child){
                const char* identity_label = child->identity ? child->identity : "(unknown)";
                const char* path_label = child->label ? child->label : "(unknown)";
                printf("%s [%s]\n", path_label, identity_label);
                char child_prefix[4096];
                snprintf(child_prefix, sizeof(child_prefix),
                         "%s%s   ", prefix, is_last ? " " : "│");
                print_source_subtree(ctx, child, child_prefix);
            }else{
              printf("error\n");
            }
        }
    }
}

void yap_ctx_print_source_tree(yap_ctx* ctx){
    if (!ctx || !ctx->root_source) return;

    printf("\nSource tree:\n");
    printf("%s\n", ctx->root_source->identity
                      ? ctx->root_source->identity
                      : "(root)");
    print_source_subtree(ctx, ctx->root_source, "");
}