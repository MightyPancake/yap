#include "yap/all.h"

void yap_free_args(yap_args args){
  darr_free(args.extra);
  darr_free(args.backend_flags);
  darr_free(args.frontend_flags);
}
