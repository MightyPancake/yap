#ifndef YAP_TYPES_H
#define YAP_TYPES_H

//Hashmap
#include "hashmap.h"
typedef struct hashmap* map;

#include "config.h"

//Helper macros
#define base_type(PT) __typeof__(* ( (PT) 0 ))
#define ptr_type(T) __typeof__(T)*

typedef struct yap_args{
  char* output_file;
  darr source_paths;
  bool show_cflags;
}yap_args;

//Types
typedef struct yap_source{
  char* path;
  size_t sz;
  char* content;
}yap_source;

typedef enum yap_type_kind{
  yap_type_primitive,
  yap_type_ptr,
  yap_type_fn
}yap_type_kind;

typedef struct yap_prim_type{
  char* name;
  size_t sz;
}yap_prim_type;

typedef struct yap_type{
  yap_type_kind kind;
  union {
    yap_prim_type primitive;
    void* subtype;
  };
}yap_type;

typedef struct yap_var{
  char* name;
  yap_type type;
}yap_var;

typedef struct yap_block{
  darr statements;
}yap_block;

typedef struct yap_fn_def{
  const char* name;
  darr args;
  yap_type ret_typ;
  yap_block body;
}yap_fn_def;

typedef struct yap_scope{
  void* parent;
  map variables;
}yap_scope;

typedef enum yap_def_kind{
  yap_def_kind_none,
  yap_def_kind_error,
  yap_def_kind_function
}yap_def_kind;

typedef struct yap_def{
  yap_def_kind kind;
  union{
    yap_fn_def fn_def;
  };
}yap_def;

typedef struct yap_state{
  darr sources;
  darr definitions;
  yap_scope* scope;
}yap_state;

//Hashmap functions for types
#define new_map(typ, hsh, comp) ((map)(hashmap_new(sizeof(typ), 0, 0, 0, hsh, comp, NULL, NULL)))
#define define_map_for(T) \
uint64_t map_hash_##T##_f(const void *item, uint64_t seed0, uint64_t seed1); \
int map_cmp_##T##_f(const void* a, const void* b, void *udata); \
map new_##T##_map();

#define declare_map_for(T) \
uint64_t map_hash_##T##_f(const void *item, uint64_t seed0, uint64_t seed1) {return hashmap_murmur(((ptr_type(yap_##T))item)->name, strlen(((ptr_type(yap_##T))item)->name), seed0, seed1);} \
int map_cmp_##T##_f(const void* a, const void* b, void *udata){return strcmp(((ptr_type(yap_##T))(a))->name, ((ptr_type(yap_##T))(b))->name); } \
map new_##T##_map(){return new_map(yap_##T, map_hash_##T##_f, map_cmp_##T##_f);}

#endif //YAP_TYPES_H
