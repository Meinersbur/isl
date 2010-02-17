/*
 * Copyright 2010      INRIA Saclay
 *
 * Use of this software is governed by the GNU LGPLv2.1 license
 *
 * Written by Sven Verdoolaege, INRIA Saclay - Ile-de-France,
 * Parc Club Orsay Universite, ZAC des vignes, 4 rue Jacques Monod,
 * 91893 Orsay, France 
 */

#include "isl_map.h"
#include "isl_map_private.h"
#include "isl_seq.h"
 
/*
 * The transitive closure implementation is based on the paper
 * "Computing the Transitive Closure of a Union of Affine Integer
 * Tuple Relations" by Anna Beletska, Denis Barthou, Wlodzimierz Bielecki and
 * Albert Cohen.
 */

/* Given a set of n offsets v_i (the rows of "steps"), construct a relation
 * of the given dimension specification (Z^{n+1} -> Z^{n+1})
 * that maps an element x to any element that can be reached
 * by taking a non-negative number of steps along any of
 * the extended offsets v'_i = [v_i 1].
 * That is, construct
 *
 * { [x] -> [y] : exists k_i >= 0, y = x + \sum_i k_i v'_i }
 *
 * For any element in this relation, the number of steps taken
 * is equal to the difference in the final coordinates.
 */
static __isl_give isl_map *path_along_steps(__isl_take isl_dim *dim,
	__isl_keep isl_mat *steps)
{
	int i, j, k;
	struct isl_basic_map *path = NULL;
	unsigned d;
	unsigned n;
	unsigned nparam;

	if (!dim || !steps)
		goto error;

	d = isl_dim_size(dim, isl_dim_in);
	n = steps->n_row;
	nparam = isl_dim_size(dim, isl_dim_param);

	path = isl_basic_map_alloc_dim(isl_dim_copy(dim), n, d, n);

	for (i = 0; i < n; ++i) {
		k = isl_basic_map_alloc_div(path);
		if (k < 0)
			goto error;
		isl_assert(steps->ctx, i == k, goto error);
		isl_int_set_si(path->div[k][0], 0);
	}

	for (i = 0; i < d; ++i) {
		k = isl_basic_map_alloc_equality(path);
		if (k < 0)
			goto error;
		isl_seq_clr(path->eq[k], 1 + isl_basic_map_total_dim(path));
		isl_int_set_si(path->eq[k][1 + nparam + i], 1);
		isl_int_set_si(path->eq[k][1 + nparam + d + i], -1);
		if (i == d - 1)
			for (j = 0; j < n; ++j)
				isl_int_set_si(path->eq[k][1 + nparam + 2 * d + j], 1);
		else
			for (j = 0; j < n; ++j)
				isl_int_set(path->eq[k][1 + nparam + 2 * d + j],
					    steps->row[j][i]);
	}

	for (i = 0; i < n; ++i) {
		k = isl_basic_map_alloc_inequality(path);
		if (k < 0)
			goto error;
		isl_seq_clr(path->ineq[k], 1 + isl_basic_map_total_dim(path));
		isl_int_set_si(path->ineq[k][1 + nparam + 2 * d + i], 1);
	}

	isl_dim_free(dim);

	path = isl_basic_map_simplify(path);
	path = isl_basic_map_finalize(path);
	return isl_map_from_basic_map(path);
error:
	isl_dim_free(dim);
	isl_basic_map_free(path);
	return NULL;
}

#define IMPURE		0
#define PURE_PARAM	1
#define PURE_VAR	2

/* Return PURE_PARAM if only the coefficients of the parameters are non-zero.
 * Return PURE_VAR if only the coefficients of the set variables are non-zero.
 * Return IMPURE otherwise.
 */
static int purity(__isl_keep isl_basic_set *bset, isl_int *c)
{
	unsigned d;
	unsigned n_div;
	unsigned nparam;

	n_div = isl_basic_set_dim(bset, isl_dim_div);
	d = isl_basic_set_dim(bset, isl_dim_set);
	nparam = isl_basic_set_dim(bset, isl_dim_param);

	if (isl_seq_first_non_zero(c + 1 + nparam + d, n_div) != -1)
		return IMPURE;
	if (isl_seq_first_non_zero(c + 1, nparam) == -1)
		return PURE_VAR;
	if (isl_seq_first_non_zero(c + 1 + nparam, d) == -1)
		return PURE_PARAM;
	return IMPURE;
}

/* Given a set of offsets "delta", construct a relation of the
 * given dimension specification (Z^{n+1} -> Z^{n+1}) that
 * is an overapproximation of the relations that
 * maps an element x to any element that can be reached
 * by taking a non-negative number of steps along any of
 * the elements in "delta".
 * That is, construct an approximation of
 *
 *	{ [x] -> [y] : exists f \in \delta, k \in Z :
 *					y = x + k [f, 1] and k >= 0 }
 *
 * For any element in this relation, the number of steps taken
 * is equal to the difference in the final coordinates.
 *
 * In particular, let delta be defined as
 *
 *	\delta = [p] -> { [x] : A x + a >= and B p + b >= 0 and
 *				C x + C'p + c >= 0 }
 *
 * then the relation is constructed as
 *
 *	{ [x] -> [y] : exists [f, k] \in Z^{n+1} : y = x + f and
 *			A f + k a >= 0 and B p + b >= 0 and k >= 1 }
 *	union { [x] -> [x] }
 *
 * Existentially quantified variables in \delta are currently ignored.
 * This is safe, but leads to an additional overapproximation.
 */
static __isl_give isl_map *path_along_delta(__isl_take isl_dim *dim,
	__isl_take isl_basic_set *delta)
{
	isl_basic_map *path = NULL;
	unsigned d;
	unsigned n_div;
	unsigned nparam;
	unsigned off;
	int i, k;

	if (!delta)
		goto error;
	n_div = isl_basic_set_dim(delta, isl_dim_div);
	d = isl_basic_set_dim(delta, isl_dim_set);
	nparam = isl_basic_set_dim(delta, isl_dim_param);
	path = isl_basic_map_alloc_dim(isl_dim_copy(dim), n_div + d + 1,
			d + 1 + delta->n_eq, delta->n_ineq + 1);
	off = 1 + nparam + 2 * (d + 1) + n_div;

	for (i = 0; i < n_div + d + 1; ++i) {
		k = isl_basic_map_alloc_div(path);
		if (k < 0)
			goto error;
		isl_int_set_si(path->div[k][0], 0);
	}

	for (i = 0; i < d + 1; ++i) {
		k = isl_basic_map_alloc_equality(path);
		if (k < 0)
			goto error;
		isl_seq_clr(path->eq[k], 1 + isl_basic_map_total_dim(path));
		isl_int_set_si(path->eq[k][1 + nparam + i], 1);
		isl_int_set_si(path->eq[k][1 + nparam + d + 1 + i], -1);
		isl_int_set_si(path->eq[k][off + i], 1);
	}

	for (i = 0; i < delta->n_eq; ++i) {
		int p = purity(delta, delta->eq[i]);
		if (p == IMPURE)
			continue;
		k = isl_basic_map_alloc_equality(path);
		if (k < 0)
			goto error;
		isl_seq_clr(path->eq[k], 1 + isl_basic_map_total_dim(path));
		if (p == PURE_VAR) {
			isl_seq_cpy(path->eq[k] + off,
				    delta->eq[i] + 1 + nparam, d);
			isl_int_set(path->eq[k][off + d], delta->eq[i][0]);
		} else
			isl_seq_cpy(path->eq[k], delta->eq[i], 1 + nparam);
	}

	for (i = 0; i < delta->n_ineq; ++i) {
		int p = purity(delta, delta->ineq[i]);
		if (p == IMPURE)
			continue;
		k = isl_basic_map_alloc_inequality(path);
		if (k < 0)
			goto error;
		isl_seq_clr(path->ineq[k], 1 + isl_basic_map_total_dim(path));
		if (p == PURE_VAR) {
			isl_seq_cpy(path->ineq[k] + off,
				    delta->ineq[i] + 1 + nparam, d);
			isl_int_set(path->ineq[k][off + d], delta->ineq[i][0]);
		} else
			isl_seq_cpy(path->ineq[k], delta->ineq[i], 1 + nparam);
	}

	k = isl_basic_map_alloc_inequality(path);
	if (k < 0)
		goto error;
	isl_seq_clr(path->ineq[k], 1 + isl_basic_map_total_dim(path));
	isl_int_set_si(path->ineq[k][0], -1);
	isl_int_set_si(path->ineq[k][off + d], 1);
			
	isl_basic_set_free(delta);
	path = isl_basic_map_finalize(path);
	return isl_basic_map_union(path,
				isl_basic_map_identity(isl_dim_domain(dim)));
error:
	isl_dim_free(dim);
	isl_basic_set_free(delta);
	isl_basic_map_free(path);
	return NULL;
}

/* Given a dimenion specification Z^{n+1} -> Z^{n+1} and a parameter "param",
 * construct a map that equates the parameter to the difference
 * in the final coordinates and imposes that this difference is positive.
 * That is, construct
 *
 *	{ [x,x_s] -> [y,y_s] : k = y_s - x_s > 0 }
 */
static __isl_give isl_map *equate_parameter_to_length(__isl_take isl_dim *dim,
	unsigned param)
{
	struct isl_basic_map *bmap;
	unsigned d;
	unsigned nparam;
	int k;

	d = isl_dim_size(dim, isl_dim_in);
	nparam = isl_dim_size(dim, isl_dim_param);
	bmap = isl_basic_map_alloc_dim(dim, 0, 1, 1);
	k = isl_basic_map_alloc_equality(bmap);
	if (k < 0)
		goto error;
	isl_seq_clr(bmap->eq[k], 1 + isl_basic_map_total_dim(bmap));
	isl_int_set_si(bmap->eq[k][1 + param], -1);
	isl_int_set_si(bmap->eq[k][1 + nparam + d - 1], -1);
	isl_int_set_si(bmap->eq[k][1 + nparam + d + d - 1], 1);

	k = isl_basic_map_alloc_inequality(bmap);
	if (k < 0)
		goto error;
	isl_seq_clr(bmap->ineq[k], 1 + isl_basic_map_total_dim(bmap));
	isl_int_set_si(bmap->ineq[k][1 + param], 1);
	isl_int_set_si(bmap->ineq[k][0], -1);

	bmap = isl_basic_map_finalize(bmap);
	return isl_map_from_basic_map(bmap);
error:
	isl_basic_map_free(bmap);
	return NULL;
}

/* Check whether "path" is acyclic, where the last coordinates of domain
 * and range of path encode the number of steps taken.
 * That is, check whether
 *
 *	{ d | d = y - x and (x,y) in path }
 *
 * does not contain any element with positive last coordinate (positive length)
 * and zero remaining coordinates (cycle).
 */
static int is_acyclic(__isl_take isl_map *path)
{
	int i;
	int acyclic;
	unsigned dim;
	struct isl_set *delta;

	delta = isl_map_deltas(path);
	dim = isl_set_dim(delta, isl_dim_set);
	for (i = 0; i < dim; ++i) {
		if (i == dim -1)
			delta = isl_set_lower_bound_si(delta, isl_dim_set, i, 1);
		else
			delta = isl_set_fix_si(delta, isl_dim_set, i, 0);
	}

	acyclic = isl_set_is_empty(delta);
	isl_set_free(delta);

	return acyclic;
}

/* Given a union of basic maps R = \cup_i R_i \subseteq D \times D
 * and a dimension specification (Z^{n+1} -> Z^{n+1}),
 * construct a map that is an overapproximation of the map
 * that takes an element from the space D \times Z to another
 * element from the same space, such that the first n coordinates of the
 * difference between them is a sum of differences between images
 * and pre-images in one of the R_i and such that the last coordinate
 * is equal to the number of steps taken.
 * That is, let
 *
 *	\Delta_i = { y - x | (x, y) in R_i }
 *
 * then the constructed map is an overapproximation of
 *
 *	{ (x) -> (x + d) | \exists k_i >= 0, \delta_i \in \Delta_i :
 *				d = (\sum_i k_i \delta_i, \sum_i k_i) }
 *
 * The elements of the singleton \Delta_i's are collected as the
 * rows of the steps matrix.  For all these \Delta_i's together,
 * a single path is constructed.
 * For each of the other \Delta_i's, we compute an overapproximation
 * of the paths along elements of \Delta_i.
 * Since each of these paths performs an addition, composition is
 * symmetric and we can simply compose all resulting paths in any order.
 */
static __isl_give isl_map *construct_extended_path(__isl_take isl_dim *dim,
	__isl_keep isl_map *map, int *project)
{
	struct isl_mat *steps = NULL;
	struct isl_map *path = NULL;
	unsigned d;
	int i, j, n;

	d = isl_map_dim(map, isl_dim_in);

	path = isl_map_identity(isl_dim_domain(isl_dim_copy(dim)));

	steps = isl_mat_alloc(map->ctx, map->n, d);
	if (!steps)
		goto error;

	n = 0;
	for (i = 0; i < map->n; ++i) {
		struct isl_basic_set *delta;

		delta = isl_basic_map_deltas(isl_basic_map_copy(map->p[i]));

		for (j = 0; j < d; ++j) {
			int fixed;

			fixed = isl_basic_set_fast_dim_is_fixed(delta, j,
							    &steps->row[n][j]);
			if (fixed < 0) {
				isl_basic_set_free(delta);
				goto error;
			}
			if (!fixed)
				break;
		}


		if (j < d) {
			path = isl_map_apply_range(path,
				path_along_delta(isl_dim_copy(dim), delta));
		} else {
			isl_basic_set_free(delta);
			++n;
		}
	}

	if (n > 0) {
		steps->n_row = n;
		path = isl_map_apply_range(path,
				path_along_steps(isl_dim_copy(dim), steps));
	}

	if (project && *project) {
		*project = is_acyclic(isl_map_copy(path));
		if (*project < 0)
			goto error;
	}

	isl_dim_free(dim);
	isl_mat_free(steps);
	return path;
error:
	isl_dim_free(dim);
	isl_mat_free(steps);
	isl_map_free(path);
	return NULL;
}

/* Given a union of basic maps R = \cup_i R_i \subseteq D \times D
 * and a dimension specification (Z^{n+1} -> Z^{n+1}),
 * construct a map that is an overapproximation of the map
 * that takes an element from the dom R \times Z to an
 * element from ran R \times Z, such that the first n coordinates of the
 * difference between them is a sum of differences between images
 * and pre-images in one of the R_i and such that the last coordinate
 * is equal to the number of steps taken.
 * That is, let
 *
 *	\Delta_i = { y - x | (x, y) in R_i }
 *
 * then the constructed map is an overapproximation of
 *
 *	{ (x) -> (x + d) | \exists k_i >= 0, \delta_i \in \Delta_i :
 *				d = (\sum_i k_i \delta_i, \sum_i k_i) and
 *				x in dom R and x + d in ran R}
 */
static __isl_give isl_map *construct_extended_power(__isl_take isl_dim *dim,
	__isl_keep isl_map *map, int *project)
{
	struct isl_set *domain = NULL;
	struct isl_set *range = NULL;
	struct isl_map *app = NULL;
	struct isl_map *path = NULL;

	domain = isl_map_domain(isl_map_copy(map));
	domain = isl_set_coalesce(domain);
	range = isl_map_range(isl_map_copy(map));
	range = isl_set_coalesce(range);
	app = isl_map_from_domain_and_range(domain, range);
	app = isl_map_add(app, isl_dim_in, 1);
	app = isl_map_add(app, isl_dim_out, 1);

	path = construct_extended_path(dim, map, project);
	app = isl_map_intersect(app, path);

	return app;
}

/* Given a union of basic maps R = \cup_i R_i \subseteq D \times D,
 * construct a map that is an overapproximation of the map
 * that takes an element from the space D to another
 * element from the same space, such that the difference between
 * them is a strictly positive sum of differences between images
 * and pre-images in one of the R_i.
 * The number of differences in the sum is equated to parameter "param".
 * That is, let
 *
 *	\Delta_i = { y - x | (x, y) in R_i }
 *
 * then the constructed map is an overapproximation of
 *
 *	{ (x) -> (x + d) | \exists k_i >= 0, \delta_i \in \Delta_i :
 *				d = \sum_i k_i \delta_i and k = \sum_i k_i > 0 }
 *
 * We first construct an extended mapping with an extra coordinate
 * that indicates the number of steps taken.  In particular,
 * the difference in the last coordinate is equal to the number
 * of steps taken to move from a domain element to the corresponding
 * image element(s).
 * In the final step, this difference is equated to the parameter "param"
 * and made positive.  The extra coordinates are subsequently projected out.
 */
static __isl_give isl_map *construct_power(__isl_keep isl_map *map,
	unsigned param, int *project)
{
	struct isl_map *app = NULL;
	struct isl_map *diff;
	struct isl_dim *dim = NULL;
	unsigned d;

	if (!map)
		return NULL;

	dim = isl_map_get_dim(map);

	d = isl_dim_size(dim, isl_dim_in);
	dim = isl_dim_add(dim, isl_dim_in, 1);
	dim = isl_dim_add(dim, isl_dim_out, 1);

	app = construct_extended_power(isl_dim_copy(dim), map, project);

	diff = equate_parameter_to_length(dim, param);
	app = isl_map_intersect(app, diff);
	app = isl_map_project_out(app, isl_dim_in, d, 1);
	app = isl_map_project_out(app, isl_dim_out, d, 1);

	return app;
}

/* Shift variable at position "pos" up by one.
 * That is, replace the corresponding variable v by v - 1.
 */
static __isl_give isl_basic_map *basic_map_shift_pos(
	__isl_take isl_basic_map *bmap, unsigned pos)
{
	int i;

	bmap = isl_basic_map_cow(bmap);
	if (!bmap)
		return NULL;

	for (i = 0; i < bmap->n_eq; ++i)
		isl_int_sub(bmap->eq[i][0], bmap->eq[i][0], bmap->eq[i][pos]);

	for (i = 0; i < bmap->n_ineq; ++i)
		isl_int_sub(bmap->ineq[i][0],
				bmap->ineq[i][0], bmap->ineq[i][pos]);

	for (i = 0; i < bmap->n_div; ++i) {
		if (isl_int_is_zero(bmap->div[i][0]))
			continue;
		isl_int_sub(bmap->div[i][1],
				bmap->div[i][1], bmap->div[i][1 + pos]);
	}

	return bmap;
}

/* Shift variable at position "pos" up by one.
 * That is, replace the corresponding variable v by v - 1.
 */
static __isl_give isl_map *map_shift_pos(__isl_take isl_map *map, unsigned pos)
{
	int i;

	map = isl_map_cow(map);
	if (!map)
		return NULL;

	for (i = 0; i < map->n; ++i) {
		map->p[i] = basic_map_shift_pos(map->p[i], pos);
		if (!map->p[i])
			goto error;
	}
	ISL_F_CLR(map, ISL_MAP_NORMALIZED);
	return map;
error:
	isl_map_free(map);
	return NULL;
}

/* Check whether the overapproximation of the power of "map" is exactly
 * the power of "map".  Let R be "map" and A_k the overapproximation.
 * The approximation is exact if
 *
 *	A_1 = R
 *	A_k = A_{k-1} \circ R			k >= 2
 *
 * Since A_k is known to be an overapproximation, we only need to check
 *
 *	A_1 \subset R
 *	A_k \subset A_{k-1} \circ R		k >= 2
 *
 */
static int check_power_exactness(__isl_take isl_map *map,
	__isl_take isl_map *app, unsigned param)
{
	int exact;
	isl_map *app_1;
	isl_map *app_2;

	app_1 = isl_map_fix_si(isl_map_copy(app), isl_dim_param, param, 1);

	exact = isl_map_is_subset(app_1, map);
	isl_map_free(app_1);

	if (!exact || exact < 0) {
		isl_map_free(app);
		isl_map_free(map);
		return exact;
	}

	app_2 = isl_map_lower_bound_si(isl_map_copy(app),
					isl_dim_param, param, 2);
	app_1 = map_shift_pos(app, 1 + param);
	app_1 = isl_map_apply_range(map, app_1);

	exact = isl_map_is_subset(app_2, app_1);

	isl_map_free(app_1);
	isl_map_free(app_2);

	return exact;
}

/* Check whether the overapproximation of the power of "map" is exactly
 * the power of "map", possibly after projecting out the power (if "project"
 * is set).
 *
 * If "project" is set and if "steps" can only result in acyclic paths,
 * then we check
 *
 *	A = R \cup (A \circ R)
 *
 * where A is the overapproximation with the power projected out, i.e.,
 * an overapproximation of the transitive closure.
 * More specifically, since A is known to be an overapproximation, we check
 *
 *	A \subset R \cup (A \circ R)
 *
 * Otherwise, we check if the power is exact.
 */
static int check_exactness(__isl_take isl_map *map, __isl_take isl_map *app,
	unsigned param, int project)
{
	isl_map *test;
	int exact;

	if (!project)
		return check_power_exactness(map, app, param);

	map = isl_map_project_out(map, isl_dim_param, param, 1);
	app = isl_map_project_out(app, isl_dim_param, param, 1);

	test = isl_map_apply_range(isl_map_copy(map), isl_map_copy(app));
	test = isl_map_union(test, isl_map_copy(map));

	exact = isl_map_is_subset(app, test);

	isl_map_free(app);
	isl_map_free(test);

	isl_map_free(map);

	return exact;
error:
	isl_map_free(app);
	isl_map_free(map);
	return -1;
}

/* Compute the positive powers of "map", or an overapproximation.
 * The power is given by parameter "param".  If the result is exact,
 * then *exact is set to 1.
 * If project is set, then we are actually interested in the transitive
 * closure, so we can use a more relaxed exactness check.
 */
static __isl_give isl_map *map_power(__isl_take isl_map *map, unsigned param,
	int *exact, int project)
{
	struct isl_map *app = NULL;

	if (exact)
		*exact = 1;

	map = isl_map_remove_empty_parts(map);
	if (!map)
		return NULL;

	if (isl_map_fast_is_empty(map))
		return map;

	isl_assert(map->ctx, param < isl_map_dim(map, isl_dim_param), goto error);
	isl_assert(map->ctx,
		isl_map_dim(map, isl_dim_in) == isl_map_dim(map, isl_dim_out),
		goto error);

	app = construct_power(map, param, exact ? &project : NULL);

	if (exact &&
	    (*exact = check_exactness(isl_map_copy(map), isl_map_copy(app),
				      param, project)) < 0)
		goto error;

	isl_map_free(map);
	return app;
error:
	isl_map_free(map);
	isl_map_free(app);
	return NULL;
}

/* Compute the positive powers of "map", or an overapproximation.
 * The power is given by parameter "param".  If the result is exact,
 * then *exact is set to 1.
 */
__isl_give isl_map *isl_map_power(__isl_take isl_map *map, unsigned param,
	int *exact)
{
	return map_power(map, param, exact, 0);
}

/* Compute the transitive closure  of "map", or an overapproximation.
 * If the result is exact, then *exact is set to 1.
 * Simply compute the powers of map and then project out the parameter
 * describing the power.
 */
__isl_give isl_map *isl_map_transitive_closure(__isl_take isl_map *map,
	int *exact)
{
	unsigned param;

	if (!map)
		goto error;

	param = isl_map_dim(map, isl_dim_param);
	map = isl_map_add(map, isl_dim_param, 1);
	map = map_power(map, param, exact, 1);
	map = isl_map_project_out(map, isl_dim_param, param, 1);

	return map;
error:
	isl_map_free(map);
	return NULL;
}
