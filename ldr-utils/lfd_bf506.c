/*
 * File: lfd_bf506.c
 *
 * Copyright 2006-2010 Analog Devices Inc.
 * Licensed under the GPL-2, see the file COPYING in this dir
 *
 * Description:
 * Format handlers for LDR files on the BF50[46].
 */

#define __LFD_INTERNAL
#include "ldr.h"

static const char * const bf506_aliases[] = { "BF504", "BF506", NULL };
static const struct lfd_target bf506_lfd_target = {
	.name = "BF506",
	.description = "Blackfin LDR handler for BF504/BF506",
	.aliases = bf506_aliases,
	.uart_boot = true,
	.iovec = {
		.read_block_header = bf54x_lfd_read_block_header,
		.display_dxe = bf54x_lfd_display_dxe,
		.write_block = bf54x_lfd_write_block,
		.dump_block = bf54x_lfd_dump_block,
	},
};

__attribute__((constructor))
static void bf506_lfd_target_register(void)
{
	lfd_target_register(&bf506_lfd_target);
}
