#ifndef YAP_NODE_H
#define YAP_NODE_H

#define yap_node_type(NODE) const char* NODE##_type = ts_node_type(NODE)
#define yap_node_start(NODE) uint32_t NODE##_start = ts_node_start_byte(NODE)
#define yap_node_end(NODE) uint32_t NODE##_end = ts_node_end_byte(NODE)

#define yap_node_lim(NODE) yap_node_start(NODE); yap_node_end(NODE)

#define yap_node_start_point(NODE) TSPoint NODE##_start_point = ts_node_start_point(NODE)
#define yap_node_end_point(NODE) TSPoint NODE##_end_point = ts_node_end_point(NODE)

#define yap_node_point(NODE) yap_node_start_point(NODE); yap_node_end_point(NODE)

#define yap_common_parse(NODE) \
  yap_node_type(NODE); \
  yap_node_start(NODE); \
  yap_node_end(NODE)

//These require to be freed after use
#define yap_node_val(NODE) char* NODE##_val = strndup(src->content + NODE##_start, NODE##_end - NODE##_start)
#define yap_node_str(NODE) char* NODE##_str = ts_node_string(NODE)

#endif //YAP_NODE_H
