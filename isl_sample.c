#include "isl_sample.h"
#include "isl_sample_piplib.h"
#include "isl_vec.h"
#include "isl_mat.h"
#include "isl_map_private.h"
#include "isl_equalities.h"

struct isl_vec *isl_basic_set_sample(struct isl_ctx *ctx,
	struct isl_basic_set *bset)
{
	if (!bset)
		return NULL;

	if (F_ISSET(bset, ISL_BASIC_SET_EMPTY)) {
		isl_basic_set_free(ctx, bset);
		return isl_vec_alloc(ctx, 0);
	}

	isl_assert(ctx, bset->nparam == 0, goto error);
	isl_assert(ctx, bset->n_div == 0, goto error);

	if (bset->n_eq > 0) {
		struct isl_mat *T;
		struct isl_vec *sample;

		bset = isl_basic_set_remove_equalities(ctx, bset, &T, NULL);
		sample = isl_basic_set_sample(ctx, bset);
		if (sample && sample->size != 0)
			sample = isl_mat_vec_product(ctx, T, sample);
		else
			isl_mat_free(ctx, T);
		return sample;
	}
	return isl_pip_basic_set_sample(ctx, bset);
error:
	isl_basic_set_free(ctx, bset);
	return NULL;
}
