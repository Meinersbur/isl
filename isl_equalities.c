#include "isl_mat.h"
#include "isl_seq.h"
#include "isl_map_private.h"
#include "isl_equalities.h"

/* Use the n equalities of bset to unimodularly transform the
 * variables x such that n transformed variables x1' have a constant value
 * and rewrite the constraints of bset in terms of the remaining
 * transformed variables x2'.  The matrix pointed to by T maps
 * the new variables x2' back to the original variables x, while T2
 * maps the original variables to the new variables.
 *
 * Let the equalities of bset be
 *
 *		M x - c = 0
 *
 * Compute the (left) Hermite normal form of M,
 *
 *		M [U1 U2] = M U = H = [H1 0]
 * or
 *		              M = H Q = [H1 0] [Q1]
 *                                             [Q2]
 *
 * with U, Q unimodular, Q = U^{-1} (and H lower triangular).
 * Define the transformed variables as
 *
 *		x = [U1 U2] [ x1' ] = [U1 U2] [Q1] x
 *		            [ x2' ]           [Q2]
 *
 * The equalities then become
 *
 *		H1 x1' - c = 0   or   x1' = H1^{-1} c = c'
 *
 * If any of the c' is non-integer, then the original set has no
 * integer solutions (since the x' are a unimodular transformation
 * of the x).
 * Otherwise, the transformation is given by
 *
 *		x = U1 H1^{-1} c + U2 x2'
 *
 * The inverse transformation is simply
 *
 *		x2' = Q2 x
 */
static struct isl_basic_set *compress_variables(struct isl_ctx *ctx,
	struct isl_basic_set *bset, struct isl_mat **T, struct isl_mat **T2)
{
	int i;
	struct isl_mat *H = NULL, *C = NULL, *H1, *U = NULL, *U1, *U2, *TC;

	if (T)
		*T = NULL;
	if (T2)
		*T2 = NULL;
	if (!bset)
		goto error;
	isl_assert(ctx, bset->nparam == 0, goto error);
	isl_assert(ctx, bset->n_div == 0, goto error);
	isl_assert(ctx, bset->n_eq <= bset->dim, goto error);
	if (bset->n_eq == 0)
		return bset;

	H = isl_mat_sub_alloc(ctx, bset->eq, 0, bset->n_eq, 1, bset->dim);
	H = isl_mat_left_hermite(ctx, H, 0, &U, T2);
	if (!H || !U || (T2 && !*T2))
		goto error;
	if (T2) {
		*T2 = isl_mat_drop_rows(ctx, *T2, 0, bset->n_eq);
		*T2 = isl_mat_lin_to_aff(ctx, *T2);
		if (!*T2)
			goto error;
	}
	C = isl_mat_alloc(ctx, 1+bset->n_eq, 1);
	if (!C)
		goto error;
	isl_int_set_si(C->row[0][0], 1);
	isl_mat_sub_neg(ctx, C->row+1, bset->eq, bset->n_eq, 0, 0, 1);
	H1 = isl_mat_sub_alloc(ctx, H->row, 0, H->n_row, 0, H->n_row);
	H1 = isl_mat_lin_to_aff(ctx, H1);
	TC = isl_mat_inverse_product(ctx, H1, C);
	if (!TC)
		goto error;
	isl_mat_free(ctx, H);
	if (!isl_int_is_one(TC->row[0][0])) {
		for (i = 0; i < bset->n_eq; ++i) {
			if (!isl_int_is_divisible_by(TC->row[1+i][0], TC->row[0][0])) {
				isl_mat_free(ctx, TC);
				isl_mat_free(ctx, U);
				if (T2) {
					isl_mat_free(ctx, *T2);
					*T2 = NULL;
				}
				return isl_basic_set_set_to_empty(bset);
			}
			isl_seq_scale_down(TC->row[1+i], TC->row[1+i], TC->row[0][0], 1);
		}
		isl_int_set_si(TC->row[0][0], 1);
	}
	U1 = isl_mat_sub_alloc(ctx, U->row, 0, U->n_row, 0, bset->n_eq);
	U1 = isl_mat_lin_to_aff(ctx, U1);
	U2 = isl_mat_sub_alloc(ctx, U->row, 0, U->n_row,
				bset->n_eq, U->n_row - bset->n_eq);
	U2 = isl_mat_lin_to_aff(ctx, U2);
	isl_mat_free(ctx, U);
	TC = isl_mat_product(ctx, U1, TC);
	TC = isl_mat_aff_direct_sum(ctx, TC, U2);
	bset = isl_basic_set_preimage(ctx, bset, T ? isl_mat_copy(ctx, TC) : TC);
	if (T)
		*T = TC;
	return bset;
error:
	isl_mat_free(ctx, H);
	isl_mat_free(ctx, U);
	if (T2)
		isl_mat_free(ctx, *T2);
	isl_basic_set_free(bset);
	if (T)
		*T = NULL;
	if (T2)
		*T2 = NULL;
	return NULL;
}

struct isl_basic_set *isl_basic_set_remove_equalities(
	struct isl_basic_set *bset, struct isl_mat **T, struct isl_mat **T2)
{
	if (T)
		*T = NULL;
	if (T2)
		*T2 = NULL;
	if (!bset)
		return NULL;
	isl_assert(bset->ctx, bset->nparam == 0, goto error);
	bset = isl_basic_set_gauss(bset, NULL);
	if (F_ISSET(bset, ISL_BASIC_SET_EMPTY))
		return bset;
	bset = compress_variables(bset->ctx, bset, T, T2);
	return bset;
error:
	isl_basic_set_free(bset);
	*T = NULL;
	return NULL;
}

/* Check if dimension dim belongs to a residue class
 *		i_dim \equiv r mod m
 * with m != 1 and if so return m in *modulo and r in *residue.
 */
int isl_basic_set_dim_residue_class(struct isl_basic_set *bset,
	int pos, isl_int *modulo, isl_int *residue)
{
	struct isl_ctx *ctx;
	struct isl_mat *H = NULL, *U = NULL, *C, *H1, *U1;
	unsigned total;

	if (!bset || !modulo || !residue)
		return -1;

	ctx = bset->ctx;
	total = bset->nparam + bset->dim + bset->n_div;
	H = isl_mat_sub_alloc(ctx, bset->eq, 0, bset->n_eq, 1, total);
	H = isl_mat_left_hermite(ctx, H, 0, &U, NULL);
	if (!H)
		return -1;

	isl_seq_gcd(U->row[bset->nparam + pos]+bset->n_eq,
			total-bset->n_eq, modulo);
	if (isl_int_is_zero(*modulo) || isl_int_is_one(*modulo)) {
		isl_int_set_si(*residue, 0);
		isl_mat_free(ctx, H);
		isl_mat_free(ctx, U);
		return 0;
	}

	C = isl_mat_alloc(ctx, 1+bset->n_eq, 1);
	if (!C)
		goto error;
	isl_int_set_si(C->row[0][0], 1);
	isl_mat_sub_neg(ctx, C->row+1, bset->eq, bset->n_eq, 0, 0, 1);
	H1 = isl_mat_sub_alloc(ctx, H->row, 0, H->n_row, 0, H->n_row);
	H1 = isl_mat_lin_to_aff(ctx, H1);
	C = isl_mat_inverse_product(ctx, H1, C);
	isl_mat_free(ctx, H);
	U1 = isl_mat_sub_alloc(ctx, U->row, bset->nparam+pos, 1, 0, bset->n_eq);
	U1 = isl_mat_lin_to_aff(ctx, U1);
	isl_mat_free(ctx, U);
	C = isl_mat_product(ctx, U1, C);
	if (!C)
		goto error;
	if (!isl_int_is_divisible_by(C->row[1][0], C->row[0][0])) {
		bset = isl_basic_set_copy(bset);
		bset = isl_basic_set_set_to_empty(bset);
		isl_basic_set_free(bset);
		isl_int_set_si(*modulo, 0);
		isl_int_set_si(*residue, 0);
		return 0;
	}
	isl_int_divexact(*residue, C->row[1][0], C->row[0][0]);
	isl_int_fdiv_r(*residue, *residue, *modulo);
	isl_mat_free(ctx, C);
	return 0;
error:
	isl_mat_free(ctx, H);
	isl_mat_free(ctx, U);
	return -1;
}
