#ifndef BLACKFIN_MEMORY_MAP_H
#define BLACKFIN_MEMORY_MAP_H

extern int parse_memory_map(struct target *target, const char *memory_map);
extern int parse_memory_map_bfinplus(struct target *target, const char *memory_map);

#endif
