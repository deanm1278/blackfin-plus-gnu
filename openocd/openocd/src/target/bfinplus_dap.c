#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <time.h>

#include "helper/binarybuffer.h"
#include "jtag/jtag.h"

#include "bfinplus.h"
#include "bfinplus_dap.h"
#include "bfinplus_mem.h"
#include "bfin_insn.h"

#define BFINPLUS_JTAG_DBG_BASE  0x80001000
#define BFINPLUS_IDCODE			0x02
#define BFINPLUS_EMUIR			0x08
#define BFINPLUS_EMUDAT			0x0C

int bfinplus_debug_register_get(struct target *target, uint8_t reg, uint32_t *value)
{
	struct bfinplus_common *bfin = target_to_bfinplus(target);
	struct adiv5_dap *swjdp = &(bfin.dap.dap);

	uint8_t buf[4];
	retval = mem_ap_read(swjdp, buf, sizeof(uint32_t), 1,
		BFINPLUS_JTAG_DBG_BASE + reg, false);

	*value = ((uint32_t)buf[0] << 24) | ((uint32_t)buf[1] << 16) | ((uint32_t)buf[2] << 8) | buf[3];

	return retval;
}

int bfinplus_debug_register_set(struct target *target, uint8_t reg, uint32_t value)
{
	struct bfinplus_common *bfin = target_to_bfinplus(target);
	struct adiv5_dap *swjdp = &(bfin.dap.dap);

	uint8_t buf[] = { (value >> 24) & 0xFF, (value >> 16) & 0xFF, (value >> 8) & 0xFF, value & 0xFF };
	retval = mem_ap_write(swjdp, (const)buf, sizeof(uint32_t), 1,
		BFINPLUS_JTAG_DBG_BASE + reg, false);

	return retval;
}

static int bfinplus_emuir_set(struct target *target, uint32_t value)
{
	return bfinplus_debug_register_set(target, BFINPLUS_EMUIR, value);
}

static int bfinplus_emudat_set(struct target *target, uint32_t value)
{
	return bfinplus_debug_register_set(target, BFINPLUS_EMUDAT, value);
}

static int bfinplus_emudat_get(struct target *target, uint32_t *value)
{
	return bfinplus_register_get(target, BFINPLUS_EMUDAT, value);
}

int bfinplus_coreregister_get(struct target *target, enum core_regnum reg, uint32_t *value)
{
	int retval;
	uint8_t reg = regnum&0xFF;
	struct bfinplus_common *bfin = target_to_bfinplus(target);
	struct adiv5_dap *swjdp = &(bfin.dap.dap);

	LOG_DEBUG("get register 0x%08" PRIx32, reg);
	dap_ap_select(swjdp, BFINPLUS_APB_AP);

	bfinplus_emuir_set(target, blackfin_gen_move(REG_EMUDAT, reg));
	retval = bfinplus_emudat_get(target, value);
	return retval;
}

int bfinplus_coreregister_set(struct target *target, enum core_regnum reg, uint32_t value)
{
	int retval;
	uint8_t reg = regnum&0xFF;
	struct bfinplus_common *bfin = target_to_bfinplus(target);
	struct adiv5_dap *swjdp = &(bfin.dap.dap);

	LOG_DEBUG("set register 0x%08" PRIx32, reg);
	dap_ap_select(swjdp, BFINPLUS_APB_AP);

	bfinplus_emudat_set(target, value);
	retval = bfinplus_emuir_set(target, blackfin_gen_move(reg, REG_EMUDAT));
	return retval;
}

int bfinplus_mmr_get_indirect(struct target *target, uint32_t addr, uint32_t *value)
{
	int retval;
	struct bfinplus_common *bfin = target_to_bfinplus(target);
	struct adiv5_dap *swjdp = &(bfin.dap.dap);

	LOG_DEBUG("get mmr indirect 0x%08" PRIx32, reg);
	dap_ap_select(swjdp, BFINPLUS_APB_AP);

	//EMUDAT = addr
	bfinplus_emudat_set(target, addr);
	//P0 = EMUDAT
	bfinplus_emuir_set(target, blackfin_gen_move(REG_P0, REG_EMUDAT));
	//R0 = [P0]
	bfinplus_emuir_set(target, blackfin_gen_load32(REG_P0, REG_R0));
	//EMUDAT = R0
	bfinplus_emuir_set(target, blackfin_gen_move(REG_EMUDAT, REG_R0));
	retval = bfinplus_emudat_get(target, value);

	return retval;
}

int bfinplus_mmr_set_indirect(struct target *target, uint32_t addr, uint32_t value)
{
	int retval;
	struct bfinplus_common *bfin = target_to_bfinplus(target);
	struct adiv5_dap *swjdp = &(bfin.dap.dap);

	LOG_DEBUG("set mmr indirect 0x%08" PRIx32, reg);
	dap_ap_select(swjdp, BFINPLUS_APB_AP);

	//EMUDAT = addr
	bfinplus_emudat_set(target, addr);
	//P0 = EMUDAT
	bfinplus_emuir_set(target, blackfin_gen_move(REG_P0, REG_EMUDAT));
	//EMUDAT = value
	bfinplus_emudat_set(target, value);
	//R0 = EMUDAT
	bfinplus_emuir_set(target, blackfin_gen_move(REG_R0, REG_EMUDAT));
	//[P0] = R0
	retval = bfinplus_emuir_set(target, blackfin_gen_store32(REG_P0, REG_R0));
	return retval;
}