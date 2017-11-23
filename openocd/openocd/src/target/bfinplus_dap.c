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

static uint32_t bfinplus_get_r0(struct target *target)
{
	uint32_t val;
	bfinplus_coreregister_get(target, REG_R0, &val);
	return val;
}

static uint32_t bfinplus_get_p0(struct target *target)
{
	uint32_t val;
	bfinplus_coreregister_get(target, REG_P0, &val);
	return val;
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

static int bfinplus_set_r0(struct target *target, uint32_t value)
{
	return bfinplus_coreregister_set(target, REG_R0, value);
}

static int bfinplus_set_p0(struct target *target, uint32_t value)
{
	return bfinplus_coreregister_set(target, REG_P0, value);
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
	//TODO: check cache status?	
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
	//TODO: check cache status?	
	retval = mem_ap_write(swjdp, (const)buf, sizeof(uint32_t), 1, addr, false);

	return retval;
}

int bfinplus_read_mem(struct target *target, uint32_t addr,
		uint32_t size, uint32_t count, uint8_t *buf)
{
	struct bfinplus_common *bfin = target_to_bfinplus(target);
	struct adiv5_dap *swjdp = &(bfin->dap.dap);

	//TODO: set CSW based on size?
	//TODO: check cache status?	
	return mem_ap_sel_read_buf(swjdp, BFINPLUS_AHB_AP, buf, size, count, addr);
}

int bfinplus_write_mem(struct target *target, uint32_t addr,
		uint32_t size, uint32_t count, const uint8_t *buf)
{
	struct bfinplus_common *bfin = target_to_bfinplus(target);
	struct adiv5_dap *swjdp = &(bfin->dap.dap);

	//TODO: set CSW based on size?
	//TODO: check cache status?	
	return mem_ap_write(swjdp, buf, size, count, addr, false);
}

void bfinplus_wpu_init(struct target *target)
{
	uint32_t p0, r0;
	uint32_t wpiactl, wpdactl;

	LOG_DEBUG("-");

	p0 = bfinplus_get_p0(target);
	r0 = bfinplus_get_r0(target);

	bfinplus_set_p0(target, WPIACTL);

	wpiactl = WPIACTL_WPPWR;bfinplus_set

	bfinplus_set_r0(target, wpiactl);

	bfinplus_emuir_set(target, blackfin_gen_store32_offset(REG_P0, 0, REG_R0), TAP_IDLE);

	wpiactl |= WPIACTL_EMUSW5 | WPIACTL_EMUSW4 | WPIACTL_EMUSW3;
	wpiactl |= WPIACTL_EMUSW2 | WPIACTL_EMUSW1 | WPIACTL_EMUSW0;

	bfinplus_set_r0(target, wpiactl);
	bfinplus_emuir_set(target, blackfin_gen_store32_offset(REG_P0, 0, REG_R0), TAP_IDLE);

	wpdactl = WPDACTL_WPDSRC1_A | WPDACTL_WPDSRC0_A;

	bfinplus_set_r0(target, wpdactl);
	bfinplus_emuir_set(target, blackfin_gen_store32_offset(REG_P0, WPDACTL - WPIACTL, REG_R0),
					   TAP_IDLE);

	bfinplus_set_r0(target, 0);
	bfinplus_emuir_set(target, blackfin_gen_store32_offset(REG_P0, WPSTAT - WPIACTL, REG_R0),
					   TAP_IDLE);

	dap->wpiactl = wpiactl;
	dap->wpdactl = wpdactl;

	bfinplus_set_p0(target, p0);
	bfinplus_set_r0(target, r0);
}

static uint32_t wpiaen[] = {
	WPIACTL_WPIAEN0,
	WPIACTL_WPIAEN1,
	WPIACTL_WPIAEN2,
	WPIACTL_WPIAEN3,
	WPIACTL_WPIAEN4,
	WPIACTL_WPIAEN5,
};

void bfinplus_wpu_set_wpia(struct target *target, int n, uint32_t addr, int enable)
{
	uint32_t p0, r0;

	p0 = bfinplus_get_p0(target);
	r0 = bfinplus_get_r0(target);

	struct bfinplus_common *bfin = target_to_bfinplus(target);
	struct adiv5_dap *dap = &(bfin->dap.dap);

	bfinplus_coreregister_set(target, REG_P0, WPIACTL);
	if (enable)
	{
		dap->wpiactl += wpiaen[n];
		bfinplus_coreregister_set(target, REG_R0, addr);
		bfinplus_emuir_set(target,
			blackfin_gen_store32_offset(REG_P0, WPIA0 + 4 * n - WPIACTL, REG_R0),
			TAP_IDLE);
	}
	else
	{
		dap->wpiactl &= ~wpiaen[n];
	}

	bfinplus_coreregister_set(target, REG_R0, dap->wpiactl);
	bfinplus_emuir_set(target,
		blackfin_gen_store32_offset(REG_P0, 0, REG_R0),
		TAP_IDLE);

	bfinplus_set_p0(target, p0);
	bfinplus_set_r0(target, r0);
}

void bfinplus_wpu_set_wpda(struct target *target, int n)
{
	uint32_t p0, r0;
	uint32_t addr = dap->hwwps[n].addr;
	uint32_t len = dap->hwwps[n].len;
	int mode = dap->hwwps[n].mode;
	bool range = dap->hwwps[n].range;

	struct bfinplus_common *bfin = target_to_bfinplus(target);
	struct adiv5_dap *dap = &(bfin->dap.dap);

	p0 = bfinplus_get_p0(target);
	r0 = bfinplus_get_r0(target);

	bfinplus_coreregister_set(target, REG_P0, WPDACTL);

	/* Currently LEN > 4 iff RANGE.  But in future, it can change.  */
	if (len > 4 || range)
	{
		assert (n == 0);

		switch (mode)
		{
		case WPDA_DISABLE:
			dap->wpdactl &= ~WPDACTL_WPDREN01;
			break;
        case WPDA_WRITE:
			dap->wpdactl &= ~WPDACTL_WPDACC0_R;
			dap->wpdactl |= WPDACTL_WPDREN01 | WPDACTL_WPDACC0_W;
			break;
        case WPDA_READ:
			dap->wpdactl &= ~WPDACTL_WPDACC0_W;
			dap->wpdactl |= WPDACTL_WPDREN01 | WPDACTL_WPDACC0_R;
			break;
        case WPDA_ALL:
			dap->wpdactl |= WPDACTL_WPDREN01 | WPDACTL_WPDACC0_A;
			break;
        default:
			abort ();
        }
		
		if (mode != WPDA_DISABLE)
        {
			bfinplus_coreregister_set(target, REG_R0, addr - 1);
			bfinplus_emuir_set(target,
				blackfin_gen_store32_offset(REG_P0, WPDA0 - WPDACTL, REG_R0),
				TAP_IDLE);
			bfinplus_coreregister_set(target, REG_R0, addr + len - 1);
			bfinplus_emuir_set(target,
				blackfin_gen_store32_offset(REG_P0, WPDA0 + 4 - WPDACTL, REG_R0),
				TAP_IDLE);
        }
    }
	else
	{
		if (n == 0)
			switch (mode)
			{
			case WPDA_DISABLE:
				dap->wpdactl &= ~WPDACTL_WPDAEN0;
				break;
			case WPDA_WRITE:
				dap->wpdactl &= ~WPDACTL_WPDACC0_R;
				dap->wpdactl |= WPDACTL_WPDAEN0 | WPDACTL_WPDACC0_W;
				break;
			case WPDA_READ:
				dap->wpdactl &= ~WPDACTL_WPDACC0_W;
				dap->wpdactl |= WPDACTL_WPDAEN0 | WPDACTL_WPDACC0_R;
				break;
			case WPDA_ALL:
				dap->wpdactl |= WPDACTL_WPDAEN0 | WPDACTL_WPDACC0_A;
				break;
			default:
				abort ();
			}
		else
			switch (mode)
			{
			case WPDA_DISABLE:
				dap->wpdactl &= ~WPDACTL_WPDAEN1;
				break;
			case WPDA_WRITE:
				dap->wpdactl &= ~WPDACTL_WPDACC1_R;
				dap->wpdactl |= WPDACTL_WPDAEN1 | WPDACTL_WPDACC1_W;
				break;
			case WPDA_READ:
				dap->wpdactl &= ~WPDACTL_WPDACC1_W;
				dap->wpdactl |= WPDACTL_WPDAEN1 | WPDACTL_WPDACC1_R;
				break;
			case WPDA_ALL:
				dap->wpdactl |= WPDACTL_WPDAEN1 | WPDACTL_WPDACC1_A;
				break;
			default:
				abort ();
			}
		if (mode != WPDA_DISABLE)
        {
			bfinplus_coreregister_set(target, REG_R0, addr);
			bfinplus_emuir_set(target,
				blackfin_gen_store32_offset(REG_P0, WPDA0 + 4 * n - WPDACTL, REG_R0),
				TAP_IDLE);
        }
    }

	bfinplus_coreregister_set(target, REG_R0, dap->wpdactl);
	bfinplus_emuir_set(target,
		blackfin_gen_store32_offset(REG_P0, 0, REG_R0),
		TAP_IDLE);

	bfinplus_set_p0(target, p0);
	bfinplus_set_r0(target, r0);
}

void bfinplus_core_reset(struct target *)
{
	//TODO: this
	return ERROR_OK;
}

void bfinplus_system_reset(struct target *)
{
	//TODO: this
	return ERROR_OK;
}