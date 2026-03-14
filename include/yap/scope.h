#ifndef YAP_SCOPE_H
#define YAP_SCOPE_H

yap_scope* yap_new_scope(void* parent);
void yap_free_scope(yap_scope* sc);

const yap_var* yap_scope_get_var(yap_scope* sc, char* name);
void yap_scope_set_var(yap_scope* sc, yap_var* var);
const yap_var* yap_scope_remove_var(yap_scope* sc, char* name);
const yap_var* yap_scope_get_var_recursive(yap_scope* sc, char* name);

#endif //YAP_SCOPE_H
