#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "helper/log.h"

#include "target.h"
#include "blackfin.h"
#include "blackfin_config.h"


#if !defined(HAVE_LIBEXPAT)

int blackfin_parse_config(struct target *target, const char *configs)
{
	LOG_WARNING("%s: cannot parse Blackfin XML config file; XML support was disabled at compile time", target_name(target));
	return ERROR_FAIL;
}

#else /* HAVE_LIBEXPAT */

#include "xml_support.h"

/* Callback called by Expat on start of element.
   This function handles the following elements:
   * processor: unused now
   * core: contains the memory map for the core
   * memory: a memory region
   * property: unused now
   The target name is in format PROCESSOR or PROCESSOR.CORE  */
static void blackfin_config_start_element(void *data_, const XML_Char *name,
									 const XML_Char **attrs)
{
	struct blackfin_common *blackfin = data_;

	if (strcmp(name, "processor") == 0)
	{
		/* not used for now */
	}
	else if (strcmp(name, "config") == 0)
	{
		const XML_Char *config_name;
		uint32_t value; 

		config_name = xml_find_attribute(attrs, "name");
		if (xml_parse_uint32(xml_find_attribute(attrs, "value"), &value) < 0)
			return;
		if (strcmp(config_name, "mdma_d0") == 0)
			blackfin->mdma_d0 = value;
		else if (strcmp(config_name, "mdma_s0") == 0)
			blackfin->mdma_s0 = value;
	}
}

static void blackfin_config_end_element(void *data_, const XML_Char *name)
{
	return;
}

static void blackfin_config_character_data(void *data_, const XML_Char *s, int len)
{
	return;
}

int blackfin_parse_config(struct target *target, const char *configs)
{
	struct blackfin_common *blackfin = target_to_blackfin(target);
	int retval;

	XML_Parser parser = XML_ParserCreateNS(NULL, '!');
	if (parser == NULL)
		return ERROR_FAIL;

	XML_SetElementHandler(parser, blackfin_config_start_element, blackfin_config_end_element);
	XML_SetCharacterDataHandler(parser, blackfin_config_character_data);
	XML_SetUserData(parser, blackfin);

	retval = XML_Parse(parser, configs, strlen(configs), 1);
	if (retval != XML_STATUS_OK)
	{
		enum XML_Error err = XML_GetErrorCode(parser);
		LOG_ERROR("%s: cannot parse Blackfin XML config file [%s]",
			target_name(target), XML_ErrorString(err));
		return ERROR_FAIL;
	}

	return ERROR_OK;
}

#endif

