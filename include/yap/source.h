#ifndef YAP_SOURCE_H
#define YAP_SOURCE_H

void yap_free_source(yap_source src);
char* yap_pos_string(yap_source s, unsigned int line, unsigned int col);

#endif //YAP_SOURCE_H
