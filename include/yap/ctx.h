#ifndef YAP_CONTEXT_H
#define YAP_CONTEXT_H

//Basic
void yap_ctx_push_var(yap_ctx* ctx, yap_var var);

//Scope manipulation
yap_scope* yap_ctx_current_scope(yap_ctx* ctx);
yap_scope* yap_ctx_new_scope(yap_ctx* ctx, yap_scope* parent);
yap_scope* yap_ctx_push_new_scope(yap_ctx* ctx);
yap_scope* yap_ctx_push_new_loop_scope(yap_ctx* ctx);
yap_scope* yap_ctx_pop_scope(yap_ctx* ctx);

//Module manipulation
yap_module* yap_ctx_get_module(yap_ctx* ctx, char* name);
yap_module* yap_ctx_create_new_module(yap_ctx* ctx, char* name);
yap_module* yap_ctx_switch_module(yap_ctx* ctx, char* name);

//Source manipulation
void yap_ctx_push_source(yap_ctx* st, yap_source src);
yap_source yap_ctx_pop_source(yap_ctx* st);

//Types
yap_type_id yap_ctx_push_type(yap_ctx* ctx, yap_type typ);
yap_type* yap_ctx_get_type(yap_ctx* ctx, yap_type_id id);
yap_type* yap_ctx_get_type_by_name(yap_ctx* ctx, char* name);
yap_type_id yap_ctx_get_type_id_by_name(yap_ctx* ctx, char* name);
yap_type_id yap_ctx_push_named_type(yap_ctx* ctx, char* name, char* c_name, yap_type typ);
yap_type_id yap_ctx_push_new_primitive_type(yap_ctx* ctx, size_t bytes, bool is_signed, bool is_float, char* name, char* mangled_name, char* c_name);
yap_type yap_primitive_type(size_t bytes, bool is_signed, bool is_float, char* name, char* mangled_name, char* c_name);
yap_type_id yap_ctx_coerce_type_id_to_id(yap_ctx* ctx, yap_type_id src_id);
yap_type yap_ctx_coerce_type(yap_ctx* ctx, yap_type src);
yap_type yap_untyped_type(yap_type_id default_id);
bool yap_ctx_type_ids_eq(yap_ctx* ctx, yap_type_id left_id, yap_type_id right_id);
bool yap_ctx_types_eq(yap_ctx* ctx, yap_type left, yap_type right);
bool yap_ctx_type_compatible(yap_ctx* ctx, yap_type type1, yap_type type2);
bool yap_ctx_type_id_compatible(yap_ctx* ctx, yap_type_id id1, yap_type_id id2);
char* yap_ctx_type_id_to_string(yap_ctx* ctx, yap_type_id id);
char* yap_ctx_type_to_string(yap_ctx* ctx, yap_type typ);
char* yap_ctx_type_to_mangle_string(yap_ctx* ctx, yap_type typ);
yap_type_id yap_ctx_insert_type_if_not_exists(yap_ctx* ctx, yap_type typ);

//Errors
void yap_ctx_push_error(yap_ctx* st, yap_error err);
bool yap_ctx_dispatch_errors(yap_ctx* ctx);

//Memory
void* yap_ctx_malloc(yap_ctx* ctx, size_t bytes);
void* yap_ctx_one_raw(yap_ctx* ctx, size_t bytes);
void* yap_ctx_one_cpy_raw(yap_ctx* ctx, const void* src, size_t bytes);

#define yap_ctx_one(CTX, T) ((T*)yap_ctx_one_raw((CTX), sizeof(T)))
#define yap_ctx_one_cpy(CTX, V) ({ \
	__typeof__(V)* _yap_ctx_one_cpy_tmp = yap_ctx_one((CTX), __typeof__(V)); \
	if (_yap_ctx_one_cpy_tmp) *_yap_ctx_one_cpy_tmp = (V); \
	_yap_ctx_one_cpy_tmp; \
})

char* yap_ctx_strus_newf(yap_ctx* ctx, const char* fmt, ...);
char* yap_ctx_strus_cpy(yap_ctx* ctx, char* src);

#define yap_ctx_darr_new(CTX, T, ...) quake_darr_new(&(CTX)->arena, T, ##__VA_ARGS__)

#endif //YAP_CONTEXT_H
