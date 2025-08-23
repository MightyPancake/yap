#include <stdarg.h>
#include <stdio.h>
#include "yap/all.h"

void yap_logf(const char tag[], const char tag_col[], const char msg_col[], const char* fmt, ...){
    va_list args;
    va_start(args, fmt);
    printf("[%s%s"aesc_reset"]\t%s", tag_col, tag, msg_col);
    vprintf(fmt, args);
    va_end(args);
    printf("\n"aesc_reset);
}

