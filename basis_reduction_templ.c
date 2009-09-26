#include <stdlib.h>
#include "isl_basis_reduction.h"

static void save_alpha(GBR_LP *lp, int first, int n, GBR_type *alpha)
{
	int i;

	for (i = 0; i < n; ++i)
		GBR_lp_get_alpha(lp, first + i, &alpha[i]);
}

/* This function implements the algorithm described in
 * "An Implementation of the Generalized Basis Reduction Algorithm
 *  for Integer Programming" of Cook el al. to compute a reduced basis.
 * We use \epsilon = 1/4.
 *
 * If ctx->gbr_only_first is set, the user is only interested
 * in the first direction.  In this case we stop the basis reduction when
 * the width in the first direction becomes smaller than 2.
 */
struct isl_mat *isl_tab_reduced_basis(struct isl_tab *tab)
{
	unsigned dim;
	struct isl_ctx *ctx;
	struct isl_mat *basis;
	int unbounded;
	int i;
	GBR_LP *lp = NULL;
	GBR_type F_old, alpha, F_new;
	int row;
	isl_int tmp;
	struct isl_vec *b_tmp;
	GBR_type *F = NULL;
	GBR_type *alpha_buffer[2] = { NULL, NULL };
	GBR_type *alpha_saved;
	GBR_type F_saved;
	int use_saved = 0;
	isl_int mu[2];
	GBR_type mu_F[2];
	GBR_type two;
	GBR_type one;
	int n_zero = 0;
	int empty = 0;
	int fixed = 0;
	int fixed_saved = 0;
	int mu_fixed[2];

	if (!tab)
		return NULL;

	ctx = tab->mat->ctx;
	dim = tab->n_var;
	basis = isl_mat_identity(ctx, dim);
	if (!basis)
		return NULL;

	if (dim == 1)
		return basis;

	isl_int_init(tmp);
	isl_int_init(mu[0]);
	isl_int_init(mu[1]);

	GBR_init(alpha);
	GBR_init(F_old);
	GBR_init(F_new);
	GBR_init(F_saved);
	GBR_init(mu_F[0]);
	GBR_init(mu_F[1]);
	GBR_init(two);
	GBR_init(one);

	b_tmp = isl_vec_alloc(ctx, dim);
	if (!b_tmp)
		goto error;

	F = isl_alloc_array(ctx, GBR_type, dim);
	alpha_buffer[0] = isl_alloc_array(ctx, GBR_type, dim);
	alpha_buffer[1] = isl_alloc_array(ctx, GBR_type, dim);
	alpha_saved = alpha_buffer[0];

	if (!F || !alpha_buffer[0] || !alpha_buffer[1])
		goto error;

	for (i = 0; i < dim; ++i) {
		GBR_init(F[i]);
		GBR_init(alpha_buffer[0][i]);
		GBR_init(alpha_buffer[1][i]);
	}

	GBR_set_ui(two, 2);
	GBR_set_ui(one, 1);

	lp = GBR_lp_init(tab);
	if (!lp)
		goto error;

	i = 0;

	GBR_lp_set_obj(lp, basis->row[0], dim);
	ctx->stats->gbr_solved_lps++;
	unbounded = GBR_lp_solve(lp);
	isl_assert(ctx, !unbounded, goto error);
	GBR_lp_get_obj_val(lp, &F[0]);

	if (GBR_lt(F[0], one)) {
		if (!GBR_is_zero(F[0])) {
			empty = GBR_lp_cut(lp, basis->row[0]);
			if (empty)
				goto done;
			GBR_set_ui(F[0], 0);
		}
		n_zero++;
	}

	do {
		if (i+1 == n_zero) {
			GBR_lp_set_obj(lp, basis->row[i+1], dim);
			ctx->stats->gbr_solved_lps++;
			unbounded = GBR_lp_solve(lp);
			isl_assert(ctx, !unbounded, goto error);
			GBR_lp_get_obj_val(lp, &F_new);
			fixed = GBR_lp_is_fixed(lp);
			GBR_set_ui(alpha, 0);
		} else
		if (use_saved) {
			row = GBR_lp_next_row(lp);
			GBR_set(F_new, F_saved);
			fixed = fixed_saved;
			GBR_set(alpha, alpha_saved[i]);
		} else {
			row = GBR_lp_add_row(lp, basis->row[i], dim);
			GBR_lp_set_obj(lp, basis->row[i+1], dim);
			ctx->stats->gbr_solved_lps++;
			unbounded = GBR_lp_solve(lp);
			isl_assert(ctx, !unbounded, goto error);
			GBR_lp_get_obj_val(lp, &F_new);
			fixed = GBR_lp_is_fixed(lp);

			GBR_lp_get_alpha(lp, row, &alpha);

			if (i > 0)
				save_alpha(lp, row-i, i, alpha_saved);

			GBR_lp_del_row(lp);
		}
		GBR_set(F[i+1], F_new);

		GBR_floor(mu[0], alpha);
		GBR_ceil(mu[1], alpha);

		if (isl_int_eq(mu[0], mu[1]))
			isl_int_set(tmp, mu[0]);
		else {
			int j;

			for (j = 0; j <= 1; ++j) {
				isl_int_set(tmp, mu[j]);
				isl_seq_combine(b_tmp->el,
						ctx->one, basis->row[i+1],
						tmp, basis->row[i], dim);
				GBR_lp_set_obj(lp, b_tmp->el, dim);
				ctx->stats->gbr_solved_lps++;
				unbounded = GBR_lp_solve(lp);
				isl_assert(ctx, !unbounded, goto error);
				GBR_lp_get_obj_val(lp, &mu_F[j]);
				mu_fixed[j] = GBR_lp_is_fixed(lp);
				if (i > 0)
					save_alpha(lp, row-i, i, alpha_buffer[j]);
			}

			if (GBR_lt(mu_F[0], mu_F[1]))
				j = 0;
			else
				j = 1;

			isl_int_set(tmp, mu[j]);
			GBR_set(F_new, mu_F[j]);
			fixed = mu_fixed[j];
			alpha_saved = alpha_buffer[j];
		}
		isl_seq_combine(basis->row[i+1], ctx->one, basis->row[i+1],
				tmp, basis->row[i], dim);

		if (i+1 == n_zero && fixed) {
			if (!GBR_is_zero(F[i+1])) {
				empty = GBR_lp_cut(lp, basis->row[i+1]);
				if (empty)
					goto done;
				GBR_set_ui(F[i+1], 0);
			}
			n_zero++;
		}

		GBR_set(F_old, F[i]);

		use_saved = 0;
		/* mu_F[0] = 4 * F_new; mu_F[1] = 3 * F_old */
		GBR_set_ui(mu_F[0], 4);
		GBR_mul(mu_F[0], mu_F[0], F_new);
		GBR_set_ui(mu_F[1], 3);
		GBR_mul(mu_F[1], mu_F[1], F_old);
		if (GBR_lt(mu_F[0], mu_F[1])) {
			basis = isl_mat_swap_rows(basis, i, i + 1);
			if (i > 0) {
				use_saved = 1;
				GBR_set(F_saved, F_new);
				fixed_saved = fixed;
				GBR_lp_del_row(lp);
				--i;
			} else {
				GBR_set(F[0], F_new);
				if (ctx->gbr_only_first && GBR_lt(F[0], two))
					break;

				if (fixed) {
					if (!GBR_is_zero(F[0])) {
						empty = GBR_lp_cut(lp, basis->row[0]);
						if (empty)
							goto done;
						GBR_set_ui(F[0], 0);
					}
					n_zero++;
				}
			}
		} else {
			GBR_lp_add_row(lp, basis->row[i], dim);
			++i;
		}
	} while (i < dim-1);

	if (0) {
done:
		if (empty < 0) {
error:
			isl_mat_free(basis);
			basis = NULL;
		}
	}

	GBR_lp_delete(lp);

	if (alpha_buffer[1])
		for (i = 0; i < dim; ++i) {
			GBR_clear(F[i]);
			GBR_clear(alpha_buffer[0][i]);
			GBR_clear(alpha_buffer[1][i]);
		}
	free(F);
	free(alpha_buffer[0]);
	free(alpha_buffer[1]);

	isl_vec_free(b_tmp);

	GBR_clear(alpha);
	GBR_clear(F_old);
	GBR_clear(F_new);
	GBR_clear(F_saved);
	GBR_clear(mu_F[0]);
	GBR_clear(mu_F[1]);
	GBR_clear(two);
	GBR_clear(one);

	isl_int_clear(tmp);
	isl_int_clear(mu[0]);
	isl_int_clear(mu[1]);

	return basis;
}

struct isl_mat *isl_basic_set_reduced_basis(struct isl_basic_set *bset)
{
	struct isl_mat *basis;
	struct isl_tab *tab;

	isl_assert(bset->ctx, bset->n_eq == 0, return NULL);

	tab = isl_tab_from_basic_set(bset);
	basis = isl_tab_reduced_basis(tab);

	isl_tab_free(tab);

	return basis;
}
