#include "yap/all.h"

yap_ctx* yap_ctx_new(){
    yap_log("Creating new ctx");
    yap_ctx* ctx = mem_one_cpy(((yap_ctx){
        .sources=darr_new(yap_source, 64),
        .source_codes=darr_new(yap_source_coude, 1),
        .scope=yap_new_scope(NULL),
        .errors=darr_new(yap_error, 8)
     }));
    return ctx;
}

yap_source_code yap_source_code_new(yap_ctx* ctx){
  yap_log("Creating new source code");
  return (yap_source_code){
    .definitions=darr_new(yap_def, 8)
  };
}

void yap_ctx_push_source(yap_ctx* ctx, yap_source src){
  darr_push(yap_source, ctx->sources, src);
}

yap_source yap_ctx_pop_source(yap_ctx* ctx){
  return darr_pop(yap_source, ctx->sources);
}

void yap_ctx_push_error(yap_ctx* ctx, yap_error err){
  if (!ctx) return;
  darr_push(yap_error, ctx->errors, err);
}
