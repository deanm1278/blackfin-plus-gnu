#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "helper/log.h"
#include "helper/types.h"
#include "helper/binarybuffer.h"
#include "arm_adi_v5.h"

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
    //bfinplus_dbgctl_bit_set_esstep(target);
    bfinplus->is_stepping = 1;
  }
  else if (!step && bfinplus->is_stepping)
  {
    //bfinplus_dbgctl_bit_clear_esstep(target);
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
  //bfinplus_emulation_return(target);

  return ERROR_OK;
}

static int bfinplus_resume(struct target *target, int current,
    uint32_t address, int handle_breakpoints, int debug_execution)
{
  int retval;

  retval = bfinplus_resume_1(target, current, address, handle_breakpoints, debug_execution, false);

  return retval;
}

static int bfinplus_assert_reset(struct target *target)
{
  struct bfinplus_common *bfinplus = target_to_bfinplus(target);

  LOG_DEBUG("target->state: %s", target_state_name(target));

  bfinplus_system_reset(target);
  bfinplus_core_reset(target);

  target->state = TARGET_HALTED;

  /* Reset should bring the core out of core fault.  */
  bfinplus->is_corefault = 0;

  /* Reset should invalidate register cache.  */
  //TODO: do we need this
  //register_cache_invalidate(bfinplus->core_cache);

  if (!target->reset_halt)
  {
    bfinplus_resume(target, 1, 0, 0, 0);
    target->state = TARGET_RUNNING;
  }

  return ERROR_OK;
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
  retval = bfinplus_read_mem(target, address, size, count, buffer);

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

static int bfinplus_add_breakpoint(struct target *target, struct breakpoint *breakpoint)
{
  struct bfinplus_common *bfinplus = target_to_bfinplus(target);
  struct bfinplus_dap *dap = &bfinplus->dap;
  int retval;

  assert(breakpoint->set == 0);

  if (breakpoint->type == BKPT_SOFT)
  {
    const uint8_t *bp;

    if (breakpoint->length == 2)
      bp = bfinplus_breakpoint_16;
    else
      bp = bfinplus_breakpoint_32;

    retval = bfinplus_read_memory(target, breakpoint->address,
      breakpoint->length, 1, breakpoint->orig_instr);
    if (retval != ERROR_OK)
      return retval;

    retval = bfinplus_write_memory(target, breakpoint->address,
      breakpoint->length, 1, bp);
    if (retval != ERROR_OK)
      return retval;
  }
  else /* BKPT_HARD */
  {
    int i;
    for (i = 0; i < BFINPLUS_MAX_HWBREAKPOINTS; i++)
      if (dap->hwbps[i] == (uint32_t) -1)
        break;
    if (i == BFINPLUS_MAX_HWBREAKPOINTS)
    {
      LOG_ERROR("%s: no more hardware breakpoint available for 0x%08x",
        target_name(target), breakpoint->address);
      return ERROR_FAIL;
    }

    dap->hwbps[i] = breakpoint->address;
    if (bfinplus->is_locked)
    {
      LOG_WARNING("%s: target is locked, hardware breakpoint [0x%08x] will be set when it's unlocked", target_name(target), breakpoint->address);
    }
    else
    {
      bfinplus_wpu_set_wpia(target, i, breakpoint->address, 1);
    }
  }

  breakpoint->set = 1;

  return ERROR_OK;
}

static int bfinplus_remove_breakpoint(struct target *target, struct breakpoint *breakpoint)
{
  struct bfinplus_common *bfinplus = target_to_bfinplus(target);
  struct bfinplus_dap *dap = &bfinplus->dap;
  int retval;

  assert(breakpoint->set != 0);

  

  if (breakpoint->type == BKPT_SOFT)
  {
    retval = bfinplus_write_memory(target, breakpoint->address,
      breakpoint->length, 1, breakpoint->orig_instr);
    if (retval != ERROR_OK)
      return retval;
  }
  else /* BKPT_HARD */
  {
    int i;
    for (i = 0; i < BFINPLUS_MAX_HWBREAKPOINTS; i++)
      if (dap->hwbps[i] == breakpoint->address)
        break;

    if (i == BFINPLUS_MAX_HWBREAKPOINTS)
    {
      LOG_ERROR("%s: no hardware breakpoint at 0x%08x",
        target_name(target), breakpoint->address);
      return ERROR_FAIL;
    }

    dap->hwbps[i] = -1;
    if (bfinplus->is_locked)
    {
      /* The hardware breakpoint will only be set when target is
         unlocked.  Target cannot be locked again after it's unlocked.
         So if we are removing a hardware breakpoint when target is
         locked, that hardware breakpoint has never been set.
         So we can just quietly remove it.  */
    }
    else
    {
      bfinplus_wpu_set_wpia(target, i, breakpoint->address, 0);
    }
  }

  breakpoint->set = 0;

  return ERROR_OK;
}

static int bfinplus_add_watchpoint(struct target *target, struct watchpoint *watchpoint)
{
  struct bfinplus_common *bfinplus = target_to_bfinplus(target);
  struct bfinplus_dap *dap = &bfinplus->dap;
  int i;
  bool range;

  /* TODO  provide a method to always use range watchpoint.  */

  if (target->state != TARGET_HALTED)
    return ERROR_TARGET_NOT_HALTED;

  if (watchpoint->length <= 4
    && dap->hwwps[0].used
    && (dap->hwwps[1].used
      || dap->hwwps[0].range))
  {
    LOG_ERROR("all hardware watchpoints are in use.");
    return ERROR_TARGET_RESOURCE_NOT_AVAILABLE;
  }

  if (watchpoint->length > 4
    && (dap->hwwps[0].used
      || dap->hwwps[1].used))
  {
    LOG_ERROR("no enough hardware watchpoints.");
    return ERROR_TARGET_RESOURCE_NOT_AVAILABLE;
  }

  if (watchpoint->length <= 4)
  {
    i = dap->hwwps[0].used ? 1 : 0;
    range = false;
  }
  else
  {
    i = 0;
    range = true;
  }

  dap->hwwps[i].addr = watchpoint->address;
  dap->hwwps[i].len = watchpoint->length;
  dap->hwwps[i].range = range;
  dap->hwwps[i].used = true;
  switch (watchpoint->rw)
  {
  case WPT_READ:
    dap->hwwps[i].mode = WPDA_READ;
    break;
  case WPT_WRITE:
    dap->hwwps[i].mode = WPDA_WRITE;
    break;
  case WPT_ACCESS:
    dap->hwwps[i].mode = WPDA_ALL;
    break;
  }
  
  if (bfinplus->is_locked)
  {
    LOG_WARNING("%s: target is locked, hardware watchpoint [0x%08x] will be set when it's unlocked", target_name(target), watchpoint->address);
  }
  else
  {
    bfinplus_wpu_set_wpda(target, i);
  }

  return ERROR_OK;
}

static int bfinplus_remove_watchpoint(struct target *target, struct watchpoint *watchpoint)
{
  struct bfinplus_common *bfinplus = target_to_bfinplus(target);
  struct bfinplus_dap *dap = &bfinplus->dap;
  int mode;
  int i;

  /* TODO  provide a method to always use range watchpoint.  */

  if (target->state != TARGET_HALTED)
    return ERROR_TARGET_NOT_HALTED;

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

  for (i = 0; i < BFINPLUS_MAX_HWWATCHPOINTS; i++)
    if (dap->hwwps[i].addr == watchpoint->address
      && dap->hwwps[i].len == watchpoint->length
      && dap->hwwps[i].mode == mode)
      break;

  if (i == BFINPLUS_MAX_HWWATCHPOINTS)
  {
    LOG_ERROR("no hardware watchpoint at 0x%08X length %d.",
          watchpoint->address, watchpoint->length);
    return ERROR_FAIL;
  }

  if (watchpoint->length > 4)
  {
    assert (i == 0 && dap->hwwps[i].range);
  }

  dap->hwwps[i].mode = WPDA_DISABLE;
  
  if (bfinplus->is_locked)
  {
    /* See the comment in bfinplus_remove_breakpoint.  */
  }
  else
  {
    bfinplus_wpu_set_wpda(target, i);
  }

  dap->hwwps[i].addr = 0;
  dap->hwwps[i].len = 0;
  dap->hwwps[i].range = false;
  dap->hwwps[i].used = false;

  return ERROR_OK;
}

static int bfinplus_target_create(struct target *target, Jim_Interp *interp)
{
  struct bfinplus_common *bfinplus = calloc(1, sizeof(struct bfinplus_common));
  struct Jim_Obj *objPtr;
  const char *map, *config;
  char *end;

  target->arch_info = bfinplus;

  bfinplus->common_magic = BFINPLUS_COMMON_MAGIC;
  strcpy(bfinplus->part, target_name(target));
  end = strchr(bfinplus->part, '.');
  if (end)
    *end = '\0';

  objPtr = Jim_GetGlobalVariableStr(interp, "MEMORY_MAP", JIM_NONE);
  map = Jim_GetString(objPtr, NULL);
  parse_memory_map(target, map);

#if 0
  objPtr = Jim_GetGlobalVariableStr(interp, "BLACKFIN_CONFIG", JIM_NONE);
  config = Jim_GetString(objPtr, NULL);
  blackfin_parse_config(target, config);
#endif

  objPtr = Jim_GetGlobalVariableStr(interp, "SDRRC", JIM_NONE);
  if (objPtr)
  {
    struct bfinplus_sdram_config *sdram_config;
    long value;
    int retval;
    sdram_config = malloc(sizeof (bfinplus->sdram_config));
    if (!sdram_config)
    {
      LOG_ERROR("%s: malloc(%"PRIzu") failed",
            target_name(target), sizeof (bfinplus->sdram_config));
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

    bfinplus->sdram_config = sdram_config;
  }
  else
    bfinplus->sdram_config = NULL;

  objPtr = Jim_GetGlobalVariableStr(interp, "DDRCTL0", JIM_NONE);
  if (objPtr)
  {
    struct bfinplus_ddr_config *ddr_config;
    long value;
    int retval;
    ddr_config = malloc(sizeof (bfinplus->ddr_config));
    if (!ddr_config)
    {
      LOG_ERROR("%s: malloc(%"PRIzu") failed",
            target_name(target), sizeof (bfinplus->ddr_config));
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

    bfinplus->ddr_config = ddr_config;
  }
  else
    bfinplus->ddr_config = NULL;

  bfinplus->dmem_control_valid_p = 0;
  bfinplus->imem_control_valid_p = 0;
  bfinplus->dmem_control = 0;
  bfinplus->imem_control = 0;

  return ERROR_OK;
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

static int bfinplus_init_target(struct command_context *cmd_ctx,
    struct target *target)
{
  struct bfinplus_common *bfinplus = target_to_bfinplus(target);

  bfinplus->dap.target = target;
  bfinplus_build_reg_cache(target);
  return ERROR_OK;
}

static int bfinplus_calculate_wait_clocks(void)
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

static int bfinplus_examine(struct target *target)
{
  struct bfinplus_common *bfinplus = target_to_bfinplus(target);
  struct bfinplus_dap *dap = &bfinplus->dap;
  int i;

  if (!target_was_examined(target))
  {
    target_set_examined(target);

    dap->dbgctl = 0;
    dap->dbgstat = 0;
    dap->emuir_a = BLACKFIN_INSN_ILLEGAL;
    dap->emuir_b = BLACKFIN_INSN_ILLEGAL;
    dap->emudat_out = 0;
    dap->emudat_in = 0;

    dap->wpiactl = 0;
    dap->wpdactl = 0;
    dap->wpstat = 0;
    for (i = 0; i < BFINPLUS_MAX_HWBREAKPOINTS; i++)
      dap->hwbps[i] = -1;
    for (i = 0; i < BFINPLUS_MAX_HWWATCHPOINTS; i++)
    {
      dap->hwwps[i].addr = 0;
      dap->hwwps[i].len = 0;
      dap->hwwps[i].mode = WPDA_DISABLE;
      dap->hwwps[i].range = false;
      dap->hwwps[i].used = false;
    }

    dap->emupc = -1;
  }

  bfinplus_wait_clocks = bfinplus_calculate_wait_clocks();

  return ERROR_OK;
}

COMMAND_HANDLER(bfinplus_handle_wpu_init_command)
{
  struct target *target = get_current_target(CMD_CTX);
  struct bfinplus_common *bfinplus = target_to_bfinplus(target);
  struct bfinplus_dap *dap = &bfinplus->dap;

  if (!target_was_examined(target))
  {
    LOG_ERROR("target not examined yet");
    return ERROR_FAIL;
  }

  bfinplus_wpu_init(target);
  return ERROR_OK;
}

COMMAND_HANDLER(bfinplus_handle_sdram_init_command)
{
  struct target *target = get_current_target(CMD_CTX);
  struct bfinplus_common *bfinplus = target_to_bfinplus(target);
  int retval;

  if (!target_was_examined(target))
  {
    LOG_ERROR("target not examined yet");
    return ERROR_FAIL;
  }
  if (!bfinplus->sdram_config)
  {
    LOG_ERROR("no SDRAM config");
    return ERROR_FAIL;
  }

  retval = bfinplus_sdram_init(target);
  return retval;
}

COMMAND_HANDLER(bfinplus_handle_ddr_init_command)
{
  struct target *target = get_current_target(CMD_CTX);
  struct bfinplus_common *bfinplus = target_to_bfinplus(target);
  int retval;

  if (!target_was_examined(target))
  {
    LOG_ERROR("target not examined yet");
    return ERROR_FAIL;
  }
  if (!bfinplus->ddr_config)
  {
    LOG_ERROR("no DDR config");
    return ERROR_FAIL;
  }

  retval = bfinplus_ddr_init(target);
  return retval;
}

static const struct command_registration bfinplus_exec_command_handlers[] = {
  {
    .name = "wpu_init",
    .handler = bfinplus_handle_wpu_init_command,
    .mode = COMMAND_EXEC,
    .help = "Initialize Watchpoint Unit",
  },
  {
    .name = "sdram_init",
    .handler = bfinplus_handle_sdram_init_command,
    .mode = COMMAND_EXEC,
    .help = "Initialize SDRAM",
  },
  {
    .name = "ddr_init",
    .handler = bfinplus_handle_ddr_init_command,
    .mode = COMMAND_EXEC,
    .help = "Initialize DDR",
  },
  COMMAND_REGISTRATION_DONE
};

static const struct command_registration bfinplus_command_handlers[] = {
  {
    .name = "bfinplus",
    .mode = COMMAND_ANY,
    .help = "bfinplus command group",
    .chain = bfinplus_exec_command_handlers,
  },
  COMMAND_REGISTRATION_DONE
};

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