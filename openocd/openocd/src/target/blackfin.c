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

#include "helper/log.h"
#include "helper/types.h"
#include "helper/binarybuffer.h"

#include "breakpoints.h"
#include "register.h"
#include "target.h"
#include "target_type.h"
#include "blackfin.h"
#include "blackfin_jtag.h"
#include "blackfin_mem.h"
#include "blackfin_memory_map.h"
#include "blackfin_config.h"

#include "jtag/jtag.h"
#include "jtag/drivers/ft2232.h"

#undef UPSTREAM_GDB_SUPPORT

/* Core Registers
  BFIN_R0_REGNUM = 0,
  BFIN_R1_REGNUM,
  BFIN_R2_REGNUM,
  BFIN_R3_REGNUM,
  BFIN_R4_REGNUM,
  BFIN_R5_REGNUM,
  BFIN_R6_REGNUM,
  BFIN_R7_REGNUM,
  BFIN_P0_REGNUM,
  BFIN_P1_REGNUM,
  BFIN_P2_REGNUM,
  BFIN_P3_REGNUM,
  BFIN_P4_REGNUM,
  BFIN_P5_REGNUM,
  BFIN_SP_REGNUM,
  BFIN_FP_REGNUM,
  BFIN_I0_REGNUM,
  BFIN_I1_REGNUM,
  BFIN_I2_REGNUM,
  BFIN_I3_REGNUM,
  BFIN_M0_REGNUM,
  BFIN_M1_REGNUM,
  BFIN_M2_REGNUM,
  BFIN_M3_REGNUM,
  BFIN_B0_REGNUM,
  BFIN_B1_REGNUM,
  BFIN_B2_REGNUM,
  BFIN_B3_REGNUM,
  BFIN_L0_REGNUM,
  BFIN_L1_REGNUM,
  BFIN_L2_REGNUM,
  BFIN_L3_REGNUM,
  BFIN_A0_DOT_X_REGNUM,
  BFIN_AO_DOT_W_REGNUM,
  BFIN_A1_DOT_X_REGNUM,
  BFIN_A1_DOT_W_REGNUM,
  BFIN_ASTAT_REGNUM,
  BFIN_RETS_REGNUM,
  BFIN_LC0_REGNUM,
  BFIN_LT0_REGNUM,
  BFIN_LB0_REGNUM,
  BFIN_LC1_REGNUM,
  BFIN_LT1_REGNUM,
  BFIN_LB1_REGNUM,
  BFIN_CYCLES_REGNUM,
  BFIN_CYCLES2_REGNUM,
  BFIN_USP_REGNUM,
  BFIN_SEQSTAT_REGNUM,
  BFIN_SYSCFG_REGNUM,
  BFIN_RETI_REGNUM,
  BFIN_RETX_REGNUM,
  BFIN_RETN_REGNUM,
  BFIN_RETE_REGNUM,
*/

struct blackfin_core_reg
{
	uint32_t num;
	struct target *target;
	/* can we remove the following line. */
	struct blackfin_common *blackfin_common;
};

/* This enum was copied from GDB.  */
enum gdb_regnum
{
  /* Core Registers */
  BFIN_R0_REGNUM = 0,
  BFIN_R1_REGNUM,
  BFIN_R2_REGNUM,
  BFIN_R3_REGNUM,
  BFIN_R4_REGNUM,
  BFIN_R5_REGNUM,
  BFIN_R6_REGNUM,
  BFIN_R7_REGNUM,
  BFIN_P0_REGNUM,
  BFIN_P1_REGNUM,
  BFIN_P2_REGNUM,
  BFIN_P3_REGNUM,
  BFIN_P4_REGNUM,
  BFIN_P5_REGNUM,
  BFIN_SP_REGNUM,
  BFIN_FP_REGNUM,
  BFIN_I0_REGNUM,
  BFIN_I1_REGNUM,
  BFIN_I2_REGNUM,
  BFIN_I3_REGNUM,
  BFIN_M0_REGNUM,
  BFIN_M1_REGNUM,
  BFIN_M2_REGNUM,
  BFIN_M3_REGNUM,
  BFIN_B0_REGNUM,
  BFIN_B1_REGNUM,
  BFIN_B2_REGNUM,
  BFIN_B3_REGNUM,
  BFIN_L0_REGNUM,
  BFIN_L1_REGNUM,
  BFIN_L2_REGNUM,
  BFIN_L3_REGNUM,
  BFIN_A0_DOT_X_REGNUM,
  BFIN_AO_DOT_W_REGNUM,
  BFIN_A1_DOT_X_REGNUM,
  BFIN_A1_DOT_W_REGNUM,
  BFIN_ASTAT_REGNUM,
  BFIN_RETS_REGNUM,
  BFIN_LC0_REGNUM,
  BFIN_LT0_REGNUM,
  BFIN_LB0_REGNUM,
  BFIN_LC1_REGNUM,
  BFIN_LT1_REGNUM,
  BFIN_LB1_REGNUM,
  BFIN_CYCLES_REGNUM,
  BFIN_CYCLES2_REGNUM,
  BFIN_USP_REGNUM,
  BFIN_SEQSTAT_REGNUM,
  BFIN_SYSCFG_REGNUM,
  BFIN_RETI_REGNUM,
  BFIN_RETX_REGNUM,
  BFIN_RETN_REGNUM,
  BFIN_RETE_REGNUM,

  /* Pseudo Registers */
  BFIN_PC_REGNUM,
#ifndef UPSTREAM_GDB_SUPPORT
  BFIN_CC_REGNUM,
  BFIN_TEXT_ADDR,		/* Address of .text section.  */
  BFIN_TEXT_END_ADDR,		/* Address of the end of .text section.  */
  BFIN_DATA_ADDR,		/* Address of .data section.  */

  BFIN_FDPIC_EXEC_REGNUM,
  BFIN_FDPIC_INTERP_REGNUM,

  /* MMRs */
  BFIN_IPEND_REGNUM,
#endif

  /* LAST ENTRY SHOULD NOT BE CHANGED.  */
  BFIN_NUM_REGS			/* The number of all registers.  */
};

/* Map gdb register number to core register number.  */
static enum core_regnum map_gdb_core[] = {
  /* Core Registers */
  REG_R0, REG_R1, REG_R2, REG_R3, REG_R4, REG_R5, REG_R6, REG_R7,
  REG_P0, REG_P1, REG_P2, REG_P3, REG_P4, REG_P5, REG_SP, REG_FP,
  REG_I0, REG_I1, REG_I2, REG_I3, REG_M0, REG_M1, REG_M2, REG_M3,
  REG_B0, REG_B1, REG_B2, REG_B3, REG_L0, REG_L1, REG_L2, REG_L3,
  REG_A0x, REG_A0w, REG_A1x, REG_A1w, REG_ASTAT, REG_RETS,
  REG_LC0, REG_LT0, REG_LB0, REG_LC1, REG_LT1, REG_LB1,
  REG_CYCLES, REG_CYCLES2,
  REG_USP, REG_SEQSTAT, REG_SYSCFG, REG_RETI, REG_RETX, REG_RETN, REG_RETE,

  /* Pseudo Registers */
  REG_RETE,
#ifndef UPSTREAM_GDB_SUPPORT
  -1 /* REG_CC */,
  -1 /* REG_TEXT_ADDR */,
  -1 /* REG_TEXT_END_ADDR */,
  -1 /* REG_DATA_ADDR */,
  -1 /* REG_FDPIC_EXEC_REGNUM */,
  -1 /* REG_FDPIC_INTERP_REGNUM */,

  /* MMRs */
  -1				/* REG_IPEND */
#endif
};

static char* blackfin_core_reg_list[] =
{
	"r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7",
	"p0", "p1", "p2", "p3", "p4", "p5", "sp", "fp",
	"i0", "i1", "i2", "i3", "m0", "m1", "m2", "m3",
	"b0", "b1", "b2", "b3", "l0", "l1", "l2", "l3",
	"a0.x", "a0.w", "a1.x", "a1.w", "astat", "rets",
	"lc0", "lt0", "lb0", "lc1", "lt1", "lb1",
	"cycles", "cycles2", "usp", "seqstat", "syscfg",
	"reti", "retx", "retn", "rete"
};

static struct blackfin_core_reg blackfin_core_reg_list_arch_info[BLACKFINNUMCOREREGS] =
{
	{0, NULL, NULL},
	{1, NULL, NULL},
	{2, NULL, NULL},
	{3, NULL, NULL},
	{4, NULL, NULL},
	{5, NULL, NULL},
	{6, NULL, NULL},
	{7, NULL, NULL},
	{8, NULL, NULL},
	{9, NULL, NULL},
	{10, NULL, NULL},
	{11, NULL, NULL},
	{12, NULL, NULL},
	{13, NULL, NULL},
	{14, NULL, NULL},
	{15, NULL, NULL},
	{16, NULL, NULL},
	{17, NULL, NULL},
	{18, NULL, NULL},
	{19, NULL, NULL},
	{20, NULL, NULL},
	{21, NULL, NULL},
	{22, NULL, NULL},
	{23, NULL, NULL},
	{24, NULL, NULL},
	{25, NULL, NULL},
	{26, NULL, NULL},
	{27, NULL, NULL},
	{28, NULL, NULL},
	{29, NULL, NULL},
	{30, NULL, NULL},
	{31, NULL, NULL},
	{32, NULL, NULL},
	{33, NULL, NULL},
	{34, NULL, NULL},
	{35, NULL, NULL},
	{36, NULL, NULL},
	{37, NULL, NULL},
	{38, NULL, NULL},
	{39, NULL, NULL},
	{40, NULL, NULL},
	{41, NULL, NULL},
	{42, NULL, NULL},
	{43, NULL, NULL},
	{44, NULL, NULL},
	{45, NULL, NULL},
	{46, NULL, NULL},
	{47, NULL, NULL},
	{48, NULL, NULL},
	{49, NULL, NULL},
	{50, NULL, NULL},
	{51, NULL, NULL},
	{52, NULL, NULL},
};

static uint8_t blackfin_gdb_dummy_value[] = {0, 0, 0, 0};

static struct reg blackfin_gdb_dummy_reg =
{
	.name = "GDB dummy register",
	.value = blackfin_gdb_dummy_value,
	.dirty = 0,
	.valid = 1,
	.size = 32,
	.arch_info = NULL,
};

#define EMUCAUSE_EMUEXCPT				0x0
#define EMUCAUSE_EMUIN					0x1
#define EMUCAUSE_WATCHPOINT				0x2
#define EMUCAUSE_PM0_OVERFLOW			0x4
#define EMUCAUSE_PM1_OVERFLOW			0x5
#define EMUCAUSE_SINGLE_STEP			0x8

#define WPDA_DISABLE					0
#define WPDA_WRITE						1
#define WPDA_READ						2
#define WPDA_ALL						3

static const uint8_t blackfin_breakpoint_16[] = { 0x25, 0x0 };
static const uint8_t blackfin_breakpoint_32[] = { 0x25, 0x0, 0x0, 0x0 };

int blackfin_wait_clocks = -1;

/* This function reads hardware register and update the value in cache.  */
static int blackfin_get_core_reg(struct reg *reg)
{
	struct blackfin_core_reg *blackfin_reg = reg->arch_info;
	struct target *target = blackfin_reg->target;
	struct blackfin_common *blackfin = target_to_blackfin(target);
	struct blackfin_jtag *jtag_info = &blackfin->jtag_info;
	uint32_t value;

	if (target->state != TARGET_HALTED)
	{
		return ERROR_TARGET_NOT_HALTED;
	}

	value = blackfin_register_get(jtag_info, map_gdb_core[blackfin_reg->num]);
	buf_set_u32(reg->value, 0, 32, value);
	reg->valid = 1;
	reg->dirty = 0;

	return ERROR_OK;
}

/* This function does not set hardware register.  It just sets the register in
   the cache and set it's dirty flag.  When restoring context, all dirty
   registers will be written back to the hardware.  */
static int blackfin_set_core_reg(struct reg *reg, uint8_t *buf)
{
	struct blackfin_core_reg *blackfin_reg = reg->arch_info;
	struct target *target = blackfin_reg->target;
	uint32_t value;

	if (target->state != TARGET_HALTED)
	{
		return ERROR_TARGET_NOT_HALTED;
	}

	value = buf_get_u32(buf, 0, 32);
	buf_set_u32(reg->value, 0, 32, value);
	reg->dirty = 1;
	reg->valid = 1;

	return ERROR_OK;
}

static const struct reg_arch_type blackfin_reg_type =
{
	.get = blackfin_get_core_reg,
	.set = blackfin_set_core_reg,
};


/* We does not actually save all registers here.  We just invalidate
   all registers in the cache.  When an invalid register in the cache
   is read, its value will then be fetched from hardware.  */
static void blackfin_save_context(struct target *target)
{
	struct blackfin_common *blackfin = target_to_blackfin(target);
	int i;

	for (i = 0; i < BLACKFINNUMCOREREGS; i++)
	{
		blackfin->core_cache->reg_list[i].valid = 0;
		blackfin->core_cache->reg_list[i].dirty = 0;
	}
}

/* Write dirty registers to hardware.  */
static void blackfin_restore_context(struct target *target)
{
	struct blackfin_common *blackfin = target_to_blackfin(target);
	struct blackfin_jtag *jtag_info = &blackfin->jtag_info;
	int i;

	for (i = 0; i < BLACKFINNUMCOREREGS; i++)
	{
		struct reg *reg = &blackfin->core_cache->reg_list[i];
		struct blackfin_core_reg *blackfin_reg = reg->arch_info;

		if (reg->dirty)
		{
			uint32_t value = buf_get_u32(reg->value, 0, 32);
			blackfin_register_set(jtag_info, map_gdb_core[blackfin_reg->num], value);
		}
	}
}

static void blackfin_debug_entry(struct target *target)
{
	blackfin_save_context(target);
}

static int blackfin_poll(struct target *target)
{
	struct blackfin_common *blackfin = target_to_blackfin(target);
	struct blackfin_jtag *jtag_info = &blackfin->jtag_info;
	enum target_state prev_target_state = target->state;

	blackfin_emupc_get(jtag_info);
	/* If the core is already in fault, don't reset EMUPC.  */
	if (!blackfin_dbgstat_is_core_fault(jtag_info))
		blackfin_emupc_reset(jtag_info);
	blackfin_read_dbgstat(jtag_info);

	if (blackfin_dbgstat_is_in_reset(jtag_info))
		target->state = TARGET_RESET;
	else if (blackfin_dbgstat_is_core_fault(jtag_info))
	{
		if (prev_target_state != TARGET_HALTED)
		{
			LOG_WARNING("%s: a double fault has occurred at EMUPC [0x%08X]",
				target_name(target), jtag_info->emupc);
			target->state = TARGET_HALTED;
			target->debug_reason = DBG_REASON_UNDEFINED;
			blackfin->is_corefault = 1;
		}

		if (prev_target_state == TARGET_RUNNING
			|| prev_target_state == TARGET_RESET)
		{
			target_call_event_callbacks(target, TARGET_EVENT_HALTED);
		}
		else if (prev_target_state == TARGET_DEBUG_RUNNING)
		{
			target_call_event_callbacks(target, TARGET_EVENT_DEBUG_HALTED);
		}
	}
	else if (blackfin_dbgstat_is_emuready(jtag_info))
			 
	{
		if (prev_target_state != TARGET_HALTED)
		{
			uint16_t cause;

			LOG_DEBUG("Target halted");
			target->state = TARGET_HALTED;

			cause = blackfin_dbgstat_emucause(jtag_info);

			blackfin_wpstat_get(jtag_info);

			if ((jtag_info->wpstat & 0x3f) && !(jtag_info->wpstat & 0xc0))
			{
				target->debug_reason = DBG_REASON_BREAKPOINT;
			}
			else if (!(jtag_info->wpstat & 0x3f) && (jtag_info->wpstat & 0xc0))
			{
				target->debug_reason = DBG_REASON_WATCHPOINT;
			}
			else if ((jtag_info->wpstat & 0x3f) && (jtag_info->wpstat & 0xc0))
			{
				target->debug_reason = DBG_REASON_WPTANDBKPT;
			}
			else
			switch (cause)
			{
			case EMUCAUSE_EMUEXCPT:
				target->debug_reason = DBG_REASON_BREAKPOINT;
				break;

			case EMUCAUSE_SINGLE_STEP:
				target->debug_reason = DBG_REASON_SINGLESTEP;
				break;

			case EMUCAUSE_WATCHPOINT:
				abort();
				break;

			case EMUCAUSE_EMUIN:
				target->debug_reason = DBG_REASON_DBGRQ;
				break;

			case EMUCAUSE_PM0_OVERFLOW:
			case EMUCAUSE_PM1_OVERFLOW:
			default:
				target->debug_reason = DBG_REASON_UNDEFINED;
				break;
			}

			if (jtag_info->wpstat & 0xff)
			{
			    if (jtag_info->wpstat & 0xc0)
					blackfin_single_step(jtag_info, blackfin->is_stepping);
			    blackfin_wpstat_clear(jtag_info);
			}

			blackfin_debug_entry(target);

			if (prev_target_state == TARGET_RUNNING
				|| prev_target_state == TARGET_RESET)
			{
				target_call_event_callbacks(target, TARGET_EVENT_HALTED);
			}
			else if (prev_target_state == TARGET_DEBUG_RUNNING)
			{
				target_call_event_callbacks(target, TARGET_EVENT_DEBUG_HALTED);
			}
		}
	}
	else
	{
		target->state = TARGET_RUNNING;
		target->debug_reason = DBG_REASON_NOTHALTED;
	}

	if (prev_target_state != target->state)
		LOG_DEBUG("target->state: ==> %s", target_state_name(target));

	return ERROR_OK;
}

static int blackfin_arch_state(struct target *target)
{
	return ERROR_FAIL;
}

static int blackfin_target_request_data(struct target *target,
		uint32_t size, uint8_t *buffer)
{
	return ERROR_FAIL;
}

static int blackfin_halt(struct target *target)
{
	struct blackfin_common *blackfin = target_to_blackfin(target);
	struct blackfin_jtag *jtag_info = &blackfin->jtag_info;

	LOG_DEBUG("target->state: %s", target_state_name(target));

	if (target->state == TARGET_HALTED)
	{
		LOG_DEBUG("target was already halted");
		return ERROR_OK;
	}

	if (target->state == TARGET_UNKNOWN)
	{
		LOG_WARNING("target was in unknown state when halt was requested");
	}

	if (target->state == TARGET_RESET)
	{
		/* FIXME it might be possible to halt while in reset,
		   see other targets for a example.  */
		LOG_ERROR("can't request a halt while in reset");
		return ERROR_TARGET_FAILURE;
	}

	blackfin_emulation_enable(jtag_info);
	blackfin_emulation_trigger(jtag_info);

	target->debug_reason = DBG_REASON_DBGRQ;

	return ERROR_OK;
}

static int blackfin_resume_1(struct target *target, int current,
		uint32_t address, int handle_breakpoints, int debug_execution, bool step)
{
	struct blackfin_common *blackfin = target_to_blackfin(target);
	struct blackfin_jtag *jtag_info = &blackfin->jtag_info;

	/* We don't handle this for now. */
	assert(debug_execution == 0);

	/* FIXME handle handle_breakpoints !!!*/

	if (target->state != TARGET_HALTED)
	{
		LOG_WARNING("target not halted");
		return ERROR_TARGET_NOT_HALTED;
	}

	if (!debug_execution)
	{
		target_free_all_working_areas(target);
	}

	blackfin_emupc_reset(jtag_info);

	blackfin_restore_context(target);

	if (!current)
	{
		blackfin_register_set(jtag_info, REG_RETE, address);
	}

	if (blackfin->status_pending_p && blackfin->pending_is_breakpoint)
	{
		int retval;
		uint8_t buf[2];

		retval = blackfin_read_mem(jtag_info, blackfin->pending_stop_pc, 2, buf);
		assert(retval == ERROR_OK);

		if (buf[0] != blackfin_breakpoint_16[0]
			|| buf[1] != blackfin_breakpoint_16[1])
		{
			/* The breakpoint is gone. Consume the pending status.  */
			blackfin->pending_is_breakpoint = 0;
			blackfin->status_pending_p = 0;
		}

		if (blackfin->status_pending_p)
			return ERROR_OK;
	}

	if (step && !blackfin->is_stepping)
	{
		blackfin_dbgctl_bit_set_esstep(jtag_info);
		blackfin->is_stepping = 1;
	}
	else if (!step && blackfin->is_stepping)
	{
		blackfin_dbgctl_bit_clear_esstep(jtag_info);
		blackfin->is_stepping = 0;
	}

	if (!debug_execution)
	{
		target->state = TARGET_RUNNING;
		target_call_event_callbacks(target, TARGET_EVENT_RESUMED);
		LOG_DEBUG("target resumed at 0x%" PRIx32, address);
	}
	else
	{
		target->state = TARGET_DEBUG_RUNNING;
		target_call_event_callbacks(target, TARGET_EVENT_DEBUG_RESUMED);
		LOG_DEBUG("target debug resumed at 0x%" PRIx32, address);
	}

	blackfin->is_running = 1;
	blackfin->is_interrupted = 0;
	blackfin->dmem_control_valid_p = 0;
	blackfin->imem_control_valid_p = 0;
	blackfin_emulation_return(jtag_info);

	return ERROR_OK;
}

static int blackfin_resume(struct target *target, int current,
		uint32_t address, int handle_breakpoints, int debug_execution)
{
	int retval;

	retval = blackfin_resume_1(target, current, address, handle_breakpoints, debug_execution, false);

	return retval;
}

static int blackfin_step(struct target *target, int current,
		uint32_t address, int handle_breakpoints)
{
	int retval;

	retval = blackfin_resume_1(target, current, address, handle_breakpoints, 0, true);

	return retval;
}

static int blackfin_assert_reset(struct target *target)
{
	struct blackfin_common *blackfin = target_to_blackfin(target);
	struct blackfin_jtag *jtag_info = &blackfin->jtag_info;

	LOG_DEBUG("target->state: %s", target_state_name(target));

	blackfin_system_reset(jtag_info);
	blackfin_core_reset(jtag_info);

	target->state = TARGET_HALTED;

	/* Reset should bring the core out of core fault.  */
	blackfin->is_corefault = 0;

	/* Reset should invalidate register cache.  */
	register_cache_invalidate(blackfin->core_cache);

	if (!target->reset_halt)
	{
		blackfin_resume(target, 1, 0, 0, 0);
		target->state = TARGET_RUNNING;
	}

	return ERROR_OK;
}

static int blackfin_deassert_reset(struct target *target)
{
	return ERROR_OK;
}

static int blackfin_get_gdb_reg_list(struct target *target, struct reg **reg_list[],
									 int *reg_list_size, enum target_register_class reg_class)
{
	struct blackfin_common *blackfin = target_to_blackfin(target);
	int i;

	*reg_list_size = BFIN_NUM_REGS;
	*reg_list = malloc(sizeof(struct reg*) * (*reg_list_size));

	for (i = 0; i < BLACKFINNUMCOREREGS; i++)
		(*reg_list)[i] = &blackfin->core_cache->reg_list[i];

	for (i = BLACKFINNUMCOREREGS; i < BFIN_NUM_REGS; i++)
	{
		if (i == BFIN_PC_REGNUM)
			(*reg_list)[i] = &blackfin->core_cache->reg_list[BFIN_RETE_REGNUM];
		/* TODO handle CC */
		else
			(*reg_list)[i] = &blackfin_gdb_dummy_reg;
	}

	return ERROR_OK;
}

static int blackfin_read_memory(struct target *target, uint32_t address,
		uint32_t size, uint32_t count, uint8_t *buffer)
{
	struct blackfin_common *blackfin = target_to_blackfin(target);
	struct blackfin_jtag *jtag_info = &blackfin->jtag_info;
	int retval;

	LOG_DEBUG("address: 0x%8.8" PRIx32 ", size: 0x%8.8" PRIx32 ", count: 0x%8.8" PRIx32 "", address, size, count);

	if (target->state != TARGET_HALTED)
	{
		LOG_WARNING("target not halted");
		return ERROR_TARGET_NOT_HALTED;
	}

	retval = blackfin_read_mem(jtag_info, address, size * count, buffer);

	return retval;
}

static int blackfin_write_memory(struct target *target, uint32_t address,
		uint32_t size, uint32_t count, const uint8_t *buffer)
{
	struct blackfin_common *blackfin = target_to_blackfin(target);
	struct blackfin_jtag *jtag_info = &blackfin->jtag_info;
	int retval;

	LOG_DEBUG("address: 0x%8.8" PRIx32 ", size: 0x%8.8" PRIx32 ", count: 0x%8.8" PRIx32 "",
			address, size, count);

	if (target->state != TARGET_HALTED)
	{
		LOG_WARNING("target not halted");
		return ERROR_TARGET_NOT_HALTED;
	}

	retval = blackfin_write_mem(jtag_info, address, size * count, buffer);

	return retval;
}

static int blackfin_checksum_memory(struct target *target,
		uint32_t address, uint32_t count, uint32_t* checksum)
{
	return ERROR_FAIL;
}

static int blackfin_blank_check_memory(struct target *target,
		uint32_t address, uint32_t count, uint32_t* blank)
{
	return ERROR_FAIL;
}

static int blackfin_run_algorithm(struct target *target,
		int num_mem_params, struct mem_param *mem_params,
		int num_reg_params, struct reg_param *reg_params,
		uint32_t entry_point, uint32_t exit_point,
		int timeout_ms, void *arch_info)
{
	return ERROR_FAIL;
}

static int blackfin_add_breakpoint(struct target *target, struct breakpoint *breakpoint)
{
	struct blackfin_common *blackfin = target_to_blackfin(target);
	struct blackfin_jtag *jtag_info = &blackfin->jtag_info;
	int retval;

	assert(breakpoint->set == 0);

	blackfin_emupc_reset(jtag_info);

	if (breakpoint->type == BKPT_SOFT)
	{
		const uint8_t *bp;

		if (breakpoint->length == 2)
			bp = blackfin_breakpoint_16;
		else
			bp = blackfin_breakpoint_32;

		retval = blackfin_read_memory(target, breakpoint->address,
			breakpoint->length, 1, breakpoint->orig_instr);
		if (retval != ERROR_OK)
			return retval;

		retval = blackfin_write_memory(target, breakpoint->address,
			breakpoint->length, 1, bp);
		if (retval != ERROR_OK)
			return retval;
	}
	else /* BKPT_HARD */
	{
		int i;
		for (i = 0; i < BLACKFIN_MAX_HWBREAKPOINTS; i++)
			if (jtag_info->hwbps[i] == (uint32_t) -1)
				break;
		if (i == BLACKFIN_MAX_HWBREAKPOINTS)
		{
			LOG_ERROR("%s: no more hardware breakpoint available for 0x%08x",
				target_name(target), breakpoint->address);
			return ERROR_FAIL;
		}

		jtag_info->hwbps[i] = breakpoint->address;
		if (blackfin->is_locked)
		{
			LOG_WARNING("%s: target is locked, hardware breakpoint [0x%08x] will be set when it's unlocked", target_name(target), breakpoint->address);
		}
		else
		{
			blackfin_wpu_set_wpia(jtag_info, i, breakpoint->address, 1);
		}
	}

	breakpoint->set = 1;

	return ERROR_OK;
}

static int blackfin_remove_breakpoint(struct target *target, struct breakpoint *breakpoint)
{
	struct blackfin_common *blackfin = target_to_blackfin(target);
	struct blackfin_jtag *jtag_info = &blackfin->jtag_info;
	int retval;

	assert(breakpoint->set != 0);

	blackfin_emupc_reset(jtag_info);

	if (breakpoint->type == BKPT_SOFT)
	{
		retval = blackfin_write_memory(target, breakpoint->address,
			breakpoint->length, 1, breakpoint->orig_instr);
		if (retval != ERROR_OK)
			return retval;
	}
	else /* BKPT_HARD */
	{
		int i;
		for (i = 0; i < BLACKFIN_MAX_HWBREAKPOINTS; i++)
			if (jtag_info->hwbps[i] == breakpoint->address)
				break;

		if (i == BLACKFIN_MAX_HWBREAKPOINTS)
		{
			LOG_ERROR("%s: no hardware breakpoint at 0x%08x",
				target_name(target), breakpoint->address);
			return ERROR_FAIL;
		}

		jtag_info->hwbps[i] = -1;
		if (blackfin->is_locked)
		{
			/* The hardware breakpoint will only be set when target is
			   unlocked.  Target cannot be locked again after it's unlocked.
			   So if we are removing a hardware breakpoint when target is
			   locked, that hardware breakpoint has never been set.
			   So we can just quietly remove it.  */
		}
		else
		{
			blackfin_wpu_set_wpia(jtag_info, i, breakpoint->address, 0);
		}
	}

	breakpoint->set = 0;

	return ERROR_OK;
}

static int blackfin_add_watchpoint(struct target *target, struct watchpoint *watchpoint)
{
	struct blackfin_common *blackfin = target_to_blackfin(target);
	struct blackfin_jtag *jtag_info = &blackfin->jtag_info;
	int i;
	bool range;

	/* TODO  provide a method to always use range watchpoint.  */

	if (target->state != TARGET_HALTED)
		return ERROR_TARGET_NOT_HALTED;

	blackfin_emupc_reset(jtag_info);

	if (watchpoint->length <= 4
		&& jtag_info->hwwps[0].used
		&& (jtag_info->hwwps[1].used
			|| jtag_info->hwwps[0].range))
	{
		LOG_ERROR("all hardware watchpoints are in use.");
		return ERROR_TARGET_RESOURCE_NOT_AVAILABLE;
	}

	if (watchpoint->length > 4
		&& (jtag_info->hwwps[0].used
			|| jtag_info->hwwps[1].used))
	{
		LOG_ERROR("no enough hardware watchpoints.");
		return ERROR_TARGET_RESOURCE_NOT_AVAILABLE;
	}

	if (watchpoint->length <= 4)
	{
		i = jtag_info->hwwps[0].used ? 1 : 0;
		range = false;
	}
	else
	{
		i = 0;
		range = true;
	}

	jtag_info->hwwps[i].addr = watchpoint->address;
	jtag_info->hwwps[i].len = watchpoint->length;
	jtag_info->hwwps[i].range = range;
	jtag_info->hwwps[i].used = true;
	switch (watchpoint->rw)
	{
	case WPT_READ:
		jtag_info->hwwps[i].mode = WPDA_READ;
		break;
	case WPT_WRITE:
		jtag_info->hwwps[i].mode = WPDA_WRITE;
		break;
	case WPT_ACCESS:
		jtag_info->hwwps[i].mode = WPDA_ALL;
		break;
	}
	
	if (blackfin->is_locked)
	{
		LOG_WARNING("%s: target is locked, hardware watchpoint [0x%08x] will be set when it's unlocked", target_name(target), watchpoint->address);
	}
	else
	{
		blackfin_wpu_set_wpda(jtag_info, i);
	}

	return ERROR_OK;
}

static int blackfin_remove_watchpoint(struct target *target, struct watchpoint *watchpoint)
{
	struct blackfin_common *blackfin = target_to_blackfin(target);
	struct blackfin_jtag *jtag_info = &blackfin->jtag_info;
	int mode;
	int i;

	/* TODO  provide a method to always use range watchpoint.  */

	if (target->state != TARGET_HALTED)
		return ERROR_TARGET_NOT_HALTED;

	blackfin_emupc_reset(jtag_info);

	switch (watchpoint->rw)
	{
	case WPT_READ:
		mode = WPDA_READ;
		break;
	case WPT_WRITE:
		mode = WPDA_WRITE;
		break;
	case WPT_ACCESS:
		mode = WPDA_ALL;
		break;
	default:
		return ERROR_FAIL;
	}

	for (i = 0; i < BLACKFIN_MAX_HWWATCHPOINTS; i++)
		if (jtag_info->hwwps[i].addr == watchpoint->address
			&& jtag_info->hwwps[i].len == watchpoint->length
			&& jtag_info->hwwps[i].mode == mode)
			break;

	if (i == BLACKFIN_MAX_HWWATCHPOINTS)
	{
		LOG_ERROR("no hardware watchpoint at 0x%08X length %d.",
				  watchpoint->address, watchpoint->length);
		return ERROR_FAIL;
	}

	if (watchpoint->length > 4)
	{
		assert (i == 0 && jtag_info->hwwps[i].range);
	}

	jtag_info->hwwps[i].mode = WPDA_DISABLE;
	
	if (blackfin->is_locked)
	{
		/* See the comment in blackfin_remove_breakpoint.  */
	}
	else
	{
		blackfin_wpu_set_wpda(jtag_info, i);
	}

	jtag_info->hwwps[i].addr = 0;
	jtag_info->hwwps[i].len = 0;
	jtag_info->hwwps[i].range = false;
	jtag_info->hwwps[i].used = false;

	return ERROR_OK;
}

static int blackfin_target_create(struct target *target, Jim_Interp *interp)
{
	struct blackfin_common *blackfin = calloc(1, sizeof(struct blackfin_common));
	struct Jim_Obj *objPtr;
	const char *map, *config;
	char *end;

	target->arch_info = blackfin;

	blackfin->common_magic = BLACKFIN_COMMON_MAGIC;
	strcpy(blackfin->part, target_name(target));
	end = strchr(blackfin->part, '.');
	if (end)
		*end = '\0';

	objPtr = Jim_GetGlobalVariableStr(interp, "MEMORY_MAP", JIM_NONE);
	map = Jim_GetString(objPtr, NULL);
	parse_memory_map(target, map);

	objPtr = Jim_GetGlobalVariableStr(interp, "BLACKFIN_CONFIG", JIM_NONE);
	config = Jim_GetString(objPtr, NULL);
	blackfin_parse_config(target, config);

	objPtr = Jim_GetGlobalVariableStr(interp, "SDRRC", JIM_NONE);
	if (objPtr)
	{
		struct blackfin_sdram_config *sdram_config;
		long value;
		int retval;
		sdram_config = malloc(sizeof (blackfin->sdram_config));
		if (!sdram_config)
		{
			LOG_ERROR("%s: malloc(%"PRIzu") failed",
					  target_name(target), sizeof (blackfin->sdram_config));
			return ERROR_FAIL;
		}
		retval = Jim_GetLong(interp, objPtr, &value);
		if (retval == JIM_OK)
			sdram_config->sdrrc = value;
		else
			return ERROR_FAIL;

		objPtr = Jim_GetGlobalVariableStr(interp, "SDBCTL", JIM_NONE);
		if (!objPtr)
		{
			LOG_ERROR("%s: SDBCTL is not defined", target_name(target));
			return ERROR_FAIL;
		}
		retval = Jim_GetLong(interp, objPtr, &value);
		if (retval == JIM_OK)
			sdram_config->sdbctl = value;
		else
			return ERROR_FAIL;

		objPtr = Jim_GetGlobalVariableStr(interp, "SDGCTL", JIM_NONE);
		if (!objPtr)
		{
			LOG_ERROR("%s: SDGCTL is not defined", target_name(target));
			return ERROR_FAIL;
		}
		retval = Jim_GetLong(interp, objPtr, &value);
		if (retval == JIM_OK)
			sdram_config->sdgctl = value;
		else
			return ERROR_FAIL;

		blackfin->sdram_config = sdram_config;
	}
	else
		blackfin->sdram_config = NULL;

	objPtr = Jim_GetGlobalVariableStr(interp, "DDRCTL0", JIM_NONE);
	if (objPtr)
	{
		struct blackfin_ddr_config *ddr_config;
		long value;
		int retval;
		ddr_config = malloc(sizeof (blackfin->ddr_config));
		if (!ddr_config)
		{
			LOG_ERROR("%s: malloc(%"PRIzu") failed",
					  target_name(target), sizeof (blackfin->ddr_config));
			return ERROR_FAIL;
		}
		retval = Jim_GetLong(interp, objPtr, &value);
		if (retval == JIM_OK)
			ddr_config->ddrctl0 = value;
		else
			return ERROR_FAIL;

		objPtr = Jim_GetGlobalVariableStr(interp, "DDRCTL1", JIM_NONE);
		if (!objPtr)
		{
			LOG_ERROR("%s: DDRCTL1 is not defined", target_name(target));
			return ERROR_FAIL;
		}
		retval = Jim_GetLong(interp, objPtr, &value);
		if (retval == JIM_OK)
			ddr_config->ddrctl1 = value;
		else
			return ERROR_FAIL;

		objPtr = Jim_GetGlobalVariableStr(interp, "DDRCTL2", JIM_NONE);
		if (!objPtr)
		{
			LOG_ERROR("%s: DDRCTL2 is not defined", target_name(target));
			return ERROR_FAIL;
		}
		retval = Jim_GetLong(interp, objPtr, &value);
		if (retval == JIM_OK)
			ddr_config->ddrctl2 = value;
		else
			return ERROR_FAIL;

		blackfin->ddr_config = ddr_config;
	}
	else
		blackfin->ddr_config = NULL;

	blackfin->dmem_control_valid_p = 0;
	blackfin->imem_control_valid_p = 0;
	blackfin->dmem_control = 0;
	blackfin->imem_control = 0;

	return ERROR_OK;
}

static struct reg_cache *blackfin_build_reg_cache(struct target *target)
{
	/* get pointers to arch-specific information */
	struct blackfin_common *blackfin = target_to_blackfin(target);

	int num_regs = BLACKFINNUMCOREREGS;
	struct reg_cache **cache_p = register_get_last_cache_p(&target->reg_cache);
	struct reg_cache *cache = malloc(sizeof(struct reg_cache));
	struct reg *reg_list = malloc(sizeof(struct reg) * num_regs);
	struct blackfin_core_reg *arch_info = malloc(sizeof(struct blackfin_core_reg) * num_regs);
	int i;

	register_init_dummy(&blackfin_gdb_dummy_reg);

	/* Build the process context cache */
	cache->name = "blackfin registers";
	cache->next = NULL;
	cache->reg_list = reg_list;
	cache->num_regs = num_regs;
	(*cache_p) = cache;
	blackfin->core_cache = cache;

	for (i = 0; i < num_regs; i++)
	{
		arch_info[i] = blackfin_core_reg_list_arch_info[i];
		arch_info[i].target = target;
		arch_info[i].blackfin_common = blackfin;
		reg_list[i].name = blackfin_core_reg_list[i];
		reg_list[i].feature = NULL;
		reg_list[i].size = 32;
		reg_list[i].value = calloc(1, 4);
		reg_list[i].dirty = 0;
		reg_list[i].valid = 0;
		reg_list[i].type = &blackfin_reg_type;
		reg_list[i].arch_info = &arch_info[i];
	}

	return cache;
}

static int blackfin_init_target(struct command_context *cmd_ctx,
		struct target *target)
{
	struct blackfin_common *blackfin = target_to_blackfin(target);

	blackfin->jtag_info.target = target;
	blackfin_build_reg_cache(target);
	return ERROR_OK;
}

static int blackfin_calculate_wait_clocks(void)
{
	int clocks;
	int frequency;

	/* The following default numbers of wait clock for various cables are
	   tested on a BF537 stamp board, on which U-Boot is running.
	   CCLK is set to 62MHz and SCLK is set to 31MHz, which is the lowest
	   frequency I can set in BF537 stamp Linux kernel.

	   The test is done by dumping memory from 0x20000000 to 0x20000010 using
	   GDB and gdbproxy:

	   (gdb) dump memory u-boot.bin 0x20000000 0x20000010
	   (gdb) shell hexdump -C u-boot.bin

	   With an incorrect number of wait clocks, the first 4 bytes will be
	   duplicated by the second 4 bytes.  */

	clocks = -1;
	frequency = jtag_get_speed_khz();

	if (strcmp(jtag_get_name(), "ftdi") == 0)
	{
		/* For full speed ftdi device, we only need 3 wait clocks.  For
		   high speed ftdi device, we need 5 wait clocks when its frequency
		   is less than or equal to 6000.  We need a function like

			  bool ftdi_is_high_speed(void)

		   to distinguish them.  But currently ftdi driver does not provide
		   such a function.  So we just set wait clocks to 5 for both cases. */

		if (frequency <= 6000)
			clocks = 5;
		else if (frequency <= 15000)
			clocks = 12;
		else
			clocks = 21;
	}
	else if (strcmp(jtag_get_name(), "ice1000") == 0
			 || strcmp(jtag_get_name(), "ice2000") == 0)
	{
		if (frequency <= 5000)
			clocks = 5;
		else if (frequency <= 10000)
			clocks = 11;
		else if (frequency <= 17000)
			clocks = 19;
		else /* <= 25MHz */
			clocks = 30;
	}
	else
	{
		/* intended empty */
	}

	if (clocks == -1)
	{
		clocks = 30;
		LOG_WARNING("%s: untested cable running at %d KHz, set wait_clocks to %d",
			jtag_get_name(), frequency, clocks);
	}

	return clocks;
}

static int blackfin_examine(struct target *target)
{
	struct blackfin_common *blackfin = target_to_blackfin(target);
	struct blackfin_jtag *jtag_info = &blackfin->jtag_info;
	int i;

	if (!target_was_examined(target))
	{
		target_set_examined(target);

		jtag_info->dbgctl = 0;
		jtag_info->dbgstat = 0;
		jtag_info->emuir_a = BLACKFIN_INSN_ILLEGAL;
		jtag_info->emuir_b = BLACKFIN_INSN_ILLEGAL;
		jtag_info->emudat_out = 0;
		jtag_info->emudat_in = 0;

		jtag_info->wpiactl = 0;
		jtag_info->wpdactl = 0;
		jtag_info->wpstat = 0;
		for (i = 0; i < BLACKFIN_MAX_HWBREAKPOINTS; i++)
			jtag_info->hwbps[i] = -1;
		for (i = 0; i < BLACKFIN_MAX_HWWATCHPOINTS; i++)
		{
			jtag_info->hwwps[i].addr = 0;
			jtag_info->hwwps[i].len = 0;
			jtag_info->hwwps[i].mode = WPDA_DISABLE;
			jtag_info->hwwps[i].range = false;
			jtag_info->hwwps[i].used = false;
		}

		jtag_info->emupc = -1;

		blackfin_emulation_enable(jtag_info);
	}

	blackfin_wait_clocks = blackfin_calculate_wait_clocks();

	return ERROR_OK;
}

COMMAND_HANDLER(blackfin_handle_wpu_init_command)
{
	struct target *target = get_current_target(CMD_CTX);
	struct blackfin_common *blackfin = target_to_blackfin(target);
	struct blackfin_jtag *jtag_info = &blackfin->jtag_info;

	if (!target_was_examined(target))
	{
		LOG_ERROR("target not examined yet");
		return ERROR_FAIL;
	}

	blackfin_wpu_init(jtag_info);
	return ERROR_OK;
}

COMMAND_HANDLER(blackfin_handle_sdram_init_command)
{
	struct target *target = get_current_target(CMD_CTX);
	struct blackfin_common *blackfin = target_to_blackfin(target);
	struct blackfin_jtag *jtag_info = &blackfin->jtag_info;
	int retval;

	if (!target_was_examined(target))
	{
		LOG_ERROR("target not examined yet");
		return ERROR_FAIL;
	}
	if (!blackfin->sdram_config)
	{
		LOG_ERROR("no SDRAM config");
		return ERROR_FAIL;
	}

	retval = blackfin_sdram_init(jtag_info);
	return retval;
}

COMMAND_HANDLER(blackfin_handle_ddr_init_command)
{
	struct target *target = get_current_target(CMD_CTX);
	struct blackfin_common *blackfin = target_to_blackfin(target);
	struct blackfin_jtag *jtag_info = &blackfin->jtag_info;
	int retval;

	if (!target_was_examined(target))
	{
		LOG_ERROR("target not examined yet");
		return ERROR_FAIL;
	}
	if (!blackfin->ddr_config)
	{
		LOG_ERROR("no DDR config");
		return ERROR_FAIL;
	}

	retval = blackfin_ddr_init(jtag_info);
	return retval;
}

static const struct command_registration blackfin_exec_command_handlers[] = {
	{
		.name = "wpu_init",
		.handler = blackfin_handle_wpu_init_command,
		.mode = COMMAND_EXEC,
		.help = "Initialize Watchpoint Unit",
	},
	{
		.name = "sdram_init",
		.handler = blackfin_handle_sdram_init_command,
		.mode = COMMAND_EXEC,
		.help = "Initialize SDRAM",
	},
	{
		.name = "ddr_init",
		.handler = blackfin_handle_ddr_init_command,
		.mode = COMMAND_EXEC,
		.help = "Initialize DDR",
	},
	COMMAND_REGISTRATION_DONE
};

static const struct command_registration blackfin_command_handlers[] = {
	{
		.name = "blackfin",
		.mode = COMMAND_ANY,
		.help = "Blackfin command group",
		.chain = blackfin_exec_command_handlers,
	},
	COMMAND_REGISTRATION_DONE
};

struct target_type blackfin_target =
{
	.name = "blackfin",

	.poll = blackfin_poll,
	.arch_state = blackfin_arch_state,

	.target_request_data = blackfin_target_request_data,

	.halt = blackfin_halt,
	.resume = blackfin_resume,
	.step = blackfin_step,

	.assert_reset = blackfin_assert_reset,
	.deassert_reset = blackfin_deassert_reset,
	.soft_reset_halt = NULL,

	.get_gdb_reg_list = blackfin_get_gdb_reg_list,

	.read_memory = blackfin_read_memory,
	.write_memory = blackfin_write_memory,
	.checksum_memory = blackfin_checksum_memory,
	.blank_check_memory = blackfin_blank_check_memory,

	.run_algorithm = blackfin_run_algorithm,

	.add_breakpoint = blackfin_add_breakpoint,
	.remove_breakpoint = blackfin_remove_breakpoint,
	.add_watchpoint = blackfin_add_watchpoint,
	.remove_watchpoint = blackfin_remove_watchpoint,

	.commands = blackfin_command_handlers,
	.target_create = blackfin_target_create,
	.init_target = blackfin_init_target,
	.examine = blackfin_examine,
};
