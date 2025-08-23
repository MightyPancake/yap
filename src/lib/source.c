#include "yap/all.h"

void yap_free_source(yap_source s){
    free(s.path);
    free(s.content);
}

char* yap_pos_string(yap_source s, unsigned int line, unsigned int col){
    return strus_newf("%s:%u:%u", s.path, line+1, col);
}

