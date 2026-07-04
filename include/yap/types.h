#ifndef YAP_TYPES_H
#define YAP_TYPES_H

//Hashmap
#include "hashmap.h"
typedef struct hashmap* map;

#include "config.h"
#include "utils/utils.h"

//Helper macros
#define base_type(PT) __typeof__(* ( (PT) 0 ))
#define ptr_type(T) __typeof__(T)*

//Yap forward declarations
#include "forward_decl.h"

//Source
#include "source.h"

//Code location
#include "loc.h"

//Errors
#include "error.h"

//Node kinds
#include "node_kinds.h"

//Nodes (parsing output types)
#include "nodes.h"

//Semantic tree types
#include "semtree.h"

typedef struct yap_args{
  char* output_file;
  darr(char*) extra;
  bool show_modules_path;
  char* command;
  // --gen-c-bind: header to generate bindings from (e.g. "<stdio.h>")
  char* gen_c_bind_header;
  // --prefix: symbol prefix for wrapper library (e.g. "yap_io_")
  char* gen_c_bind_prefix;
  bool run;
  darr(char*) backend_flags;
  darr(char*) frontend_flags;
  char* backend_component;
  char* frontend_component;
  char* semantic_component;
}yap_args;

kenobi_new_struct_free(yap_module,
  char* name;
  char* prefix; //Prefix for name mangling, usually derived from the module name
  darr(yap_decl_node) decls; //Parse-level declarations in this module
  void* module_ctx; //This is specific to compiler back end
  yap_scope* scope; //Module-level scope for namespaced symbol lookup
  darr(char*) lib_paths; //Paths to static/shared libraries for this module
);

typedef void (*yap_print_error_fn)(yap_error);
typedef void (*yap_gen_decl_fn)(yap_ctx* ctx, yap_decl decl);
typedef void* (*yap_ensure_symbol_fn)(yap_ctx* ctx, const char* name);
typedef void (*yap_set_macro_name_fn)(const char* name);
typedef void (*yap_set_macro_loc_fn)(yap_source* src, yap_loc loc);
typedef void (*yap_pop_macro_loc_fn)(void);

kenobi_new_struct_free(yap_ctx,
  //Arena
  quake arena; //Memory arena for all allocations in the compiler, freed at the end of compilation. Speeds up allocation and deallocation significantly.
  
  //Sources
  yap_source* root_source;
  darr(yap_source*) sources; //darr of yap_source, represents the source files being compiled.
  darr(yap_source*) source_stack;
  darr(yap_error) errors; //darr of yap_error
  //TODO: Do we make scopes dynamic and lose them after parsing or introduce a new 'scope'
  darr(yap_scope*) scopes; //Array holding all scopes
  darr(yap_scope*) current_scopes; //stack of scopes for codegen. Top is current, bottom is global.
  yap_scope* global_scope;

  //Modules
  map modules; //map of named modules
  char* current_module_name; //resolve via yap_ctx_current_module(ctx), don't read directly

  //Semantic declarations (global, not per-module)
  darr(yap_decl) semantic_decls;

  //Counter for anonymous names (__anon_<tag>_N). Ctx-global, not per-source:
  //all sources emit into one C translation unit, so per-source counters would
  //let two files mint the same name.
  yap_anon_id anon_id;

  //Types
  darr(yap_type) types; //yap_type_id points to types in this array
  map named_types; //map of named types
  //Cached type ids for primitives and untyped literals for fast access during parsing and type inference
  yap_type_id internal_error_type_id; //Type ID representing an internal compiler error, used for error handling when the compiler encounters an unexpected state.
  yap_type_id void_type_id; //cached type_id for void
  yap_type_id blob_type_id; //cached type_id for blobs
  yap_type_id int_type_id;  //cached type_id for i32
  yap_type_id bool_type_id; //cached type_id for bool
  yap_type_id float_type_id; //cached type_id for f32
  yap_type_id untyped_int_type_id;  //cached type_id for untyped integer literals
  yap_type_id untyped_float_type_id; //cached type_id for untyped float literals
  yap_type_id untyped_byte_type_id;  //cached type_id for untyped byte literals
  //Comptime types (opaque handles for metaprogramming)
  yap_type_id yexpr_type_id;
  yap_type_id ytype_type_id;
  yap_type_id ystmt_type_id;
  yap_type_id yfn_type_id;
  yap_type_id yident_type_id;
  yap_type_id yexprblueprint_type_id; //cached type_id for yExprBlueprint (a yExpr template with named holes)
  yap_type_id ystmtblueprint_type_id; //cached type_id for yStmtBlueprint (a yStmt template with named holes)
  yap_type_id yexprlist_type_id;  //cached type_id for yExprList (growable list of yExpr)
  yap_type_id ystmtlist_type_id;  //cached type_id for yStmtList (growable list of yStmt)
  //Comptime builder templates (yapi.md): yStructT/yEnumT/yUnionT/yFnT
  yap_type_id ystructt_type_id;
  yap_type_id yenumt_type_id;
  yap_type_id yuniont_type_id;
  yap_type_id yfnt_type_id;
  //External
  yap_print_error_fn print_error;
  yap_gen_decl_fn gen_decl;
  yap_ensure_symbol_fn ensure_symbol;
  yap_set_macro_name_fn set_macro_name;
  yap_set_macro_loc_fn set_macro_loc;
  yap_pop_macro_loc_fn pop_macro_loc;

  //Module lookup paths
  darr(char*) module_lookup_paths;

  //Args
  yap_args* args;

  //Parser ctx
  void* parser_ctx;

  //Backend build state (e.g. TCCState for yap-c)
  void* build_state;

  int run_exit_code;
);

//Hashmap functions for types
#define new_map(typ, hsh, comp) ((map)(hashmap_new(sizeof(typ), 0, 0, 0, hsh, comp, NULL, NULL)))
#define define_map_for(T) \
uint64_t map_hash_##T##_f(const void *item, uint64_t seed0, uint64_t seed1); \
int map_cmp_##T##_f(const void* a, const void* b, void *udata); \
map new_##T##_map();

#define declare_map_for(T) \
uint64_t map_hash_##T##_f(const void *item, uint64_t seed0, uint64_t seed1) {return hashmap_murmur(((ptr_type(yap_##T))item)->name, strlen(((ptr_type(yap_##T))item)->name), seed0, seed1);} \
int map_cmp_##T##_f(const void* a, const void* b, void *udata){(void)udata; return strcmp(((ptr_type(yap_##T))(a))->name, ((ptr_type(yap_##T))(b))->name); } \
map new_##T##_map(){return new_map(yap_##T, map_hash_##T##_f, map_cmp_##T##_f);}

#endif //YAP_TYPES_H
