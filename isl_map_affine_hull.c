#include "isl_ctx.h"
#include "isl_seq.h"
#include "isl_set.h"
#include "isl_lp.h"
#include "isl_map.h"
#include "isl_map_private.h"

struct isl_basic_map *isl_basic_map_affine_hull(struct isl_ctx *ctx,
						struct isl_basic_map *bmap)
{
	int i;
	isl_int opt;

	bmap = isl_basic_map_cow(ctx, bmap);
	if (!bmap)
		return NULL;

	isl_int_init(opt);
	for (i = 0; i < bmap->n_ineq; ++i) {
		enum isl_lp_result res;
		res = isl_solve_lp(bmap, 1, bmap->ineq[i]+1, ctx->one, &opt);
		if (res == isl_lp_unbounded)
			continue;
		if (res == isl_lp_error)
			goto error;
		if (res == isl_lp_empty) {
			bmap = isl_basic_map_set_to_empty(ctx, bmap);
			break;
		}
		isl_int_add(opt, opt, bmap->ineq[i][0]);
		if (isl_int_is_zero(opt)) {
			isl_basic_map_inequality_to_equality(ctx, bmap, i);
			--i;
		}
	}
	isl_basic_map_free_inequality(ctx, bmap, bmap->n_ineq);
	isl_int_clear(opt);
	return isl_basic_map_finalize(ctx, bmap);
error:
	isl_int_clear(opt);
	isl_basic_map_free(ctx, bmap);
	return NULL;
}

struct isl_basic_set *isl_basic_set_affine_hull(struct isl_ctx *ctx,
						struct isl_basic_set *bset)
{
	return (struct isl_basic_set *)
		isl_basic_map_affine_hull(ctx, (struct isl_basic_map *)bset);
}

/* Make eq[row][col] of both bmaps equal so we can add the row
 * add the column to the common matrix.
 * Note that because of the echelon form, the columns of row row
 * after column col are zero.
 */
static void set_common_multiple(
	struct isl_basic_map *bmap1, struct isl_basic_map *bmap2,
	unsigned row, unsigned col)
{
	isl_int m, c;

	if (isl_int_eq(bmap1->eq[row][col], bmap2->eq[row][col]))
		return;

	isl_int_init(c);
	isl_int_init(m);
	isl_int_lcm(m, bmap1->eq[row][col], bmap2->eq[row][col]);
	isl_int_divexact(c, m, bmap1->eq[row][col]);
	isl_seq_scale(bmap1->eq[row], bmap1->eq[row], c, col+1);
	isl_int_divexact(c, m, bmap2->eq[row][col]);
	isl_seq_scale(bmap2->eq[row], bmap2->eq[row], c, col+1);
	isl_int_clear(c);
	isl_int_clear(m);
}

/* Delete a given equality, moving all the following equalities one up.
 */
static void delete_row(struct isl_basic_map *bmap, unsigned row)
{
	isl_int *t;
	int r;

	t = bmap->eq[row];
	bmap->n_eq--;
	for (r = row; r < bmap->n_eq; ++r)
		bmap->eq[r] = bmap->eq[r+1];
	bmap->eq[bmap->n_eq] = t;
}

/* Make first row entries in column col of bmap1 identical to
 * those of bmap2, using the fact that entry bmap1->eq[row][col]=a
 * is non-zero.  Initially, these elements of bmap1 are all zero.
 * For each row i < row, we set
 *		A[i] = a * A[i] + B[i][col] * A[row]
 *		B[i] = a * B[i]
 * so that
 *		A[i][col] = B[i][col] = a * old(B[i][col])
 */
static void construct_column(
	struct isl_basic_map *bmap1, struct isl_basic_map *bmap2,
	unsigned row, unsigned col)
{
	int r;
	isl_int a;
	isl_int b;
	unsigned total;

	isl_int_init(a);
	isl_int_init(b);
	total = 1 + bmap1->nparam + bmap1->n_in + bmap1->n_out + bmap1->n_div;
	for (r = 0; r < row; ++r) {
		if (isl_int_is_zero(bmap2->eq[r][col]))
			continue;
		isl_int_gcd(b, bmap2->eq[r][col], bmap1->eq[row][col]);
		isl_int_divexact(a, bmap1->eq[row][col], b);
		isl_int_divexact(b, bmap2->eq[r][col], b);
		isl_seq_combine(bmap1->eq[r], a, bmap1->eq[r],
					      b, bmap1->eq[row], total);
		isl_seq_scale(bmap2->eq[r], bmap2->eq[r], a, total);
	}
	isl_int_clear(a);
	isl_int_clear(b);
	delete_row(bmap1, row);
}

/* Make first row entries in column col of bmap1 identical to
 * those of bmap2, using only these entries of the two matrices.
 * Let t be the last row with different entries.
 * For each row i < t, we set
 *	A[i] = (A[t][col]-B[t][col]) * A[i] + (B[i][col]-A[i][col) * A[t]
 *	B[i] = (A[t][col]-B[t][col]) * B[i] + (B[i][col]-A[i][col) * B[t]
 * so that
 *	A[i][col] = B[i][col] = old(A[t][col]*B[i][col]-A[i][col]*B[t][col])
 */
static int transform_column(
	struct isl_basic_map *bmap1, struct isl_basic_map *bmap2,
	unsigned row, unsigned col)
{
	int i, t;
	isl_int a, b, g;
	unsigned total;

	for (t = row-1; t >= 0; --t)
		if (isl_int_ne(bmap1->eq[t][col], bmap2->eq[t][col]))
			break;
	if (t < 0)
		return 0;

	total = 1 + bmap1->nparam + bmap1->n_in + bmap1->n_out + bmap1->n_div;
	isl_int_init(a);
	isl_int_init(b);
	isl_int_init(g);
	isl_int_sub(b, bmap1->eq[t][col], bmap2->eq[t][col]);
	for (i = 0; i < t; ++i) {
		isl_int_sub(a, bmap2->eq[i][col], bmap1->eq[i][col]);
		isl_int_gcd(g, a, b);
		isl_int_divexact(a, a, g);
		isl_int_divexact(g, b, g);
		isl_seq_combine(bmap1->eq[i], g, bmap1->eq[i], a, bmap1->eq[t],
				total);
		isl_seq_combine(bmap2->eq[i], g, bmap2->eq[i], a, bmap2->eq[t],
				total);
	}
	isl_int_clear(a);
	isl_int_clear(b);
	isl_int_clear(g);
	delete_row(bmap1, t);
	delete_row(bmap2, t);
	return 1;
}

/* The implementation is based on Section 5.2 of Michael Karr,
 * "Affine Relationships Among Variables of a Program",
 * except that the echelon form we use starts from the last column
 * and that we are dealing with integer coefficients.
 */
static struct isl_basic_map *affine_hull(struct isl_ctx *ctx,
	struct isl_basic_map *bmap1, struct isl_basic_map *bmap2)
{
	unsigned total;
	int col;
	int row;

	total = 1 + bmap1->nparam + bmap1->n_in + bmap1->n_out + bmap1->n_div;

	row = 0;
	for (col = total-1; col >= 0; --col) {
		int is_zero1 = row >= bmap1->n_eq ||
			isl_int_is_zero(bmap1->eq[row][col]);
		int is_zero2 = row >= bmap2->n_eq ||
			isl_int_is_zero(bmap2->eq[row][col]);
		if (!is_zero1 && !is_zero2) {
			set_common_multiple(bmap1, bmap2, row, col);
			++row;
		} else if (!is_zero1 && is_zero2) {
			construct_column(bmap1, bmap2, row, col);
		} else if (is_zero1 && !is_zero2) {
			construct_column(bmap2, bmap1, row, col);
		} else {
			if (transform_column(bmap1, bmap2, row, col))
				--row;
		}
	}
	isl_basic_map_free(ctx, bmap2);
	return bmap1;
}

struct isl_basic_map *isl_map_affine_hull(struct isl_ctx *ctx,
						struct isl_map *map)
{
	int i;
	struct isl_basic_map *bmap;

	map = isl_map_compute_divs(ctx, map);
	map = isl_map_cow(ctx, map);
	if (!map)
		return NULL;

	if (map->n == 0) {
		bmap = isl_basic_map_empty(ctx,
					    map->nparam, map->n_in, map->n_out);
		isl_map_free(ctx, map);
		return bmap;
	}

	for (i = 1; i < map->n; ++i)
		map->p[0] = isl_basic_map_align_divs(ctx, map->p[0], map->p[i]);
	for (i = 1; i < map->n; ++i)
		map->p[i] = isl_basic_map_align_divs(ctx, map->p[i], map->p[0]);

	for (i = 0; i < map->n; ++i) {
		map->p[i] = isl_basic_map_affine_hull(ctx, map->p[i]);
		map->p[i] = isl_basic_map_gauss(ctx, map->p[i], NULL);
		if (!map->p[i])
			goto error;
	}
	while (map->n > 1) {
		map->p[0] = affine_hull(ctx, map->p[0], map->p[--map->n]);
		if (!map->p[0])
			goto error;
	}
	bmap = isl_basic_map_copy(ctx, map->p[0]);
	isl_map_free(ctx, map);
	bmap = isl_basic_map_finalize(ctx, bmap);
	return isl_basic_map_simplify(ctx, bmap);
error:
	isl_map_free(ctx, map);
	return NULL;
}

struct isl_basic_set *isl_set_affine_hull(struct isl_ctx *ctx,
						struct isl_set *set)
{
	return (struct isl_basic_set *)
		isl_map_affine_hull(ctx, (struct isl_map *)set);
}
