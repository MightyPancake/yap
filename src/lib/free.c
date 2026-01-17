#include "yap/all.h"
#include "yap/types.h"

void yap_def_free(yap_def def){
  switch(def.kind){
    case yap_def_func:
      yap_func_def_free(def.func_def);
      break;
    default: break;
  }
}

void yap_func_def_free(yap_func_def fn_def){
  yap_log("Freeing func def");
  yap_block_free(fn_def.body);
}

void yap_block_free(yap_block block){
  yap_log("Freeing block");
  switch(block.kind){
    case yap_block_valid:
      for_darr(i, yap_statement, statement, block.statements)
        yap_statement_free(statement);
      darr_free(block.statements);
      break;
    default: break;
  }
}
void yap_ctx_free(yap_ctx* st){
  yap_log("Freeing state");
  //free sources
  for_darr(i, yap_source, src, st->sources){
    yap_free_source(src);
  }
  darr_free(st->sources);

  for_darr(i, yap_source_code, src_code, st->source_codes){
    yap_source_code_free(src_code);
  }
  darr_free(st->source_codes);

  yap_free_scope(st->scope);
  free(st);
}

void yap_source_code_free(yap_source_code src_code){
  yap_log("Freeing source code");
  for_darr(i, yap_def, def, src_code.definitions){
    yap_log("Freeing def #%d");
    yap_def_free(def);
  }
  darr_free(src_code.definitions);
}

void yap_statement_free(yap_statement statement){
  yap_log("Freeing statement");
  switch(statement.kind){
    case yap_statement_error:
      // free(statement.error_msg);
      break;
    case yap_statement_expr:
      yap_expr_free(statement.expr);
      break;
    default:
      break;
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
    case yap_expr_literal:
      yap_literal_free(expr.literal);
    default:
      break;
  }
}

void yap_literal_free(yap_literal lit){
  yap_log("Freeing literal");
  switch(lit.kind){
    case yap_literal_numerical:
      free(lit.text);
      break;
    default:
      break;
  }
}

void yap_assignment_free(yap_assignment as){
  yap_expr_free(*((yap_expr*)as.left));
  yap_expr_free(*((yap_expr*)as.right));
  free(as.left);
  free(as.right);
}

void yap_bin_expr_free(yap_bin_expr bin_expr){
  yap_expr_free(*((yap_expr*)bin_expr.left));
  yap_expr_free(*((yap_expr*)bin_expr.right));
  free(bin_expr.left);
  free(bin_expr.right);
}
