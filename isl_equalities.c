#include "isl_mat.h"
#include "isl_seq.h"
#include "isl_map_private.h"
#include "isl_equalities.h"

/* Given a set of modulo constraints
 *
 *		c + A y = 0 mod d
 *
 * this function computes a particular solution y_0
 *
 * The input is given as a matrix B = [ c A ] and a vector d.
 *
 * The output is matrix containing the solution y_0 or
 * a zero-column matrix if the constraints admit no integer solution.
 *
 * The given set of constrains is equivalent to
 *
 *		c + A y = -D x
 *
 * with D = diag d and x a fresh set of variables.
 * Reducing both c and A modulo d does not change the
 * value of y in the solution and may lead to smaller coefficients.
 * Let M = [ D A ] and [ H 0 ] = M U, the Hermite normal form of M.
 * Then
 *		  [ x ]
 *		M [ y ] = - c
 * and so
 *		               [ x ]
 *		[ H 0 ] U^{-1} [ y ] = - c
 * Let
 *		[ A ]          [ x ]
 *		[ B ] = U^{-1} [ y ]
 * then
 *		H A + 0 B = -c
 *
 * so B may be chosen arbitrarily, e.g., B = 0, and then
 *
 *		       [ x ] = [ -c ]
 *		U^{-1} [ y ] = [  0 ]
 * or
 *		[ x ]     [ -c ]
 *		[ y ] = U [  0 ]
 * specifically,
 *
 *		y = U_{2,1} (-c)
 *
 * If any of the coordinates of this y are non-integer
 * then the constraints admit no integer solution and
 * a zero-column matrix is returned.
 */
static struct isl_mat *particular_solution(struct isl_ctx *ctx,
			struct isl_mat *B, struct isl_vec *d)
{
	int i, j;
	struct isl_mat *M = NULL;
	struct isl_mat *C = NULL;
	struct isl_mat *U = NULL;
	struct isl_mat *H = NULL;
	struct isl_mat *cst = NULL;
	struct isl_mat *T = NULL;

	M = isl_mat_alloc(ctx, B->n_row, B->n_row + B->n_col - 1);
	C = isl_mat_alloc(ctx, 1 + B->n_row, 1);
	if (!M || !C)
		goto error;
	isl_int_set_si(C->row[0][0], 1);
	for (i = 0; i < B->n_row; ++i) {
		isl_seq_clr(M->row[i], B->n_row);
		isl_int_set(M->row[i][i], d->block.data[i]);
		isl_int_neg(C->row[1 + i][0], B->row[i][0]);
		isl_int_fdiv_r(C->row[1+i][0], C->row[1+i][0], M->row[i][i]);
		for (j = 0; j < B->n_col - 1; ++j)
			isl_int_fdiv_r(M->row[i][B->n_row + j],
					B->row[i][1 + j], M->row[i][i]);
	}
	M = isl_mat_left_hermite(ctx, M, 0, &U, NULL);
	if (!M || !U)
		goto error;
	H = isl_mat_sub_alloc(ctx, M->row, 0, B->n_row, 0, B->n_row);
	H = isl_mat_lin_to_aff(ctx, H);
	C = isl_mat_inverse_product(ctx, H, C);
	if (!C)
		goto error;
	for (i = 0; i < B->n_row; ++i) {
		if (!isl_int_is_divisible_by(C->row[1+i][0], C->row[0][0]))
			break;
		isl_int_divexact(C->row[1+i][0], C->row[1+i][0], C->row[0][0]);
	}
	if (i < B->n_row)
		cst = isl_mat_alloc(ctx, B->n_row, 0);
	else
		cst = isl_mat_sub_alloc(ctx, C->row, 1, B->n_row, 0, 1);
	T = isl_mat_sub_alloc(ctx, U->row, B->n_row, B->n_col - 1, 0, B->n_row);
	cst = isl_mat_product(ctx, T, cst);
	isl_mat_free(ctx, M);
	isl_mat_free(ctx, C);
	isl_mat_free(ctx, U);
	return cst;
error:
	isl_mat_free(ctx, M);
	isl_mat_free(ctx, C);
	isl_mat_free(ctx, U);
	return NULL;
}

/* Given a set of modulo constraints
 *
 *		c + A y = 0 mod d
 *
 * this function returns an affine transformation T,
 *
 *		y = T y'
 *
 * that bijectively maps the integer vectors y' to integer
 * vectors y that satisfy the modulo constraints.
 *
 * The implementation closely follows the description in Section 2.5.3
 * of B. Meister, "Stating and Manipulating Periodicity in the Polytope
 * Model.  Applications to Program Analysis and Optimization".
 *
 * The input is given as a matrix B = [ c A ] and a vector d.
 * Each element of the vector d corresponds to a row in B.
 * The output is a lower triangular matrix.
 * If no integer vector y satisfies the given constraints then
 * a matrix with zero columns is returned.
 *
 * We first compute a particular solution y_0 to the given set of
 * modulo constraints in particular_solution.  If no such solution
 * exists, then we return a zero-columned transformation matrix.
 * Otherwise, we compute the generic solution to
 *
 *		A y = 0 mod d
 *
 * Let K be the right kernel of A, then any y = K y'' is a solution.
 * Any multiple of a unit vector s_j e_j such that for each row i,
 * we have that s_j a_{i,j} is a multiple of d_i, is also a generator
 * for the set of solutions.  The smallest such s_j can be obtained
 * by taking the lcm of all the a_{i,j}/gcd(a_{i,j},d).
 * That is, y = K y'' + S y''', with S = diag s is the general solution.
 * To obtain a minimal representation we compute the Hermite normal
 * form [ G 0 ] of [ S K ].
 * The affine transformation matrix returned is then
 *
 *		[  1   0  ]
 *		[ y_0  G  ]
 *
 * as any y = y_0 + G y' with y' integer is a solution to the original
 * modulo constraints.
 */
struct isl_mat *isl_mat_parameter_compression(struct isl_ctx *ctx,
			struct isl_mat *B, struct isl_vec *d)
{
	int i, j;
	struct isl_mat *cst = NULL;
	struct isl_mat *K = NULL;
	struct isl_mat *M = NULL;
	struct isl_mat *T;
	isl_int v;

	if (!B || !d)
		goto error;
	isl_assert(ctx, B->n_row == d->size, goto error);
	cst = particular_solution(ctx, B, d);
	if (!cst)
		goto error;
	if (cst->n_col == 0) {
		T = isl_mat_alloc(ctx, B->n_col, 0);
		isl_mat_free(ctx, cst);
		isl_mat_free(ctx, B);
		isl_vec_free(ctx, d);
		return T;
	}
	T = isl_mat_sub_alloc(ctx, B->row, 0, B->n_row, 1, B->n_col - 1);
	K = isl_mat_right_kernel(ctx, T);
	if (!K)
		goto error;
	M = isl_mat_alloc(ctx, B->n_col - 1, B->n_col - 1 + K->n_col);
	if (!M)
		goto error;
	isl_mat_sub_copy(ctx, M->row, K->row, M->n_row,
				B->n_col - 1, 0, K->n_col);
	for (i = 0; i < M->n_row; ++i) {
		isl_seq_clr(M->row[i], B->n_col - 1);
		isl_int_set_si(M->row[i][i], 1);
	}
	isl_int_init(v);
	for (i = 0; i < B->n_row; ++i)
		for (j = 0; j < B->n_col - 1; ++j) {
			isl_int_gcd(v, B->row[i][1+j], d->block.data[i]);
			isl_int_divexact(v, d->block.data[i], v);
			isl_int_lcm(M->row[j][j], M->row[j][j], v);
		}
	isl_int_clear(v);
	M = isl_mat_left_hermite(ctx, M, 0, NULL, NULL);
	if (!M)
		goto error;
	T = isl_mat_alloc(ctx, 1 + B->n_col - 1, B->n_col);
	if (!T)
		goto error;
	isl_int_set_si(T->row[0][0], 1);
	isl_seq_clr(T->row[0] + 1, T->n_col - 1);
	isl_mat_sub_copy(ctx, T->row + 1, cst->row, cst->n_row, 0, 0, 1);
	isl_mat_sub_copy(ctx, T->row + 1, M->row, M->n_row, 1, 0, T->n_col - 1);
	isl_mat_free(ctx, K);
	isl_mat_free(ctx, M);
	isl_mat_free(ctx, cst);
	isl_mat_free(ctx, B);
	isl_vec_free(ctx, d);
	return T;
error:
	isl_mat_free(ctx, K);
	isl_mat_free(ctx, M);
	isl_mat_free(ctx, cst);
	isl_mat_free(ctx, B);
	isl_vec_free(ctx, d);
	return NULL;
}

/* Given a set of equalities
 *
 *		M x - c = 0
 *
 * this function computes unimodular transformation from a lower-dimensional
 * space to the original space that bijectively maps the integer points x'
 * in the lower-dimensional space to the integer points x in the original
 * space that satisfy the equalities.
 *
 * The input is given as a matrix B = [ -c M ] and the out is a
 * matrix that maps [1 x'] to [1 x].
 * If T2 is not NULL, then *T2 is set to a matrix mapping [1 x] to [1 x'].
 *
 * First compute the (left) Hermite normal form of M,
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
struct isl_mat *isl_mat_variable_compression(struct isl_ctx *ctx,
			struct isl_mat *B, struct isl_mat **T2)
{
	int i;
	struct isl_mat *H = NULL, *C = NULL, *H1, *U = NULL, *U1, *U2, *TC;
	unsigned dim;

	if (T2)
		*T2 = NULL;
	if (!B)
		goto error;

	dim = B->n_col - 1;
	H = isl_mat_sub_alloc(ctx, B->row, 0, B->n_row, 1, dim);
	H = isl_mat_left_hermite(ctx, H, 0, &U, T2);
	if (!H || !U || (T2 && !*T2))
		goto error;
	if (T2) {
		*T2 = isl_mat_drop_rows(ctx, *T2, 0, B->n_row);
		*T2 = isl_mat_lin_to_aff(ctx, *T2);
		if (!*T2)
			goto error;
	}
	C = isl_mat_alloc(ctx, 1+B->n_row, 1);
	if (!C)
		goto error;
	isl_int_set_si(C->row[0][0], 1);
	isl_mat_sub_neg(ctx, C->row+1, B->row, B->n_row, 0, 0, 1);
	H1 = isl_mat_sub_alloc(ctx, H->row, 0, H->n_row, 0, H->n_row);
	H1 = isl_mat_lin_to_aff(ctx, H1);
	TC = isl_mat_inverse_product(ctx, H1, C);
	if (!TC)
		goto error;
	isl_mat_free(ctx, H);
	if (!isl_int_is_one(TC->row[0][0])) {
		for (i = 0; i < B->n_row; ++i) {
			if (!isl_int_is_divisible_by(TC->row[1+i][0], TC->row[0][0])) {
				isl_mat_free(ctx, B);
				isl_mat_free(ctx, TC);
				isl_mat_free(ctx, U);
				if (T2) {
					isl_mat_free(ctx, *T2);
					*T2 = NULL;
				}
				return isl_mat_alloc(ctx, 1 + B->n_col, 0);
			}
			isl_seq_scale_down(TC->row[1+i], TC->row[1+i], TC->row[0][0], 1);
		}
		isl_int_set_si(TC->row[0][0], 1);
	}
	U1 = isl_mat_sub_alloc(ctx, U->row, 0, U->n_row, 0, B->n_row);
	U1 = isl_mat_lin_to_aff(ctx, U1);
	U2 = isl_mat_sub_alloc(ctx, U->row, 0, U->n_row,
				B->n_row, U->n_row - B->n_row);
	U2 = isl_mat_lin_to_aff(ctx, U2);
	isl_mat_free(ctx, U);
	TC = isl_mat_product(ctx, U1, TC);
	TC = isl_mat_aff_direct_sum(ctx, TC, U2);

	isl_mat_free(ctx, B);

	return TC;
error:
	isl_mat_free(ctx, B);
	isl_mat_free(ctx, H);
	isl_mat_free(ctx, U);
	if (T2) {
		isl_mat_free(ctx, *T2);
		*T2 = NULL;
	}
	return NULL;
}

/* Use the n equalities of bset to unimodularly transform the
 * variables x such that n transformed variables x1' have a constant value
 * and rewrite the constraints of bset in terms of the remaining
 * transformed variables x2'.  The matrix pointed to by T maps
 * the new variables x2' back to the original variables x, while T2
 * maps the original variables to the new variables.
 */
static struct isl_basic_set *compress_variables(struct isl_ctx *ctx,
	struct isl_basic_set *bset, struct isl_mat **T, struct isl_mat **T2)
{
	struct isl_mat *B, *TC;
	unsigned dim;

	if (T)
		*T = NULL;
	if (T2)
		*T2 = NULL;
	if (!bset)
		goto error;
	isl_assert(ctx, isl_basic_set_n_param(bset) == 0, goto error);
	isl_assert(ctx, bset->n_div == 0, goto error);
	dim = isl_basic_set_n_dim(bset);
	isl_assert(ctx, bset->n_eq <= dim, goto error);
	if (bset->n_eq == 0)
		return bset;

	B = isl_mat_sub_alloc(ctx, bset->eq, 0, bset->n_eq, 0, 1 + dim);
	TC = isl_mat_variable_compression(ctx, B, T2);
	if (!TC)
		goto error;
	if (TC->n_col == 0) {
		isl_mat_free(ctx, TC);
		if (T2) {
			isl_mat_free(ctx, *T2);
			*T2 = NULL;
		}
		return isl_basic_set_set_to_empty(bset);
	}

	bset = isl_basic_set_preimage(ctx, bset, T ? isl_mat_copy(ctx, TC) : TC);
	if (T)
		*T = TC;
	return bset;
error:
	isl_basic_set_free(bset);
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
	isl_assert(bset->ctx, isl_basic_set_n_param(bset) == 0, goto error);
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
	unsigned nparam;

	if (!bset || !modulo || !residue)
		return -1;

	ctx = bset->ctx;
	total = isl_basic_set_total_dim(bset);
	nparam = isl_basic_set_n_param(bset);
	H = isl_mat_sub_alloc(ctx, bset->eq, 0, bset->n_eq, 1, total);
	H = isl_mat_left_hermite(ctx, H, 0, &U, NULL);
	if (!H)
		return -1;

	isl_seq_gcd(U->row[nparam + pos]+bset->n_eq,
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
	U1 = isl_mat_sub_alloc(ctx, U->row, nparam+pos, 1, 0, bset->n_eq);
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
