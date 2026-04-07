#include "yap/all.h"

declare_map_for(var);

yap_scope yap_new_scope(void* parent){
    return (yap_scope){
        .parent=parent,
        .variables=new_var_map()
    };
}

void yap_scope_set_var(yap_scope* sc, yap_var var){
    yap_log("Setting variable '%s' in scope", var.name);
    // yap_var* var_ptr = mem_one_cpy(var);
    hashmap_set(sc->variables, &var);
}

const yap_var* yap_scope_get_var(yap_scope* sc, char* name){
    yap_var dummy = (yap_var){.name=name};
    return hashmap_get(sc->variables, &dummy);
}

const yap_var* yap_scope_remove_var(yap_scope* sc, char* name){
    const yap_var dummy = (yap_var){.name=name};
    return hashmap_delete(sc->variables, &dummy);
}

const yap_var* yap_scope_get_var_recursive(yap_scope* sc, char* name){
    const yap_var* var = yap_scope_get_var(sc, name);
    if (var) return var;
    if (sc->parent) return yap_scope_get_var_recursive(sc->parent, name);
    return NULL;
}