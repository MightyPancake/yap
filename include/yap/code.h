#ifndef YAP_CODE_H
#define YAP_CODE_H

#include <stdarg.h>
#include "yap/types.h"
#define yap_error_result(T, MSG) ((T){.kind=T##_error, .err=(yap_error){\
  .kind=yap_error_no_pos, \
  .msg=(MSG) \
}})

#define yap_error_range_result(T, MSG, SRC, RANGE) ((T){.kind=T##_error, .err=(yap_error){\
  .kind=yap_error_pos, \
  .msg=(MSG), \
  .src=SRC,\
  .range=RANGE \
}})

void yap_emit_error_rangef(yap_ctx* ctx, yap_source* src, yap_code_range range, const char* fmt, ...);

#define yap_emit_error_at(CTX, SRC, WITH_RANGE, FMT, ...) \
  yap_emit_error_rangef((CTX), (SRC), (WITH_RANGE).range, (FMT) __VA_OPT__(,) __VA_ARGS__)

yap_source_code yap_new_source_code();

#endif //YAP_CODE_H
