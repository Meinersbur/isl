#include "isl_ctx.h"
#include "isl_seq.h"
#include "isl_set.h"
#include "isl_lp.h"
#include "isl_map.h"
#include "isl_map_private.h"
#include "isl_equalities.h"
#include "isl_sample.h"

struct isl_basic_map *isl_basic_map_implicit_equalities(
						struct isl_basic_map *bmap)
{
	int i;
	isl_int opt;
	struct isl_ctx *ctx;

	if (!bmap)
		return bmap;

	if (F_ISSET(bmap, ISL_BASIC_MAP_EMPTY))
		return bmap;
	if (F_ISSET(bmap, ISL_BASIC_MAP_NO_IMPLICIT))
		return bmap;

	ctx = bmap->ctx;
	isl_int_init(opt);
	for (i = 0; i < bmap->n_ineq; ++i) {
		enum isl_lp_result res;
		res = isl_solve_lp(bmap, 1, bmap->ineq[i]+1, ctx->one, &opt, NULL);
		if (res == isl_lp_unbounded)
			continue;
		if (res == isl_lp_error)
			goto error;
		if (res == isl_lp_empty) {
			bmap = isl_basic_map_set_to_empty(bmap);
			break;
		}
		isl_int_add(opt, opt, bmap->ineq[i][0]);
		if (isl_int_is_zero(opt)) {
			isl_basic_map_inequality_to_equality(bmap, i);
			--i;
		}
	}
	isl_int_clear(opt);

	F_SET(bmap, ISL_BASIC_MAP_NO_IMPLICIT);
	return bmap;
error:
	isl_int_clear(opt);
	isl_basic_map_free(bmap);
	return NULL;
}

/* Make eq[row][col] of both bmaps equal so we can add the row
 * add the column to the common matrix.
 * Note that because of the echelon form, the columns of row row
 * after column col are zero.
 */
static void set_common_multiple(
	struct isl_basic_set *bset1, struct isl_basic_set *bset2,
	unsigned row, unsigned col)
{
	isl_int m, c;

	if (isl_int_eq(bset1->eq[row][col], bset2->eq[row][col]))
		return;

	isl_int_init(c);
	isl_int_init(m);
	isl_int_lcm(m, bset1->eq[row][col], bset2->eq[row][col]);
	isl_int_divexact(c, m, bset1->eq[row][col]);
	isl_seq_scale(bset1->eq[row], bset1->eq[row], c, col+1);
	isl_int_divexact(c, m, bset2->eq[row][col]);
	isl_seq_scale(bset2->eq[row], bset2->eq[row], c, col+1);
	isl_int_clear(c);
	isl_int_clear(m);
}

/* Delete a given equality, moving all the following equalities one up.
 */
static void delete_row(struct isl_basic_set *bset, unsigned row)
{
	isl_int *t;
	int r;

	t = bset->eq[row];
	bset->n_eq--;
	for (r = row; r < bset->n_eq; ++r)
		bset->eq[r] = bset->eq[r+1];
	bset->eq[bset->n_eq] = t;
}

/* Make first row entries in column col of bset1 identical to
 * those of bset2, using the fact that entry bset1->eq[row][col]=a
 * is non-zero.  Initially, these elements of bset1 are all zero.
 * For each row i < row, we set
 *		A[i] = a * A[i] + B[i][col] * A[row]
 *		B[i] = a * B[i]
 * so that
 *		A[i][col] = B[i][col] = a * old(B[i][col])
 */
static void construct_column(
	struct isl_basic_set *bset1, struct isl_basic_set *bset2,
	unsigned row, unsigned col)
{
	int r;
	isl_int a;
	isl_int b;
	unsigned total;

	isl_int_init(a);
	isl_int_init(b);
	total = 1 + bset1->dim;
	for (r = 0; r < row; ++r) {
		if (isl_int_is_zero(bset2->eq[r][col]))
			continue;
		isl_int_gcd(b, bset2->eq[r][col], bset1->eq[row][col]);
		isl_int_divexact(a, bset1->eq[row][col], b);
		isl_int_divexact(b, bset2->eq[r][col], b);
		isl_seq_combine(bset1->eq[r], a, bset1->eq[r],
					      b, bset1->eq[row], total);
		isl_seq_scale(bset2->eq[r], bset2->eq[r], a, total);
	}
	isl_int_clear(a);
	isl_int_clear(b);
	delete_row(bset1, row);
}

/* Make first row entries in column col of bset1 identical to
 * those of bset2, using only these entries of the two matrices.
 * Let t be the last row with different entries.
 * For each row i < t, we set
 *	A[i] = (A[t][col]-B[t][col]) * A[i] + (B[i][col]-A[i][col) * A[t]
 *	B[i] = (A[t][col]-B[t][col]) * B[i] + (B[i][col]-A[i][col) * B[t]
 * so that
 *	A[i][col] = B[i][col] = old(A[t][col]*B[i][col]-A[i][col]*B[t][col])
 */
static int transform_column(
	struct isl_basic_set *bset1, struct isl_basic_set *bset2,
	unsigned row, unsigned col)
{
	int i, t;
	isl_int a, b, g;
	unsigned total;

	for (t = row-1; t >= 0; --t)
		if (isl_int_ne(bset1->eq[t][col], bset2->eq[t][col]))
			break;
	if (t < 0)
		return 0;

	total = 1 + bset1->dim;
	isl_int_init(a);
	isl_int_init(b);
	isl_int_init(g);
	isl_int_sub(b, bset1->eq[t][col], bset2->eq[t][col]);
	for (i = 0; i < t; ++i) {
		isl_int_sub(a, bset2->eq[i][col], bset1->eq[i][col]);
		isl_int_gcd(g, a, b);
		isl_int_divexact(a, a, g);
		isl_int_divexact(g, b, g);
		isl_seq_combine(bset1->eq[i], g, bset1->eq[i], a, bset1->eq[t],
				total);
		isl_seq_combine(bset2->eq[i], g, bset2->eq[i], a, bset2->eq[t],
				total);
	}
	isl_int_clear(a);
	isl_int_clear(b);
	isl_int_clear(g);
	delete_row(bset1, t);
	delete_row(bset2, t);
	return 1;
}

/* The implementation is based on Section 5.2 of Michael Karr,
 * "Affine Relationships Among Variables of a Program",
 * except that the echelon form we use starts from the last column
 * and that we are dealing with integer coefficients.
 */
static struct isl_basic_set *affine_hull(
	struct isl_basic_set *bset1, struct isl_basic_set *bset2)
{
	unsigned total;
	int col;
	int row;

	total = 1 + bset1->dim;

	row = 0;
	for (col = total-1; col >= 0; --col) {
		int is_zero1 = row >= bset1->n_eq ||
			isl_int_is_zero(bset1->eq[row][col]);
		int is_zero2 = row >= bset2->n_eq ||
			isl_int_is_zero(bset2->eq[row][col]);
		if (!is_zero1 && !is_zero2) {
			set_common_multiple(bset1, bset2, row, col);
			++row;
		} else if (!is_zero1 && is_zero2) {
			construct_column(bset1, bset2, row, col);
		} else if (is_zero1 && !is_zero2) {
			construct_column(bset2, bset1, row, col);
		} else {
			if (transform_column(bset1, bset2, row, col))
				--row;
		}
	}
	isl_basic_set_free(bset2);
	isl_assert(ctx, row == bset1->n_eq, goto error);
	return bset1;
error:
	isl_basic_set_free(bset1);
	return NULL;
}

static struct isl_basic_set *isl_basic_set_from_vec(struct isl_ctx *ctx,
	struct isl_vec *vec)
{
	int i;
	int k;
	struct isl_basic_set *bset = NULL;

	if (!vec)
		return NULL;
	isl_assert(ctx, vec->size != 0, goto error);

	bset = isl_basic_set_alloc(ctx, 0, vec->size - 1, 0, vec->size - 1, 0);
	for (i = bset->dim - 1; i >= 0; --i) {
		k = isl_basic_set_alloc_equality(bset);
		if (k < 0)
			goto error;
		isl_seq_clr(bset->eq[k], 1 + bset->dim);
		isl_int_neg(bset->eq[k][0], vec->block.data[1 + i]);
		isl_int_set(bset->eq[k][1 + i], vec->block.data[0]);
	}
	isl_vec_free(ctx, vec);

	return bset;
error:
	isl_basic_set_free(bset);
	isl_vec_free(ctx, vec);
	return NULL;
}

static struct isl_basic_set *outside_point(struct isl_ctx *ctx,
	struct isl_basic_set *bset, isl_int *eq, int up)
{
	struct isl_basic_set *slice = NULL;
	struct isl_vec *sample;
	struct isl_basic_set *point;
	int k;

	slice = isl_basic_set_copy(bset);
	if (!slice)
		goto error;
	slice = isl_basic_set_extend(slice, 0, slice->dim, 0, 1, 0);
	k = isl_basic_set_alloc_equality(slice);
	if (k < 0)
		goto error;
	isl_seq_cpy(slice->eq[k], eq, 1 + slice->dim);
	if (up)
		isl_int_add_ui(slice->eq[k][0], slice->eq[k][0], 1);
	else
		isl_int_sub_ui(slice->eq[k][0], slice->eq[k][0], 1);

	sample = isl_basic_set_sample(slice);
	if (!sample)
		goto error;
	if (sample->size == 0) {
		isl_vec_free(ctx, sample);
		point = isl_basic_set_empty(ctx, 0, bset->dim);
	} else
		point = isl_basic_set_from_vec(ctx, sample);

	return point;
error:
	isl_basic_set_free(slice);
	return NULL;
}

/* After computing the rational affine hull (by detecting the implicit
 * equalities), we remove all equalities found so far, compute
 * the integer affine hull of what is left, and then add the original
 * equalities back in.
 *
 * The integer affine hull is constructed by successively looking
 * a point that is affinely independent of the points found so far.
 * In particular, for each equality satisfied by the points so far,
 * we check if there is any point on the corresponding hyperplane
 * shifted by one (in either direction).
 */
struct isl_basic_map *isl_basic_map_affine_hull(struct isl_basic_map *bmap)
{
	int i, j;
	struct isl_mat *T2 = NULL;
	struct isl_basic_set *bset = NULL;
	struct isl_basic_set *hull = NULL;
	struct isl_vec *sample;
	struct isl_ctx *ctx;

	bmap = isl_basic_map_implicit_equalities(bmap);
	if (!bmap)
		return NULL;
	ctx = bmap->ctx;
	if (bmap->n_ineq == 0)
		return bmap;

	bset = isl_basic_map_underlying_set(isl_basic_map_copy(bmap));
	bset = isl_basic_set_remove_equalities(bset, NULL, &T2);

	sample = isl_basic_set_sample(isl_basic_set_copy(bset));
	hull = isl_basic_set_from_vec(ctx, sample);

	for (i = 0; i < bset->dim; ++i) {
		struct isl_basic_set *point;
		for (j = 0; j < hull->n_eq; ++j) {
			point = outside_point(ctx, bset, hull->eq[j], 1);
			if (!point)
				goto error;
			if (!F_ISSET(point, ISL_BASIC_SET_EMPTY))
				break;
			isl_basic_set_free(point);
			point = outside_point(ctx, bset, hull->eq[j], 0);
			if (!point)
				goto error;
			if (!F_ISSET(point, ISL_BASIC_SET_EMPTY))
				break;
			isl_basic_set_free(point);
		}
		if (j == hull->n_eq)
			break;
		hull = affine_hull(hull, point);
	}

	isl_basic_set_free(bset);
	bset = NULL;
	bmap = isl_basic_map_cow(bmap);
	if (!bmap)
		goto error;
	isl_basic_map_free_inequality(bmap, bmap->n_ineq);
	if (T2)
		hull = isl_basic_set_preimage(ctx, hull, T2);
	bmap = isl_basic_map_intersect(bmap,
					isl_basic_map_overlying_set(hull,
						isl_basic_map_copy(bmap)));

	return isl_basic_map_finalize(bmap);
error:
	isl_mat_free(ctx, T2);
	isl_basic_set_free(bset);
	isl_basic_set_free(hull);
	isl_basic_map_free(bmap);
	return NULL;
}

struct isl_basic_set *isl_basic_set_affine_hull(struct isl_basic_set *bset)
{
	return (struct isl_basic_set *)
		isl_basic_map_affine_hull((struct isl_basic_map *)bset);
}

struct isl_basic_map *isl_map_affine_hull(struct isl_map *map)
{
	int i;
	struct isl_basic_map *model = NULL;
	struct isl_basic_map *hull = NULL;
	struct isl_set *set;

	if (!map)
		return NULL;

	if (map->n == 0) {
		hull = isl_basic_map_empty(map->ctx,
					map->nparam, map->n_in, map->n_out);
		isl_map_free(map);
		return hull;
	}

	map = isl_map_align_divs(map);
	model = isl_basic_map_copy(map->p[0]);
	set = isl_map_underlying_set(map);
	set = isl_set_cow(set);
	if (!set)
		goto error;

	for (i = 0; i < set->n; ++i) {
		set->p[i] = isl_basic_set_cow(set->p[i]);
		set->p[i] = isl_basic_set_affine_hull(set->p[i]);
		set->p[i] = isl_basic_set_gauss(set->p[i], NULL);
		if (!set->p[i])
			goto error;
	}
	set = isl_set_remove_empty_parts(set);
	if (set->n == 0) {
		hull = isl_basic_map_empty(set->ctx,
				    model->nparam, model->n_in, model->n_out);
		isl_basic_map_free(model);
	} else {
		struct isl_basic_set *bset;
		while (set->n > 1) {
			set->p[0] = affine_hull(set->p[0], set->p[--set->n]);
			if (!set->p[0])
				goto error;
		}
		bset = isl_basic_set_copy(set->p[0]);
		hull = isl_basic_map_overlying_set(bset, model);
	}
	isl_set_free(set);
	hull = isl_basic_map_simplify(hull);
	return isl_basic_map_finalize(hull);
error:
	isl_basic_map_free(model);
	isl_set_free(set);
	return NULL;
}

struct isl_basic_set *isl_set_affine_hull(struct isl_set *set)
{
	return (struct isl_basic_set *)
		isl_map_affine_hull((struct isl_map *)set);
}
