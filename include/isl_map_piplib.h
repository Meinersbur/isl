#ifndef ISL_MAP_PIPLIB_H
#define ISL_MAP_PIPLIB_H

#include <isl_map.h>
#include <isl_piplib.h>

#if defined(__cplusplus)
extern "C" {
#endif

PipMatrix *isl_basic_map_to_pip(struct isl_basic_map *bmap, unsigned pip_param,
			 unsigned extra_front, unsigned extra_back);

struct isl_map *isl_map_from_quast(struct isl_ctx *ctx, PipQuast *q,
		unsigned keep,
		struct isl_basic_set *context,
		struct isl_set **rest);
struct isl_map *pip_isl_basic_map_lexmax(struct isl_ctx *ctx,
		struct isl_basic_map *bmap, struct isl_basic_set *dom,
		struct isl_set **empty);
struct isl_map *pip_isl_basic_map_lexmin(struct isl_ctx *ctx,
		struct isl_basic_map *bmap, struct isl_basic_set *dom,
		struct isl_set **empty);
struct isl_map *pip_isl_basic_map_compute_divs(struct isl_ctx *ctx,
		struct isl_basic_map *bmap);

#if defined(__cplusplus)
}
#endif

#endif
