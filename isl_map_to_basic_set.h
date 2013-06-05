#ifndef ISL_HMAP_MAP_BASIC_SET_H
#define ISL_HMAP_MAP_BASIC_SET_H

#include <isl/ctx.h>
#include <isl/hash.h>
#include <isl/map.h>
#include <isl/set.h>

struct isl_map_to_basic_set {
	isl_ctx *ctx;
	struct isl_hash_table table;
};
typedef struct isl_map_to_basic_set	isl_map_to_basic_set;

__isl_give isl_map_to_basic_set *isl_map_to_basic_set_alloc( isl_ctx *ctx,
	int min_size);
void *isl_map_to_basic_set_free(__isl_take isl_map_to_basic_set *map2bset);

isl_ctx *isl_map_to_basic_set_get_ctx(__isl_keep isl_map_to_basic_set *hmap);

int isl_map_to_basic_set_has(__isl_keep isl_map_to_basic_set *map2bset,
	__isl_keep isl_map *key);
__isl_give isl_basic_set *isl_map_to_basic_set_get(
	__isl_keep isl_map_to_basic_set *map2bset, __isl_take isl_map *key);
int isl_map_to_basic_set_set(__isl_keep isl_map_to_basic_set *map2bset,
	__isl_take isl_map *key, __isl_take isl_basic_set *val);

int isl_map_to_basic_set_foreach(__isl_keep isl_map_to_basic_set *hmap,
	int (*fn)(__isl_take isl_map *key, __isl_take isl_basic_set *val,
		    void *user), void *user);

#endif
