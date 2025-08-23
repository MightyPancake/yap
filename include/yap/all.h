#ifndef YAP_H
#define YAP_H

#include "utils/utils.h"

//Tree-sitter parsing function
#include "tree_sitter/api.h"
const TSLanguage *tree_sitter_yap(void);

//All types are in one header
#include "types.h"

//Functions are grouped based on their scope
#include "log.h"
#include "source.h"
#include "scope.h"
#include "state.h"
#include "var.h"
#include "parse.h"
#include "node.h"
#include "args.h"

#include "os_dependant.h"

#include "compiler.h"

//Misc
int parse_file(const char* path);
#define for_ts_children(N, C) for(TSNode C=ts_node_child(N, 0); !ts_node_is_null(C); C=ts_node_next_sibling(C))

#define str_switch(S, C) if (!strcmp(S, C))
#define str_case(S, C) else str_switch(S, C)

void yap_print_tree(TSNode root, int depth);

#endif
