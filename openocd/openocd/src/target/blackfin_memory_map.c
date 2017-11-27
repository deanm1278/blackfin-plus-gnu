#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "helper/log.h"

#include "target.h"
#include "blackfin.h"
#include "bfinplus.h"
#include "blackfin_memory_map.h"


#if !defined(HAVE_LIBEXPAT)

int parse_memory_map(struct target *target, const char *memory_map)
{
	LOG_WARNING("%s: cannot parse XML memory map; XML support was disabled at compile time", target_name(target));
	return ERROR_FAIL;
}

#else /* HAVE_LIBEXPAT */

#include "xml_support.h"

struct memory_map_parsing_data
{
	const char *name;
	struct blackfin_mem_map *mem_map;
	struct blackfin_l1_map *l1_map;
	char *processor;
	char *core;
	bool skip;
};


/* Callback called by Expat on start of element.
   This function handles the following elements:
   * processor: unused now
   * core: contains the memory map for the core
   * memory: a memory region
   * property: unused now
   The target name is in format PROCESSOR or PROCESSOR.CORE  */
static void memory_map_start_element(void *data_, const XML_Char *name,
									 const XML_Char **attrs)
{
	struct memory_map_parsing_data *data = data_;

	if (strcmp(name, "processor") == 0)
	{
		const char *attr = xml_find_attribute(attrs, "name");
		if (!attr)
			attr = "";
		data->processor = strdup(attr);
		if (!data->processor)
			LOG_ERROR("%s: strdup(%s) failed", data->name, attr);
	}
	else if (strcmp(name, "core") == 0)
	{
		char buf[40];
		const char *attr = xml_find_attribute(attrs, "name");

		if (!attr)
		{
			attr = "";
			sprintf(buf, "%s", data->processor);
		}
		else
			sprintf(buf, "%s.%s", data->processor, attr);

		data->core = strdup(attr);
		if (!data->core)
			LOG_ERROR("%s: strdup(%s) failed", data->name, attr);

		if (strcmp(buf, data->name) == 0)
			data->skip = false;
		else
			data->skip = true;
	}
	else if (strcmp(name, "memory") == 0 && !data->skip)
	{
		const XML_Char *memory_name;
		uint32_t *start = NULL, *end = NULL;

		memory_name = xml_find_attribute(attrs, "name");
		if (!data->core)
		{
			if (strcmp(memory_name, "sdram") == 0)
			{
				start = &data->mem_map->sdram;
				end =  &data->mem_map->sdram_end;
			}
			else if (strcmp(memory_name, "async_mem") == 0)
			{
				start = &data->mem_map->async_mem;
				end =  &data->mem_map->async_mem_end;
			}
			else if (strcmp(memory_name, "flash") == 0)
			{
				start = &data->mem_map->flash;
				end =  &data->mem_map->flash_end;
			}
			else if (strcmp(memory_name, "boot_rom") == 0)
			{
				start = &data->mem_map->boot_rom;
				end =  &data->mem_map->boot_rom_end;
			}
			else if (strcmp(memory_name, "l2_sram") == 0)
			{
				start = &data->mem_map->l2_sram;
				end =  &data->mem_map->l2_sram_end;
			}
			else if (strcmp(memory_name, "sysmmr") == 0)
			{
				start = &data->mem_map->sysmmr;
				end = NULL;
			}
			else if (strcmp(memory_name, "coremmr") == 0)
			{
				start = &data->mem_map->coremmr;
				end = NULL;
			}
			else if (strcmp(memory_name, "l1") == 0)
			{
				start = &data->mem_map->l1;
				end =  &data->mem_map->l1_end;
			}
		}
		else
		{
			if (strcmp(memory_name, "l1") == 0)
			{
				start = &data->l1_map->l1;
				end =  &data->l1_map->l1_end;
			}
			else if (strcmp(memory_name, "l1_data_a") == 0)
			{
				start = &data->l1_map->l1_data_a;
				end =  &data->l1_map->l1_data_a_end;
			}
			else if (strcmp(memory_name, "l1_data_a_cache") == 0)
			{
				start = &data->l1_map->l1_data_a_cache;
				end =  &data->l1_map->l1_data_a_cache_end;
			}
			else if (strcmp(memory_name, "l1_data_b") == 0)
			{
				start = &data->l1_map->l1_data_b;
				end =  &data->l1_map->l1_data_b_end;
			}
			else if (strcmp(memory_name, "l1_data_b_cache") == 0)
			{
				start = &data->l1_map->l1_data_b_cache;
				end =  &data->l1_map->l1_data_b_cache_end;
			}
			else if (strcmp(memory_name, "l1_code") == 0)
			{
				start = &data->l1_map->l1_code;
				end =  &data->l1_map->l1_code_end;
			}
			else if (strcmp(memory_name, "l1_code_cache") == 0)
			{
				start = &data->l1_map->l1_code_cache;
				end =  &data->l1_map->l1_code_cache_end;
			}
			else if (strcmp(memory_name, "l1_code_rom") == 0)
			{
				start = &data->l1_map->l1_code_rom;
				end =  &data->l1_map->l1_code_rom_end;
			}
			else if (strcmp(memory_name, "l1_scratch") == 0)
			{
				start = &data->l1_map->l1_scratch;
				end =  &data->l1_map->l1_scratch_end;
			}
		}

		if (xml_parse_uint32(xml_find_attribute(attrs, "start"), start) < 0)
			return;

		if (end)
		{
			if (xml_parse_uint32(xml_find_attribute(attrs, "length"), end) < 0)
				return;
			*end += *start;
		}
	}
	else if (strcmp(name, "property") == 0)
	{
		/* Ignore for now */
	}
}

static void memory_map_end_element(void *data_, const XML_Char *name)
{
	struct memory_map_parsing_data *data = data_;

	if (strcmp(name, "processor") == 0)
	{
		free(data->processor);
		data->processor = NULL;
	}
	else if (strcmp(name, "core") == 0)
	{
		free(data->core);
		data->core = NULL;
		data->skip = false;
	}
}

static void memory_map_character_data(void *data_, const XML_Char *s, int len)
{
	return;
}

int parse_memory_map(struct target *target, const char *memory_map)
{
	struct blackfin_common *blackfin = target_to_blackfin(target);
	int retval;

	struct memory_map_parsing_data data;

	data.name = blackfin->part;
	data.mem_map = calloc(1, sizeof (struct blackfin_mem_map));
	if (!data.mem_map)
	{
		LOG_ERROR("%s: malloc(%"PRIzu") failed",
			target_name(target), sizeof (struct blackfin_mem_map));
		return ERROR_FAIL;
	}
	data.l1_map = calloc(1, sizeof (struct blackfin_l1_map));
	if (!data.l1_map)
	{
		LOG_ERROR("%s: malloc(%"PRIzu") failed",
			target_name(target), sizeof (struct blackfin_l1_map));
		free(data.mem_map);
		return ERROR_FAIL;
	}
	data.processor = NULL;
	data.core = NULL;
	data.skip = false;

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
		free(data.mem_map);
		free(data.l1_map);
		return ERROR_FAIL;
	}

	blackfin->mem_map = data.mem_map;
	blackfin->l1_map = data.l1_map;

	return ERROR_OK;
}

int parse_memory_map_bfinplus(struct target *target, const char *memory_map)
{
	struct bfinplus_common *blackfin = target_to_bfinplus(target);
	int retval;

	struct memory_map_parsing_data data;

	data.name = blackfin->part;
	data.mem_map = calloc(1, sizeof (struct blackfin_mem_map));
	if (!data.mem_map)
	{
		LOG_ERROR("%s: malloc(%"PRIzu") failed",
			target_name(target), sizeof (struct blackfin_mem_map));
		return ERROR_FAIL;
	}
	data.l1_map = calloc(1, sizeof (struct blackfin_l1_map));
	if (!data.l1_map)
	{
		LOG_ERROR("%s: malloc(%"PRIzu") failed",
			target_name(target), sizeof (struct blackfin_l1_map));
		free(data.mem_map);
		return ERROR_FAIL;
	}
	data.processor = NULL;
	data.core = NULL;
	data.skip = false;

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
		free(data.mem_map);
		free(data.l1_map);
		return ERROR_FAIL;
	}

	blackfin->mem_map = data.mem_map;
	blackfin->l1_map = data.l1_map;

	return ERROR_OK;
}

#endif

