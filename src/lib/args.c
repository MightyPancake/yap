#include "yap/all.h"

void yap_free_args(yap_args args){
  // for_darr(i, arg, args.extra){
  //   free(arg);
  // }
  darr_free(args.extra);
}
