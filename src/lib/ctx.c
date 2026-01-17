#include "yap/all.h"

yap_ctx* yap_ctx_new(){
    yap_log("Creating new ctx");
    yap_ctx* ctx = mem_one_cpy(((yap_ctx){
         .sources=darr_new(yap_source, 64),
         .source_codes=darr_new(yap_source_coude, 1),
         .scope=yap_new_scope(NULL),
     }));
    return ctx;
}

yap_source_code yap_source_code_new(){
  yap_log("Creating new source code");
  return (yap_source_code){
    .definitions=darr_new(yap_def, 8)
  };
}

void yap_ctx_push_source(yap_ctx* st, yap_source src){
  darr_push(yap_source, st->sources, src);
}

yap_source yap_ctx_pop_source(yap_ctx* st){
  return darr_pop(yap_source, st->sources);
}
