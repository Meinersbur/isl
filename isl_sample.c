#include "isl_sample.h"
#include "isl_sample_piplib.h"
#include "isl_vec.h"

struct isl_vec *isl_basic_set_sample(struct isl_ctx *ctx,
	struct isl_basic_set *bset)
{
	if (F_ISSET(bset, ISL_BASIC_SET_EMPTY)) {
		isl_basic_set_free(ctx, bset);
		return isl_vec_alloc(ctx, 0);
	}
	return isl_pip_basic_set_sample(ctx, bset);
}
