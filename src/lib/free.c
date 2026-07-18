#include "yap/all.h"
#include "yap/types.h"

void yap_decl_free(yap_decl decl){
  switch(decl.kind){
    case yap_decl_func_def:
      yap_func_decl_free(decl.func_decl);
      break;
    case yap_decl_func_decl:
      break;
    default: break;
  }
}

void yap_func_decl_free(yap_func_decl fn_decl){
  yap_log("Freeing func def");
  // darr_free(fn_decl.args);
  yap_block_free(fn_decl.body);
}

void yap_block_free(yap_block block){
  yap_log("Freeing block");
  switch(block.kind){
    case yap_block_valid:
      for_darr(i, statement, block.statements)
        yap_statement_free(statement);
      break;
    default: break;
  }
}
void yap_ctx_free(yap_ctx ctx){
  yap_log("Freeing state");
  // Free modules
  void* item;
  size_t iter = 0;
  while (hashmap_iter(ctx.modules, &iter, &item)) {
    yap_module* module = item;
    yap_module_free(*module);
  }
  hashmap_free(ctx.modules);

  for_darr(i, err, ctx.errors){
    yap_error_free(err);
  }
  darr_free(ctx.errors);

  for_darr(i, sc, ctx.scopes){
    yap_scope_free(*sc);
    // No need to free the scope itself since it's allocated in the arena
    // free(sc);
  }
  darr_free(ctx.current_scopes);
  darr_free(ctx.scopes);

  //Types
  for_darr(i, typ, ctx.types){
    yap_type_free(typ);
  }
  darr_free(ctx.types);
  //Named types
  hashmap_free(ctx.named_types);

  // Free semantic declarations
  darr_free(ctx.semantic_decls);

  // Free macro-method registry (entries' strings are arena-allocated, only the darr's own array needs releasing)
  darr_free(ctx.macro_methods);

  // Free module lookup paths
  for_darr(i, p, ctx.module_lookup_paths){
    free(p);
  }
  darr_free(ctx.module_lookup_paths);

  // Free sources
  yap_log("Freeing %d sources", darr_len(ctx.sources));
  for_darr(i, src, ctx.sources){
    yap_free_source(*src);
  }
  darr_free(ctx.sources);
  darr_free(ctx.source_stack);

  //Free arena
  quake_free(&ctx.arena);
}

void yap_module_free(yap_module module){
  yap_log("Freeing module '%s'", module.name);
  darr_free(module.decls);
  for_darr(i, lp, module.lib_paths) free(lp);
  darr_free(module.lib_paths);
}

void yap_type_free(yap_type typ){
  yap_log("Freeing type");
  switch(typ.kind){
    case yap_type_primitive:
    break;
    default: break;
  }
}

void yap_statement_free(yap_statement statement){
  yap_log("Freeing statement");
  switch(statement.kind){
    case yap_statement_error:
      break;
    case yap_statement_expr:
      yap_expr_free(statement.expr);
      break;
    case yap_statement_var_decl:
      yap_var_decl_free(statement.var_decl);
      break;
    default:
      break;
  }
}

void yap_var_decl_free(yap_var_decl var_decl){
  yap_log("Freeing variable declaration");
  if (var_decl.has_init){
    yap_expr_free(var_decl.init);
  }
}

void yap_expr_free(yap_expr expr){
  yap_log("Freeing expr");
  switch(expr.kind){
    case yap_expr_bin:
      yap_bin_expr_free(expr.bin_expr);
      break;
    case yap_expr_assignment:
      yap_assignment_free(expr.assignment);
      break;
    case yap_expr_literal:
      yap_literal_free(expr.literal);
      break;
    default:
      break;
  }
}

void yap_literal_free(yap_literal lit){
  yap_log("Freeing literal");
  switch(lit.kind){
    default:
      break;
  }
}

void yap_assignment_free(yap_assignment as){
  yap_expr_free(*((yap_expr*)as.left));
  yap_expr_free(*((yap_expr*)as.right));
  // free(as.left);
  // free(as.right);
}

void yap_bin_expr_free(yap_bin_expr bin_expr){
  yap_expr_free(*((yap_expr*)bin_expr.left));
  yap_expr_free(*((yap_expr*)bin_expr.right));
  // free(bin_expr.left);
  // free(bin_expr.right);
}

void yap_error_free(yap_error err){
  yap_log("Freeing error");
  free(err.msg);
}

void yap_scope_free(yap_scope sc){
  yap_log("Freeing scope");
    void* item;
    size_t iter = 0;
    while (hashmap_iter(sc.variables, &iter, &item)) {
        yap_var* var = item;
        yap_var_free(*var);
        // free(var);
    }
    hashmap_free(sc.variables);
}

void yap_var_free(yap_var var){
  (void)var;
  // yap_log("Freeing variable '%s'", var.name);
  // free(var.name);
}

void yap_free_source(yap_source s){
  darr_free(s.imports);
  switch (s.kind){
    case yap_source_root:
      return;
    default:
      free(s.content);
      break;
  }
}