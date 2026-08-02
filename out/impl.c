#line 0 "yap_c_output.c"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "types.h"
#include "prototypes.h"

int main(int argc, char** argv){
_yap_slice_2471c74dfadca618 args = { .data = argv, .len = (unsigned long)argc };
int code = 0;
if (args.len == 0)
{
code = code + 1;
}
if (args.data[0][0] == 0)
{
code = code + 1;
}
return code;
}
