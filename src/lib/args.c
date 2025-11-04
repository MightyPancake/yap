#include "yap/all.h"

void yap_free_args(yap_args args){
  darr_free(args.extra);
}
