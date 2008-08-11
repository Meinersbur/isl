#include "isl_sample.h"
#include "isl_sample_piplib.h"
#include "isl_vec.h"
#include "isl_mat.h"
#include "isl_map_private.h"
#include "isl_equalities.h"

static struct isl_vec *point_sample(struct isl_ctx *ctx,
	struct isl_basic_set *bset)
{
	struct isl_vec *sample;
	isl_basic_set_free(ctx, bset);
	sample = isl_vec_alloc(ctx, 1);
	if (!sample)
		return NULL;
	isl_int_set_si(sample->block.data[0], 1);
	return sample;
}

static struct isl_vec *interval_sample(struct isl_ctx *ctx,
	struct isl_basic_set *bset)
{
	struct isl_vec *sample;

	bset = isl_basic_set_simplify(ctx, bset);
	if (!bset)
		return NULL;
	if (bset->n_eq > 0)
		return isl_basic_set_sample(ctx, bset);
	sample = isl_vec_alloc(ctx, 2);
	isl_int_set_si(sample->block.data[0], 1);
	if (bset->n_ineq == 0)
		isl_int_set_si(sample->block.data[1], 0);
	else {
		int i;
		isl_int t;
		isl_int_init(t);
		if (isl_int_is_one(bset->ineq[0][1]))
			isl_int_neg(sample->block.data[1], bset->ineq[0][0]);
		else
			isl_int_set(sample->block.data[1], bset->ineq[0][0]);
		for (i = 1; i < bset->n_ineq; ++i) {
			isl_seq_inner_product(sample->block.data,
						bset->ineq[i], 2, &t);
			if (isl_int_is_neg(t))
				break;
		}
		isl_int_clear(t);
		if (i < bset->n_ineq) {
			isl_vec_free(ctx, sample);
			sample = isl_vec_alloc(ctx, 0);
		}
	}
	isl_basic_set_free(ctx, bset);
	return sample;
}

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
	if (bset->dim == 0)
		return point_sample(ctx, bset);
	if (bset->dim == 1)
		return interval_sample(ctx, bset);
	return isl_pip_basic_set_sample(ctx, bset);
error:
	isl_basic_set_free(ctx, bset);
	return NULL;
}
