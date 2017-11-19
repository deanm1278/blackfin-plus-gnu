/***************************************************************************
 *   Copyright (C) 2011 by Analog Devices, Inc.                            *
 *   Written by Jie Zhang <jie.zhang@analog.com>                           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef BLACKFIN_H
#define BLACKFIN_H

#include "target.h"
#include "blackfin_jtag.h"
#include "blackfin_mem.h"

#define BLACKFINNUMCOREREGS 53
#define BLACKFIN_COMMON_MAGIC 0x05330533
#define BLACKFIN_PART_MAXLEN 20

struct blackfin_common
{
	uint32_t common_magic;
	char part[BLACKFIN_PART_MAXLEN];
	struct blackfin_jtag jtag_info;
	const struct blackfin_mem_map *mem_map;
	const struct blackfin_l1_map *l1_map;
	const struct blackfin_sdram_config *sdram_config;
	const struct blackfin_ddr_config *ddr_config;
	uint32_t mdma_s0;
	uint32_t mdma_d0;
	struct reg_cache *core_cache;
	//	uint32_t core_regs[BLACKFINNUMCOREREGS];

	unsigned int is_locked:1;
	unsigned int is_running:1;
	unsigned int is_interrupted:1;
	unsigned int is_stepping:1;
	unsigned int is_corefault:1;

	unsigned int leave_stopped:1;
	unsigned int status_pending_p:1;
	unsigned int pending_is_breakpoint:1;

	unsigned int l1_data_a_cache_enabled:1;
	unsigned int l1_data_b_cache_enabled:1;
	unsigned int l1_code_cache_enabled:1;

	unsigned int dmem_control_valid_p:1;
	unsigned int imem_control_valid_p:1;

	int pending_signal;
	uint32_t pending_stop_pc;

	uint32_t dmem_control;
	uint32_t imem_control;
};

static inline struct blackfin_common *
target_to_blackfin(struct target *target)
{
	return (struct blackfin_common *)target->arch_info;
}

extern int blackfin_wait_clocks;

#endif /* BLACKFIN_H */
