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
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>

#include "helper/log.h"
#include "helper/types.h"
#include "jtag/jtag.h"
#include "blackfin.h"
#include "blackfin_jtag.h"
#include "blackfin_mem.h"

#define DMA_CONFIG_FLOW_STOP	0x0000
#define DMA_CONFIG_NDSIZE_0		0x0000
#define DMA_CONFIG_DI_EN		0x0080
#define DMA_CONFIG_DI_SEL		0x0040
#define DMA_CONFIG_SYNC			0x0020
#define DMA_CONFIG_DMA2D		0x0010
#define DMA_CONFIG_WDSIZE_8		0x0000
#define DMA_CONFIG_WDSIZE_16	0x0004
#define DMA_CONFIG_WDSIZE_32	0x0008
#define DMA_CONFIG_WDSIZE_MASK	0x000c
#define DMA_CONFIG_WNR			0x0002
#define DMA_CONFIG_DMAEN		0x0001

#define DMA_IRQ_STATUS_DMA_RUN	0x0008
#define DMA_IRQ_STATUS_DFETCH	0x0004
#define DMA_IRQ_STATUS_DMA_ERR	0x0002
#define DMA_IRQ_STATUS_DMA_DONE	0x0001

#define EBIU_SDGCTL				0xffc00a10
#define EBIU_SDBCTL				0xffc00a14
#define EBIU_SDRRC				0xffc00a18
#define EBIU_SDSTAT				0xffc00a1c

#define EBIU_DDRCTL0			0xffc00a20
#define EBIU_DDRCTL1			0xffc00a24
#define EBIU_DDRCTL2			0xffc00a28
#define EBIU_DDRCTL3			0xffc00a2c
#define EBIU_RSTCTL				0xffc00a3c

#define SICA_SYSCR				0xffc00104
#define SICA_SYSCR_COREB_SRAM_INIT	0x0020

#define DMEM_CONTROL			0xffe00004
#define DCPLB_ADDR0				0xffe00100
#define DCPLB_DATA0				0xffe00200
#define IMEM_CONTROL			0xffe01004
#define ICPLB_ADDR0				0xffe01100
#define ICPLB_DATA0				0xffe01200

#define ENICPLB					0x00000002
#define IMC						0x00000004

#define ENDCPLB					0x00000002
#define DMC						0x0000000c
#define ACACHE_BSRAM			0x00000008
#define ACACHE_BCACHE			0x0000000c

#define PAGE_SIZE_MASK			0x00030000
#define PAGE_SIZE_4MB			0x00030000
#define PAGE_SIZE_1MB			0x00020000
#define PAGE_SIZE_4KB			0x00010000
#define PAGE_SIZE_1KB			0x00000000
#define CPLB_L1_AOW				0x00008000
#define CPLB_WT					0x00004000
#define CPLB_L1_CHBL			0x00001000
#define CPLB_MEM_LEV			0x00000200
#define CPLB_LRUPRIO			0x00000100
#define CPLB_DIRTY				0x00000080
#define CPLB_SUPV_WR			0x00000010
#define CPLB_USER_WR			0x00000008
#define CPLB_USER_RD			0x00000004
#define CPLB_LOCK				0x00000002
#define CPLB_VALID				0x00000001

#define L1_DMEMORY	(PAGE_SIZE_1MB | CPLB_SUPV_WR | CPLB_USER_WR | CPLB_USER_RD | CPLB_VALID)
#define SDRAM_DNON_CHBL (PAGE_SIZE_4MB | CPLB_SUPV_WR | CPLB_USER_WR | CPLB_USER_RD | CPLB_VALID)
#define SDRAM_DGEN_WB	(PAGE_SIZE_4MB | CPLB_L1_CHBL | CPLB_DIRTY | CPLB_SUPV_WR | CPLB_USER_WR | CPLB_USER_RD | CPLB_VALID)
#define SDRAM_DGEN_WT	(PAGE_SIZE_4MB | CPLB_L1_CHBL | CPLB_WT | CPLB_L1_AOW | CPLB_SUPV_WR | CPLB_USER_WR | CPLB_USER_RD | CPLB_VALID)
#define L1_IMEMORY	(PAGE_SIZE_1MB | CPLB_USER_RD | CPLB_VALID)
#define SDRAM_INON_CHBL	(PAGE_SIZE_4MB | CPLB_USER_RD | CPLB_VALID)
#define SDRAM_IGENERIC	(PAGE_SIZE_4MB | CPLB_L1_CHBL | CPLB_MEM_LEV | CPLB_USER_RD | CPLB_VALID)
/* The following DCPLB DATA are used by gdbproxy to prevent DCPLB missing exception.  */
#define DNON_CHBL_4MB	(PAGE_SIZE_4MB | CPLB_SUPV_WR | CPLB_USER_WR | CPLB_USER_RD | CPLB_VALID)
#define DNON_CHBL_1MB	(PAGE_SIZE_1MB | CPLB_SUPV_WR | CPLB_USER_WR | CPLB_USER_RD | CPLB_VALID)
#define DNON_CHBL_4KB	(PAGE_SIZE_4KB | CPLB_SUPV_WR | CPLB_USER_WR | CPLB_USER_RD | CPLB_VALID)
#define DNON_CHBL_1KB	(PAGE_SIZE_1KB | CPLB_SUPV_WR | CPLB_USER_WR | CPLB_USER_RD | CPLB_VALID)

#define CACHE_LINE_BYTES		32

#define BFIN_DCPLB_NUM			16
#define BFIN_ICPLB_NUM			16

#define DTEST_COMMAND                   0xffe00300
#define DTEST_DATA0                     0xffe00400
#define DTEST_DATA1                     0xffe00404

#define ITEST_COMMAND                   0xffe01300
#define ITEST_DATA0                     0xffe01400
#define ITEST_DATA1                     0xffe01404

#define IN_RANGE(addr, lo, hi) ((addr) >= (lo) && (addr) < (hi))
#define IN_MAP(addr, map) IN_RANGE (addr, map, map##_end)
#define MAP_LEN(map) ((map##_end) - (map))

struct blackfin_dma
{
	uint32_t next_desc_ptr;
	uint32_t start_addr;
	uint16_t config;
	uint16_t x_count;
	uint16_t x_modify;
	uint16_t y_count;
	uint16_t y_modify;
	uint32_t curr_desc_ptr;
	uint32_t curr_addr;
	uint16_t irq_status;
	uint16_t peripheral_map;
	uint16_t curr_x_count;
	uint16_t curr_y_count;
};

struct blackfin_test_data
{
	uint32_t command_addr, data0_addr, data1_addr;
	uint32_t data0_off, data1_off;
	uint32_t data0;
	uint32_t data1;
};

#define RTI_LIMIT(blackfin) ((blackfin->l1_map->l1_code_end - blackfin->l1_map->l1_code) / 8)


static uint32_t mmr_read_clobber_r0(struct blackfin_jtag *jtag_info, int32_t offset, uint32_t size)
{
    uint32_t value;

    assert(size == 2 || size == 4);

    if (offset == 0)
    {
        if (size == 2)
            blackfin_emuir_set_2(jtag_info,
				blackfin_gen_load16z(REG_R0, REG_P0),
				blackfin_gen_move(REG_EMUDAT, REG_R0), TAP_DRPAUSE);
        else
            blackfin_emuir_set_2(jtag_info,
				blackfin_gen_load32(REG_R0, REG_P0),
				blackfin_gen_move(REG_EMUDAT, REG_R0), TAP_DRPAUSE);
    }
    else
    {
        if (size == 2)
            blackfin_emuir_set(jtag_info,
				blackfin_gen_load16z_offset(REG_R0, REG_P0, offset), TAP_IDLE);
        else
            blackfin_emuir_set(jtag_info,
				blackfin_gen_load32_offset(REG_R0, REG_P0, offset), TAP_IDLE);
        blackfin_emuir_set(jtag_info, blackfin_gen_move(REG_EMUDAT, REG_R0), TAP_DRPAUSE);
    }
    value = blackfin_emudat_get(jtag_info, TAP_IDLE);

    return value;
}

static uint32_t mmr_read(struct blackfin_jtag *jtag_info, uint32_t addr, uint32_t size)
{
	struct blackfin_common *blackfin = target_to_blackfin(jtag_info->target);
	uint32_t p0, r0;
	uint32_t value;

	if (addr == DMEM_CONTROL)
    {
		if (size != 4)
		{
			LOG_ERROR("DMEM_CONTROL can only be accessed as 32-bit word");
			/* Return a weird value to notice people.  */
			return 0xfffffff;
		}

		if (blackfin->dmem_control_valid_p)
			return blackfin->dmem_control;
    }
	else if (addr == IMEM_CONTROL)
    {
		if (size != 4)
		{
			LOG_ERROR("IMEM_CONTROL can only be accessed as 32-bit word");
			/* Return a weird value to notice people.  */
			return 0xfffffff;
		}

		if (blackfin->imem_control_valid_p)
			return blackfin->imem_control;
    }


	p0 = blackfin_get_p0(jtag_info);
	r0 = blackfin_get_r0(jtag_info);

	blackfin_set_p0(jtag_info, addr);
	value = mmr_read_clobber_r0(jtag_info, 0, size);

	blackfin_set_p0(jtag_info, p0);
	blackfin_set_r0(jtag_info, r0);

	if (addr == DMEM_CONTROL)
    {
		blackfin->dmem_control = value;
		blackfin->dmem_control_valid_p = 1;
    }
	else if (addr == IMEM_CONTROL)
    {
		blackfin->imem_control = value;
		blackfin->imem_control_valid_p = 1;
    }

	return value;
}

static void mmr_write_clobber_r0(struct blackfin_jtag *jtag_info, int32_t offset, uint32_t data, uint32_t size)
{
    assert (size == 2 || size == 4);

    blackfin_emudat_set(jtag_info, data, TAP_DRPAUSE);

    if (offset == 0)
    {
        if (size == 2)
            blackfin_emuir_set_2(jtag_info,
				blackfin_gen_move(REG_R0, REG_EMUDAT),
				blackfin_gen_store16(REG_P0, REG_R0), TAP_IDLE);
        else
            blackfin_emuir_set_2(jtag_info,
				blackfin_gen_move(REG_R0, REG_EMUDAT),
				blackfin_gen_store32(REG_P0, REG_R0), TAP_IDLE);
    }
    else
    {
        blackfin_emuir_set(jtag_info, blackfin_gen_move(REG_R0, REG_EMUDAT), TAP_IDLE);
        if (size == 2)
            blackfin_emuir_set(jtag_info, blackfin_gen_store16_offset(REG_P0, offset, REG_R0), TAP_IDLE);
        else
            blackfin_emuir_set(jtag_info, blackfin_gen_store32_offset(REG_P0, offset, REG_R0), TAP_IDLE);
    }
}

static void mmr_write(struct blackfin_jtag *jtag_info, uint32_t addr, uint32_t data, int size)
{
	struct blackfin_common *blackfin = target_to_blackfin(jtag_info->target);
	uint32_t p0, r0;

	if (addr == DMEM_CONTROL)
    {
		if (size != 4)
		{
			LOG_ERROR("DMEM_CONTROL can only be accessed as 32-bit word");
			return;
		}

		if (blackfin->dmem_control_valid_p
			&& blackfin->dmem_control == data)
			return;
		else
		{
			blackfin->dmem_control = data;
			blackfin->dmem_control_valid_p = 1;
		}
    }
	else if (addr == IMEM_CONTROL)
    {
		if (size != 4)
		{
			LOG_ERROR("IMEM_CONTROL can only be accessed as 32-bit word");
			return;
		}

		if (blackfin->imem_control_valid_p
			&& blackfin->imem_control == data)
			return;
		else
		{
			blackfin->imem_control = data;
			blackfin->imem_control_valid_p = 1;
		}
    }

	p0 = blackfin_get_p0(jtag_info);
	r0 = blackfin_get_r0(jtag_info);

	blackfin_set_p0(jtag_info, addr);
	mmr_write_clobber_r0(jtag_info, 0, data, size);

	blackfin_set_p0(jtag_info, p0);
	blackfin_set_r0(jtag_info, r0);
}

static void cache_status_get(struct blackfin_jtag *jtag_info)
{
	struct blackfin_common *blackfin = target_to_blackfin(jtag_info->target);
	uint32_t p0, r0;

	/* Instead of calling mmr_read twice, we save one of context save
	   and restore.  */
	if (!blackfin->dmem_control_valid_p
		&& !blackfin->imem_control_valid_p)
    {
		p0 = blackfin_get_p0(jtag_info);
		r0 = blackfin_get_r0(jtag_info);

		blackfin_set_p0(jtag_info, DMEM_CONTROL);

		blackfin->dmem_control = mmr_read_clobber_r0(jtag_info, 0, 4);
		blackfin->dmem_control_valid_p = 1;
		blackfin->imem_control = mmr_read_clobber_r0(jtag_info, IMEM_CONTROL - DMEM_CONTROL, 4);
		blackfin->imem_control_valid_p = 1;

		blackfin_set_p0(jtag_info, p0);
		blackfin_set_r0(jtag_info, r0);
    }
	else if (!blackfin->dmem_control_valid_p)
		/* No need to set dmem_control and dmem_control_valid_p here.
		   mmr_read will handle them.  */
		mmr_read(jtag_info, DMEM_CONTROL, 4);
	else if (!blackfin->imem_control_valid_p)
		/* No need to set imem_control and imem_control_valid_p here.
		   mmr_read will handle them.  */
		mmr_read(jtag_info, IMEM_CONTROL, 4);

	if (blackfin->imem_control & IMC)
		blackfin->l1_code_cache_enabled = 1;
	else
		blackfin->l1_code_cache_enabled = 0;

	if ((blackfin->dmem_control & DMC) == ACACHE_BCACHE)
	{
		blackfin->l1_data_a_cache_enabled = 1;
		blackfin->l1_data_b_cache_enabled = 1;
	}
	else if ((blackfin->dmem_control & DMC) == ACACHE_BSRAM)
	{
		blackfin->l1_data_a_cache_enabled = 1;
		blackfin->l1_data_b_cache_enabled = 0;
	}
	else
	{
		blackfin->l1_data_a_cache_enabled = 0;
		blackfin->l1_data_b_cache_enabled = 0;
	}
}

int blackfin_sdram_init(struct blackfin_jtag *jtag_info)
{
	struct blackfin_common *blackfin = target_to_blackfin(jtag_info->target);
	uint32_t p0, r0, value;

	p0 = blackfin_get_p0(jtag_info);
	r0 = blackfin_get_r0(jtag_info);

	blackfin_set_p0(jtag_info, EBIU_SDGCTL);

	/* Check if SDRAM has been enabled already.
	   If so, don't enable it again.  */
	value = mmr_read_clobber_r0(jtag_info, EBIU_SDSTAT - EBIU_SDGCTL, 2);
	if ((value & 0x8) == 0)
	{
		LOG_DEBUG("%s: SDRAM has already been enabled", target_name(jtag_info->target));
		return ERROR_OK;
	}

	mmr_write_clobber_r0(jtag_info, EBIU_SDRRC - EBIU_SDGCTL,
						 blackfin->sdram_config->sdrrc, 2);
	mmr_write_clobber_r0(jtag_info, EBIU_SDBCTL - EBIU_SDGCTL,
						 blackfin->sdram_config->sdbctl, 2);
	mmr_write_clobber_r0(jtag_info, 0, blackfin->sdram_config->sdgctl, 4);
	blackfin_emuir_set(jtag_info, BLACKFIN_INSN_SSYNC, TAP_IDLE);

	blackfin_set_p0(jtag_info, p0);
	blackfin_set_r0(jtag_info, r0);

	return ERROR_OK;
}

int blackfin_ddr_init(struct blackfin_jtag *jtag_info)
{
	struct blackfin_common *blackfin = target_to_blackfin(jtag_info->target);
	uint32_t p0, r0, value;

	p0 = blackfin_get_p0(jtag_info);
	r0 = blackfin_get_r0(jtag_info);

	blackfin_set_p0(jtag_info, EBIU_DDRCTL0);

	value = mmr_read_clobber_r0(jtag_info, EBIU_RSTCTL - EBIU_DDRCTL0, 2);
	mmr_write_clobber_r0(jtag_info, EBIU_RSTCTL - EBIU_DDRCTL0, value | 0x1, 2);
	blackfin_emuir_set(jtag_info, BLACKFIN_INSN_SSYNC, TAP_IDLE);

	mmr_write_clobber_r0(jtag_info, 0, blackfin->ddr_config->ddrctl0, 4);
	mmr_write_clobber_r0(jtag_info, EBIU_DDRCTL1 - EBIU_DDRCTL0,
						 blackfin->ddr_config->ddrctl1, 4);
	mmr_write_clobber_r0(jtag_info, EBIU_DDRCTL2 - EBIU_DDRCTL0,
						 blackfin->ddr_config->ddrctl2, 4);
	blackfin_emuir_set(jtag_info, BLACKFIN_INSN_SSYNC, TAP_IDLE);

	blackfin_set_p0(jtag_info, p0);
	blackfin_set_r0(jtag_info, r0);

	return ERROR_OK;
}

static void dcplb_enable_clobber_p0r0(struct blackfin_jtag *jtag_info)
{
	struct blackfin_common *blackfin = target_to_blackfin(jtag_info->target);

	blackfin_set_p0(jtag_info, DMEM_CONTROL);

	if (!blackfin->dmem_control_valid_p)
    {
		blackfin_emuir_set(jtag_info, BLACKFIN_INSN_CSYNC, TAP_IDLE);
		blackfin->dmem_control = mmr_read_clobber_r0(jtag_info, 0, 4);
		blackfin->dmem_control_valid_p = 1;
    }

	if (blackfin->dmem_control & ENDCPLB)
		return;

	blackfin->dmem_control |= ENDCPLB;
	mmr_write_clobber_r0(jtag_info, 0, blackfin->dmem_control, 4);
	blackfin_emuir_set(jtag_info, BLACKFIN_INSN_SSYNC, TAP_IDLE);
}

static int dcplb_disable_clobber_p0r0(struct blackfin_jtag *jtag_info)
{
	struct blackfin_common *blackfin = target_to_blackfin(jtag_info->target);
	int orig;

	blackfin_set_p0(jtag_info, DMEM_CONTROL);

	if (!blackfin->dmem_control_valid_p)
	{
		blackfin_emuir_set(jtag_info, BLACKFIN_INSN_CSYNC, TAP_IDLE);
		blackfin->dmem_control = mmr_read_clobber_r0(jtag_info, 0, 4);
		blackfin->dmem_control_valid_p = 1;
	}

	orig = blackfin->dmem_control & ENDCPLB;

	if (orig)
	{
		blackfin->dmem_control &= ~ENDCPLB;
		mmr_write_clobber_r0(jtag_info, 0, blackfin->dmem_control, 4);
		blackfin_emuir_set(jtag_info, BLACKFIN_INSN_SSYNC, TAP_IDLE);
	}

	return orig;
}

static void dma_context_save_clobber_p0r0(struct blackfin_jtag *jtag_info, uint32_t base, struct blackfin_dma *dma)
{
	blackfin_set_p0(jtag_info, base);

	dma->start_addr = mmr_read_clobber_r0(jtag_info, 0x04, 4);
	dma->config = mmr_read_clobber_r0(jtag_info, 0x08, 2);
	dma->x_count = mmr_read_clobber_r0(jtag_info, 0x10, 2);
	dma->x_modify = mmr_read_clobber_r0(jtag_info, 0x14, 2);
	dma->irq_status = mmr_read_clobber_r0(jtag_info, 0x28, 2);
}

static void dma_context_restore_clobber_p0r0(struct blackfin_jtag *jtag_info, uint32_t base, struct blackfin_dma *dma)
{
	blackfin_set_p0(jtag_info, base);

	mmr_write_clobber_r0(jtag_info, 0x04, dma->start_addr, 4);
	mmr_write_clobber_r0(jtag_info, 0x10, dma->x_count, 2);
	mmr_write_clobber_r0(jtag_info, 0x14, dma->x_modify, 2);
	mmr_write_clobber_r0(jtag_info, 0x08, dma->config, 2);
}

static void log_dma(uint32_t base, struct blackfin_dma *dma)
{
	LOG_DEBUG("DMA base [0x%08X]", base);
	LOG_DEBUG("START_ADDR [0x%08X]", dma->start_addr);
	LOG_DEBUG("CONFIG [0x%04X]", dma->config);
	LOG_DEBUG("X_COUNT [0x%04X]", dma->x_count);
	LOG_DEBUG("X_MODIFY [0x%04X]", dma->x_modify);
	LOG_DEBUG("IRQ_STATUS [0x%04X]", dma->irq_status);
}

static int dma_copy(struct blackfin_jtag *jtag_info, uint32_t dest, uint32_t src, uint32_t size)
{
	struct blackfin_common *blackfin = target_to_blackfin(jtag_info->target);
	struct blackfin_dma mdma_s0_save, mdma_d0_save;
	struct blackfin_dma mdma_s0, mdma_d0;
	uint32_t p0, r0;
	uint16_t s0_irq_status, d0_irq_status;
	int retval;
	const struct timespec dma_wait = {0, 50000000};

	blackfin_emupc_reset(jtag_info);

	p0 = blackfin_get_p0(jtag_info);
	r0 = blackfin_get_r0(jtag_info);

	dma_context_save_clobber_p0r0(jtag_info, blackfin->mdma_s0, &mdma_s0_save);
	dma_context_save_clobber_p0r0(jtag_info, blackfin->mdma_d0, &mdma_d0_save);

	log_dma(blackfin->mdma_s0, &mdma_s0_save);
	log_dma(blackfin->mdma_d0, &mdma_d0_save);

	s0_irq_status = mdma_s0_save.irq_status;
	d0_irq_status = mdma_d0_save.irq_status;

	while ((s0_irq_status & DMA_IRQ_STATUS_DMA_RUN) ||
		   (d0_irq_status & DMA_IRQ_STATUS_DMA_RUN))
    {
		if ((s0_irq_status & DMA_IRQ_STATUS_DMA_ERR) ||
			(d0_irq_status & DMA_IRQ_STATUS_DMA_ERR))
			break;

		LOG_DEBUG("wait DMA done: S0:0x%08X [0x%04X] D0:0x%08X [0x%04X]",
				src, s0_irq_status, dest, d0_irq_status);

		nanosleep(&dma_wait, NULL);

		blackfin_set_p0(jtag_info, blackfin->mdma_s0);
		s0_irq_status = mmr_read_clobber_r0(jtag_info, 0x28, 2);
		blackfin_set_p0(jtag_info, blackfin->mdma_d0);
		d0_irq_status = mmr_read_clobber_r0(jtag_info, 0x28, 2);
    }

	if (s0_irq_status & DMA_IRQ_STATUS_DMA_ERR)
		LOG_DEBUG("clear MDMA0_S0 DMA ERR: IRQ_STATUS [0x%04X]",
				s0_irq_status);
	if (s0_irq_status & DMA_IRQ_STATUS_DMA_DONE)
		LOG_DEBUG("clear MDMA_S0 DMA DONE: IRQ_STATUS [0x%04X]",
				s0_irq_status);
	if (d0_irq_status & DMA_IRQ_STATUS_DMA_ERR)
		LOG_DEBUG("clear MDMA0_D0 DMA ERR: IRQ_STATUS [0x%04X]",
				d0_irq_status);
	if (d0_irq_status & DMA_IRQ_STATUS_DMA_DONE)
		LOG_DEBUG("clear MDMA_D0 DMA DONE: IRQ_STATUS [0x%04X]",
				d0_irq_status);

	if (s0_irq_status & (DMA_IRQ_STATUS_DMA_ERR | DMA_IRQ_STATUS_DMA_DONE))
    {
		blackfin_set_p0(jtag_info, blackfin->mdma_s0);
		mmr_write_clobber_r0(jtag_info,
			0x28, DMA_IRQ_STATUS_DMA_ERR | DMA_IRQ_STATUS_DMA_DONE, 2);
    }

	if (d0_irq_status & (DMA_IRQ_STATUS_DMA_ERR | DMA_IRQ_STATUS_DMA_DONE))
    {
		blackfin_set_p0(jtag_info, blackfin->mdma_d0);
		mmr_write_clobber_r0(jtag_info,
			0x28, DMA_IRQ_STATUS_DMA_ERR | DMA_IRQ_STATUS_DMA_DONE, 2);
    }

	blackfin_set_p0(jtag_info, blackfin->mdma_s0);
	s0_irq_status = mmr_read_clobber_r0(jtag_info, 0x28, 2);
	blackfin_set_p0(jtag_info, blackfin->mdma_d0);
	d0_irq_status = mmr_read_clobber_r0(jtag_info, 0x28, 2);

	LOG_DEBUG("before dma copy MDMA0_S0 IRQ_STATUS [0x%04X]", s0_irq_status);
	LOG_DEBUG("before dma copy MDMA0_D0 IRQ_STATUS [0x%04X]", d0_irq_status);

	mdma_s0.start_addr = src;
	mdma_s0.x_count = size;
	mdma_s0.x_modify = 1;
	mdma_s0.config = DMA_CONFIG_FLOW_STOP | DMA_CONFIG_NDSIZE_0;
	mdma_s0.config |= DMA_CONFIG_WDSIZE_8 | DMA_CONFIG_DMAEN | DMA_CONFIG_DI_EN;

	mdma_d0.start_addr = dest;
	mdma_d0.x_count = size;
	mdma_d0.x_modify = 1;
	mdma_d0.config =
		DMA_CONFIG_FLOW_STOP | DMA_CONFIG_NDSIZE_0 | DMA_CONFIG_WNR;
	mdma_d0.config |= DMA_CONFIG_WDSIZE_8 | DMA_CONFIG_DMAEN | DMA_CONFIG_DI_EN;

	dma_context_restore_clobber_p0r0(jtag_info, blackfin->mdma_s0, &mdma_s0);
	dma_context_restore_clobber_p0r0(jtag_info, blackfin->mdma_d0, &mdma_d0);

 wait_dma:

	blackfin_set_p0(jtag_info, blackfin->mdma_s0);
	s0_irq_status = mmr_read_clobber_r0(jtag_info, 0x28, 2);
	blackfin_set_p0(jtag_info, blackfin->mdma_d0);
	d0_irq_status = mmr_read_clobber_r0(jtag_info, 0x28, 2);

	if (s0_irq_status & DMA_IRQ_STATUS_DMA_ERR)
    {
		LOG_ERROR("MDMA0_S0 DMA error: 0x%08X: IRQ_STATUS [0x%04X]",
			src, s0_irq_status);
		retval = ERROR_FAIL;
		goto finish_dma_copy;
    }
	if (d0_irq_status & DMA_IRQ_STATUS_DMA_ERR)
    {
		LOG_ERROR("MDMA0_D0 DMA error: 0x%08X: IRQ_STATUS [0x%04X]",
			dest, d0_irq_status);
		retval = ERROR_FAIL;
		goto finish_dma_copy;
    }
	else if (!(s0_irq_status & DMA_IRQ_STATUS_DMA_DONE))
    {
		LOG_INFO("MDMA_S0 wait for done: IRQ_STATUS [0x%04X]", s0_irq_status);
		nanosleep (&dma_wait, NULL);
		goto wait_dma;
    }
	else if (!(d0_irq_status & DMA_IRQ_STATUS_DMA_DONE))
    {
		LOG_INFO("MDMA_D0 wait for done: IRQ_STATUS [0x%04X]", d0_irq_status);
		nanosleep (&dma_wait, NULL);
		goto wait_dma;
    }
	else
		retval = ERROR_OK;

	LOG_DEBUG("done dma copy MDMA_S0: 0x%08X: IRQ_STATUS [0x%04X]",
		src, s0_irq_status);

	LOG_DEBUG("done dma copy MDMA_D0: 0x%08X: IRQ_STATUS [0x%04X]",
		dest, d0_irq_status);

	if ((s0_irq_status & DMA_IRQ_STATUS_DMA_ERR)
		|| (s0_irq_status & DMA_IRQ_STATUS_DMA_DONE))
    {
		blackfin_set_p0(jtag_info, blackfin->mdma_s0);
		mmr_write_clobber_r0(jtag_info,
			0x28, DMA_IRQ_STATUS_DMA_ERR | DMA_IRQ_STATUS_DMA_DONE, 2);
    }

	if ((d0_irq_status & DMA_IRQ_STATUS_DMA_ERR)
		|| (d0_irq_status & DMA_IRQ_STATUS_DMA_DONE))
    {
		blackfin_set_p0(jtag_info, blackfin->mdma_d0);
		mmr_write_clobber_r0(jtag_info,
			0x28, DMA_IRQ_STATUS_DMA_ERR | DMA_IRQ_STATUS_DMA_DONE, 2);
    }


	dma_context_restore_clobber_p0r0(jtag_info, blackfin->mdma_s0, &mdma_s0_save);
	dma_context_restore_clobber_p0r0(jtag_info, blackfin->mdma_d0, &mdma_d0_save);

 finish_dma_copy:

	blackfin_set_p0(jtag_info, p0);
	blackfin_set_r0(jtag_info, r0);

	return retval;
}

/* TODO optimize it by using blackfin_emudat_defer_get and blackfin_emudat_get_done.  */

static int memory_read_1(struct blackfin_jtag *jtag_info, uint32_t addr, uint8_t *buf, int size)
{
	uint32_t p0, r0;
	bool dcplb_enabled;

	assert (size > 0);

	p0 = blackfin_get_p0(jtag_info);
	r0 = blackfin_get_r0(jtag_info);

	dcplb_enabled = dcplb_disable_clobber_p0r0(jtag_info);

	blackfin_set_p0(jtag_info, addr);

	if ((addr & 0x3) != 0)
		blackfin_emuir_set_2(jtag_info,
				blackfin_gen_load8zpi(REG_R0, REG_P0),
				blackfin_gen_move(REG_EMUDAT, REG_R0), TAP_DRPAUSE);

	while ((addr & 0x3) != 0 && size != 0)
    {
		*buf++ = blackfin_emudat_get(jtag_info, TAP_IDLE);
		addr++;
		size--;
    }
	if (size == 0)
		goto finish_read;

	if (size >= 4)
		blackfin_emuir_set_2(jtag_info,
				blackfin_gen_load32pi(REG_R0, REG_P0),
				blackfin_gen_move(REG_EMUDAT, REG_R0), TAP_DRPAUSE);

	for (; size >= 4; size -= 4)
    {
		uint32_t data;

		data = blackfin_emudat_get(jtag_info, TAP_IDLE);
		*buf++ = data & 0xff;
		*buf++ = (data >> 8) & 0xff;
		*buf++ = (data >> 16) & 0xff;
		*buf++ = (data >> 24) & 0xff;
    }

	if (size == 0)
		goto finish_read;

	blackfin_emuir_set_2 (jtag_info,
		blackfin_gen_load8zpi(REG_R0, REG_P0),
		blackfin_gen_move(REG_EMUDAT, REG_R0), TAP_DRPAUSE);

	for (; size > 0; size--)
		*buf++ = blackfin_emudat_get(jtag_info, TAP_IDLE);

 finish_read:

	if (dcplb_enabled)
		dcplb_enable_clobber_p0r0(jtag_info);

	blackfin_set_p0(jtag_info, p0);
	blackfin_set_r0(jtag_info, r0);

	return 0;
}

static int memory_read(struct blackfin_jtag *jtag_info, uint32_t addr, uint32_t size, uint8_t *buf)
{
	struct blackfin_common *blackfin = target_to_blackfin(jtag_info->target);
	uint32_t s;

	if ((addr & 0x3) != 0)
    {
		s = 4 - (addr & 0x3);
		s = size < s ? size : s;
		memory_read_1(jtag_info, addr, buf, s);
		size -= s;
		addr += s;
		buf += s;
    }

	while (size > 0)
    {
		blackfin_emupc_reset(jtag_info);

		/* The overhead should be no larger than 0x20.  */
		s = (RTI_LIMIT(blackfin) - 0x20) * 4;
		s = size < s ? size : s;
		memory_read_1(jtag_info, addr, buf, s);
		size -= s;
		addr += s;
		buf += s;
    }

	return ERROR_OK;
}

static int memory_write_1 (struct blackfin_jtag *jtag_info, uint32_t addr, uint8_t *buf, int size)
{
	uint32_t p0, r0;
	bool dcplb_enabled;

	assert (size > 0);

	p0 = blackfin_get_p0(jtag_info);
	r0 = blackfin_get_r0(jtag_info);

	dcplb_enabled = dcplb_disable_clobber_p0r0(jtag_info);

	blackfin_set_p0(jtag_info, addr);

	if ((addr & 0x3) != 0)
		blackfin_emuir_set_2(jtag_info,
				blackfin_gen_move(REG_R0, REG_EMUDAT),
				blackfin_gen_store8pi(REG_P0, REG_R0), TAP_DRPAUSE);

	while ((addr & 0x3) != 0 && size != 0)
    {
		blackfin_emudat_set(jtag_info, *buf++, TAP_IDLE);
		addr++;
		size--;
    }
	if (size == 0)
		goto finish_write;

	if (size >= 4)
		blackfin_emuir_set_2(jtag_info,
				blackfin_gen_move(REG_R0, REG_EMUDAT),
				blackfin_gen_store32pi(REG_P0, REG_R0), TAP_DRPAUSE);

	for (; size >= 4; size -= 4)
    {
		uint32_t data;

		data = *buf++;
		data |= (*buf++) << 8;
		data |= (*buf++) << 16;
		data |= (*buf++) << 24;
		blackfin_emudat_set(jtag_info, data, TAP_IDLE);
    }

	if (size == 0)
		goto finish_write;

	blackfin_emuir_set_2(jtag_info,
		blackfin_gen_move(REG_R0, REG_EMUDAT),
		blackfin_gen_store8pi(REG_P0, REG_R0), TAP_DRPAUSE);

	for (; size > 0; size--)
		blackfin_emudat_set(jtag_info, *buf++, TAP_IDLE);

 finish_write:

	if (dcplb_enabled)
		dcplb_enable_clobber_p0r0(jtag_info);

	blackfin_set_p0(jtag_info, p0);
	blackfin_set_r0(jtag_info, r0);

	return 0;
}

static int memory_write(struct blackfin_jtag *jtag_info, uint32_t addr, uint32_t size, uint8_t *buf)
{
	struct blackfin_common *blackfin = target_to_blackfin(jtag_info->target);
	uint32_t s;

	if ((addr & 0x3) != 0)
    {
		s = 4 - (addr & 0x3);
		s = size < s ? size : s;
		memory_write_1(jtag_info, addr, buf, s);
		size -= s;
		addr += s;
		buf += s;
    }

	while (size > 0)
    {
		blackfin_emupc_reset(jtag_info);

		/* The overhead should be no larger than 0x20.  */
		s = (RTI_LIMIT(blackfin) - 0x20) * 4;
		s = size < s ? size : s;
		memory_write_1(jtag_info, addr, buf, s);
		size -= s;
		addr += s;
		buf += s;
    }

	return ERROR_OK;
}

static int dma_sram_read(struct blackfin_jtag *jtag_info, uint32_t address,
		uint32_t size, uint8_t *buffer)
{
	struct blackfin_common *blackfin = target_to_blackfin(jtag_info->target);
	uint32_t l1_addr;
	uint8_t *tmp;
	int retval;

	l1_addr = blackfin->l1_map->l1_data_a;

	assert(size < 0x4000);

	tmp = (uint8_t *) malloc(size);
	if (tmp == NULL)
		abort();

	memory_read(jtag_info, l1_addr, size, tmp);

	retval = dma_copy(jtag_info, l1_addr, address, size);

	memory_read(jtag_info, l1_addr, size, buffer);

	memory_write(jtag_info, l1_addr, size, buffer);

	free(tmp);

	return retval;
}

static int dma_sram_write(struct blackfin_jtag *jtag_info, uint32_t address,
		uint32_t size, uint8_t *buffer)
{
	struct blackfin_common *blackfin = target_to_blackfin(jtag_info->target);
	uint32_t l1_addr;
	uint8_t *tmp;
	int retval;

	l1_addr = blackfin->l1_map->l1_data_a;

	assert(l1_addr);
	assert(size <= MAP_LEN(blackfin->l1_map->l1_data_a));

	tmp = (uint8_t *) malloc(size);
	if (tmp == NULL)
		abort();

	memory_read(jtag_info, l1_addr, size, tmp);

	memory_write(jtag_info, l1_addr, size, buffer);

	retval = dma_copy(jtag_info, address, l1_addr, size);

	memory_write(jtag_info, l1_addr, size, tmp);

	free(tmp);

	return retval;
}

/* Read Instruction SRAM using Instruction Test Registers.  */

static void test_command (struct blackfin_jtag *jtag_info, uint32_t addr, bool write_p,
		uint32_t command_addr, uint32_t *command_value)
{
	struct blackfin_common *blackfin = target_to_blackfin(jtag_info->target);

    /* The shifting here is a bit funky, but should be straight forward and
       easier to maintain than hand coded masks.  So, start with the break
       down of the [DI]TEST_COMMAND MMR in the HRM and follow along:

       We need to put bit 11 of the address into bit 26 of the MMR.  So first
       we mask off ADDR[11] with:
       (addr & (1 << 11))

       Then we shift it from its current position (11) to its new one (26):
       ((...) << (26 - 11))
	*/

    /* Start with the bits ITEST/DTEST share.  */
    *command_value =
        ((addr & (0x03 << 12)) << (16 - 12)) | /* ADDR[13:12] -> MMR[17:16] */
        ((addr & (0x01 << 14)) << (14 - 14)) | /* ADDR[14]    -> MMR[14]    */
        ((addr & (0xff <<  3)) << ( 3 -  3)) | /* ADDR[10:3]  -> MMR[10:3]  */
        (1 << 2)                             | /* 1 (data)    -> MMR[2]     */
        (write_p << 1);                        /* read/write  -> MMR[1]     */

    /* Now for the bits that aren't the same.  */
    if (command_addr == ITEST_COMMAND)
        *command_value |=
            ((addr & (0x03 << 10)) << (26 - 10));  /* ADDR[11:10] -> MMR[27:26] */
    else
        *command_value |=
            ((addr & (0x01 << 11)) << (26 - 11)) | /* ADDR[11] -> MMR[26] */
            ((addr & (0x01 << 21)) << (24 - 21));  /* ADDR[21] -> MMR[24] */

    /* Now, just for fun, some parts are slightly different.  */
    if (command_addr == DTEST_COMMAND)
    {
        /* BF50x has no additional needs.  */
        if (!strcasecmp (blackfin->part, "BF518"))
        {
            /* MMR[23]:
               0 - Data Bank A (0xff800000) / Inst Bank A (0xffa00000)
               1 - Data Bank B (0xff900000) / Inst Bank B (0xffa04000)
			*/
            if ((addr & 0xfff04000) == 0xffa04000 ||
                (addr & 0xfff00000) == 0xff900000)
                *command_value |= (1 << 23);
        }
        else if (!strcasecmp (blackfin->part, "BF526") ||
                 !strcasecmp (blackfin->part, "BF527") ||
                 !strcasecmp (blackfin->part, "BF533") ||
                 !strcasecmp (blackfin->part, "BF534") ||
                 !strcasecmp (blackfin->part, "BF537") ||
                 !strcasecmp (blackfin->part, "BF538") ||
                 !strcasecmp (blackfin->part, "BF548") ||
                 !strcasecmp (blackfin->part, "BF548M"))
        {
            /* MMR[23]:
               0 - Data Bank A (0xff800000) / Inst Bank A (0xffa00000)
               1 - Data Bank B (0xff900000) / Inst Bank B (0xffa08000)
			*/
            if ((addr & 0xfff08000) == 0xffa08000 ||
                (addr & 0xfff00000) == 0xff900000)
                *command_value |= (1 << 23);
        }
        else if (!strcasecmp (blackfin->part, "BF561"))
        {
            /* MMR[23]:
               0 - Data Bank A (Core A: 0xff800000 Core B: 0xff400000)
			   Inst Bank A (Core A: 0xffa00000 Core B: 0xff600000)
               1 - Data Bank B (Core A: 0xff900000 Core B: 0xff500000)
			   N/A for Inst (no Bank B)
			*/
            uint32_t hi = (addr >> 20);
            if (hi == 0xff9 || hi == 0xff5)
                *command_value |= (1 << 23);
        }
        else if (!strcasecmp (blackfin->part, "BF592"))
        {
            /* ADDR[15] -> MMR[15]
               MMR[22]:
               0 - L1 Inst (0xffa00000)
               1 - L1 ROM  (0xffa10000)
			*/
            *command_value |= (addr & (1 << 15));
            if ((addr >> 16) == 0xffa1)
                *command_value |= (1 << 22);
        }
    }
}

/* Do one ITEST read.  ADDR should be aligned to 8 bytes. BUF should
   be enough large to hold 64 bits.  */
static void itest_read_clobber_r0 (struct blackfin_jtag *jtag_info,
		struct blackfin_test_data *test_data, uint32_t addr, uint8_t *buf)
{
	uint32_t command, data1, data0;

	assert ((addr & 0x7) == 0);

	test_command (jtag_info, addr, false, test_data->command_addr, &command);

	mmr_write_clobber_r0 (jtag_info, 0, command, 4);
	blackfin_emuir_set (jtag_info, BLACKFIN_INSN_CSYNC, TAP_IDLE);
	data1 = mmr_read_clobber_r0 (jtag_info, test_data->data1_off, 4);
	data0 = mmr_read_clobber_r0 (jtag_info, test_data->data0_off, 4);

	*buf++ = data0 & 0xff;
	*buf++ = (data0 >> 8) & 0xff;
	*buf++ = (data0 >> 16) & 0xff;
	*buf++ = (data0 >> 24) & 0xff;
	*buf++ = data1 & 0xff;
	*buf++ = (data1 >> 8) & 0xff;
	*buf++ = (data1 >> 16) & 0xff;
	*buf++ = (data1 >> 24) & 0xff;
}

/* Do one ITEST write.  ADDR should be aligned to 8 bytes. BUF should
   be enough large to hold 64 bits.  */
static void itest_write_clobber_r0 (struct blackfin_jtag *jtag_info,
		struct blackfin_test_data *test_data, uint32_t addr, uint8_t *buf)
{
	uint32_t command, data1, data0;

	assert ((addr & 0x7) == 0);

	test_command(jtag_info, addr, true, test_data->command_addr, &command);

	data0 = *buf++;
	data0 |= (*buf++) << 8;
	data0 |= (*buf++) << 16;
	data0 |= (*buf++) << 24;
	data1 = *buf++;
	data1 |= (*buf++) << 8;
	data1 |= (*buf++) << 16;
	data1 |= (*buf++) << 24;

	mmr_write_clobber_r0(jtag_info, test_data->data1_off, data1, 4);
	mmr_write_clobber_r0(jtag_info, test_data->data0_off, data0, 4);
	mmr_write_clobber_r0(jtag_info, 0, command, 4);
	blackfin_emuir_set(jtag_info, BLACKFIN_INSN_CSYNC, TAP_IDLE);
}

static void test_context_save_clobber_r0(struct blackfin_jtag *jtag_info, struct blackfin_test_data *test_data)
{
	test_data->data1 = mmr_read_clobber_r0(jtag_info, test_data->data1_off, 4);
	test_data->data0 = mmr_read_clobber_r0(jtag_info, test_data->data0_off, 4);
}

static void test_context_restore_clobber_r0(struct blackfin_jtag *jtag_info, struct blackfin_test_data *test_data)
{
	mmr_write_clobber_r0 (jtag_info, test_data->data1_off, test_data->data1, 4);
	mmr_write_clobber_r0 (jtag_info, test_data->data0_off, test_data->data0, 4);
	/* Yeah, TEST_COMMAND is reset to 0, i.e. clobbered!  But it should
	   not do any harm to any reasonable user programs.  Codes trying to
	   peek TEST_COMMAND might be affected.  But why do such codes
	   exist?  */
	mmr_write_clobber_r0 (jtag_info, 0, 0, 4);
	blackfin_emuir_set (jtag_info, BLACKFIN_INSN_CSYNC, TAP_IDLE);
}

static void test_command_mmrs(struct blackfin_jtag *jtag_info, uint32_t addr, int icache, uint32_t *command_addr, uint32_t *data0_addr, uint32_t *data1_addr)
{
    if (icache) {
        *command_addr = ITEST_COMMAND;
        *data0_addr = ITEST_DATA0;
        *data1_addr = ITEST_DATA1;
    } else {
        *command_addr = DTEST_COMMAND;
        *data0_addr = DTEST_DATA0;
        *data1_addr = DTEST_DATA1;
    }
}

static int itest_sram_1(struct blackfin_jtag *jtag_info, uint32_t addr, uint8_t *buf, uint32_t size, int write_p)
{
	struct blackfin_common *blackfin = target_to_blackfin(jtag_info->target);
	struct blackfin_test_data test_data;
	uint32_t p0, r0;
	uint8_t data[8];

	p0 = blackfin_get_p0(jtag_info);
	r0 = blackfin_get_r0(jtag_info);

	test_command_mmrs(jtag_info, addr,
		IN_MAP(addr, blackfin->l1_map->l1_code_cache),
		&test_data.command_addr,
		&test_data.data0_addr,
		&test_data.data1_addr);
	test_data.data0_off = test_data.data0_addr - test_data.command_addr;
	test_data.data1_off = test_data.data1_addr - test_data.command_addr;
	blackfin_register_set(jtag_info, REG_P0, test_data.command_addr);

	test_context_save_clobber_r0(jtag_info, &test_data);

	if ((addr & 0x7) != 0)
    {
		uint32_t aligned_addr = addr & 0xfffffff8;

		itest_read_clobber_r0(jtag_info, &test_data, aligned_addr, data);

		if (write_p)
		{
			while ((addr & 0x7) != 0 && size != 0)
			{
				data[addr & 0x7] = *buf++;
				addr++;
				size--;
			}

			itest_write_clobber_r0(jtag_info, &test_data, aligned_addr, data);
		}
		else
		{
			while ((addr & 0x7) != 0 && size != 0)
			{
				*buf++ = data[addr & 0x7];
				addr++;
				size--;
			}
		}
    }

	for (; size >= 8; size -= 8)
    {
		if (write_p)
			itest_write_clobber_r0(jtag_info, &test_data, addr, buf);
		else
			itest_read_clobber_r0(jtag_info, &test_data, addr, buf);
		addr += 8;
		buf += 8;
    }

	if (size != 0)
    {
		itest_read_clobber_r0(jtag_info, &test_data, addr, data);

		if (write_p)
		{
			memcpy(data, buf, size);
			itest_write_clobber_r0(jtag_info, &test_data, addr, data);
		}
		else
		{
			memcpy(buf, data, size);
		}
    }

	test_context_restore_clobber_r0(jtag_info, &test_data);

	blackfin_set_p0(jtag_info, p0);
	blackfin_set_r0(jtag_info, r0);

	return 0;
}

static int itest_sram(struct blackfin_jtag *jtag_info, uint32_t addr,
		uint32_t size, uint8_t *buf, bool write_p)
{
	struct blackfin_common *blackfin = target_to_blackfin(jtag_info->target);
    uint32_t s;

	if ((addr & 0x7) != 0)
    {
		s = 8 - (addr & 0x7);
		s = size < s ? size : s;
		itest_sram_1(jtag_info, addr, buf, s, write_p);
		size -= s;
		addr += s;
		buf += s;
    }

	while (size > 0)
    {
		blackfin_emupc_reset(jtag_info);

		/* The overhead should be no larger than 0x20.  */
		s = (RTI_LIMIT(blackfin) - 0x20) / 25 * 32;
		s = size < s ? size : s;
		/* We also need to keep transfers from spanning SRAM banks.
		   The core itest logic looks up appropriate MMRs once per
		   call, and different SRAM banks might require a different
		   set of MMRs.  */
		if ((addr / 0x4000) != ((addr + s) / 0x4000))
			s -= ((addr + s) % 0x4000);
		itest_sram_1(jtag_info, addr, buf, s, write_p);
		size -= s;
		addr += s;
		buf += s;
    }

	return ERROR_OK;
}

static int sram_read_write(struct blackfin_jtag *jtag_info, uint32_t addr,
		uint32_t size, uint8_t *buf, bool dma_p, bool write_p)
{
	struct blackfin_common *blackfin = target_to_blackfin(jtag_info->target);

	assert(!dma_p || blackfin->mdma_d0 != 0);

	/* FIXME  Add an option to control using dma or itest. BF592 can only use itest for its L1 ROM. */
	if (dma_p)
	{
		if (write_p)
			return dma_sram_write(jtag_info, addr, size, buf);
		else
			return dma_sram_read(jtag_info, addr, size, buf);
	}
	else
		return itest_sram(jtag_info, addr, size, buf, write_p);
}

static int sram_read(struct blackfin_jtag *jtag_info, uint32_t addr,
		uint32_t size, uint8_t *buf, bool dma_p)
{
	return sram_read_write(jtag_info, addr, size, buf, dma_p, false);
}

static int sram_write(struct blackfin_jtag *jtag_info, uint32_t addr,
		uint32_t size, uint8_t *buf, bool dma_p)
{
	return sram_read_write(jtag_info, addr, size, buf, dma_p, true);
}

int blackfin_read_mem(struct blackfin_jtag *jtag_info, uint32_t addr,
		uint32_t size, uint8_t *buf)
{
	struct blackfin_common *blackfin = target_to_blackfin(jtag_info->target);
	int retval;
	uint32_t value;
	uint32_t end;

	LOG_DEBUG("bfin_read_mem(0x%08X, %d)", addr, size);

	blackfin_emupc_reset(jtag_info);

	if (!IN_MAP(addr, blackfin->mem_map->l1))
		goto skip_l1;

	if (IN_MAP(addr, blackfin->l1_map->l1))
		cache_status_get(jtag_info);

	if (IN_MAP(addr, blackfin->l1_map->l1_code))
	{
		if (!blackfin->l1_code_cache_enabled &&
			(blackfin->l1_map->l1_code_end == blackfin->l1_map->l1_code_cache))
			end = blackfin->l1_map->l1_code_cache_end;
		else
			end = blackfin->l1_map->l1_code_end;

		if (addr + size > end)
			size = end - addr;

		retval = sram_read(jtag_info, addr, size, buf, false);
		goto done;
	}
	else if (!blackfin->l1_code_cache_enabled &&
			 IN_MAP(addr, blackfin->l1_map->l1_code_cache))
	{
		if (addr + size > blackfin->l1_map->l1_code_cache_end)
			size = blackfin->l1_map->l1_code_cache_end - addr;

		retval = sram_read(jtag_info, addr, size, buf, false);
		goto done;
	}
	else if (IN_MAP(addr, blackfin->l1_map->l1_code_rom))
	{
		if (addr + size > blackfin->l1_map->l1_code_rom_end)
			size = blackfin->l1_map->l1_code_rom_end - addr;

		retval = sram_read(jtag_info, addr, size, buf, false);
		goto done;
	}
	else if (IN_MAP(addr, blackfin->l1_map->l1_data_a))
	{
		if (!blackfin->l1_data_a_cache_enabled &&
			(blackfin->l1_map->l1_data_a_end == blackfin->l1_map->l1_data_a_cache))
			end = blackfin->l1_map->l1_data_a_cache_end;
		else
			end = blackfin->l1_map->l1_data_a_end;

		if (addr + size > end)
			size = end - addr;

		retval = memory_read(jtag_info, addr, size, buf);
		goto done;
	}
	else if (!blackfin->l1_data_a_cache_enabled &&
			 IN_MAP (addr, blackfin->l1_map->l1_data_a_cache))
	{
		if (addr + size > blackfin->l1_map->l1_data_a_cache_end)
			size = blackfin->l1_map->l1_data_a_cache_end - addr;

		retval = memory_read(jtag_info, addr, size, buf);
		goto done;
	}
	else if (IN_MAP (addr, blackfin->l1_map->l1_data_b))
	{
		if (!blackfin->l1_data_b_cache_enabled
			&& (blackfin->l1_map->l1_data_b_end
				== blackfin->l1_map->l1_data_b_cache))
			end = blackfin->l1_map->l1_data_b_cache_end;
		else
			end = blackfin->l1_map->l1_data_b_end;

		if (addr + size > end)
			size = end - addr;

		retval = memory_read(jtag_info, addr, size, buf);
		goto done;
	}
	else if (!blackfin->l1_data_b_cache_enabled &&
			 IN_MAP (addr, blackfin->l1_map->l1_data_b_cache))
	{
		if (addr + size > blackfin->l1_map->l1_data_b_cache_end)
			size = blackfin->l1_map->l1_data_b_cache_end - addr;

		retval = memory_read(jtag_info, addr, size, buf);
		goto done;
	}
	else if (IN_MAP (addr, blackfin->l1_map->l1_scratch))
	{
		if (addr + size > blackfin->l1_map->l1_scratch_end)
			size = blackfin->l1_map->l1_scratch_end - addr;

		retval = memory_read(jtag_info, addr, size, buf);
		goto done;
	}
	else if (IN_MAP (addr, blackfin->l1_map->l1))
	{
		LOG_ERROR("invalid L1 memory address [0x%08X]", addr);
		retval = ERROR_FAIL;
		goto done;
	}

 skip_l1:

	/* TODO  Accurately check MMR validity.  */

	if ((IN_RANGE (addr, blackfin->mem_map->sysmmr, blackfin->mem_map->coremmr)
		 && addr + size > blackfin->mem_map->coremmr)
		|| (addr >= blackfin->mem_map->coremmr && addr + size < addr))
    {
		LOG_ERROR("bad MMR addr or size [0x%08X] size %d", addr, size);
		return ERROR_FAIL;
    }
  
	if (addr >= blackfin->mem_map->sysmmr)
    {
		if (size != 2 && size != 4)
		{
			LOG_ERROR("bad MMR size [0x%08X] size %d", addr, size);
			return ERROR_FAIL;
		}

		if ((addr & 0x1) == 1)
		{
			LOG_ERROR("odd MMR addr [0x%08X]", addr);
			return ERROR_FAIL;
		}

		value = mmr_read(jtag_info, addr, size);
		*buf++ = value & 0xff;
		*buf++ = (value >> 8) & 0xff;
		if (size == 4)
		{
			*buf++ = (value >> 16) & 0xff;
			*buf++ = (value >> 24) & 0xff;
		}
		retval = ERROR_OK;
		goto done;
    }

	if (IN_MAP (addr, blackfin->mem_map->sdram))
    {
		if (addr + size > blackfin->mem_map->sdram_end)
			size = blackfin->mem_map->sdram_end - addr;

		retval = memory_read(jtag_info, addr, size, buf);
    }
	else if (IN_MAP (addr, blackfin->mem_map->async_mem))
    {
		if (addr + size > blackfin->mem_map->async_mem_end)
			size = blackfin->mem_map->async_mem_end - addr;

		retval = memory_read(jtag_info, addr, size, buf);
    }
	/* TODO  Allow access to devices mapped to async mem.  */
	else if (IN_MAP (addr, blackfin->mem_map->boot_rom))
    {
		if (addr + size > blackfin->mem_map->boot_rom_end)
			size = blackfin->mem_map->boot_rom_end - addr;
		retval = memory_read(jtag_info, addr, size, buf);
    }
	else if (IN_MAP (addr, blackfin->mem_map->l2_sram))
    {
		if (addr + size > blackfin->mem_map->l2_sram_end)
			size = blackfin->mem_map->l2_sram_end - addr;

		retval = memory_read(jtag_info, addr, size, buf);
    }
	else
    {
			LOG_ERROR("invalid L1 memory address [0x%08X]", addr);
			retval = ERROR_FAIL;
    }

 done:

	return retval;
}

static void icache_flush(struct blackfin_jtag *jtag_info, uint32_t addr, int size)
{
	struct blackfin_common *blackfin = target_to_blackfin(jtag_info->target);
	uint32_t p0, r0;
	int i;

	assert (size > 0);

	p0 = blackfin_get_p0(jtag_info);

	if (!blackfin->imem_control_valid_p)
    {
		r0 = blackfin_get_r0(jtag_info);
		blackfin_set_p0(jtag_info, IMEM_CONTROL);
		blackfin_emuir_set(jtag_info, BLACKFIN_INSN_CSYNC, TAP_IDLE);
		blackfin->imem_control = mmr_read_clobber_r0(jtag_info, 0, 4);
		blackfin->imem_control_valid_p = 1;
		blackfin_set_r0(jtag_info, r0);
    }

	if (blackfin->imem_control & IMC)
    {
		blackfin_set_p0(jtag_info, addr);
		for (i = (size + addr % CACHE_LINE_BYTES - 1) / CACHE_LINE_BYTES + 1;
			 i > 0; i--)
			blackfin_emuir_set(jtag_info, blackfin_gen_iflush_pm(REG_P0), TAP_IDLE);
    }

	blackfin_set_p0(jtag_info, p0);
}

int blackfin_write_mem(struct blackfin_jtag *jtag_info, uint32_t addr,
		uint32_t size, const uint8_t *buffer)
{
	struct blackfin_common *blackfin = target_to_blackfin(jtag_info->target);
	int retval;
	uint32_t value;
	uint32_t end;
	uint8_t *buf = (uint8_t *) buffer; /* TODO remove this cast.  */

	LOG_DEBUG("bfin_write_mem(0x%08X, %d)", addr, size);

	blackfin_emupc_reset(jtag_info);

	if (!IN_MAP (addr, blackfin->mem_map->l1))
		goto skip_l1;

	if (IN_MAP (addr, blackfin->l1_map->l1))
		cache_status_get(jtag_info);

	if (IN_MAP(addr, blackfin->l1_map->l1_code) &&
		((!blackfin->l1_code_cache_enabled &&
		  (blackfin->l1_map->l1_code_end == blackfin->l1_map->l1_code_cache)
		  && (end = blackfin->l1_map->l1_code_cache_end))
		 || (end = blackfin->l1_map->l1_code_end))
		&& addr + size <= end)
	{
		retval = sram_write(jtag_info, addr, size, buf, false);
		icache_flush(jtag_info, addr, size);
		goto done;
	}
	else if (IN_MAP (addr, blackfin->l1_map->l1_code_cache) &&
			 addr + size <= blackfin->l1_map->l1_code_cache_end)
	{
		if (blackfin->l1_code_cache_enabled)
	    {
			LOG_ERROR("cannot write L1 icache when enabled [0x%08X] size %d", addr, size);
			return ERROR_FAIL;
	    }

		retval = sram_write(jtag_info, addr, size, buf, false);
		goto done;
	}
	else if (IN_MAP (addr, blackfin->l1_map->l1_data_a) &&
			 ((!blackfin->l1_data_a_cache_enabled &&
			   (blackfin->l1_map->l1_data_a_end == blackfin->l1_map->l1_data_a_cache)
			   && (end = blackfin->l1_map->l1_data_a_cache_end))
			  || (end = blackfin->l1_map->l1_data_a_end))
			 && addr + size <= end)
	{
		retval = memory_write(jtag_info, addr, size, buf);
		goto done;
	}
	else if (IN_MAP (addr, blackfin->l1_map->l1_data_a_cache) &&
			 addr + size <= blackfin->l1_map->l1_data_a_cache_end)
	{
		if (blackfin->l1_data_a_cache_enabled)
	    {
			LOG_ERROR("cannot write L1 dcache when enabled [0x%08X] size %d", addr, size);
			return ERROR_FAIL;
	    }

		retval = memory_write(jtag_info, addr, size, buf);
		goto done;
	}
	else if (IN_MAP(addr, blackfin->l1_map->l1_data_b) &&
			 ((!blackfin->l1_data_b_cache_enabled &&
			   (blackfin->l1_map->l1_data_b_end == blackfin->l1_map->l1_data_b_cache)
			   && (end = blackfin->l1_map->l1_data_b_cache_end))
			  || (end = blackfin->l1_map->l1_data_b_end))
			 && addr + size <= end)
	{
		retval = memory_write(jtag_info, addr, size, buf);
		goto done;
	}
	else if (IN_MAP(addr, blackfin->l1_map->l1_data_b_cache) &&
			 addr + size <= blackfin->l1_map->l1_data_b_cache_end)
	{
		if (blackfin->l1_data_b_cache_enabled)
	    {
			LOG_ERROR("cannot write L1 dcache when enabled [0x%08X] size %d", addr, size);
			return ERROR_FAIL;
	    }

		retval = memory_write(jtag_info, addr, size, buf);
		goto done;
	}
	else if (IN_MAP(addr, blackfin->l1_map->l1_scratch) &&
			 addr + size <= blackfin->l1_map->l1_scratch_end)
	{
		retval = memory_write(jtag_info, addr, size, buf);
		goto done;
	}
	else if (IN_MAP(addr, blackfin->l1_map->l1))
	{
		LOG_ERROR("cannot write reserved L1 [0x%08X] size %d", addr, size);
		return ERROR_FAIL;
	}

 skip_l1:

	/* TODO  Accurately check MMR validity.  */

	if ((IN_RANGE (addr, blackfin->mem_map->sysmmr, blackfin->mem_map->coremmr) &&
		 addr + size > blackfin->mem_map->coremmr)
		|| (addr >= blackfin->mem_map->coremmr && addr + size < addr))
	{
		LOG_ERROR("bad MMR addr or size [0x%08X] size %d", addr, size);
		return ERROR_FAIL;
	}

	if (addr >= blackfin->mem_map->sysmmr)
	{
		if (size != 2 && size != 4)
		{
			LOG_ERROR("bad MMR size [0x%08X] size %d", addr, size);
			return ERROR_FAIL;
		}

		if ((addr & 0x1) == 1)
		{
			LOG_ERROR("odd MMR addr [0x%08X]", addr);
			return ERROR_FAIL;
		}

		value = *buf++;
		value |= (*buf++) << 8;
		if (size == 4)
		{
			value |= (*buf++) << 16;
			value |= (*buf++) << 24;
		}
		mmr_write(jtag_info, addr, value, size);
		retval = ERROR_OK;
		goto done;
	}

	if ((IN_MAP (addr, blackfin->mem_map->sdram) &&
		 addr + size <= blackfin->mem_map->sdram_end)
		|| (IN_MAP (addr, blackfin->mem_map->async_mem) &&
			addr + size <= blackfin->mem_map->async_mem_end)
		|| (IN_MAP (addr, blackfin->mem_map->l2_sram) &&
			addr + size <= blackfin->mem_map->l2_sram_end))
	{
		retval = memory_write(jtag_info, addr, size, buf);
		icache_flush(jtag_info, addr, size);
	}
	else
	{
		LOG_ERROR("cannot write memory [0x%08X]", addr);
		return ERROR_FAIL;
	}

 done:

	return retval;
}
