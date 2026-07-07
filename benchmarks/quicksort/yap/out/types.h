#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifndef __YAP_EXPRLIST_DEFINED
#define __YAP_EXPRLIST_DEFINED
typedef struct { void** data; unsigned long len; } yExprList;
#endif
#ifdef __TINYC__
extern void* yapi_int(int value);
extern void* yapi_float(double value);
extern void* yapi_string(const char* value);
extern void* yapi_bool(int value);
extern void* yapi_var_value(const char* ident);
extern void* yapi_new_var(void* type_id, const char* ident);
extern void* yapi_bin_op(void* left, int op, void* right);
extern void* yapi_neg(void* expr);
extern void* yapi_not(void* expr);
extern void* yapi_bnot(void* expr);
extern void* yapi_ternary(void* cond, void* then_expr, void* else_expr);
extern void* yapi_assign(void* lval, int op, void* rval);
extern void* yapi_member(void* obj, const char* field);
extern void* yapi_opt_member(void* obj, const char* field);
extern void* yapi_index(void* obj, void* idx);
extern void* yapi_cast(void* expr, void* type_id);
extern void* yapi_deref(void* expr);
extern void* yapi_addr_of(void* expr);
extern void* yapi_increment(void* expr, int prefix);
extern void* yapi_decrement(void* expr, int prefix);
extern void* yapi_ptr_of(void* type_id);
extern void* yapi_slice_of(void* type_id);
extern void* yapi_array_of(void* type_id, int size);
extern void* yapi_type_of(void* expr);
extern void* yapi_pointee_type(void* type_id);
extern void* yapi_field_type(void* type_id, const char* name);
extern void* yapi_sizeof(void* type_id);
extern void* yapi_call0(void* func);
extern void* yapi_call1(void* func, void* a);
extern void* yapi_call2(void* func, void* a, void* b);
extern void* yapi_call3(void* func, void* a, void* b, void* c);
extern void* yapi_call_args_new(void);
extern void* yapi_call_args_push(void* list, void* expr);
extern void* yapi_call(void* func, void* args_list);
extern int yapi_kind(void* expr);
extern int yapi_is_comptime(void* expr);
extern void* yapi_var_decl(void* type_id, const char* ident);
extern void* yapi_expr_stmt(void* expr);
extern void* yapi_return_stmt(void* expr);
extern void* yapi_if_stmt(void* cond, void* then_stmt);
extern void* yapi_if_else_stmt(void* cond, void* then_stmt, void* else_stmt);
extern void* yapi_while_stmt(void* cond, void* body_stmt);
extern void* yapi_for_stmt(void* init_stmt, void* cond, void* update, void* body_stmt);
extern void* yapi_break_stmt(void);
extern void* yapi_continue_stmt(void);
extern void* yapi_block(void* stmts_list);
extern void* yapi_block_expr(void* stmts_list);
extern void* yapi_uniq(void);
extern const char* yapi_uniq_name(void);
extern void* yapi_stmt_list_new(void);
extern void* yapi_stmt_list_push(void* list, void* stmt);
extern void* yapi_struct_t(void);
extern void* yapi_enum_t(void);
extern void* yapi_union_t(void);
extern void* yapi_fn_t(void);
extern void* yapi_type(const char* name);
extern void* yapi_fn_type0(void* ret);
extern void* yapi_fn_type1(void* ret, void* p1);
extern void* yapi_fn_type2(void* ret, void* p1, void* p2);
extern void* yapi_fn_type3(void* ret, void* p1, void* p2, void* p3);
extern int yapi_type_exists(const char* name);
extern int yapi_func_exists(const char* name);
extern void yapi_log(const char* msg);
extern void yapi_error(const char* msg);
extern void yapi_warn(const char* msg);
extern void yapi_register_macro_method(void* owner_type, const char* name, const char* backing_fn_name);
extern void* yapi_hole(const char* name);
extern void* yapi_hole_stmt(const char* name);
extern void* yapi_type_hole(const char* name);
extern const char* yapi_ident_hole(const char* name);
extern void* yStructT_add_field(void* b, void* type_id, const char* name);
extern void* yStructT_finish(void* b, const char* name);
extern int yStructT_existed(void* b);
extern void* yStructT_type(void* b);
extern void* yEnumT_add_variant(void* b, const char* name);
extern void* yEnumT_add_variant_value(void* b, const char* name, void* value);
extern void* yEnumT_finish(void* b, const char* name);
extern int yEnumT_existed(void* b);
extern void* yEnumT_type(void* b);
extern void* yUnionT_add_field(void* b, void* type_id, const char* name);
extern void* yUnionT_finish(void* b, const char* name);
extern int yUnionT_existed(void* b);
extern void* yUnionT_type(void* b);
extern void* yFnT_add_param(void* b, void* type_id, const char* name);
extern void yFnT_set_return_type(void* b, void* type_id);
extern void yFnT_set_body(void* b, void* stmt);
extern void* yFnT_finish(void* b, const char* name);
extern int yFnT_existed(void* b);
extern void* yFnT_func(void* b);
extern void* yFnT_get_subject(void* b);
extern void* yFn_ref(void* fn);
extern void* yType_new_method(void* type_id);
extern void* yType_new_ref_method(void* type_id);
extern void* yExprBlueprint_fill_expr(void* self, const char* name, void* value);
extern void* yExprBlueprint_fill_type(void* self, const char* name, void* type_id);
extern void* yExprBlueprint_finish(void* self);
extern void* yStmtBlueprint_fill_expr(void* self, const char* name, void* value);
extern void* yStmtBlueprint_fill_stmt(void* self, const char* name, void* value);
extern void* yStmtBlueprint_fill_type(void* self, const char* name, void* type_id);
extern void* yStmtBlueprint_fill_ident(void* self, const char* name, const char* ident);
extern void* yStmtBlueprint_fill_var(void* self, const char* name, void* type_id, const char* ident);
extern void* yStmtBlueprint_finish(void* self);
#else
static inline void* yapi_int(int v){(void)v;return 0;}
static inline void* yapi_float(double v){(void)v;return 0;}
static inline void* yapi_string(const char* v){(void)v;return 0;}
static inline void* yapi_bool(int v){(void)v;return 0;}
static inline void* yapi_var_value(const char* v){(void)v;return 0;}
static inline void* yapi_new_var(void* t,const char* n){(void)t;(void)n;return 0;}
static inline void* yapi_bin_op(void* l,int o,void* r){(void)l;(void)o;(void)r;return 0;}
static inline void* yapi_neg(void* e){(void)e;return 0;}
static inline void* yapi_not(void* e){(void)e;return 0;}
static inline void* yapi_bnot(void* e){(void)e;return 0;}
static inline void* yapi_ternary(void* c,void* t,void* f){(void)c;(void)t;(void)f;return 0;}
static inline void* yapi_assign(void* l,int o,void* r){(void)l;(void)o;(void)r;return 0;}
static inline void* yapi_member(void* o,const char* f){(void)o;(void)f;return 0;}
static inline void* yapi_opt_member(void* o,const char* f){(void)o;(void)f;return 0;}
static inline void* yapi_index(void* o,void* i){(void)o;(void)i;return 0;}
static inline void* yapi_cast(void* e,void* t){(void)e;(void)t;return 0;}
static inline void* yapi_deref(void* e){(void)e;return 0;}
static inline void* yapi_addr_of(void* e){(void)e;return 0;}
static inline void* yapi_increment(void* e,int p){(void)e;(void)p;return 0;}
static inline void* yapi_decrement(void* e,int p){(void)e;(void)p;return 0;}
static inline void* yapi_ptr_of(void* t){(void)t;return 0;}
static inline void* yapi_slice_of(void* t){(void)t;return 0;}
static inline void* yapi_array_of(void* t,int s){(void)t;(void)s;return 0;}
static inline void* yapi_type_of(void* e){(void)e;return 0;}
static inline void* yapi_pointee_type(void* t){(void)t;return 0;}
static inline void* yapi_field_type(void* t,const char* n){(void)t;(void)n;return 0;}
static inline void* yapi_sizeof(void* t){(void)t;return 0;}
static inline void* yapi_call0(void* f){(void)f;return 0;}
static inline void* yapi_call1(void* f,void* a){(void)f;(void)a;return 0;}
static inline void* yapi_call2(void* f,void* a,void* b){(void)f;(void)a;(void)b;return 0;}
static inline void* yapi_call3(void* f,void* a,void* b,void* c){(void)f;(void)a;(void)b;(void)c;return 0;}
static inline void* yapi_call_args_new(void){return 0;}
static inline void* yapi_call_args_push(void* l,void* e){(void)l;(void)e;return 0;}
static inline void* yapi_call(void* f,void* a){(void)f;(void)a;return 0;}
static inline int yapi_kind(void* e){(void)e;return 0;}
static inline int yapi_is_comptime(void* e){(void)e;return 0;}
static inline void* yapi_var_decl(void* t,const char* n){(void)t;(void)n;return 0;}
static inline void* yapi_expr_stmt(void* e){(void)e;return 0;}
static inline void* yapi_return_stmt(void* e){(void)e;return 0;}
static inline void* yapi_if_stmt(void* c,void* t){(void)c;(void)t;return 0;}
static inline void* yapi_if_else_stmt(void* c,void* t,void* e){(void)c;(void)t;(void)e;return 0;}
static inline void* yapi_while_stmt(void* c,void* b){(void)c;(void)b;return 0;}
static inline void* yapi_for_stmt(void* i,void* c,void* u,void* b){(void)i;(void)c;(void)u;(void)b;return 0;}
static inline void* yapi_break_stmt(void){return 0;}
static inline void* yapi_continue_stmt(void){return 0;}
static inline void* yapi_block(void* s){(void)s;return 0;}
static inline void* yapi_block_expr(void* s){(void)s;return 0;}
static inline void* yapi_uniq(void){return 0;}
static inline const char* yapi_uniq_name(void){return "";}
static inline void* yapi_stmt_list_new(void){return 0;}
static inline void* yapi_stmt_list_push(void* l,void* s){(void)l;(void)s;return 0;}
static inline void* yapi_struct_t(void){return 0;}
static inline void* yapi_enum_t(void){return 0;}
static inline void* yapi_union_t(void){return 0;}
static inline void* yapi_fn_t(void){return 0;}
static inline void* yapi_type(const char* n){(void)n;return 0;}
static inline void* yapi_fn_type0(void* r){(void)r;return 0;}
static inline void* yapi_fn_type1(void* r,void* a){(void)r;(void)a;return 0;}
static inline void* yapi_fn_type2(void* r,void* a,void* b){(void)r;(void)a;(void)b;return 0;}
static inline void* yapi_fn_type3(void* r,void* a,void* b,void* c){(void)r;(void)a;(void)b;(void)c;return 0;}
static inline int yapi_type_exists(const char* n){(void)n;return 0;}
static inline int yapi_func_exists(const char* n){(void)n;return 0;}
static inline void yapi_log(const char* m){(void)m;}
static inline void yapi_error(const char* m){(void)m;}
static inline void yapi_warn(const char* m){(void)m;}
static inline void yapi_register_macro_method(void* t,const char* n,const char* f){(void)t;(void)n;(void)f;}
static inline void* yapi_hole(const char* n){(void)n;return 0;}
static inline void* yapi_hole_stmt(const char* n){(void)n;return 0;}
static inline void* yapi_type_hole(const char* n){(void)n;return 0;}
static inline const char* yapi_ident_hole(const char* n){(void)n;return "";}
static inline void* yStructT_add_field(void* b,void* t,const char* n){(void)b;(void)t;(void)n;return 0;}
static inline void* yStructT_finish(void* b,const char* n){(void)b;(void)n;return 0;}
static inline int yStructT_existed(void* b){(void)b;return 0;}
static inline void* yStructT_type(void* b){(void)b;return 0;}
static inline void* yEnumT_add_variant(void* b,const char* n){(void)b;(void)n;return 0;}
static inline void* yEnumT_add_variant_value(void* b,const char* n,void* v){(void)b;(void)n;(void)v;return 0;}
static inline void* yEnumT_finish(void* b,const char* n){(void)b;(void)n;return 0;}
static inline int yEnumT_existed(void* b){(void)b;return 0;}
static inline void* yEnumT_type(void* b){(void)b;return 0;}
static inline void* yUnionT_add_field(void* b,void* t,const char* n){(void)b;(void)t;(void)n;return 0;}
static inline void* yUnionT_finish(void* b,const char* n){(void)b;(void)n;return 0;}
static inline int yUnionT_existed(void* b){(void)b;return 0;}
static inline void* yUnionT_type(void* b){(void)b;return 0;}
static inline void* yFnT_add_param(void* b,void* t,const char* n){(void)b;(void)t;(void)n;return 0;}
static inline void yFnT_set_return_type(void* b,void* t){(void)b;(void)t;}
static inline void yFnT_set_body(void* b,void* s){(void)b;(void)s;}
static inline void* yFnT_finish(void* b,const char* n){(void)b;(void)n;return 0;}
static inline int yFnT_existed(void* b){(void)b;return 0;}
static inline void* yFnT_func(void* b){(void)b;return 0;}
static inline void* yFnT_get_subject(void* b){(void)b;return 0;}
static inline void* yFn_ref(void* f){(void)f;return 0;}
static inline void* yType_new_method(void* t){(void)t;return 0;}
static inline void* yType_new_ref_method(void* t){(void)t;return 0;}
static inline void* yExprBlueprint_fill_expr(void* s,const char* n,void* v){(void)s;(void)n;(void)v;return 0;}
static inline void* yExprBlueprint_fill_type(void* s,const char* n,void* t){(void)s;(void)n;(void)t;return 0;}
static inline void* yExprBlueprint_finish(void* s){(void)s;return 0;}
static inline void* yStmtBlueprint_fill_expr(void* s,const char* n,void* v){(void)s;(void)n;(void)v;return 0;}
static inline void* yStmtBlueprint_fill_stmt(void* s,const char* n,void* v){(void)s;(void)n;(void)v;return 0;}
static inline void* yStmtBlueprint_fill_type(void* s,const char* n,void* t){(void)s;(void)n;(void)t;return 0;}
static inline void* yStmtBlueprint_fill_ident(void* s,const char* n,const char* i){(void)s;(void)n;(void)i;return 0;}
static inline void* yStmtBlueprint_fill_var(void* s,const char* n,void* t,const char* i){(void)s;(void)n;(void)t;(void)i;return 0;}
static inline void* yStmtBlueprint_finish(void* s){(void)s;return 0;}
#endif

typedef struct _IO_marker _IO_marker;
typedef struct _IO_codecvt _IO_codecvt;
typedef struct _IO_wide_data _IO_wide_data;
typedef struct _IO_FILE _IO_FILE;
struct _IO_FILE {
int _flags;
char * _IO_read_ptr;
char * _IO_read_end;
char * _IO_read_base;
char * _IO_write_base;
char * _IO_write_ptr;
char * _IO_write_end;
char * _IO_buf_base;
char * _IO_buf_end;
char * _IO_save_base;
char * _IO_backup_base;
char * _IO_save_end;
_IO_marker * _markers;
_IO_FILE * _chain;
int _fileno;
int _flags2;
char * _short_backupbuf;
long _old_offset;
int _cur_column;
char _vtable_offset;
char * _shortbuf;
void * _lock;
long _offset;
_IO_codecvt * _codecvt;
_IO_wide_data * _wide_data;
_IO_FILE * _freeres_list;
void * _freeres_buf;
_IO_FILE * * _prevchain;
int _mode;
int _unused3;
long _total_written;
char * _unused2;
};
typedef struct _IO_cookie_io_functions_t _IO_cookie_io_functions_t;
struct _IO_cookie_io_functions_t {
long (* * read)(void *, char *, long);
long (* * write)(void *, char *, long);
int (* * seek)(void *, long *, int);
int (* * close)(void *);
};
typedef struct div_t div_t;
struct div_t {
int quot;
int rem;
};
typedef struct ldiv_t ldiv_t;
struct ldiv_t {
long quot;
long rem;
};
typedef struct lldiv_t lldiv_t;
struct lldiv_t {
long quot;
long rem;
};
typedef struct timeval timeval;
struct timeval {
long tv_sec;
long tv_usec;
};
typedef struct timespec timespec;
struct timespec {
long tv_sec;
long tv_nsec;
};
typedef struct fd_set fd_set;
struct fd_set {
long * __fds_bits;
};
typedef union pthread_mutexattr_t pthread_mutexattr_t;
union pthread_mutexattr_t {
    char * __size;
    int __align;
};
typedef union pthread_condattr_t pthread_condattr_t;
union pthread_condattr_t {
    char * __size;
    int __align;
};
typedef union pthread_attr_t pthread_attr_t;
union pthread_attr_t {
    char * __size;
    long __align;
};
typedef union pthread_rwlockattr_t pthread_rwlockattr_t;
union pthread_rwlockattr_t {
    char * __size;
    long __align;
};
typedef union pthread_barrier_t pthread_barrier_t;
union pthread_barrier_t {
    char * __size;
    long __align;
};
typedef union pthread_barrierattr_t pthread_barrierattr_t;
union pthread_barrierattr_t {
    char * __size;
    int __align;
};
typedef struct random_data random_data;
struct random_data {
int * fptr;
int * rptr;
int * state;
int rand_type;
int rand_deg;
int rand_sep;
int * end_ptr;
};
typedef struct drand48_data drand48_data;
struct drand48_data {
int * __x;
int * __old_x;
int __c;
int __init;
long __a;
};
