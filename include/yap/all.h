#ifndef YAP_H
#define YAP_H

//Tree-sitter parsing function

//All types are in one header
#include "types.h"

//Functions are grouped based on their scope
#include "log.h"
#include "source.h"
#include "scope.h"
#include "ctx.h"
#include "args.h"
#include "code.h"

#include "os_dependant.h"

#include "compiler.h"

//Misc
int parse_file(const char* path);
#define str_switch(S, C) if (!strcmp(S, C))
#define str_case(S, C) else str_switch(S, C)

#endif
