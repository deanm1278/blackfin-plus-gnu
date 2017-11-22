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

int bfinplus_debug_register_get(struct target *target, uint8_t reg, uint32_t *value)
{
	struct bfinplus_common *bfin = target_to_bfinplus(target);
	struct adiv5_dap *swjdp = &(bfin->dap.dap);

	uint8_t buf[4];
	dap_ap_select(swjdp, BFINPLUS_APB_AP);
	retval = mem_ap_read(swjdp, buf, sizeof(uint32_t), 1,
		BFINPLUS_JTAG_DBG_BASE + reg, false);

	*value = buf_get_u32(buf, 0, 32);

	return retval;
}

int bfinplus_debug_register_set(struct target *target, uint8_t reg, uint32_t value)
{
	struct bfinplus_common *bfin = target_to_bfinplus(target);
	struct adiv5_dap *swjdp = &(bfin->dap.dap);

	uint8_t buf[4];
	dap_ap_select(swjdp, BFINPLUS_APB_AP);
	buf_set_u32(buf, 0, 32, value);
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

static int bfinplus_cti_register_get(struct target *target, uint32_t ctibase,
uint32_t offset, uint32_t *value)
{
	struct bfinplus_common *bfin = target_to_bfinplus(target);
	struct adiv5_dap *swjdp = &(bfin->dap.dap);

	uint8_t buf[4];
	dap_ap_select(swjdp, BFINPLUS_APB_AP);
	retval = mem_ap_read(swjdp, buf, sizeof(uint32_t), 1,
		ctibase + offset, false);

	*value = buf_get_u32(buf, 0, 32);

	return retval;
}

static int bfinplus_cti_register_set(struct target *target, uint32_t ctibase,
uint32_t offset, uint32_t value)
{
	struct bfinplus_common *bfin = target_to_bfinplus(target);
	struct adiv5_dap *swjdp = &(bfin->dap.dap);

	uint8_t buf[4];
	dap_ap_select(swjdp, BFINPLUS_APB_AP);
	buf_set_u32(buf, 0, 32, value);
	retval = mem_ap_write(swjdp, (const)buf, sizeof(uint32_t), 1,
		ctibase + offset, false);

	return retval;
}

int bfinplus_coreregister_get(struct target *target, enum core_regnum reg, uint32_t *value)
{
	int retval;
	uint8_t reg = regnum&0xFF;
	struct bfinplus_common *bfin = target_to_bfinplus(target);
	struct adiv5_dap *swjdp = &(bfin->dap.dap);

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
	struct adiv5_dap *swjdp = &(bfin->dap.dap);

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
	struct adiv5_dap *swjdp = &(bfin->dap.dap);

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
	struct adiv5_dap *swjdp = &(bfin->dap.dap);

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

int bfinplus_get_cti(struct target *target)
{
	int retval;
	struct bfinplus_common *bfin = target_to_bfinplus(target);
	struct cti *proccti = &(bfin->dap.proccti);
	struct cti *syscti = &(bfin->dap.syscti);

	LOG_DEBUG("reading CTI registers 0x%08" PRIx32);
	
	bfinplus_cti_register_get(target, BFINPLUS_PROCCTI_BASE, 
		CTIINEN_OFFSET, &proccti->ctiinen[0]);
	bfinplus_cti_register_get(target, BFINPLUS_PROCCTI_BASE, 
		CTIOUTEN_OFFSET + (7 * sizeof(uint32_t)), &proccti->ctiouten[7]);
	bfinplus_cti_register_get(target, BFINPLUS_SYSCTI_BASE, 
		CTIOUTEN_OFFSET + (7 * sizeof(uint32_t)), &syscti->ctiouten[7]);
	bfinplus_cti_register_get(target, BFINPLUS_SYSCTI_BASE, 
		CTIOUTEN_OFFSET + (1 * sizeof(uint32_t)), &syscti->ctiouten[1]);
	retval = bfinplus_cti_register_get(target, BFINPLUS_SYSCTI_BASE, 
		CTIINEN_OFFSET + (7 * sizeof(uint32_t)), &syscti->ctiinen[7]);

	return retval;
}

int bfinplus_set_used_ctis(struct target *target, uint32_t procinen0, uint32_t procouten7,
	uint32_t sysouten7, uint32_t sysouten1, uint32_t sysinen7)
{
	int retval;
	struct bfinplus_common *bfin = target_to_bfinplus(target);
	struct cti *proccti = &(bfin->dap.proccti);
	struct cti *syscti = &(bfin->dap.syscti);

	bfinplus_cti_register_set(target, BFINPLUS_PROCCTI_BASE, CTIINEN_OFFSET, procinen0);
	bfinplus_cti_register_set(target, BFINPLUS_PROCCTI_BASE, CTIOUTEN_OFFSET + (7 * sizeof(uint32_t)), procouten7);
	bfinplus_cti_register_set(target, BFINPLUS_SYSCTI_BASE, CTIOUTEN_OFFSET + (7 * sizeof(uint32_t)), sysouten7);
	bfinplus_cti_register_set(target, BFINPLUS_SYSCTI_BASE, CTIOUTEN_OFFSET + (1 * sizeof(uint32_t)), sysouten1);
	retval = bfinplus_cti_register_set(target, BFINPLUS_SYSCTI_BASE, CTIINEN_OFFSET + (7 * sizeof(uint32_t)), sysinen7);

	proccti->ctiinen[0] = procinen0;
	proccti->ctiouten[7] = procouten7;
	syscti->ctiouten[7] = sysouten7;
	syscti->ctiouten[1] = sysouten1;
	syscti->ctiinen[7] = sysinen7;

	return retval;
}

int bfinplus_pulse_cti(struct target *)
{
	//always pulse on channel 1
	return bfinplus_cti_register_set(target, BFINPLUS_SYSCTI_BASE, CTIAPPPLUSE_OFFSET, 0x02);
}

int bfinplus_mmr_get32(struct target *target, uint8_t addr, uint32_t *value)
{
	struct bfinplus_common *bfin = target_to_bfinplus(target);
	struct adiv5_dap *swjdp = &(bfin->dap.dap);

	uint8_t buf[4];
	dap_ap_select(swjdp, BFINPLUS_AHB_AP);
	retval = mem_ap_read(swjdp, buf, sizeof(uint32_t), 1, addr, false);

	*value = buf_get_u32(buf, 0, 32);

	return retval;
}

int bfinplus_mmr_set32(struct target *target, uint8_t addr, uint32_t value)
{
	struct bfinplus_common *bfin = target_to_bfinplus(target);
	struct adiv5_dap *swjdp = &(bfin->dap.dap);

	uint8_t buf[4];
	dap_ap_select(swjdp, BFINPLUS_AHB_AP);
	buf_set_u32(buf, 0, 32, value);
	retval = mem_ap_write(swjdp, (const)buf, sizeof(uint32_t), 1, addr, false);

	return retval;
}

int bfinplus_read_mem(struct target *target, uint32_t addr,
		uint32_t size, uint32_t count, uint8_t *buf)
{
	struct bfinplus_common *bfin = target_to_bfinplus(target);
	struct adiv5_dap *swjdp = &(bfin->dap.dap);

	//TODO: set CSW based on size?
	return mem_ap_sel_read_buf(swjdp, BFINPLUS_AHB_AP, buf, size, count, addr);
}

int bfinplus_write_mem(struct target *target, uint32_t addr,
		uint32_t size, uint32_t count, const uint8_t *buf)
{
	struct bfinplus_common *bfin = target_to_bfinplus(target);
	struct adiv5_dap *swjdp = &(bfin->dap.dap);

	//TODO: set CSW based on size?
	return mem_ap_write(swjdp, buf, size, count, addr, false);
}