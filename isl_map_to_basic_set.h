#ifndef ISL_HMAP_MAP_BASIC_SET_H
#define ISL_HMAP_MAP_BASIC_SET_H

#include <isl/hash.h>
#include <isl/map.h>
#include <isl/set.h>

struct isl_map_to_basic_set {
	struct isl_hash_table table;
};
typedef struct isl_map_to_basic_set	isl_map_to_basic_set;

__isl_give isl_map_to_basic_set *isl_map_to_basic_set_alloc( isl_ctx *ctx,
	int min_size);
void *isl_map_to_basic_set_free(isl_ctx *ctx,
	__isl_take isl_map_to_basic_set *map2bset);

int isl_map_to_basic_set_has(isl_ctx *ctx,
	__isl_keep isl_map_to_basic_set *map2bset, __isl_keep isl_map *key);
__isl_give isl_basic_set *isl_map_to_basic_set_get(isl_ctx *ctx,
	__isl_keep isl_map_to_basic_set *map2bset, __isl_take isl_map *key);
int isl_map_to_basic_set_set(isl_ctx *ctx,
	__isl_keep isl_map_to_basic_set *map2bset, __isl_take isl_map *key,
	__isl_take isl_basic_set *val);

#endif
