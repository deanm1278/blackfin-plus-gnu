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
#include "bfinplus.h"
#include "bfinplus_dap.h"
#include "bfinplus_mem.h"

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

struct bfinplus_core_reg
{
	uint32_t num;
	struct target *target;
	/* can we remove the following line. */
	struct bfinplus_common *bfinplus_common;
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

static char* bfinplus_core_reg_list[] =
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

static struct bfinplus_core_reg bfinplus_core_reg_list_arch_info[BFINPLUSNUMCOREREGS] =
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

static uint8_t bfinplus_gdb_dummy_value[] = {0, 0, 0, 0};

static struct reg bfinplus_gdb_dummy_reg =
{
	.name = "GDB dummy register",
	.value = bfinplus_gdb_dummy_value,
	.dirty = 0,
	.valid = 1,
	.size = 32,
	.arch_info = NULL,
};

static const uint8_t bfinplus_breakpoint_16[] = { 0x25, 0x0 };
static const uint8_t bfinplus_breakpoint_32[] = { 0x25, 0x0, 0x0, 0x0 };

/* This function reads hardware register and update the value in cache.  */
static int bfinplus_get_core_reg(struct reg *reg)
{
	struct bfinplus_core_reg *bfinplus_reg = reg->arch_info;
	struct target *target = bfinplus_reg->target;
	struct bfinplus_common *bfinplus = target_to_bfinplus(target);
	struct bfinplus_dap *dap = &bfinplus->dap;
	uint32_t value;

	if (target->state != TARGET_HALTED)
	{
		return ERROR_TARGET_NOT_HALTED;
	}

	value = bfinplus_register_get(dap, map_gdb_core[bfinplus_reg->num]);
	buf_set_u32(reg->value, 0, 32, value);
	reg->valid = 1;
	reg->dirty = 0;

	return ERROR_OK;
}

struct target_type bfinplus_target =
{
	.name = "bfinplus",

	.poll = bfinplus_poll,
	.arch_state = bfinplus_arch_state,

	.target_request_data = bfinplus_target_request_data,

	.halt = bfinplus_halt,
	.resume = bfinplus_resume,
	.step = bfinplus_step,

	.assert_reset = bfinplus_assert_reset,
	.deassert_reset = bfinplus_deassert_reset,
	.soft_reset_halt = NULL,

	.get_gdb_reg_list = bfinplus_get_gdb_reg_list,

	.read_memory = bfinplus_read_memory,
	.write_memory = bfinplus_write_memory,
	.checksum_memory = bfinplus_checksum_memory,
	.blank_check_memory = bfinplus_blank_check_memory,

	.run_algorithm = bfinplus_run_algorithm,

	.add_breakpoint = bfinplus_add_breakpoint,
	.remove_breakpoint = bfinplus_remove_breakpoint,
	.add_watchpoint = bfinplus_add_watchpoint,
	.remove_watchpoint = bfinplus_remove_watchpoint,

	.commands = bfinplus_command_handlers,
	.target_create = bfinplus_target_create,
	.init_target = bfinplus_init_target,
	.examine = bfinplus_examine,
};