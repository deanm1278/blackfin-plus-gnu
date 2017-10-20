/*
 * File: lfd_bf592.c
 *
 * Copyright 2006-2010 Analog Devices Inc.
 * Licensed under the GPL-2, see the file COPYING in this dir
 *
 * Description:
 * Format handlers for LDR files on the BF592.
 */

#define __LFD_INTERNAL
#include "ldr.h"

static const char * const bf592_aliases[] = { "BF592", NULL };
static const struct lfd_target bf592_lfd_target = {
	.name = "BF592",
	.description = "Blackfin LDR handler for BF592",
	.aliases = bf592_aliases,
	.uart_boot = true,
	.iovec = {
		.read_block_header = bf54x_lfd_read_block_header,
		.display_dxe = bf54x_lfd_display_dxe,
		.write_block = bf54x_lfd_write_block,
		.dump_block = bf54x_lfd_dump_block,
	},
};

__attribute__((constructor))
static void bf592_lfd_target_register(void)
{
	lfd_target_register(&bf592_lfd_target);
}
