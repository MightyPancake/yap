#ifndef YAP_CONTEXT_H
#define YAP_CONTEXT_H

//Basic
//Source
void yap_ctx_push_source(yap_ctx* st, yap_source src);
yap_source yap_ctx_pop_source(yap_ctx* st);
void yap_ctx_push_error(yap_ctx* st, yap_error err);

#endif //YAP_CONTEXT_H
