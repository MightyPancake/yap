#include "yap/all.h"

yap_state* yap_new_state(){
    yap_log("Creating new state");
    yap_state* state = mem_one_cpy(((yap_state){
         .sources=darr_new(yap_source, 64),
         .definitions=darr_new(yap_def, 64),
         .scope=yap_new_scope(NULL),
     }));
    return state;
}

void yap_free_state(yap_state* st){
  //free sources
  for_darr(i, yap_source, src, st->sources){
    yap_free_source(src);
  }
  darr_free(st->sources);

  yap_free_scope(st->scope);
  free(st);
}

void yap_state_push_source(yap_state* st, yap_source src){
  darr_push(yap_source, st->sources, src);
}

yap_source yap_state_pop_source(yap_state* st){
  return darr_pop(yap_source, st->sources);
}
