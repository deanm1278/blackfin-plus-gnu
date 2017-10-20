/*
 * File: lfd_bf533.c
 *
 * Copyright 2006-2010 Analog Devices Inc.
 * Licensed under the GPL-2, see the file COPYING in this dir
 *
 * Description:
 * Format handlers for LDR files on the BF53[123].
 */

#define __LFD_INTERNAL
#include "ldr.h"

static const char * const bf533_aliases[] = { "BF531", "BF532", "BF533", "BF538", "BF539", NULL };
static const struct lfd_target bf533_lfd_target = {
	.name = "BF533",
	.description = "Blackfin LDR handler for BF531/BF532/BF533 and BF538/BF539",
	.aliases = bf533_aliases,
	.uart_boot = false,
	.iovec = {
		.read_block_header = bf53x_lfd_read_block_header,
		.display_dxe = bf53x_lfd_display_dxe,
		.write_ldr = bf53x_lfd_write_ldr,
		.write_block = bf53x_lfd_write_block,
		.dump_block = bf53x_lfd_dump_block,
	},
};

__attribute__((constructor))
static void bf533_lfd_target_register(void)
{
	lfd_target_register(&bf533_lfd_target);
}
