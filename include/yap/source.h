#ifndef YAP_SOURCE_H
#define YAP_SOURCE_H

#include "loc.h"

typedef enum {
  yap_import_module,
  yap_import_file,
} yap_import_kind;

kenobi_new_struct_free(yap_import,
  yap_import_kind kind;
  yap_loc loc; //Where this import was declared
  union {
    char* module_name; //For module imports
    char* identity; //Identity for source
  };
);

typedef enum {
  yap_source_root,
  yap_source_file,
  yap_source_incremental,
  yap_source_stdin,
  yap_source_string,
} yap_source_kind;

typedef struct yap_ctx yap_ctx;

kenobi_new_struct(yap_source,
  //Kind of source
  yap_source_kind kind;

  //Unique identifier for the source.
  //files: name + '@' + parent absolute path
  char* identity;

  //Parent scope; NULL for global scope
  yap_source* parent; 

  //Local label; it makes sense in context of the parent source
  char* label;

  //Absolute path for files; NULL for non-file sources
  char* origin;

  //Pointer to content
  char* content;

  //Size of the content
  size_t sz;

  //Context pointer
  yap_ctx* ctx;

  //Result of parsing this source
  yap_source_node* source_node;

  //Counter for generating unique names for anonymous items
  yap_anon_id anon_id;

  //List of imports in this source
  darr(yap_import) imports;

  //Where this source was imported (loc of the import statement that triggered it)
  yap_loc import_loc;
);

void yap_free_source(yap_source src);
char* yap_pos_string(yap_source s, unsigned int line, unsigned int col);
size_t yap_read_file_to_string(const char *path, char **out);
#endif //YAP_SOURCE_H
