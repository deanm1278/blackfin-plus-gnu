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
 *                                                                         *
 ***************************************************************************/

#ifndef BFIN_PLUS_MEM_H
#define BFIN_PLUS_MEM_H

struct blackfin_mem_map
{
	uint32_t sdram;
	uint32_t sdram_end;
	uint32_t async_mem;
	uint32_t flash;
	uint32_t flash_end;
	uint32_t async_mem_end;
	uint32_t boot_rom;
	uint32_t boot_rom_end;
	uint32_t l2_sram;
	uint32_t l2_sram_end;
	uint32_t l1;
	uint32_t l1_end;
	uint32_t sysmmr;
	uint32_t coremmr;
};

struct blackfin_l1_map
{
	uint32_t l1;
	uint32_t l1_data_a;
	uint32_t l1_data_a_end;
	uint32_t l1_data_a_cache;
	uint32_t l1_data_a_cache_end;
	uint32_t l1_data_b;
	uint32_t l1_data_b_end;
	uint32_t l1_data_b_cache;
	uint32_t l1_data_b_cache_end;
	uint32_t l1_code;
	uint32_t l1_code_end;
	uint32_t l1_code_cache;
	uint32_t l1_code_cache_end;
	uint32_t l1_code_rom;
	uint32_t l1_code_rom_end;
	uint32_t l1_scratch;
	uint32_t l1_scratch_end;
	uint32_t l1_end;
};

#endif /* BFIN_PLUS_MEM_H */
