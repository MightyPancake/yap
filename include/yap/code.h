#ifndef YAP_CODE_H
#define YAP_CODE_H

#include "yap/types.h"
#define yap_error_result(T, MSG) ((T){.kind=T##_error, .err=(yap_error){\
  .kind=yap_error_no_pos, \
  .msg=(MSG) \
}})

#define yap_error_range_result(T, MSG, SRC, RANGE) ((T){.kind=T##_error, .err=(yap_error){\
  .kind=yap_error_no_pos, \
  .msg=(MSG), \
  .src=SRC,\
  .range=RANGE \
}})

yap_source_code yap_new_source_code();

void yap_def_free(yap_def def);
void yap_func_def_free(yap_func_def fn_def);
void yap_block_free(yap_block block);

#endif //YAP_CODE_H
