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
	uint32_t value;

	if (target->state != TARGET_HALTED)
	{
		return ERROR_TARGET_NOT_HALTED;
	}

	bfinplus_coreregister_get(target, map_gdb_core[bfinplus_reg->num], &value);
	buf_set_u32(reg->value, 0, 32, value);
	reg->valid = 1;
	reg->dirty = 0;

	return ERROR_OK;
}

/* This function does not set hardware register.  It just sets the register in
   the cache and set it's dirty flag.  When restoring context, all dirty
   registers will be written back to the hardware.  */
static int bfinplus_set_core_reg(struct reg *reg, uint8_t *buf)
{
  struct bfinplus_core_reg *bfinplus_reg = reg->arch_info;
  struct target *target = bfinplus_reg->target;
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

static const struct reg_arch_type bfinplus_reg_type =
{
  .get = bfinplus_get_core_reg,
  .set = bfinplus_set_core_reg,
};

/* We does not actually save all registers here.  We just invalidate
   all registers in the cache.  When an invalid register in the cache
   is read, its value will then be fetched from hardware.  */
static void bfinplus_save_context(struct target *target)
{
  struct bfinplus_common *bfinplus = target_to_bfinplus(target);
  int i;

  for (i = 0; i < BFINPLUSNUMCOREREGS; i++)
  {
    bfinplus->core_cache->reg_list[i].valid = 0;
    bfinplus->core_cache->reg_list[i].dirty = 0;
  }
}

/* Write dirty registers to hardware.  */
static void bfinplus_restore_context(struct target *target)
{
  struct bfinplus_common *bfinplus = target_to_bfinplus(target);
  int i;

  for (i = 0; i < BFINPLUSNUMCOREREGS; i++)
  {
    struct reg *reg = &bfinplus->core_cache->reg_list[i];
    struct bfinplus_core_reg *bfinplus_reg = reg->arch_info;

    if (reg->dirty)
    {
      uint32_t value = buf_get_u32(reg->value, 0, 32);
      bfinplus_coreregister_set(target, map_gdb_core[bfinplus_reg->num], value);
    }
  }
}

static void bfinplus_debug_entry(struct target *target)
{
  bfinplus_save_context(target);
}

static int bfinplus_arch_state(struct target *target)
{
  return ERROR_FAIL;
}

static int bfinplus_target_request_data(struct target *target,
    uint32_t size, uint8_t *buffer)
{
  return ERROR_FAIL;
}

static int bfinplus_halt(struct target *target)
{
  struct target *target = bfinplus_reg->target;
  struct bfinplus_common *bfinplus = target_to_bfinplus(target);

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

  bfinplus_debug_register_set(target, BFINPLUS_DBG_MYSTERY1C, 0x02);
  bfinplus_debug_register_set(target, BFINPLUS_DBG_MYSTERY0, 0x01);

  target->debug_reason = DBG_REASON_DBGRQ;

  return ERROR_OK;
}

static int bfinplus_resume_1(struct target *target, int current,
    uint32_t address, int handle_breakpoints, int debug_execution, bool step)
{
  struct bfinplus_common *bfinplus = target_to_bfinplus(target);

  /* We don't handle this for now. */
  assert(debug_execution == 0);

  /* FIXME handle handle_breakpoints !!!*/

  if (target->state != TARGET_HALTED)
  {
    LOG_WARNING("target not halted");
    return ERROR_TARGET_NOT_HALTED;
  }

  bfinplus_mmr_get_indirect(target, BFINPLUS_WPIACTL, &bfinplus->dap.wpiactl);//TODO: something with this?
  bfinplus_debug_register_set(target, BFINPLUS_DBG_MYSTERY1C, 0x202);
  bfinplus_get_cti(target); //TODO: check these?
  bfinplus_set_used_ctis(target, 0x00, 0x00, 0x02, 0x01, 0x01);

  bfinplus_mmr_set32(target, BFINPLUS_TRU0_GCTL, 0x01);
  bfinplus_mmr_set32(target, BFINPLUS_TRU0_SSR69, 0x49);

  bfinplus_restore_context(target); //TODO: only restore specific regs

  if (!current)
  {
    bfinplus_coreregister_set(target, map_gdb_core[REG_RETE], address);
  }

  bfinplus_debug_register_set(target, BFINPLUS_DBG_MYSTERY0, 0x02);
  bfinplus_pulse_cti(target);

  bfinplus_set_used_ctis(target, 0x01, 0x00, 0x00, 0x01, 0x00);
  bfinplus_mmr_set32(target, BFINPLUS_TRU0_GCTL, 0x00);
  bfinplus_mmr_set32(target, BFINPLUS_TRU0_SSR69, 0x00);

  bfinplus_debug_register_set(target, BFINPLUS_DBG_MYSTERY1C, 0x02);
  //TODO: save context?

  if (step && !bfinplus->is_stepping)
  {
    //bfinplus_dbgctl_bit_set_esstep(jtag_info);
    bfinplus->is_stepping = 1;
  }
  else if (!step && bfinplus->is_stepping)
  {
    //bfinplus_dbgctl_bit_clear_esstep(jtag_info);
    bfinplus->is_stepping = 0;
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

  bfinplus->is_running = 1;
  bfinplus->is_interrupted = 0;
  bfinplus->dmem_control_valid_p = 0;
  bfinplus->imem_control_valid_p = 0;
  //bfinplus_emulation_return(jtag_info);

  return ERROR_OK;
}

static int bfinplus_resume(struct target *target, int current,
    uint32_t address, int handle_breakpoints, int debug_execution)
{
  int retval;

  retval = bfinplus_resume_1(target, current, address, handle_breakpoints, debug_execution, false);

  return retval;
}

static int bfinplus_deassert_reset(struct target *target)
{
  return ERROR_OK;
}

static int bfinplus_get_gdb_reg_list(struct target *target, struct reg **reg_list[],
                   int *reg_list_size, enum target_register_class reg_class)
{
  struct bfinplus_common *bfinplus = target_to_bfinplus(target);
  int i;

  *reg_list_size = BFIN_NUM_REGS;
  *reg_list = malloc(sizeof(struct reg*) * (*reg_list_size));

  for (i = 0; i < BFINPLUSNUMCOREREGS; i++)
    (*reg_list)[i] = &bfinplus->core_cache->reg_list[i];

  for (i = BFINPLUSNUMCOREREGS; i < BFIN_NUM_REGS; i++)
  {
    if (i == BFIN_PC_REGNUM)
      (*reg_list)[i] = &bfinplus->core_cache->reg_list[BFIN_RETE_REGNUM];
    /* TODO handle CC */
    else
      (*reg_list)[i] = &bfinplus_gdb_dummy_reg;
  }

  return ERROR_OK;
}

static int bfinplus_read_memory(struct target *target, uint32_t address,
    uint32_t size, uint32_t count, uint8_t *buffer)
{
  struct bfinplus_common *bfinplus = target_to_bfinplus(target);
  int retval;

  LOG_DEBUG("address: 0x%8.8" PRIx32 ", size: 0x%8.8" PRIx32 ", count: 0x%8.8" PRIx32 "", address, size, count);

  if (target->state != TARGET_HALTED)
  {
    LOG_WARNING("target not halted");
    return ERROR_TARGET_NOT_HALTED;
  }

  //TODO: check boundaries
  retval = bfinplus_read_mem(jtag_info, address, size, count, buffer);

  return retval;
}

static int bfinplus_write_memory(struct target *target, uint32_t address,
    uint32_t size, uint32_t count, const uint8_t *buffer)
{
  struct bfinplus_common *bfinplus = target_to_bfinplus(target);
  int retval;

  LOG_DEBUG("address: 0x%8.8" PRIx32 ", size: 0x%8.8" PRIx32 ", count: 0x%8.8" PRIx32 "",
      address, size, count);

  if (target->state != TARGET_HALTED)
  {
    LOG_WARNING("target not halted");
    return ERROR_TARGET_NOT_HALTED;
  }

  //TODO: check boundaries
  retval = bfinplus_write_mem(target, address, size, count, buffer);

  return retval;
}

static int bfinplus_checksum_memory(struct target *target,
    uint32_t address, uint32_t count, uint32_t* checksum)
{
  return ERROR_FAIL;
}

static int bfinplus_blank_check_memory(struct target *target,
    uint32_t address, uint32_t count, uint32_t* blank)
{
  return ERROR_FAIL;
}

static int bfinplus_run_algorithm(struct target *target,
    int num_mem_params, struct mem_param *mem_params,
    int num_reg_params, struct reg_param *reg_params,
    uint32_t entry_point, uint32_t exit_point,
    int timeout_ms, void *arch_info)
{
  return ERROR_FAIL;
}

static struct reg_cache *bfinplus_build_reg_cache(struct target *target)
{
  /* get pointers to arch-specific information */
  struct bfinplus_common *bfinplus = target_to_bfinplus(target);

  int num_regs = BFINPLUSNUMCOREREGS;
  struct reg_cache **cache_p = register_get_last_cache_p(&target->reg_cache);
  struct reg_cache *cache = malloc(sizeof(struct reg_cache));
  struct reg *reg_list = malloc(sizeof(struct reg) * num_regs);
  struct bfinplus_core_reg *arch_info = malloc(sizeof(struct bfinplus_core_reg) * num_regs);
  int i;

  register_init_dummy(&bfinplus_gdb_dummy_reg);

  /* Build the process context cache */
  cache->name = "bfinplus registers";
  cache->next = NULL;
  cache->reg_list = reg_list;
  cache->num_regs = num_regs;
  (*cache_p) = cache;
  bfinplus->core_cache = cache;

  for (i = 0; i < num_regs; i++)
  {
    arch_info[i] = bfinplus_core_reg_list_arch_info[i];
    arch_info[i].target = target;
    arch_info[i].bfinplus_common = bfinplus;
    reg_list[i].name = bfinplus_core_reg_list[i];
    reg_list[i].feature = NULL;
    reg_list[i].size = 32;
    reg_list[i].value = calloc(1, 4);
    reg_list[i].dirty = 0;
    reg_list[i].valid = 0;
    reg_list[i].type = &bfinplus_reg_type;
    reg_list[i].arch_info = &arch_info[i];
  }

  return cache;
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