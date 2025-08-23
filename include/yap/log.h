#ifndef YAP_LOG_H
#define YAP_LOG_H
#include <stdarg.h>

#ifdef YAP_LOG
  #ifndef YAP_LOG_TAG
    #define YAP_LOG_TAG "YAP_SHARED"
  #endif
  #ifndef YAP_LOG_TAG_COLOR
    #define YAP_LOG_TAG_COLOR aesc_blue
  #endif
  #ifndef YAP_LOG_MSG_COLOR
    #define YAP_LOG_MSG_COLOR aesc_white
  #endif
  #define yap_log(F, ...) yap_logf(YAP_LOG_TAG, YAP_LOG_TAG_COLOR, YAP_LOG_MSG_COLOR, F, ##__VA_ARGS__)
#else
  #define yap_log(F, ...)
#endif
void yap_logf(const char tag[], const char tag_col[], const char msg_col[], const char* fmt, ...);

#endif //YAP_LOG_H
