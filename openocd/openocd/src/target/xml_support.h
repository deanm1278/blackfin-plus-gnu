#ifndef XML_SUPPORT_H
#define XML_SUPPORT_H

#include <expat.h>

extern const XML_Char *xml_find_attribute(const XML_Char **attrs, const XML_Char *attr);
extern int xml_parse_uint32(const char *str, uint32_t *p);

#endif /* XML_SUPPORT_H */
