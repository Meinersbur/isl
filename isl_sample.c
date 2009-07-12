#include "isl_sample.h"
#include "isl_sample_piplib.h"
#include "isl_vec.h"
#include "isl_mat.h"
#include "isl_seq.h"
#include "isl_map_private.h"
#include "isl_equalities.h"

static struct isl_vec *empty_sample(struct isl_basic_set *bset)
{
	struct isl_vec *vec;

	vec = isl_vec_alloc(bset->ctx, 0);
	isl_basic_set_free(bset);
	return vec;
}

/* Construct a zero sample of the same dimension as bset.
 * As a special case, if bset is zero-dimensional, this
 * function creates a zero-dimensional sample point.
 */
static struct isl_vec *zero_sample(struct isl_basic_set *bset)
{
	unsigned dim;
	struct isl_vec *sample;

	dim = isl_basic_set_total_dim(bset);
	sample = isl_vec_alloc(bset->ctx, 1 + dim);
	if (sample) {
		isl_int_set_si(sample->el[0], 1);
		isl_seq_clr(sample->el + 1, dim);
	}
	isl_basic_set_free(bset);
	return sample;
}

static struct isl_vec *interval_sample(struct isl_ctx *ctx,
	struct isl_basic_set *bset)
{
	int i;
	isl_int t;
	struct isl_vec *sample;

	bset = isl_basic_set_simplify(bset);
	if (!bset)
		return NULL;
	if (isl_basic_set_fast_is_empty(bset))
		return empty_sample(bset);
	if (bset->n_eq == 0 && bset->n_ineq == 0)
		return zero_sample(bset);

	sample = isl_vec_alloc(ctx, 2);
	isl_int_set_si(sample->block.data[0], 1);

	if (bset->n_eq > 0) {
		isl_assert(bset->ctx, bset->n_eq == 1, goto error);
		isl_assert(bset->ctx, bset->n_ineq == 0, goto error);
		if (isl_int_is_one(bset->eq[0][1]))
			isl_int_neg(sample->el[1], bset->eq[0][0]);
		else {
			isl_assert(bset->ctx, isl_int_is_negone(bset->eq[0][1]),
				   goto error);
			isl_int_set(sample->el[1], bset->eq[0][0]);
		}
		isl_basic_set_free(bset);
		return sample;
	}

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
		isl_vec_free(sample);
		return empty_sample(bset);
	}

	isl_basic_set_free(bset);
	return sample;
error:
	isl_basic_set_free(bset);
	isl_vec_free(sample);
	return NULL;
}

static struct isl_mat *independent_bounds(struct isl_ctx *ctx,
	struct isl_basic_set *bset)
{
	int i, j, n;
	struct isl_mat *dirs = NULL;
	unsigned dim;

	if (!bset)
		return NULL;

	dim = isl_basic_set_n_dim(bset);
	if (bset->n_ineq == 0)
		return isl_mat_alloc(ctx, 0, dim);

	dirs = isl_mat_alloc(ctx, dim, dim);
	if (!dirs)
		return NULL;
	isl_seq_cpy(dirs->row[0], bset->ineq[0]+1, dirs->n_col);
	for (j = 1, n = 1; n < dim && j < bset->n_ineq; ++j) {
		int pos;

		isl_seq_cpy(dirs->row[n], bset->ineq[j]+1, dirs->n_col);

		pos = isl_seq_first_non_zero(dirs->row[n], dirs->n_col);
		if (pos < 0)
			continue;
		for (i = 0; i < n; ++i) {
			int pos_i;
			pos_i = isl_seq_first_non_zero(dirs->row[i], dirs->n_col);
			if (pos_i < pos)
				continue;
			if (pos_i > pos)
				break;
			isl_seq_elim(dirs->row[n], dirs->row[i], pos,
					dirs->n_col, NULL);
			pos = isl_seq_first_non_zero(dirs->row[n], dirs->n_col);
			if (pos < 0)
				break;
		}
		if (pos < 0)
			continue;
		if (i < n) {
			int k;
			isl_int *t = dirs->row[n];
			for (k = n; k > i; --k)
				dirs->row[k] = dirs->row[k-1];
			dirs->row[i] = t;
		}
		++n;
	}
	dirs->n_row = n;
	return dirs;
}

/* Find a sample integer point, if any, in bset, which is known
 * to have equalities.  If bset contains no integer points, then
 * return a zero-length vector.
 * We simply remove the known equalities, compute a sample
 * in the resulting bset, using the specified recurse function,
 * and then transform the sample back to the original space.
 */
static struct isl_vec *sample_eq(struct isl_basic_set *bset,
	struct isl_vec *(*recurse)(struct isl_basic_set *))
{
	struct isl_mat *T;
	struct isl_vec *sample;
	struct isl_ctx *ctx;

	if (!bset)
		return NULL;

	ctx = bset->ctx;
	bset = isl_basic_set_remove_equalities(bset, &T, NULL);
	sample = recurse(bset);
	if (!sample || sample->size == 0)
		isl_mat_free(ctx, T);
	else
		sample = isl_mat_vec_product(ctx, T, sample);
	return sample;
}

static struct isl_vec *sample_no_lineality(struct isl_basic_set *bset)
{
	unsigned dim;

	if (isl_basic_set_fast_is_empty(bset))
		return empty_sample(bset);
	if (bset->n_eq > 0)
		return sample_eq(bset, sample_no_lineality);
	dim = isl_basic_set_total_dim(bset);
	if (dim == 0)
		return zero_sample(bset);
	if (dim == 1)
		return interval_sample(bset->ctx, bset);

	return isl_pip_basic_set_sample(bset);
}

/* Compute an integer point in "bset" with a lineality space that
 * is orthogonal to the constraints in "bounds".
 *
 * We first perform a unimodular transformation on bset that
 * make the constraints in bounds (and therefore all constraints in bset)
 * only involve the first dimensions.  The remaining dimensions
 * then do not appear in any constraints and we can select any value
 * for them, say zero.  We therefore project out this final dimensions
 * and plug in the value zero later.  This is accomplished by simply
 * dropping the final columns of the unimodular transformation.
 */
static struct isl_vec *sample_lineality(struct isl_basic_set *bset,
	struct isl_mat *bounds)
{
	struct isl_mat *U = NULL;
	unsigned old_dim, new_dim;
	struct isl_vec *sample;
	struct isl_ctx *ctx;

	if (!bset || !bounds)
		goto error;

	ctx = bset->ctx;
	old_dim = isl_basic_set_n_dim(bset);
	new_dim = bounds->n_row;
	bounds = isl_mat_left_hermite(ctx, bounds, 0, &U, NULL);
	if (!bounds)
		goto error;
	U = isl_mat_lin_to_aff(ctx, U);
	U = isl_mat_drop_cols(ctx, U, 1 + new_dim, old_dim - new_dim);
	bset = isl_basic_set_preimage(bset, isl_mat_copy(ctx, U));
	if (!bset)
		goto error;
	isl_mat_free(ctx, bounds);

	sample = sample_no_lineality(bset);
	if (sample && sample->size != 0)
		sample = isl_mat_vec_product(ctx, U, sample);
	else
		isl_mat_free(ctx, U);
	return sample;
error:
	isl_mat_free(ctx, bounds);
	isl_mat_free(ctx, U);
	isl_basic_set_free(bset);
	return NULL;
}

struct isl_vec *isl_basic_set_sample(struct isl_basic_set *bset)
{
	struct isl_ctx *ctx;
	struct isl_mat *bounds;
	unsigned dim;
	if (!bset)
		return NULL;

	ctx = bset->ctx;
	if (isl_basic_set_fast_is_empty(bset))
		return empty_sample(bset);

	dim = isl_basic_set_n_dim(bset);
	isl_assert(ctx, isl_basic_set_n_param(bset) == 0, goto error);
	isl_assert(ctx, bset->n_div == 0, goto error);

	if (bset->n_eq > 0)
		return sample_eq(bset, isl_basic_set_sample);
	if (dim == 0)
		return zero_sample(bset);
	if (dim == 1)
		return interval_sample(ctx, bset);
	bounds = independent_bounds(ctx, bset);
	if (!bounds)
		goto error;

	if (bounds->n_row == 0) {
		isl_mat_free(ctx, bounds);
		return zero_sample(bset);
	}
	if (bounds->n_row < dim)
		return sample_lineality(bset, bounds);

	isl_mat_free(ctx, bounds);
	return sample_no_lineality(bset);
error:
	isl_basic_set_free(bset);
	return NULL;
}
