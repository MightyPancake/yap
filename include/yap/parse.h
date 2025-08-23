#ifndef YAP_PARSE_H
#define YAP_PARSE_H

yap_state* yap_parse(yap_args args);
void yap_parse_source_file(yap_source* src, TSNode node);
yap_def yap_parse_def(yap_source* src, TSNode node);
yap_def yap_parse_fn_def(yap_source* src, TSNode node);

#endif //YAP_PARSE_H
