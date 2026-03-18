#include "yap/all.h"

declare_map_for(named_type);

yap_ctx* yap_ctx_new(){
    yap_log("Creating new ctx");
    yap_ctx* ctx = mem_one_cpy(((yap_ctx){
      .sources=darr_new(yap_source),
      .source_codes=darr_new(yap_source_code),
      .scopes=darr_new(yap_scope*),
      .errors=darr_new(yap_error),
      .types=darr_new(yap_type), //yap_type_id points to types in this array
      .named_types=new_named_type_map(),
    }));
    darr_push(ctx->scopes, yap_new_scope(NULL));
    //Default types (requires <stdint.h> for fixed width integer types and <stdbool.h> for bool)
    ctx->bool_type_id = yap_ctx_push_named_type(ctx, "bool", "bool", yap_primitive_type(1, false, false));
    yap_ctx_push_named_type(ctx, "byte", "char", yap_primitive_type(1, false, false));
    ctx->int_type_id = yap_ctx_push_named_type(ctx, "i32", "int32_t", yap_primitive_type(4, true, false));
    yap_ctx_push_named_type(ctx, "u32", "uint32_t", yap_primitive_type(4, false, false));
    yap_ctx_push_named_type(ctx, "i64", "int64_t", yap_primitive_type(8, true, false));
    yap_ctx_push_named_type(ctx, "u64", "uint64_t", yap_primitive_type(8, false, false));
    ctx->float_type_id = yap_ctx_push_named_type(ctx, "f32", "float", yap_primitive_type(4, true, true));
    yap_ctx_push_named_type(ctx, "f64", "double", yap_primitive_type(8, true, true));
    
    //Untyped default types for literal coercion
    ctx->untyped_int_type_id = yap_ctx_push_type(ctx, yap_untyped_type(ctx->int_type_id));
    ctx->untyped_float_type_id = yap_ctx_push_type(ctx, yap_untyped_type(ctx->float_type_id));
    ctx->untyped_byte_type_id = yap_ctx_push_type(ctx, yap_untyped_type(ctx->bool_type_id));
    
    return ctx;
}
yap_scope* yap_ctx_current_scope(yap_ctx* ctx){
    if (darr_len(ctx->scopes) == 0) return NULL;
    return darr_last(ctx->scopes);
}

yap_type yap_primitive_type(size_t bytes, bool is_signed, bool is_float){
  return (yap_type){
    .kind = yap_type_primitive,
    .primitive = (yap_prim_type){
      .bytes = bytes,
      .is_signed = is_signed,
      .is_float = is_float
    }
  };
}

yap_source_code yap_source_code_new(){
  yap_log("Creating new source code");
  return (yap_source_code){
    .definitions=darr_new(yap_def)
  };
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
  if (!named) return -1;
  return named->id;
}

yap_type yap_untyped_type(yap_type_id default_id){
  return (yap_type){
    .kind = yap_type_untyped,
    .untyped_default = default_id
  };
}

yap_type_id yap_ctx_push_named_type(yap_ctx* ctx, char* name_p, char* c_name_p, yap_type typ){
  char* name = strus_copy(name_p);
  char* c_name = c_name_p ? strus_copy(c_name_p) : NULL;
  yap_type_id id = yap_ctx_push_type(ctx, typ);
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
      return yap_ctx_type_ids_eq(ctx, left.func.ret, right.func.ret);
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