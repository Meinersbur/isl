#include "isl_lp.h"
#include "isl_map.h"
#include "isl_map_private.h"
#include "isl_mat.h"
#include "isl_set.h"
#include "isl_seq.h"
#include "isl_equalities.h"

static struct isl_basic_set *uset_convex_hull_wrap(struct isl_set *set);

static swap_ineq(struct isl_basic_map *bmap, unsigned i, unsigned j)
{
	isl_int *t;

	if (i != j) {
		t = bmap->ineq[i];
		bmap->ineq[i] = bmap->ineq[j];
		bmap->ineq[j] = t;
	}
}

/* Compute the convex hull of a basic map, by removing the redundant
 * constraints.  If the minimal value along the normal of a constraint
 * is the same if the constraint is removed, then the constraint is redundant.
 *
 * Alternatively, we could have intersected the basic map with the
 * corresponding equality and the checked if the dimension was that
 * of a facet.
 */
struct isl_basic_map *isl_basic_map_convex_hull(struct isl_basic_map *bmap)
{
	int i;
	isl_int opt_n;
	isl_int opt_d;
	struct isl_ctx *ctx;

	bmap = isl_basic_map_implicit_equalities(bmap);
	if (!bmap)
		return NULL;

	if (F_ISSET(bmap, ISL_BASIC_MAP_EMPTY))
		return bmap;
	if (F_ISSET(bmap, ISL_BASIC_MAP_NO_REDUNDANT))
		return bmap;

	ctx = bmap->ctx;
	isl_int_init(opt_n);
	isl_int_init(opt_d);
	for (i = bmap->n_ineq-1; i >= 0; --i) {
		enum isl_lp_result res;
		swap_ineq(bmap, i, bmap->n_ineq-1);
		bmap->n_ineq--;
		res = isl_solve_lp(bmap, 0,
			bmap->ineq[bmap->n_ineq]+1, ctx->one, &opt_n, &opt_d);
		bmap->n_ineq++;
		swap_ineq(bmap, i, bmap->n_ineq-1);
		if (res == isl_lp_unbounded)
			continue;
		if (res == isl_lp_error)
			goto error;
		if (res == isl_lp_empty) {
			bmap = isl_basic_map_set_to_empty(bmap);
			break;
		}
		isl_int_addmul(opt_n, opt_d, bmap->ineq[i][0]);
		if (!isl_int_is_neg(opt_n))
			isl_basic_map_drop_inequality(bmap, i);
	}
	isl_int_clear(opt_n);
	isl_int_clear(opt_d);

	F_SET(bmap, ISL_BASIC_MAP_NO_REDUNDANT);
	return bmap;
error:
	isl_int_clear(opt_n);
	isl_int_clear(opt_d);
	isl_basic_map_free(bmap);
	return NULL;
}

struct isl_basic_set *isl_basic_set_convex_hull(struct isl_basic_set *bset)
{
	return (struct isl_basic_set *)
		isl_basic_map_convex_hull((struct isl_basic_map *)bset);
}

static int uset_is_bound(struct isl_ctx *ctx, struct isl_set *set,
	isl_int *c, unsigned len)
{
	int first;
	int j;
	isl_int opt;
	isl_int opt_denom;

	isl_int_init(opt);
	isl_int_init(opt_denom);
	first = 1;
	for (j = 0; j < set->n; ++j) {
		enum isl_lp_result res;

		if (F_ISSET(set->p[j], ISL_BASIC_SET_EMPTY))
			continue;

		res = isl_solve_lp((struct isl_basic_map*)set->p[j],
				0, c+1, ctx->one, &opt, &opt_denom);
		if (res == isl_lp_unbounded)
			break;
		if (res == isl_lp_error)
			goto error;
		if (res == isl_lp_empty) {
			set->p[j] = isl_basic_set_set_to_empty(set->p[j]);
			if (!set->p[j])
				goto error;
			continue;
		}
		if (!isl_int_is_one(opt_denom))
			isl_seq_scale(c, c, opt_denom, len);
		if (first || isl_int_lt(opt, c[0]))
			isl_int_set(c[0], opt);
		first = 0;
	}
	isl_int_clear(opt);
	isl_int_clear(opt_denom);
	isl_int_neg(c[0], c[0]);
	return j >= set->n;
error:
	isl_int_clear(opt);
	isl_int_clear(opt_denom);
	return -1;
}

/* Check if "c" is a direction with both a lower bound and an upper
 * bound in "set" that is independent of the previously found "n"
 * bounds in "dirs".
 * If so, add it to the list, with the negative of the lower bound
 * in the constant position, i.e., such that c corresponds to a bounding
 * hyperplane (but not necessarily a facet).
 */
static int is_independent_bound(struct isl_ctx *ctx,
	struct isl_set *set, isl_int *c,
	struct isl_mat *dirs, int n)
{
	int is_bound;
	int i = 0;

	isl_seq_cpy(dirs->row[n]+1, c+1, dirs->n_col-1);
	if (n != 0) {
		int pos = isl_seq_first_non_zero(dirs->row[n]+1, dirs->n_col-1);
		if (pos < 0)
			return 0;
		for (i = 0; i < n; ++i) {
			int pos_i;
			pos_i = isl_seq_first_non_zero(dirs->row[i]+1, dirs->n_col-1);
			if (pos_i < pos)
				continue;
			if (pos_i > pos)
				break;
			isl_seq_elim(dirs->row[n]+1, dirs->row[i]+1, pos,
					dirs->n_col-1, NULL);
			pos = isl_seq_first_non_zero(dirs->row[n]+1, dirs->n_col-1);
			if (pos < 0)
				return 0;
		}
	}

	isl_seq_neg(dirs->row[n] + 1, dirs->row[n] + 1, dirs->n_col - 1);
	is_bound = uset_is_bound(ctx, set, dirs->row[n], dirs->n_col);
	isl_seq_neg(dirs->row[n] + 1, dirs->row[n] + 1, dirs->n_col - 1);
	if (is_bound != 1)
		return is_bound;
	is_bound = uset_is_bound(ctx, set, dirs->row[n], dirs->n_col);
	if (is_bound != 1)
		return is_bound;
	if (i < n) {
		int k;
		isl_int *t = dirs->row[n];
		for (k = n; k > i; --k)
			dirs->row[k] = dirs->row[k-1];
		dirs->row[i] = t;
	}
	return 1;
}

/* Compute and return a maximal set of linearly independent bounds
 * on the set "set", based on the constraints of the basic sets
 * in "set".
 */
static struct isl_mat *independent_bounds(struct isl_ctx *ctx,
	struct isl_set *set)
{
	int i, j, n;
	struct isl_mat *dirs = NULL;

	dirs = isl_mat_alloc(ctx, set->dim, 1+set->dim);
	if (!dirs)
		goto error;

	n = 0;
	for (i = 0; n < set->dim && i < set->n; ++i) {
		int f;
		struct isl_basic_set *bset = set->p[i];

		for (j = 0; n < set->dim && j < bset->n_eq; ++j) {
			f = is_independent_bound(ctx, set, bset->eq[j],
						dirs, n);
			if (f < 0)
				goto error;
			if (f)
				++n;
		}
		for (j = 0; n < set->dim && j < bset->n_ineq; ++j) {
			f = is_independent_bound(ctx, set, bset->ineq[j],
						dirs, n);
			if (f < 0)
				goto error;
			if (f)
				++n;
		}
	}
	dirs->n_row = n;
	return dirs;
error:
	isl_mat_free(ctx, dirs);
	return NULL;
}

static struct isl_basic_set *isl_basic_set_set_rational(
	struct isl_basic_set *bset)
{
	if (!bset)
		return NULL;

	if (F_ISSET(bset, ISL_BASIC_MAP_RATIONAL))
		return bset;

	bset = isl_basic_set_cow(bset);
	if (!bset)
		return NULL;

	F_SET(bset, ISL_BASIC_MAP_RATIONAL);

	return isl_basic_set_finalize(bset);
}

static struct isl_set *isl_set_set_rational(struct isl_set *set)
{
	int i;

	set = isl_set_cow(set);
	if (!set)
		return NULL;
	for (i = 0; i < set->n; ++i) {
		set->p[i] = isl_basic_set_set_rational(set->p[i]);
		if (!set->p[i])
			goto error;
	}
	return set;
error:
	isl_set_free(set);
	return NULL;
}

static struct isl_basic_set *isl_basic_set_add_equality(struct isl_ctx *ctx,
	struct isl_basic_set *bset, isl_int *c)
{
	int i;
	unsigned total;

	if (F_ISSET(bset, ISL_BASIC_SET_EMPTY))
		return bset;

	isl_assert(ctx, bset->nparam == 0, goto error);
	isl_assert(ctx, bset->n_div == 0, goto error);
	bset = isl_basic_set_extend(bset, 0, bset->dim, 0, 1, 0);
	i = isl_basic_set_alloc_equality(bset);
	if (i < 0)
		goto error;
	isl_seq_cpy(bset->eq[i], c, 1 + bset->dim);
	return bset;
error:
	isl_basic_set_free(bset);
	return NULL;
}

static struct isl_set *isl_set_add_equality(struct isl_ctx *ctx,
	struct isl_set *set, isl_int *c)
{
	int i;

	set = isl_set_cow(set);
	if (!set)
		return NULL;
	for (i = 0; i < set->n; ++i) {
		set->p[i] = isl_basic_set_add_equality(ctx, set->p[i], c);
		if (!set->p[i])
			goto error;
	}
	return set;
error:
	isl_set_free(set);
	return NULL;
}

/* Given a union of basic sets, construct the constraints for wrapping
 * a facet around one of its ridges.
 * In particular, if each of n the d-dimensional basic sets i in "set"
 * contains the origin, satisfies the constraints x_1 >= 0 and x_2 >= 0
 * and is defined by the constraints
 *				    [ 1 ]
 *				A_i [ x ]  >= 0
 *
 * then the resulting set is of dimension n*(1+d) and has as contraints
 *
 *				    [ a_i ]
 *				A_i [ x_i ] >= 0
 *
 *				      a_i   >= 0
 *
 *			\sum_i x_{i,1} = 1
 */
static struct isl_basic_set *wrap_constraints(struct isl_ctx *ctx,
							struct isl_set *set)
{
	struct isl_basic_set *lp;
	unsigned n_eq;
	unsigned n_ineq;
	int i, j, k;
	unsigned dim;

	if (!set)
		return NULL;

	dim = 1 + set->dim;
	n_eq = 1;
	n_ineq = set->n;
	for (i = 0; i < set->n; ++i) {
		n_eq += set->p[i]->n_eq;
		n_ineq += set->p[i]->n_ineq;
	}
	lp = isl_basic_set_alloc(ctx, 0, dim * set->n, 0, n_eq, n_ineq);
	if (!lp)
		return NULL;
	k = isl_basic_set_alloc_equality(lp);
	isl_int_set_si(lp->eq[k][0], -1);
	for (i = 0; i < set->n; ++i) {
		isl_int_set_si(lp->eq[k][1+dim*i], 0);
		isl_int_set_si(lp->eq[k][1+dim*i+1], 1);
		isl_seq_clr(lp->eq[k]+1+dim*i+2, dim-2);
	}
	for (i = 0; i < set->n; ++i) {
		k = isl_basic_set_alloc_inequality(lp);
		isl_seq_clr(lp->ineq[k], 1+lp->dim);
		isl_int_set_si(lp->ineq[k][1+dim*i], 1);

		for (j = 0; j < set->p[i]->n_eq; ++j) {
			k = isl_basic_set_alloc_equality(lp);
			isl_seq_clr(lp->eq[k], 1+dim*i);
			isl_seq_cpy(lp->eq[k]+1+dim*i, set->p[i]->eq[j], dim);
			isl_seq_clr(lp->eq[k]+1+dim*(i+1), dim*(set->n-i-1));
		}

		for (j = 0; j < set->p[i]->n_ineq; ++j) {
			k = isl_basic_set_alloc_inequality(lp);
			isl_seq_clr(lp->ineq[k], 1+dim*i);
			isl_seq_cpy(lp->ineq[k]+1+dim*i, set->p[i]->ineq[j], dim);
			isl_seq_clr(lp->ineq[k]+1+dim*(i+1), dim*(set->n-i-1));
		}
	}
	return lp;
}

/* Given a facet "facet" of the convex hull of "set" and a facet "ridge"
 * of that facet, compute the other facet of the convex hull that contains
 * the ridge.
 *
 * We first transform the set such that the facet constraint becomes
 *
 *			x_1 >= 0
 *
 * I.e., the facet lies in
 *
 *			x_1 = 0
 *
 * and on that facet, the constraint that defines the ridge is
 *
 *			x_2 >= 0
 *
 * (This transformation is not strictly needed, all that is needed is
 * that the ridge contains the origin.)
 *
 * Since the ridge contains the origin, the cone of the convex hull
 * will be of the form
 *
 *			x_1 >= 0
 *			x_2 >= a x_1
 *
 * with this second constraint defining the new facet.
 * The constant a is obtained by settting x_1 in the cone of the
 * convex hull to 1 and minimizing x_2.
 * Now, each element in the cone of the convex hull is the sum
 * of elements in the cones of the basic sets.
 * If a_i is the dilation factor of basic set i, then the problem
 * we need to solve is
 *
 *			min \sum_i x_{i,2}
 *			st
 *				\sum_i x_{i,1} = 1
 *				    a_i   >= 0
 *				  [ a_i ]
 *				A [ x_i ] >= 0
 *
 * with
 *				    [  1  ]
 *				A_i [ x_i ] >= 0
 *
 * the constraints of each (transformed) basic set.
 * If a = n/d, then the constraint defining the new facet (in the transformed
 * space) is
 *
 *			-n x_1 + d x_2 >= 0
 *
 * In the original space, we need to take the same combination of the
 * corresponding constraints "facet" and "ridge".
 *
 * If a = -infty = "-1/0", then we just return the original facet constraint.
 * This means that the facet is unbounded, but has a bounded intersection
 * with the union of sets.
 */
static isl_int *wrap_facet(struct isl_ctx *ctx, struct isl_set *set,
	isl_int *facet, isl_int *ridge)
{
	int i;
	struct isl_mat *T = NULL;
	struct isl_basic_set *lp = NULL;
	struct isl_vec *obj;
	enum isl_lp_result res;
	isl_int num, den;
	unsigned dim;

	set = isl_set_copy(set);

	dim = 1 + set->dim;
	T = isl_mat_alloc(ctx, 3, 1 + set->dim);
	if (!T)
		goto error;
	isl_int_set_si(T->row[0][0], 1);
	isl_seq_clr(T->row[0]+1, set->dim);
	isl_seq_cpy(T->row[1], facet, 1+set->dim);
	isl_seq_cpy(T->row[2], ridge, 1+set->dim);
	T = isl_mat_right_inverse(ctx, T);
	set = isl_set_preimage(ctx, set, T);
	T = NULL;
	if (!set)
		goto error;
	lp = wrap_constraints(ctx, set);
	obj = isl_vec_alloc(ctx, dim*set->n);
	if (!obj)
		goto error;
	for (i = 0; i < set->n; ++i) {
		isl_seq_clr(obj->block.data+dim*i, 2);
		isl_int_set_si(obj->block.data[dim*i+2], 1);
		isl_seq_clr(obj->block.data+dim*i+3, dim-3);
	}
	isl_int_init(num);
	isl_int_init(den);
	res = isl_solve_lp((struct isl_basic_map *)lp, 0,
					obj->block.data, ctx->one, &num, &den);
	if (res == isl_lp_ok) {
		isl_int_neg(num, num);
		isl_seq_combine(facet, num, facet, den, ridge, dim);
	}
	isl_int_clear(num);
	isl_int_clear(den);
	isl_vec_free(ctx, obj);
	isl_basic_set_free(lp);
	isl_set_free(set);
	isl_assert(ctx, res == isl_lp_ok || res == isl_lp_unbounded, 
		   return NULL);
	return facet;
error:
	isl_basic_set_free(lp);
	isl_mat_free(ctx, T);
	isl_set_free(set);
	return NULL;
}

/* Given a set of d linearly independent bounding constraints of the
 * convex hull of "set", compute the constraint of a facet of "set".
 *
 * We first compute the intersection with the first bounding hyperplane
 * and remove the component corresponding to this hyperplane from
 * other bounds (in homogeneous space).
 * We then wrap around one of the remaining bounding constraints
 * and continue the process until all bounding constraints have been
 * taken into account.
 * The resulting linear combination of the bounding constraints will
 * correspond to a facet of the convex hull.
 */
static struct isl_mat *initial_facet_constraint(struct isl_ctx *ctx,
	struct isl_set *set, struct isl_mat *bounds)
{
	struct isl_set *slice = NULL;
	struct isl_basic_set *face = NULL;
	struct isl_mat *m, *U, *Q;
	int i;

	isl_assert(ctx, set->n > 0, goto error);
	isl_assert(ctx, bounds->n_row == set->dim, goto error);

	while (bounds->n_row > 1) {
		slice = isl_set_copy(set);
		slice = isl_set_add_equality(ctx, slice, bounds->row[0]);
		face = isl_set_affine_hull(slice);
		if (!face)
			goto error;
		if (face->n_eq == 1) {
			isl_basic_set_free(face);
			break;
		}
		m = isl_mat_alloc(ctx, 1 + face->n_eq, 1 + face->dim);
		if (!m)
			goto error;
		isl_int_set_si(m->row[0][0], 1);
		isl_seq_clr(m->row[0]+1, face->dim);
		for (i = 0; i < face->n_eq; ++i)
			isl_seq_cpy(m->row[1 + i], face->eq[i], 1 + face->dim);
		U = isl_mat_right_inverse(ctx, m);
		Q = isl_mat_right_inverse(ctx, isl_mat_copy(ctx, U));
		U = isl_mat_drop_cols(ctx, U, 1 + face->n_eq,
						face->dim - face->n_eq);
		Q = isl_mat_drop_rows(ctx, Q, 1 + face->n_eq,
						face->dim - face->n_eq);
		U = isl_mat_drop_cols(ctx, U, 0, 1);
		Q = isl_mat_drop_rows(ctx, Q, 0, 1);
		bounds = isl_mat_product(ctx, bounds, U);
		bounds = isl_mat_product(ctx, bounds, Q);
		while (isl_seq_first_non_zero(bounds->row[bounds->n_row-1],
					      bounds->n_col) == -1) {
			bounds->n_row--;
			isl_assert(ctx, bounds->n_row > 1, goto error);
		}
		if (!wrap_facet(ctx, set, bounds->row[0],
					  bounds->row[bounds->n_row-1]))
			goto error;
		isl_basic_set_free(face);
		bounds->n_row--;
	}
	return bounds;
error:
	isl_basic_set_free(face);
	isl_mat_free(ctx, bounds);
	return NULL;
}

/* Given the bounding constraint "c" of a facet of the convex hull of "set",
 * compute a hyperplane description of the facet, i.e., compute the facets
 * of the facet.
 *
 * We compute an affine transformation that transforms the constraint
 *
 *			  [ 1 ]
 *			c [ x ] = 0
 *
 * to the constraint
 *
 *			   z_1  = 0
 *
 * by computing the right inverse U of a matrix that starts with the rows
 *
 *			[ 1 0 ]
 *			[  c  ]
 *
 * Then
 *			[ 1 ]     [ 1 ]
 *			[ x ] = U [ z ]
 * and
 *			[ 1 ]     [ 1 ]
 *			[ z ] = Q [ x ]
 *
 * with Q = U^{-1}
 * Since z_1 is zero, we can drop this variable as well as the corresponding
 * column of U to obtain
 *
 *			[ 1 ]      [ 1  ]
 *			[ x ] = U' [ z' ]
 * and
 *			[ 1  ]      [ 1 ]
 *			[ z' ] = Q' [ x ]
 *
 * with Q' equal to Q, but without the corresponding row.
 * After computing the facets of the facet in the z' space,
 * we convert them back to the x space through Q.
 */
static struct isl_basic_set *compute_facet(struct isl_ctx *ctx,
	struct isl_set *set, isl_int *c)
{
	struct isl_mat *m, *U, *Q;
	struct isl_basic_set *facet;

	set = isl_set_copy(set);
	m = isl_mat_alloc(ctx, 2, 1 + set->dim);
	if (!m)
		goto error;
	isl_int_set_si(m->row[0][0], 1);
	isl_seq_clr(m->row[0]+1, set->dim);
	isl_seq_cpy(m->row[1], c, 1+set->dim);
	U = isl_mat_right_inverse(ctx, m);
	Q = isl_mat_right_inverse(ctx, isl_mat_copy(ctx, U));
	U = isl_mat_drop_cols(ctx, U, 1, 1);
	Q = isl_mat_drop_rows(ctx, Q, 1, 1);
	set = isl_set_preimage(ctx, set, U);
	facet = uset_convex_hull_wrap(set);
	facet = isl_basic_set_preimage(ctx, facet, Q);
	return facet;
error:
	isl_set_free(set);
	return NULL;
}

/* Given an initial facet constraint, compute the remaining facets.
 * We do this by running through all facets found so far and computing
 * the adjacent facets through wrapping, adding those facets that we
 * hadn't already found before.
 *
 * This function can still be significantly optimized by checking which of
 * the facets of the basic sets are also facets of the convex hull and
 * using all the facets so far to help in constructing the facets of the
 * facets
 * and/or
 * using the technique in section "3.1 Ridge Generation" of
 * "Extended Convex Hull" by Fukuda et al.
 */
static struct isl_basic_set *extend(struct isl_ctx *ctx, struct isl_set *set,
	struct isl_mat *initial)
{
	int i, j, f;
	int k;
	struct isl_basic_set *hull = NULL;
	struct isl_basic_set *facet = NULL;
	unsigned n_ineq;
	unsigned total;

	isl_assert(ctx, set->n > 0, goto error);

	n_ineq = 1;
	for (i = 0; i < set->n; ++i) {
		n_ineq += set->p[i]->n_eq;
		n_ineq += set->p[i]->n_ineq;
	}
	isl_assert(ctx, 1 + set->dim == initial->n_col, goto error);
	hull = isl_basic_set_alloc(ctx, 0, set->dim, 0, 0, n_ineq);
	hull = isl_basic_set_set_rational(hull);
	if (!hull)
		goto error;
	k = isl_basic_set_alloc_inequality(hull);
	if (k < 0)
		goto error;
	isl_seq_cpy(hull->ineq[k], initial->row[0], initial->n_col);
	for (i = 0; i < hull->n_ineq; ++i) {
		facet = compute_facet(ctx, set, hull->ineq[i]);
		if (!facet)
			goto error;
		if (facet->n_ineq + hull->n_ineq > n_ineq) {
			hull = isl_basic_set_extend(hull,
				hull->nparam, hull->dim, 0, 0, facet->n_ineq);
			n_ineq = hull->n_ineq + facet->n_ineq;
		}
		for (j = 0; j < facet->n_ineq; ++j) {
			k = isl_basic_set_alloc_inequality(hull);
			if (k < 0)
				goto error;
			isl_seq_cpy(hull->ineq[k], hull->ineq[i], 1+hull->dim);
			if (!wrap_facet(ctx, set, hull->ineq[k], facet->ineq[j]))
				goto error;
			for (f = 0; f < k; ++f)
				if (isl_seq_eq(hull->ineq[f], hull->ineq[k],
						1+hull->dim))
					break;
			if (f < k)
				isl_basic_set_free_inequality(hull, 1);
		}
		isl_basic_set_free(facet);
	}
	hull = isl_basic_set_simplify(hull);
	hull = isl_basic_set_finalize(hull);
	return hull;
error:
	isl_basic_set_free(facet);
	isl_basic_set_free(hull);
	return NULL;
}

/* Special case for computing the convex hull of a one dimensional set.
 * We simply collect the lower and upper bounds of each basic set
 * and the biggest of those.
 */
static struct isl_basic_set *convex_hull_1d(struct isl_ctx *ctx,
	struct isl_set *set)
{
	struct isl_mat *c = NULL;
	isl_int *lower = NULL;
	isl_int *upper = NULL;
	int i, j, k;
	isl_int a, b;
	struct isl_basic_set *hull;

	for (i = 0; i < set->n; ++i) {
		set->p[i] = isl_basic_set_simplify(set->p[i]);
		if (!set->p[i])
			goto error;
	}
	set = isl_set_remove_empty_parts(set);
	if (!set)
		goto error;
	isl_assert(ctx, set->n > 0, goto error);
	c = isl_mat_alloc(ctx, 2, 2);
	if (!c)
		goto error;

	if (set->p[0]->n_eq > 0) {
		isl_assert(ctx, set->p[0]->n_eq == 1, goto error);
		lower = c->row[0];
		upper = c->row[1];
		if (isl_int_is_pos(set->p[0]->eq[0][1])) {
			isl_seq_cpy(lower, set->p[0]->eq[0], 2);
			isl_seq_neg(upper, set->p[0]->eq[0], 2);
		} else {
			isl_seq_neg(lower, set->p[0]->eq[0], 2);
			isl_seq_cpy(upper, set->p[0]->eq[0], 2);
		}
	} else {
		for (j = 0; j < set->p[0]->n_ineq; ++j) {
			if (isl_int_is_pos(set->p[0]->ineq[j][1])) {
				lower = c->row[0];
				isl_seq_cpy(lower, set->p[0]->ineq[j], 2);
			} else {
				upper = c->row[1];
				isl_seq_cpy(upper, set->p[0]->ineq[j], 2);
			}
		}
	}

	isl_int_init(a);
	isl_int_init(b);
	for (i = 0; i < set->n; ++i) {
		struct isl_basic_set *bset = set->p[i];
		int has_lower = 0;
		int has_upper = 0;

		for (j = 0; j < bset->n_eq; ++j) {
			has_lower = 1;
			has_upper = 1;
			if (lower) {
				isl_int_mul(a, lower[0], bset->eq[j][1]);
				isl_int_mul(b, lower[1], bset->eq[j][0]);
				if (isl_int_lt(a, b) && isl_int_is_pos(bset->eq[j][1]))
					isl_seq_cpy(lower, bset->eq[j], 2);
				if (isl_int_gt(a, b) && isl_int_is_neg(bset->eq[j][1]))
					isl_seq_neg(lower, bset->eq[j], 2);
			}
			if (upper) {
				isl_int_mul(a, upper[0], bset->eq[j][1]);
				isl_int_mul(b, upper[1], bset->eq[j][0]);
				if (isl_int_lt(a, b) && isl_int_is_pos(bset->eq[j][1]))
					isl_seq_neg(upper, bset->eq[j], 2);
				if (isl_int_gt(a, b) && isl_int_is_neg(bset->eq[j][1]))
					isl_seq_cpy(upper, bset->eq[j], 2);
			}
		}
		for (j = 0; j < bset->n_ineq; ++j) {
			if (isl_int_is_pos(bset->ineq[j][1]))
				has_lower = 1;
			if (isl_int_is_neg(bset->ineq[j][1]))
				has_upper = 1;
			if (lower && isl_int_is_pos(bset->ineq[j][1])) {
				isl_int_mul(a, lower[0], bset->ineq[j][1]);
				isl_int_mul(b, lower[1], bset->ineq[j][0]);
				if (isl_int_lt(a, b))
					isl_seq_cpy(lower, bset->ineq[j], 2);
			}
			if (upper && isl_int_is_neg(bset->ineq[j][1])) {
				isl_int_mul(a, upper[0], bset->ineq[j][1]);
				isl_int_mul(b, upper[1], bset->ineq[j][0]);
				if (isl_int_gt(a, b))
					isl_seq_cpy(upper, bset->ineq[j], 2);
			}
		}
		if (!has_lower)
			lower = NULL;
		if (!has_upper)
			upper = NULL;
	}
	isl_int_clear(a);
	isl_int_clear(b);

	hull = isl_basic_set_alloc(ctx, 0, 1, 0, 0, 2);
	hull = isl_basic_set_set_rational(hull);
	if (!hull)
		goto error;
	if (lower) {
		k = isl_basic_set_alloc_inequality(hull);
		isl_seq_cpy(hull->ineq[k], lower, 2);
	}
	if (upper) {
		k = isl_basic_set_alloc_inequality(hull);
		isl_seq_cpy(hull->ineq[k], upper, 2);
	}
	hull = isl_basic_set_finalize(hull);
	isl_set_free(set);
	isl_mat_free(ctx, c);
	return hull;
error:
	isl_set_free(set);
	isl_mat_free(ctx, c);
	return NULL;
}

/* Project out final n dimensions using Fourier-Motzkin */
static struct isl_set *set_project_out(struct isl_ctx *ctx,
	struct isl_set *set, unsigned n)
{
	return isl_set_remove_dims(set, set->dim - n, n);
}

static struct isl_basic_set *convex_hull_0d(struct isl_set *set)
{
	struct isl_basic_set *convex_hull;

	if (!set)
		return NULL;

	if (isl_set_is_empty(set))
		convex_hull = isl_basic_set_empty(set->ctx, 0, 0);
	else
		convex_hull = isl_basic_set_universe(set->ctx, 0, 0);
	isl_set_free(set);
	return convex_hull;
}

/* Compute the convex hull of a pair of basic sets without any parameters or
 * integer divisions using Fourier-Motzkin elimination.
 * The convex hull is the set of all points that can be written as
 * the sum of points from both basic sets (in homogeneous coordinates).
 * We set up the constraints in a space with dimensions for each of
 * the three sets and then project out the dimensions corresponding
 * to the two original basic sets, retaining only those corresponding
 * to the convex hull.
 */
static struct isl_basic_set *convex_hull_pair(struct isl_basic_set *bset1,
	struct isl_basic_set *bset2)
{
	int i, j, k;
	struct isl_basic_set *bset[2];
	struct isl_basic_set *hull = NULL;
	unsigned dim;

	if (!bset1 || !bset2)
		goto error;

	dim = bset1->dim;
	hull = isl_basic_set_alloc(bset1->ctx, 0, 2 + 3 * bset1->dim, 0,
				1 + bset1->dim + bset1->n_eq + bset2->n_eq,
				2 + bset1->n_ineq + bset2->n_ineq);
	bset[0] = bset1;
	bset[1] = bset2;
	for (i = 0; i < 2; ++i) {
		for (j = 0; j < bset[i]->n_eq; ++j) {
			k = isl_basic_set_alloc_equality(hull);
			if (k < 0)
				goto error;
			isl_seq_clr(hull->eq[k], (i+1) * (1+dim));
			isl_seq_clr(hull->eq[k]+(i+2)*(1+dim), (1-i)*(1+dim));
			isl_seq_cpy(hull->eq[k]+(i+1)*(1+dim), bset[i]->eq[j],
					1+dim);
		}
		for (j = 0; j < bset[i]->n_ineq; ++j) {
			k = isl_basic_set_alloc_inequality(hull);
			if (k < 0)
				goto error;
			isl_seq_clr(hull->ineq[k], (i+1) * (1+dim));
			isl_seq_clr(hull->ineq[k]+(i+2)*(1+dim), (1-i)*(1+dim));
			isl_seq_cpy(hull->ineq[k]+(i+1)*(1+dim),
					bset[i]->ineq[j], 1+dim);
		}
		k = isl_basic_set_alloc_inequality(hull);
		if (k < 0)
			goto error;
		isl_seq_clr(hull->ineq[k], 1+hull->dim);
		isl_int_set_si(hull->ineq[k][(i+1)*(1+dim)], 1);
	}
	for (j = 0; j < 1+dim; ++j) {
		k = isl_basic_set_alloc_equality(hull);
		if (k < 0)
			goto error;
		isl_seq_clr(hull->eq[k], 1+hull->dim);
		isl_int_set_si(hull->eq[k][j], -1);
		isl_int_set_si(hull->eq[k][1+dim+j], 1);
		isl_int_set_si(hull->eq[k][2*(1+dim)+j], 1);
	}
	hull = isl_basic_set_set_rational(hull);
	hull = isl_basic_set_remove_dims(hull, dim, 2*(1+dim));
	hull = isl_basic_set_convex_hull(hull);
	isl_basic_set_free(bset1);
	isl_basic_set_free(bset2);
	return hull;
error:
	isl_basic_set_free(bset1);
	isl_basic_set_free(bset2);
	isl_basic_set_free(hull);
	return NULL;
}

/* Compute the convex hull of a set without any parameters or
 * integer divisions using Fourier-Motzkin elimination.
 * In each step, we combined two basic sets until only one
 * basic set is left.
 */
static struct isl_basic_set *uset_convex_hull_elim(struct isl_set *set)
{
	struct isl_basic_set *convex_hull = NULL;

	convex_hull = isl_set_copy_basic_set(set);
	set = isl_set_drop_basic_set(set, convex_hull);
	if (!set)
		goto error;
	while (set->n > 0) {
		struct isl_basic_set *t;
		t = isl_set_copy_basic_set(set);
		if (!t)
			goto error;
		set = isl_set_drop_basic_set(set, t);
		if (!set)
			goto error;
		convex_hull = convex_hull_pair(convex_hull, t);
	}
	isl_set_free(set);
	return convex_hull;
error:
	isl_set_free(set);
	isl_basic_set_free(convex_hull);
	return NULL;
}

static struct isl_basic_set *uset_convex_hull_wrap_with_bounds(
	struct isl_set *set, struct isl_mat *bounds)
{
	struct isl_basic_set *convex_hull = NULL;

	isl_assert(set->ctx, bounds->n_row == set->dim, goto error);
	bounds = initial_facet_constraint(set->ctx, set, bounds);
	if (!bounds)
		goto error;
	convex_hull = extend(set->ctx, set, bounds);
	isl_mat_free(set->ctx, bounds);
	isl_set_free(set);

	return convex_hull;
error:
	isl_set_free(set);
	return NULL;
}

/* Compute the convex hull of a set without any parameters or
 * integer divisions.  Depending on whether the set is bounded,
 * we pass control to the wrapping based convex hull or
 * the Fourier-Motzkin elimination based convex hull.
 * We also handle a few special cases before checking the boundedness.
 */
static struct isl_basic_set *uset_convex_hull(struct isl_set *set)
{
	int i;
	struct isl_basic_set *convex_hull = NULL;
	struct isl_mat *bounds;

	if (set->dim == 0)
		return convex_hull_0d(set);

	set = isl_set_set_rational(set);

	if (!set)
		goto error;
	for (i = 0; i < set->n; ++i) {
		set->p[i] = isl_basic_set_convex_hull(set->p[i]);
		if (!set->p[i])
			goto error;
	}
	set = isl_set_remove_empty_parts(set);
	if (!set)
		return NULL;
	if (set->n == 0) {
		convex_hull = isl_basic_set_empty(set->ctx, 0, 0);
		isl_set_free(set);
		return convex_hull;
	}
	if (set->n == 1) {
		convex_hull = isl_basic_set_copy(set->p[0]);
		isl_set_free(set);
		return convex_hull;
	}
	if (set->dim == 1)
		return convex_hull_1d(set->ctx, set);

	bounds = independent_bounds(set->ctx, set);
	if (!bounds)
		goto error;
	if (bounds->n_row == set->dim)
		return uset_convex_hull_wrap_with_bounds(set, bounds);
	isl_mat_free(set->ctx, bounds);

	return uset_convex_hull_elim(set);
error:
	isl_set_free(set);
	isl_basic_set_free(convex_hull);
	return NULL;
}

/* This is the core procedure, where "set" is a "pure" set, i.e.,
 * without parameters or divs and where the convex hull of set is
 * known to be full-dimensional.
 */
static struct isl_basic_set *uset_convex_hull_wrap(struct isl_set *set)
{
	int i;
	struct isl_basic_set *convex_hull = NULL;
	struct isl_mat *bounds;

	if (set->dim == 0) {
		convex_hull = isl_basic_set_universe(set->ctx, 0, 0);
		isl_set_free(set);
		convex_hull = isl_basic_set_set_rational(convex_hull);
		return convex_hull;
	}

	set = isl_set_set_rational(set);

	if (!set)
		goto error;
	for (i = 0; i < set->n; ++i) {
		set->p[i] = isl_basic_set_convex_hull(set->p[i]);
		if (!set->p[i])
			goto error;
	}
	set = isl_set_remove_empty_parts(set);
	if (!set)
		goto error;
	if (set->n == 1) {
		convex_hull = isl_basic_set_copy(set->p[0]);
		isl_set_free(set);
		return convex_hull;
	}
	if (set->dim == 1)
		return convex_hull_1d(set->ctx, set);

	bounds = independent_bounds(set->ctx, set);
	if (!bounds)
		goto error;
	return uset_convex_hull_wrap_with_bounds(set, bounds);
error:
	isl_set_free(set);
	return NULL;
}

/* Compute the convex hull of set "set" with affine hull "affine_hull",
 * We first remove the equalities (transforming the set), compute the
 * convex hull of the transformed set and then add the equalities back
 * (after performing the inverse transformation.
 */
static struct isl_basic_set *modulo_affine_hull(struct isl_ctx *ctx,
	struct isl_set *set, struct isl_basic_set *affine_hull)
{
	struct isl_mat *T;
	struct isl_mat *T2;
	struct isl_basic_set *dummy;
	struct isl_basic_set *convex_hull;

	dummy = isl_basic_set_remove_equalities(
			isl_basic_set_copy(affine_hull), &T, &T2);
	if (!dummy)
		goto error;
	isl_basic_set_free(dummy);
	set = isl_set_preimage(ctx, set, T);
	convex_hull = uset_convex_hull(set);
	convex_hull = isl_basic_set_preimage(ctx, convex_hull, T2);
	convex_hull = isl_basic_set_intersect(convex_hull, affine_hull);
	return convex_hull;
error:
	isl_basic_set_free(affine_hull);
	isl_set_free(set);
	return NULL;
}

/* Compute the convex hull of a map.
 *
 * The implementation was inspired by "Extended Convex Hull" by Fukuda et al.,
 * specifically, the wrapping of facets to obtain new facets.
 */
struct isl_basic_map *isl_map_convex_hull(struct isl_map *map)
{
	struct isl_basic_set *bset;
	struct isl_basic_set *affine_hull = NULL;
	struct isl_basic_map *convex_hull = NULL;
	struct isl_set *set = NULL;
	struct isl_ctx *ctx;

	if (!map)
		goto error;

	ctx = map->ctx;
	if (map->n == 0) {
		convex_hull = isl_basic_map_empty(ctx,
					    map->nparam, map->n_in, map->n_out);
		isl_map_free(map);
		return convex_hull;
	}

	set = isl_map_underlying_set(isl_map_copy(map));
	if (!set)
		goto error;

	affine_hull = isl_set_affine_hull(isl_set_copy(set));
	if (!affine_hull)
		goto error;
	if (affine_hull->n_eq != 0)
		bset = modulo_affine_hull(ctx, set, affine_hull);
	else {
		isl_basic_set_free(affine_hull);
		bset = uset_convex_hull(set);
	}

	convex_hull = isl_basic_map_overlying_set(bset,
			isl_basic_map_copy(map->p[0]));

	isl_map_free(map);
	F_CLR(convex_hull, ISL_BASIC_MAP_RATIONAL);
	return convex_hull;
error:
	isl_set_free(set);
	isl_map_free(map);
	return NULL;
}

struct isl_basic_set *isl_set_convex_hull(struct isl_set *set)
{
	return (struct isl_basic_set *)
		isl_map_convex_hull((struct isl_map *)set);
}
