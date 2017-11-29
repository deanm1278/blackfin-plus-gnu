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
#include "blackfin_insn.h"

#define BFINPLUS_JTAG_DBG_BASE  0x80001000

#define WPIACTL							0x1FC07000
#define WPIACTL_WPAND					0x02000000
#define WPIACTL_EMUSW5					0x01000000
#define WPIACTL_EMUSW4					0x00800000
#define WPIACTL_WPICNTEN5				0x00400000
#define WPIACTL_WPICNTEN4				0x00200000
#define WPIACTL_WPIAEN5					0x00100000
#define WPIACTL_WPIAEN4					0x00080000
#define WPIACTL_WPIRINV45				0x00040000
#define WPIACTL_WPIREN45				0x00020000
#define WPIACTL_EMUSW3					0x00010000
#define WPIACTL_EMUSW2					0x00008000
#define WPIACTL_WPICNTEN3				0x00004000
#define WPIACTL_WPICNTEN2				0x00002000
#define WPIACTL_WPIAEN3					0x00001000
#define WPIACTL_WPIAEN2					0x00000800
#define WPIACTL_WPIRINV23				0x00000400
#define WPIACTL_WPIREN23				0x00000200
#define WPIACTL_EMUSW1					0x00000100
#define WPIACTL_EMUSW0					0x00000080
#define WPIACTL_WPICNTEN1				0x00000040
#define WPIACTL_WPICNTEN0				0x00000020
#define WPIACTL_WPIAEN1					0x00000010
#define WPIACTL_WPIAEN0					0x00000008
#define WPIACTL_WPIRINV01				0x00000004
#define WPIACTL_WPIREN01				0x00000002
#define WPIACTL_WPPWR					0x00000001
#define WPIA0							0x1FC07040

#define WPDACTL							0x1FC07100
#define WPDACTL_WPDACC1_R				0x00002000
#define WPDACTL_WPDACC1_W				0x00001000
#define WPDACTL_WPDACC1_A				0x00003000
#define WPDACTL_WPDSRC1_1				0x00000800
#define WPDACTL_WPDSRC1_0				0x00000400
#define WPDACTL_WPDSRC1_A				0x00000c00
#define WPDACTL_WPDACC0_R				0x00000200
#define WPDACTL_WPDACC0_W				0x00000100
#define WPDACTL_WPDACC0_A				0x00000300
#define WPDACTL_WPDSRC0_1				0x00000080
#define WPDACTL_WPDSRC0_0				0x00000040
#define WPDACTL_WPDSRC0_A				0x000000c0
#define WPDACTL_WPDCNTEN1				0x00000020
#define WPDACTL_WPDCNTEN0				0x00000010
#define WPDACTL_WPDAEN1					0x00000008
#define WPDACTL_WPDAEN0					0x00000004
#define WPDACTL_WPDRINV01				0x00000002
#define WPDACTL_WPDREN01				0x00000001
#define WPDA0							0x1FC07140

#define WPSTAT							0x1FC07200

int bfinplus_debug_register_get(struct target *target, uint8_t reg, uint32_t *value)
{
	int retval;
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
	int retval;
	struct bfinplus_common *bfin = target_to_bfinplus(target);
	struct adiv5_dap *swjdp = &(bfin->dap.dap);

	uint8_t buf[4];
	dap_ap_select(swjdp, BFINPLUS_APB_AP);
	buf_set_u32(buf, 0, 32, value);
	retval = mem_ap_write(swjdp, buf, sizeof(uint32_t), 1,
		BFINPLUS_JTAG_DBG_BASE + reg, true);

	return retval;
}

static int bfinplus_emuir_set(struct target *target, uint32_t value)
{
	return bfinplus_debug_register_set(target, BFINPLUS_EMUIR, value << 16);
}

static int bfinplus_emudat_set(struct target *target, uint32_t value)
{
	return bfinplus_debug_register_set(target, BFINPLUS_EMUDAT, value);
}

static int bfinplus_emudat_get(struct target *target, uint32_t *value)
{
	return bfinplus_debug_register_get(target, BFINPLUS_EMUDAT, value);
}

int bfinplus_cti_register_get(struct target *target, uint32_t ctibase,
uint32_t offset, uint32_t *value)
{
	int retval;
	struct bfinplus_common *bfin = target_to_bfinplus(target);
	struct adiv5_dap *swjdp = &(bfin->dap.dap);

	uint8_t buf[4];
	dap_ap_select(swjdp, BFINPLUS_APB_AP);
	retval = mem_ap_read(swjdp, buf, sizeof(uint32_t), 1,
		ctibase + offset, false);

	*value = buf_get_u32(buf, 0, 32);

	return retval;
}

int bfinplus_cti_register_set(struct target *target, uint32_t ctibase,
uint32_t offset, uint32_t value)
{
	int retval;
	struct bfinplus_common *bfin = target_to_bfinplus(target);
	struct adiv5_dap *swjdp = &(bfin->dap.dap);

	uint8_t buf[4];
	dap_ap_select(swjdp, BFINPLUS_APB_AP);
	buf_set_u32(buf, 0, 32, value);
	retval = mem_ap_write(swjdp, buf, sizeof(uint32_t), 1,
		ctibase + offset, true);

	return retval;
}

int bfinplus_coreregister_get(struct target *target, enum core_regnum regnum, uint32_t *value)
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

static inline uint32_t bfinplus_get_r0(struct target *target)
{
	uint32_t val;
	bfinplus_coreregister_get(target, REG_R0, &val);
	return val;
}

static inline uint32_t bfinplus_get_p0(struct target *target)
{
	uint32_t val;
	bfinplus_coreregister_get(target, REG_P0, &val);
	return val;
}

int bfinplus_coreregister_set(struct target *target, enum core_regnum regnum, uint32_t value)
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

static inline int bfinplus_set_r0(struct target *target, uint32_t value)
{
	return bfinplus_coreregister_set(target, REG_R0, value);
}

static inline int bfinplus_set_p0(struct target *target, uint32_t value)
{
	return bfinplus_coreregister_set(target, REG_P0, value);
}

int bfinplus_mmr_get_indirect(struct target *target, uint32_t addr, uint32_t *value)
{
	int retval;
	struct bfinplus_common *bfin = target_to_bfinplus(target);
	struct adiv5_dap *swjdp = &(bfin->dap.dap);

	LOG_DEBUG("get mmr indirect 0x%08" PRIx32, addr);

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

	if (addr == BFINPLUS_L1DM_DCTL)
    {
		bfin->dmem_control = *value;
    }
	else if (addr == BFINPLUS_L1IM_ICTL)
    {
		bfin->imem_control = *value;
    }

	return retval;
}

int bfinplus_mmr_set_indirect(struct target *target, uint32_t addr, uint32_t value)
{
	int retval;
	struct bfinplus_common *bfin = target_to_bfinplus(target);
	struct adiv5_dap *swjdp = &(bfin->dap.dap);

	LOG_DEBUG("set mmr indirect 0x%08" PRIx32, addr);
	dap_ap_select(swjdp, BFINPLUS_APB_AP);

	if (addr == BFINPLUS_L1DM_DCTL)
    {
		bfin->dmem_control = value;
    }
	else if (addr == BFINPLUS_L1IM_ICTL)
    {
		bfin->imem_control = value;
    }

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

void bfinplus_cache_status_get(struct target *target)
{
	struct bfinplus_common *bfinplus = target_to_bfinplus(target);
	uint32_t val;
	LOG_DEBUG("cache status get" PRIx32);

	bfinplus_mmr_get_indirect(target, BFINPLUS_L1DM_DCTL, &val);
	bfinplus_mmr_get_indirect(target, BFINPLUS_L1IM_ICTL, &val);

	if (bfinplus->imem_control & IMC)
		bfinplus->l1_code_cache_enabled = 1;
	else
		bfinplus->l1_code_cache_enabled = 0;

	if ((bfinplus->dmem_control & DMC) == ACACHE_BCACHE)
	{
		bfinplus->l1_data_a_cache_enabled = 1;
		bfinplus->l1_data_b_cache_enabled = 1;
	}
	else if ((bfinplus->dmem_control & DMC) == ACACHE_BSRAM)
	{
		bfinplus->l1_data_a_cache_enabled = 1;
		bfinplus->l1_data_b_cache_enabled = 0;
	}
	else
	{
		bfinplus->l1_data_a_cache_enabled = 0;
		bfinplus->l1_data_b_cache_enabled = 0;
	}
}

int bfinplus_get_cti(struct target *target)
{
	int retval;
	struct bfinplus_common *bfin = target_to_bfinplus(target);
	struct bfinplus_cti *proccti = &(bfin->dap.proccti);
	struct bfinplus_cti *syscti = &(bfin->dap.syscti);

	LOG_DEBUG("reading CTI registers" PRIx32);
	
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
	struct bfinplus_cti *proccti = &(bfin->dap.proccti);
	struct bfinplus_cti *syscti = &(bfin->dap.syscti);

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

int bfinplus_pulse_cti(struct target *target)
{
	//always pulse on channel 1
	return bfinplus_cti_register_set(target, BFINPLUS_SYSCTI_BASE, CTIAPPPLUSE_OFFSET, 0x02);
}

int bfinplus_mmr_get32(struct target *target, uint32_t addr, uint32_t *value)
{
	int retval;
	struct bfinplus_common *bfin = target_to_bfinplus(target);
	struct adiv5_dap *swjdp = &(bfin->dap.dap);

	uint8_t buf[4];
	//TODO: check cache status?	
	dap_ap_select(swjdp, BFINPLUS_AHB_AP);
	retval = mem_ap_read(swjdp, buf, sizeof(uint32_t), 1, addr, true);

	*value = buf_get_u32(buf, 0, 32);

	return retval;
}

int bfinplus_mmr_set32(struct target *target, uint32_t addr, uint32_t value)
{
	int retval;
	struct bfinplus_common *bfin = target_to_bfinplus(target);
	struct adiv5_dap *swjdp = &(bfin->dap.dap);

	uint8_t buf[4];
	//cache_status_get(target);
	dap_ap_select(swjdp, BFINPLUS_AHB_AP);
	buf_set_u32(buf, 0, 32, value);
	//TODO: check cache status?	
	retval = mem_ap_write(swjdp, buf, sizeof(uint32_t), 1, addr, true);

	return retval;
}

int bfinplus_read_mem(struct target *target, uint32_t addr,
		uint32_t size, uint32_t count, uint8_t *buf)
{
	struct bfinplus_common *bfin = target_to_bfinplus(target);
	struct adiv5_dap *swjdp = &(bfin->dap.dap);

	//TODO: set CSW based on size?
	//cache_status_get(target);
	return mem_ap_sel_read_buf(swjdp, BFINPLUS_AHB_AP, buf, size, count, addr);
}

int bfinplus_write_mem(struct target *target, uint32_t addr,
		uint32_t size, uint32_t count, const uint8_t *buf)
{
	struct bfinplus_common *bfin = target_to_bfinplus(target);
	struct adiv5_dap *swjdp = &(bfin->dap.dap);

	//TODO: set CSW based on size?
	//cache_status_get(target);
	return mem_ap_write(swjdp, buf, size, count, addr, true);
}

void bfinplus_wpu_init(struct target *target)
{
	uint32_t p0, r0;
	uint32_t wpiactl, wpdactl;
	struct bfinplus_common *bfin = target_to_bfinplus(target);
	struct bfinplus_dap *dap = &(bfin->dap);

	LOG_DEBUG("-");

	p0 = bfinplus_get_p0(target);
	r0 = bfinplus_get_r0(target);

	bfinplus_set_p0(target, WPIACTL);

	wpiactl = WPIACTL_WPPWR;

	bfinplus_set_r0(target, wpiactl);

	bfinplus_emuir_set(target, blackfin_gen_store32_offset(REG_P0, 0, REG_R0));

	wpiactl |= WPIACTL_EMUSW5 | WPIACTL_EMUSW4 | WPIACTL_EMUSW3;
	wpiactl |= WPIACTL_EMUSW2 | WPIACTL_EMUSW1 | WPIACTL_EMUSW0;

	bfinplus_set_r0(target, wpiactl);
	bfinplus_emuir_set(target, blackfin_gen_store32_offset(REG_P0, 0, REG_R0));

	wpdactl = WPDACTL_WPDSRC1_A | WPDACTL_WPDSRC0_A;

	bfinplus_set_r0(target, wpdactl);
	bfinplus_emuir_set(target, blackfin_gen_store32_offset(REG_P0, WPDACTL - WPIACTL, REG_R0));

	bfinplus_set_r0(target, 0);
	bfinplus_emuir_set(target, blackfin_gen_store32_offset(REG_P0, WPSTAT - WPIACTL, REG_R0));

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
	struct bfinplus_dap *dap = &(bfin->dap);

	bfinplus_coreregister_set(target, REG_P0, WPIACTL);
	if (enable)
	{
		dap->wpiactl += wpiaen[n];
		bfinplus_coreregister_set(target, REG_R0, addr);
		bfinplus_emuir_set(target,
			blackfin_gen_store32_offset(REG_P0, WPIA0 + 4 * n - WPIACTL, REG_R0));
	}
	else
	{
		dap->wpiactl &= ~wpiaen[n];
	}

	bfinplus_coreregister_set(target, REG_R0, dap->wpiactl);
	bfinplus_emuir_set(target,
		blackfin_gen_store32_offset(REG_P0, 0, REG_R0));

	bfinplus_set_p0(target, p0);
	bfinplus_set_r0(target, r0);
}

void bfinplus_wpu_set_wpda(struct target *target, int n)
{
	uint32_t p0, r0;
	struct bfinplus_common *bfin = target_to_bfinplus(target);
	struct bfinplus_dap *dap = &(bfin->dap);

	uint32_t addr = dap->hwwps[n].addr;
	uint32_t len = dap->hwwps[n].len;
	int mode = dap->hwwps[n].mode;
	bool range = dap->hwwps[n].range;

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
				blackfin_gen_store32_offset(REG_P0, WPDA0 - WPDACTL, REG_R0));
			bfinplus_coreregister_set(target, REG_R0, addr + len - 1);
			bfinplus_emuir_set(target,
				blackfin_gen_store32_offset(REG_P0, WPDA0 + 4 - WPDACTL, REG_R0));
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
				blackfin_gen_store32_offset(REG_P0, WPDA0 + 4 * n - WPDACTL, REG_R0));
        }
    }

	bfinplus_coreregister_set(target, REG_R0, dap->wpdactl);
	bfinplus_emuir_set(target,
		blackfin_gen_store32_offset(REG_P0, 0, REG_R0));

	bfinplus_set_p0(target, p0);
	bfinplus_set_r0(target, r0);
}

void bfinplus_core_reset(struct target *target)
{
	uint32_t val;
	bfinplus_mmr_set32(target, 0x2000001C, 0x04); //RCU0_BCODE.HALT = 1

	bfinplus_mmr_set_indirect(target, BFINPLUS_L1IM_ICTL, 0x207);
	bfinplus_mmr_set_indirect(target, 0x20000000, 0x01); //RCU0_CTL.SYSRST = 1

	bfinplus_mmr_get32(target, 0x20000004, &val); //read RCU0_STAT
	if(val != 0x00002129){
		LOG_ERROR("RCU0_STAT not right");
	}

	bfinplus_mmr_set32(target, 0x20000064, 0x0); //RCU0_MSG_SET = 0
	bfinplus_mmr_set32(target, 0x20000068, 0x0); //RCU0_MSG_CLR = 0

	bfinplus_mmr_set32(target, 0x20048030, 0x00003010); //TAPC0_DBGCTL

	bfinplus_mmr_get32(target, 0x20000060, &val); //read RCU0_MSG
	if(val != 0x00410000){
		LOG_ERROR("RCU0_MSG not right");
	}
}

void bfinplus_system_reset(struct target *target)
{
	bfinplus_mmr_set_indirect(target, BFINPLUS_L1IM_ICTL, 0x207);

	bfinplus_cti_register_set(target, BFINPLUS_PROCCTI_BASE, CTILOCKACCESS_OFFSET, 0xC5ACCE55);
	bfinplus_cti_register_set(target, BFINPLUS_SYSCTI_BASE, CTILOCKACCESS_OFFSET, 0xC5ACCE55);
	bfinplus_cti_register_set(target, BFINPLUS_PROCCTI_BASE, CTILOCKACCESS_OFFSET, 0xC5ACCE55);
	bfinplus_cti_register_set(target, BFINPLUS_PROCCTI_BASE, CTICONTROL_OFFSET, 0x01);
	bfinplus_cti_register_set(target, BFINPLUS_SYSCTI_BASE, CTICONTROL_OFFSET, 0x01);

	//TODO: we can't hardcode this
	bfinplus_mmr_set32(target, 0x2000200C, 0x42042442); //CGU0_DIV
	bfinplus_mmr_set32(target, 0x20002000, 0x00002000); //CGU0_CTL

	bfinplus_mmr_set_indirect(target, BFINPLUS_L1IM_ICTL, 0x207);
	bfinplus_mmr_set_indirect(target, BFINPLUS_L1IM_ICTL, 0x000);

	bfinplus_mmr_set32(target, 0x200D007F, 0x03000000); //USB0_SOFT_RST
	bfinplus_mmr_set32(target, 0x20063020, 0x00000000); //EPPI0_CTL 
}
