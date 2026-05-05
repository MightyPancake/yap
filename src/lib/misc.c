#include "yap/all.h"

bool yap_struct_type_is_anon(yap_struct_type t){
    return t.name == NULL;
}