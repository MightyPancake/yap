#ifndef YAP_LOC_H
#define YAP_LOC_H

kenobi_new_struct_free(yap_code_pos,
  int line;
  int column;
  int offset; //bytes
);

kenobi_new_struct_free(yap_code_range,
  yap_code_pos start;
  yap_code_pos end;
);

kenobi_new_struct_free(yap_loc,
  yap_source* src;
  yap_code_range range;
);

#endif //YAP_LOC_H
