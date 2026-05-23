#ifndef YAP_SOURCE_H
#define YAP_SOURCE_H

kenobi_new_struct(yap_source,
  void* parent; //Parent scope; NULL for global scope
  char* path; //File path of the source, used for error reporting
  char* content; //Pointer to content
  size_t sz; //Size of the content
  void* ctx; //Context pointer
  yap_anon_id anon_id; //Counter for generating unique names for anonymous items
);

void yap_free_source(yap_source src);
char* yap_pos_string(yap_source s, unsigned int line, unsigned int col);

#endif //YAP_SOURCE_H
