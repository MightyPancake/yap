#line 0 "yap_c_output.c"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "types.h"
#include "prototypes.h"

int main(int argc, char** argv){
_yap_slice_41d06a1f7c3b87d9 s = ((struct { char* data; unsigned long len; }){ .data = "hello\000world", .len = 11 });
if (s.len == 11)
return 0;
return 1;
}
