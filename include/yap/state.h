#ifndef YAP_STATE_H
#define YAP_STATE_H

//Basic
yap_state* yap_new_state();
void yap_free_state(yap_state* st);
//Source
void yap_state_push_source(yap_state* st, yap_source src);
yap_source yap_state_pop_source(yap_state* st);

#endif //YAP_STATE_H
