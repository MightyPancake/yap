#include "yap/all.h"

void yap_variable_free(yap_var* var){
    free(var->name);
    free(var);
}

