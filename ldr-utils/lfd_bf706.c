/*
 * File: lfd_bf706.c
 *
 * Copyright 2006-2010 Analog Devices Inc, 2017 Dean Miller
 * Licensed under the GPL-2, see the file COPYING in this dir
 *
 * Description:
 * Format handlers for LDR files on the BF70[01234567].
 */

#define __LFD_INTERNAL
#include "ldr.h"

static const char * const bf706_aliases[] = { "BF700", "BF701", "BF702", "BF703", "BF704", "BF705", "BF706", "BF707", NULL };
static const struct lfd_target bf706_lfd_target = {
	.name = "BF706",
	.description = "Blackfin LDR handler for BF700/BF701/BF702/BF703/BF704/BF705/BF706/BF707.",
	.aliases = bf706_aliases,
	.uart_boot = false,
	.iovec = {
		.read_block_header = bf54x_lfd_read_block_header,
		.display_dxe = bf54x_lfd_display_dxe,
		.write_block = bf54x_lfd_write_block,
		.dump_block = bf54x_lfd_dump_block,
	},
};

__attribute__((constructor))
static void bf706_lfd_target_register(void)
{
	lfd_target_register(&bf706_lfd_target);
}
