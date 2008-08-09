#include "isl_map_piplib.h"

struct isl_map *isl_pip_basic_map_lexmax(struct isl_ctx *ctx,
		struct isl_basic_map *bmap, struct isl_basic_set *dom,
		struct isl_set **empty)
{
	isl_basic_map_free(ctx, bmap);
	isl_basic_set_free(ctx, dom);
	return NULL;
}

struct isl_map *isl_pip_basic_map_lexmin(struct isl_ctx *ctx,
		struct isl_basic_map *bmap, struct isl_basic_set *dom,
		struct isl_set **empty)
{
	isl_basic_map_free(ctx, bmap);
	isl_basic_set_free(ctx, dom);
	return NULL;
}

struct isl_map *isl_pip_basic_map_compute_divs(struct isl_ctx *ctx,
		struct isl_basic_map *bmap)
{
	isl_basic_map_free(ctx, bmap);
	return NULL;
}
