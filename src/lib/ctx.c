#include "yap/all.h"

declare_map_for(named_type);
declare_map_for(module);

yap_ctx* yap_ctx_new(){
    yap_log("Creating new ctx");
    yap_ctx* ctx = mem_one_cpy(((yap_ctx){
      .arena = quake_new(),
      .sources=darr_new(yap_source),
      .source_codes=darr_new(yap_source_code),
      .scopes=darr_new(yap_scope*),
      .current_scopes=darr_new(yap_scope*),
      .errors=darr_new(yap_error),
      .modules=new_module_map(),
      .current_module=NULL,
      .types=darr_new(yap_type), //yap_type_id points to types in this array
      .named_types=new_named_type_map(),
    }));
    yap_ctx_push_new_scope(ctx); //Push global scope
    ctx->global_scope = yap_ctx_current_scope(ctx);
    yap_ctx_create_new_module(ctx, "main");
    yap_ctx_switch_module(ctx, "main");
    //Default types (requires <stdint.h> for fixed width integer types and <stdbool.h> for bool)
    yap_ctx_push_new_primitive_type(ctx, 0, false, false, "none", "v", "void"); //This is a dummy type used for invalid/empty types. Basically, we can return 0 for error in this case
    ctx->void_type_id = yap_ctx_push_new_primitive_type(ctx, 0, false, false, "none", "v", "void");
    ctx->bool_type_id = yap_ctx_push_new_primitive_type(ctx, 1, false, false, "bool", "b", "bool");
    yap_ctx_push_new_primitive_type(ctx, 1, false, false, "byte", "c", "char");
    ctx->int_type_id = yap_ctx_push_new_primitive_type(ctx, 4, true, false, "i32", "i32", "int32_t");
    yap_ctx_push_new_primitive_type(ctx, 4, false, false, "u32", "u32", "uint32_t");
    yap_ctx_push_new_primitive_type(ctx, 8, true, false, "i64", "i64", "int64_t");
    yap_ctx_push_new_primitive_type(ctx, 8, false, false, "u64", "u64", "uint64_t");
    ctx->float_type_id = yap_ctx_push_new_primitive_type(ctx, 4, true, true, "f32", "f32", "float");
    yap_ctx_push_new_primitive_type(ctx, 8, true, true, "f64", "f64", "double");

    //Untyped default types for literal coercion
    ctx->untyped_int_type_id = yap_ctx_push_type(ctx, yap_untyped_type(ctx->int_type_id));
    ctx->untyped_float_type_id = yap_ctx_push_type(ctx, yap_untyped_type(ctx->float_type_id));
    ctx->untyped_byte_type_id = yap_ctx_push_type(ctx, yap_untyped_type(ctx->bool_type_id));
    
    return ctx;
}

yap_module* yap_ctx_get_module(yap_ctx* ctx, char* name){
  if (!ctx || !name) return NULL;
  const yap_module dummy = {.name = name};
  return (yap_module*)hashmap_get(ctx->modules, &dummy);
}

yap_module* yap_ctx_create_new_module(yap_ctx* ctx, char* name){
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

  yap_scope* module_scope = yap_ctx_new_scope(ctx, ctx->global_scope);
  if (!module_scope) return NULL;

  yap_module new_module = {
    .name = yap_ctx_strus_cpy(ctx, name),
    .scope = module_scope
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

  while (darr_len(ctx->current_scopes) > 0){
    darr_pop(ctx->current_scopes);
  }

  darr_push(ctx->current_scopes, ctx->global_scope);
  darr_push(ctx->current_scopes, module->scope);
  ctx->current_module = module;
  return module;
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

yap_source_code yap_source_code_new(){
  yap_log("Creating new source code");
  return (yap_source_code){};
}

void yap_ctx_push_source(yap_ctx* ctx, yap_source src){
  darr_push(ctx->sources, src);
}

yap_source yap_ctx_pop_source(yap_ctx* ctx){
  return darr_pop(ctx->sources);
}

void yap_ctx_push_error(yap_ctx* ctx, yap_error err){
  if (!ctx) return;
  darr_push(ctx->errors, err);
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
  char* name = yap_ctx_strus_cpy(ctx, name_p);
  char* c_name = c_name_p ? yap_ctx_strus_cpy(ctx, c_name_p) : NULL;
  yap_type_id id = yap_ctx_push_type(ctx, typ);
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
  if(coerced_t1.kind == yap_type_primitive && t2.kind == yap_type_primitive) return true;
  return false;
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
      if (darr_len(left.structure.fields) != darr_len(right.structure.fields)) return false;
      for (size_t i = 0; i < darr_len(left.structure.fields); i++){
        if (!yap_ctx_type_ids_eq(ctx, left.structure.fields[i], right.structure.fields[i])) return false;
      }
      return true;
    case yap_type_blob:
      return left.blob.field_count == right.blob.field_count;
    case yap_type_untyped:
      return yap_ctx_type_ids_eq(ctx, left.untyped_default, right.untyped_default);
    default:
      return false;
  }
}

char* yap_ctx_type_to_mangle_string(yap_ctx* ctx, yap_type typ){
  char* res;
  switch(typ.kind){
    case yap_type_primitive:
      yap_log("Getting mangle string for primitive type with mangled name '%s'", typ.primitive.mangled_name);
      return strus_copy(typ.primitive.mangled_name);
    case yap_type_ptr: {
      yap_log("Getting mangle string for pointer type");
      yap_type* sub_type = yap_ctx_get_type(ctx, typ.pointer_type);
      if (!sub_type) return NULL;
      char* sub = yap_ctx_type_to_mangle_string(ctx, *sub_type);
      if (!sub) return NULL;
      res = strus_newf("P%s", sub);
      free(sub);
      return res;
    }
    case yap_type_func: {
      yap_log("Getting mangle string for function type");
      res = strus_copy("FP"); //Function Pointer
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
      return strus_newf("%d%s", (int)strlen(typ.structure.name), typ.structure.name);
    case yap_type_untyped:
      yap_log("Getting mangle string for untyped type");
      return yap_ctx_type_to_mangle_string(ctx, yap_ctx_coerce_type(ctx, typ));
    case yap_type_blob:
    default:
      return NULL;
  }
}

yap_type_id yap_ctx_insert_type_if_not_exists(yap_ctx* ctx, yap_type typ){
  char* name = NULL;
  switch(typ.kind){
    case yap_type_primitive:
      name = yap_ctx_strus_cpy(ctx, typ.primitive.mangled_name);
      break;
    case yap_type_struct:
      name = yap_ctx_strus_cpy(ctx, typ.structure.name);
      break;
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
  return yap_ctx_push_named_type(ctx, name, NULL, typ);
}