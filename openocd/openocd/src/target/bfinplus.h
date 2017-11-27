#ifndef BFIN_PLUS_H
#define BFIN_PLUS_H

#include "target.h"
#include "bfinplus_mem.h"
#include "bfinplus_dap.h"

#define BFINPLUSNUMCOREREGS 53
#define BFINPLUS_COMMON_MAGIC 0x13751375
#define BFINPLUS_PART_MAXLEN 20

struct bfinplus_common {
	uint32_t common_magic;
	char part[BFINPLUS_PART_MAXLEN];
	struct arm_jtag jtag_info;
	struct bfinplus_dap dap;
	const struct blackfin_mem_map *mem_map;
	const struct blackfin_l1_map *l1_map;
	const struct blackfin_sdram_config *sdram_config;
	const struct blackfin_ddr_config *ddr_config;

	struct reg_cache *core_cache;

	unsigned int is_locked:1;
	unsigned int is_running:1;
	unsigned int is_interrupted:1;
	unsigned int is_stepping:1;
	unsigned int is_corefault:1;

  	unsigned int dmem_control_valid_p:1;
  	unsigned int imem_control_valid_p:1;

	unsigned int leave_stopped:1;

	unsigned int l1_data_a_cache_enabled:1;
	unsigned int l1_data_b_cache_enabled:1;
	unsigned int l1_code_cache_enabled:1;

	uint32_t dmem_control;
	uint32_t imem_control;
};

static inline struct bfinplus_common *
target_to_bfinplus(struct target *target)
{
	return (struct bfinplus_common *)target->arch_info;
}

extern int bfinplus_wait_clocks;

#endif /* BFIN_PLUS_H */