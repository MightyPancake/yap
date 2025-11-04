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
void yap_free_state(yap_state* st){
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
    case yap_expr_bin_op:
      yap_bin_op_free(expr.bin_op);
      break;
    case yap_expr_assignment:
      yap_assignment_free(expr.assignment);
    default:
      break;
  }
}

void yap_assignment_free(yap_assignment as){
  yap_expr_free(*((yap_expr*)(as.expr)));
  free(as.expr);
}

void yap_bin_op_free(yap_bin_op bin_op){
  yap_expr_free(*((yap_expr*)bin_op.left));
  yap_expr_free(*((yap_expr*)bin_op.right));
  free(bin_op.left);
  free(bin_op.right);
}
