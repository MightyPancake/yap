#ifndef YAP_CONTEXT_H
#define YAP_CONTEXT_H

//Basic
yap_scope* yap_ctx_current_scope(yap_ctx* ctx);
void yap_ctx_push_source(yap_ctx* st, yap_source src);
yap_source yap_ctx_pop_source(yap_ctx* st);
void yap_ctx_push_error(yap_ctx* st, yap_error err);
yap_type_id yap_ctx_push_type(yap_ctx* ctx, yap_type typ);
yap_type* yap_ctx_get_type(yap_ctx* ctx, yap_type_id id);
yap_type* yap_ctx_get_type_by_name(yap_ctx* ctx, char* name);
yap_type_id yap_ctx_get_type_id_by_name(yap_ctx* ctx, char* name);
yap_type_id yap_ctx_push_named_type(yap_ctx* ctx, char* name, char* c_name, yap_type typ);
yap_type yap_primitive_type(size_t bytes, bool is_signed, bool is_float);
yap_type_id yap_ctx_coerce_type_id_to_id(yap_ctx* ctx, yap_type_id src_id);
yap_type yap_ctx_coerce_type(yap_ctx* ctx, yap_type src);
yap_type yap_untyped_type(yap_type_id default_id);
bool yap_ctx_type_ids_eq(yap_ctx* ctx, yap_type_id left_id, yap_type_id right_id);
bool yap_ctx_types_eq(yap_ctx* ctx, yap_type left, yap_type right);


#endif //YAP_CONTEXT_H
