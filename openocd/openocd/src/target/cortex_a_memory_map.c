#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "helper/log.h"

#include "target.h"
#include "armv7a.h"
#include "cortex_a_memory_map.h"


#if !defined(HAVE_LIBEXPAT)

int cortex_a_parse_memory_map(struct target *target, const char *memory_map)
{
	LOG_WARNING("%s: cannot parse XML memory map; XML support was disabled at compile time", target_name(target));
	return ERROR_FAIL;
}

#else /* HAVE_LIBEXPAT */

#include "xml_support.h"

struct cortex_a_memory_map_parsing_data
{
	char *processor;
	uint32_t ahb_mem;
	uint32_t ahb_mem_end;
};


/* Callback called by Expat on start of element.
   This function handles the following elements:
   * processor: unused now
   * memory: a memory region
   * property: unused now
   The target name is in format PROCESSOR  */
static void memory_map_start_element(void *data_, const XML_Char *name,
									 const XML_Char **attrs)
{
	struct cortex_a_memory_map_parsing_data *data = data_;

	if (strcmp(name, "processor") == 0)
	{
		const char *attr = xml_find_attribute(attrs, "name");
		if (!attr)
			attr = "";
		data->processor = strdup(attr);
		if (!data->processor)
			LOG_ERROR("cortex_a: strdup(%s) failed", attr);
	}
	else if (strcmp(name, "memory") == 0)
	{
		const XML_Char *memory_name;
		uint32_t *start = NULL, *end = NULL;

		memory_name = xml_find_attribute(attrs, "name");
		if (strcmp(memory_name, "ahb_mem") == 0)
		{
			start = &data->ahb_mem;
			end =  &data->ahb_mem_end;
		}

		if (xml_parse_uint32(xml_find_attribute(attrs, "start"), start) < 0)
			return;

		if (end)
		{
			if (xml_parse_uint32(xml_find_attribute(attrs, "length"), end) < 0)
				return;
			*end += *start - 1;
		}
	}
	else if (strcmp(name, "property") == 0)
	{
		/* Ignore for now */
	}
}

static void memory_map_end_element(void *data_, const XML_Char *name)
{
	struct cortex_a_memory_map_parsing_data *data = data_;

	if (strcmp(name, "processor") == 0)
	{
		free(data->processor);
		data->processor = NULL;
	}
}

static void memory_map_character_data(void *data_, const XML_Char *s, int len)
{
	return;
}

int cortex_a_parse_memory_map(struct target *target, const char *memory_map)
{
	struct armv7a_common *armv7a = target_to_armv7a(target);
	struct adiv5_dap *dap = armv7a->arm.dap;
	int retval;

	struct cortex_a_memory_map_parsing_data data;

	data.processor = NULL;

	XML_Parser parser = XML_ParserCreateNS(NULL, '!');
	if (parser == NULL)
		return ERROR_FAIL;

	XML_SetElementHandler(parser, memory_map_start_element, memory_map_end_element);
	XML_SetCharacterDataHandler(parser, memory_map_character_data);
	XML_SetUserData(parser, &data);

	retval = XML_Parse(parser, memory_map, strlen(memory_map), 1);
	if (retval != XML_STATUS_OK)
	{
		enum XML_Error err = XML_GetErrorCode(parser);
		LOG_ERROR("%s: cannot parse XML memory map [%s]",
			target_name(target), XML_ErrorString(err));
		return ERROR_FAIL;
	}

	dap->ahb_mem_start_address = data.ahb_mem;
	dap->ahb_mem_end_address = data.ahb_mem_end;

	return ERROR_OK;
}

#endif

