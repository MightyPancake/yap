#ifndef YAP_VERSION_H
#define YAP_VERSION_H

bool yap_version_parse(const char* str, yap_version* out);
int yap_version_cmp(yap_version a, yap_version b);
bool yap_version_satisfies_caret(yap_version req, yap_version have);

#endif //YAP_VERSION_H
