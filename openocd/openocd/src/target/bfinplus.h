#ifndef BFIN_PLUS_H
#define BFIN_PLUS_H

#include "target.h"
#include "bfinplus_mem.h"
#include "bfinplus_dap.h"

#define BFINPLUSNUMCOREREGS 53
#define BFINPLUS_COMMON_MAGIC 0x13751375
#define BFINPLUS_PART_MAXLEN 20

struct bfinplus_common {
	uint32_t common_magic
	struct bfinplus_dap dap;
	const struct blackfin_mem_map *mem_map;
	const struct blackfin_l1_map *l1_map;

	struct reg_cache *core_cache;

	unsigned int is_locked:1;
	unsigned int is_running:1;
	unsigned int is_interrupted:1;
	unsigned int is_stepping:1;
	unsigned int is_corefault:1;

	unsigned int leave_stopped:1;
	unsigned int status_pending_p:1;
	unsigned int pending_is_breakpoint:1;

	unsigned int l1_data_a_cache_enabled:1;
	unsigned int l1_data_b_cache_enabled:1;
	unsigned int l1_code_cache_enabled:1;
};

static inline struct bfinplus_common *
target_to_bfinplus(struct target *target)
{
	return (struct bfinplus_common *)target->arch_info;
}

#endif /* BFIN_PLUS_H */