#ifndef BFINPLUS_DAP_H
#define BFINPLUS_DAP_H

#include "arm_adi_v5.h"
#include "target.h"
#include "blackfin_insn.h"

#define WPDA_DISABLE					0
#define WPDA_WRITE						1
#define WPDA_READ						2
#define WPDA_ALL						3

#define BFIN_PLUS_AHB_AP 0
#define BFIN_PLUS_APB_AP 1
#define BFIN_PLUS_JTAG_AP 2

#define BFINPLUS_IDCODE			0x02
#define BFINPLUS_EMUIR			0x08
#define BFINPLUS_EMUDAT			0x0C
#define BFINPLUS_DSCR			0x18

//spooO000oky mystery regs
#define BFINPLUS_DBG_MYSTERY0	0x00
#define BFINPLUS_DBG_MYSTERY1C	0x1C

#define BFINPLUS_TRU0_GCTL 0x200017F4
#define BFINPLUS_TRU0_SSR69 0x20001114
#define BFINPLUS_WPIACTL 0x1FC07000

#define BFINPLUS_SYSCTI_SYS_DBGRESTART 0x07
#define BFINPLUS_PROCCTI_DBGRESTART 0x07

#define BFINPLUS_PROCCTI_BASE 0x20013000
#define BFINPLUS_SYSCTI_BASE 0x2001A000

#define CTICONTROL_OFFSET 0x00
#define CTILOCK_OFFSET 0x08
#define CTIAPPPLUSE_OFFSET 0x1C
#define CTIINEN_OFFSET 0x20
#define CTIOUTEN_OFFSET 0xA0
#define CTILOCKACCESS_OFFSET 0xFB0

struct bfinplus_hwwpt
{
	uint32_t addr;
	uint32_t len;
	int mode;
	/* If range is true, this hardware watchpoint is combined with the
	   next watchpoint to form a range watchpoint.  */
	bool range;
	/* True if this hardware watchpoint has been used, otherwise false.  */
	bool used;
};

#define BFINPLUS_MAX_HWBREAKPOINTS		6
#define BFINPLUS_MAX_HWWATCHPOINTS		2

struct bfinplus_cti
{
	uint32_t ctiinen[8];
	uint32_t ctiouten[8];
}

struct bfinplus_dap
{
	struct target *target;
	struct adiv5_dap dap;
	struct bfinplus_cti syscti;
	struct bfinplus_cti proccti;

	uint32_t wpiactl;
	uint32_t wpdactl;
	uint32_t wpstat;

	uint32_t hwbps[BLACKFIN_MAX_HWBREAKPOINTS];
	struct blackfin_hwwpt hwwps[BLACKFIN_MAX_HWWATCHPOINTS];
}

extern int bfinplus_debug_register_set(struct target *, uint8_t, uint32_t);
extern int bfinplus_debug_register_get(struct target *, uint8_t, uint32_t *);

extern int bfinplus_coreregister_set(struct target *, enum core_regnum, uint32_t);
extern int bfinplus_coreregister_get(struct target *, enum core_regnum, uint32_t *);

extern int bfinplus_mmr_set_indirect(struct target *, uint32_t, uint32_t);
extern int bfinplus_mmr_get_indirect(struct target *, uint32_t, uint32_t *);

extern int bfinplus_mmr_set32(struct target *, uint32_t, uint32_t);
extern int bfinplus_mmr_get32(struct target *, uint32_t, uint32_t *);

extern int bfinplus_get_cti(struct target *);
extern int bfinplus_pulse_cti(struct target *);
extern int bfinplus_set_used_ctis(struct target *, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);

extern int bfinplus_read_mem(struct target *, uint32_t, uint32_t, uint32_t, uint8_t *);
extern int bfinplus_write_mem(struct target *, uint32_t, uint32_t, uint32_t, const uint8_t *);

extern void bfinplus_wpu_init(struct target *);
extern void bfinplus_wpu_set_wpia(struct target *, int, uint32_t, int);
extern void bfinplus_wpu_set_wpda(struct target *, int);

extern void bfinplus_core_reset(struct target *);
extern void bfinplus_system_reset(struct target *);

#endif /* BFIN_PLUS_DAP_H */