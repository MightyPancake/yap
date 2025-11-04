#include "yap/all.h"

yap_state* yap_state_new(){
    yap_log("Creating new state");
    yap_state* state = mem_one_cpy(((yap_state){
         .sources=darr_new(yap_source, 64),
         .source_codes=darr_new(yap_source_coude, 1),
         .scope=yap_new_scope(NULL),
     }));
    return state;
}

yap_source_code yap_source_code_new(){
  yap_log("Creating new source code");
  return (yap_source_code){
    .definitions=darr_new(yap_def, 8)
  };
}

void yap_state_push_source(yap_state* st, yap_source src){
  darr_push(yap_source, st->sources, src);
}

yap_source yap_state_pop_source(yap_state* st){
  return darr_pop(yap_source, st->sources);
}
