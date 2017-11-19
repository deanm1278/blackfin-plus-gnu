#ifdef HAVE_CONFIG_H
#include "config.h"
#endif


#if defined(HAVE_LIBEXPAT)

#include <expat.h>

const XML_Char *xml_find_attribute(const XML_Char **attrs, const XML_Char *attr)
{
	const XML_Char **p;
	for (p = attrs; *p; p += 2)
	{
		const char *name = p[0];
		const char *val = p[1];
		if (strcmp(name, attr) == 0)
			return val;
	}
	return NULL;
}

int xml_parse_uint32(const char *str, uint32_t *p)
{
	char *endptr;
	unsigned long result;

	if (*str == '\0')
		return -1;

	result = strtol(str, &endptr, 0);
	if (*endptr != '\0')
		return -1;

	*p = result;
	return 0;
}

#endif /* HAVE_LIBEXPAT */
