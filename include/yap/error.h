#ifndef YAP_ERROR_H
#define YAP_ERROR_H

kenobi_new_struct_free(yap_error,
  enum {
    yap_error_no_pos, //Errors without position (ie. no source)
    yap_error_pos, //Errors with position
    yap_error_calc_offset //Not used yet
  } kind;
  yap_source* src;
  yap_code_range range;
  yap_loc loc;
  char* msg;
);

#endif //YAP_ERROR_H
