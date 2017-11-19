#ifndef BFINPLUS_DAP_H
#define BFINPLUS_DAP_H

#define BFIN_PLUS_AHB_AP 0
#define BFIN_PLUS_APB_AP 1
#define BFIN_PLUS_JTAG_AP 2

struct bfinplus_dap
{
	struct target *target;
	struct adiv5_dap dap;

	uint64_t emuir_a;
    uint64_t emuir_b;
    uint64_t emudat_out;
    uint64_t emudat_in;

	uint32_t wpiactl;
	uint32_t wpdactl;
	uint32_t wpstat;
}

extern int bfinplus_debug_register_set(struct target *, uint8_t, uint32_t);
extern int bfinplus_debug_register_get(struct target *, uint8_t, uint32_t *);

extern int bfinplus_coreregister_set(struct target *, enum core_regnum, uint32_t);
extern int bfinplus_coreregister_get(struct target *, enum core_regnum, uint32_t *);

extern int bfinplus_mmr_set_indirect(struct target *, uint32_t, uint32_t);
extern int bfinplus_mmr_get_indirect(struct target *, uint32_t, uint32_t *);

extern int bfinplus_mmr_set32(struct target *, uint32_t, uint32_t);
extern int bfinplus_mmr_get32(struct target *, uint32_t, uint32_t *);

#endif /* BFIN_PLUS_DAP_H */