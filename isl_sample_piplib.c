#include "isl_mat.h"
#include "isl_vec.h"
#include "isl_seq.h"
#include "isl_piplib.h"
#include "isl_sample_piplib.h"

static void swap_inequality(struct isl_basic_set *bset, int a, int b)
{
	isl_int *t = bset->ineq[a];
	bset->ineq[a] = bset->ineq[b];
	bset->ineq[b] = t;
}

static struct isl_mat *independent_bounds(struct isl_ctx *ctx,
	struct isl_basic_set *bset)
{
	int i, j, n;
	struct isl_mat *dirs = NULL;
	struct isl_mat *bounds = NULL;
	unsigned dim;

	if (!bset)
		return NULL;

	dim = isl_basic_set_n_dim(bset);
	bounds = isl_mat_alloc(ctx, 1+dim, 1+dim);
	if (!bounds)
		return NULL;

	isl_int_set_si(bounds->row[0][0], 1);
	isl_seq_clr(bounds->row[0]+1, dim);
	bounds->n_row = 1;

	if (bset->n_ineq == 0)
		return bounds;

	dirs = isl_mat_alloc(ctx, dim, dim);
	if (!dirs) {
		isl_mat_free(ctx, bounds);
		return NULL;
	}
	isl_seq_cpy(dirs->row[0], bset->ineq[0]+1, dirs->n_col);
	isl_seq_cpy(bounds->row[1], bset->ineq[0], bounds->n_col);
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
		isl_seq_cpy(bounds->row[n], bset->ineq[j], bounds->n_col);
	}
	isl_mat_free(ctx, dirs);
	bounds->n_row = 1+n;
	return bounds;
}

/* Skew into positive orthant and project out lineality space */
static struct isl_basic_set *isl_basic_set_skew_to_positive_orthant(
	struct isl_basic_set *bset, struct isl_mat **T)
{
	struct isl_mat *U = NULL;
	struct isl_mat *bounds = NULL;
	int i, j;
	unsigned old_dim, new_dim;
	struct isl_ctx *ctx;

	*T = NULL;
	if (!bset)
		return NULL;

	ctx = bset->ctx;
	isl_assert(ctx, isl_basic_set_n_param(bset) == 0, goto error);
	isl_assert(ctx, bset->n_div == 0, goto error);
	isl_assert(ctx, bset->n_eq == 0, goto error);
	
	old_dim = isl_basic_set_n_dim(bset);
	/* Try to move (multiples of) unit rows up. */
	for (i = 0, j = 0; i < bset->n_ineq; ++i) {
		int pos = isl_seq_first_non_zero(bset->ineq[i]+1, old_dim);
		if (pos < 0)
			continue;
		if (isl_seq_first_non_zero(bset->ineq[i]+1+pos+1,
						old_dim-pos-1) >= 0)
			continue;
		if (i != j)
			swap_inequality(bset, i, j);
		++j;
	}
	bounds = independent_bounds(ctx, bset);
	if (!bounds)
		goto error;
	new_dim = bounds->n_row - 1;
	bounds = isl_mat_left_hermite(ctx, bounds, 1, &U, NULL);
	if (!bounds)
		goto error;
	U = isl_mat_drop_cols(ctx, U, 1 + new_dim, old_dim - new_dim);
	bset = isl_basic_set_preimage(bset, isl_mat_copy(ctx, U));
	if (!bset)
		goto error;
	*T = U;
	isl_mat_free(ctx, bounds);
	return bset;
error:
	isl_mat_free(ctx, bounds);
	isl_mat_free(ctx, U);
	isl_basic_set_free(bset);
	return NULL;
}

struct isl_vec *isl_pip_basic_set_sample(struct isl_basic_set *bset)
{
	PipOptions	*options = NULL;
	PipMatrix	*domain = NULL;
	PipQuast	*sol = NULL;
	struct isl_vec *vec = NULL;
	unsigned	dim;
	struct isl_mat *T;
	struct isl_ctx *ctx;

	if (!bset)
		goto error;
	ctx = bset->ctx;
	isl_assert(ctx, isl_basic_set_n_param(bset) == 0, goto error);
	isl_assert(ctx, bset->n_div == 0, goto error);
	bset = isl_basic_set_skew_to_positive_orthant(bset, &T);
	if (!bset)
		goto error;
	dim = isl_basic_set_n_dim(bset);
	domain = isl_basic_map_to_pip((struct isl_basic_map *)bset, 0, 0, 0);
	if (!domain)
		goto error;

	options = pip_options_init();
	if (!options)
		goto error;
	sol = pip_solve(domain, NULL, -1, options);
	if (!sol)
		goto error;
	if (!sol->list) {
		vec = isl_vec_alloc(ctx, 0);
		isl_mat_free(ctx, T);
	} else {
		PipList *l;
		int i;
		vec = isl_vec_alloc(ctx, 1 + dim);
		if (!vec)
			goto error;
		isl_int_set_si(vec->block.data[0], 1);
		for (i = 0, l = sol->list; l && i < dim; ++i, l = l->next) {
			isl_seq_cpy_from_pip(&vec->block.data[1+i],
					&l->vector->the_vector[0], 1);
			isl_assert(ctx, !entier_zero_p(l->vector->the_deno[0]),
					goto error);
		}
		isl_assert(ctx, i == dim, goto error);
		vec = isl_mat_vec_product(ctx, T, vec);
	}

	pip_quast_free(sol);
	pip_options_free(options);
	pip_matrix_free(domain);

	isl_basic_set_free(bset);
	return vec;
error:
	isl_vec_free(ctx, vec);
	isl_basic_set_free(bset);
	if (sol)
		pip_quast_free(sol);
	if (domain)
		pip_matrix_free(domain);
	return NULL;
}
