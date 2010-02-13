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
 
/*
 * The transitive closure implementation is based on the paper
 * "Computing the Transitive Closure of a Union of Affine Integer
 * Tuple Relations" by Anna Beletska, Denis Barthou, Wlodzimierz Bielecki and
 * Albert Cohen.
 */

/* Given a union of translations (uniform dependences), return a matrix
 * with as many rows as there are disjuncts in the union and as many
 * columns as there are input dimensions (which should be equal to
 * the number of output dimensions).
 * Each row contains the translation performed by the corresponding disjunct.
 * If "map" turns out not to be a union of translations, then the contents
 * of the returned matrix are undefined and *ok is set to 0.
 */
static __isl_give isl_mat *extract_steps(__isl_keep isl_map *map, int *ok)
{
	int i, j;
	struct isl_mat *steps;
	unsigned dim = isl_map_dim(map, isl_dim_in);

	*ok = 1;

	steps = isl_mat_alloc(map->ctx, map->n, dim);
	if (!steps)
		return NULL;

	for (i = 0; i < map->n; ++i) {
		struct isl_basic_set *delta;

		delta = isl_basic_map_deltas(isl_basic_map_copy(map->p[i]));

		for (j = 0; j < dim; ++j) {
			int fixed;

			fixed = isl_basic_set_fast_dim_is_fixed(delta, j,
							    &steps->row[i][j]);
			if (fixed < 0) {
				isl_basic_set_free(delta);
				goto error;
			}
			if (!fixed)
				break;
		}

		isl_basic_set_free(delta);

		if (j < dim)
			break;
	}

	if (i < map->n)
		*ok = 0;

	return steps;
error:
	isl_mat_free(steps);
	return NULL;
}

/* Given a set of n offsets v_i (the rows of "steps"), construct a relation
 * of the given dimension specification that maps a element x to any
 * element that can be reached by taking a positive number of steps
 * along any of the offsets, where the number of steps k is equal to
 * parameter "param".  That is, construct
 *
 * { [x] -> [y] : exists k_i >= 0, y = x + \sum_i k_i v_i, k = \sum_i k_i > 0 }
 *
 */
static __isl_give isl_map *path_along_steps(__isl_take isl_dim *dim,
	__isl_keep isl_mat *steps, unsigned param)
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

	path = isl_basic_map_alloc_dim(isl_dim_copy(dim), n, d + 1, n + 1);

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
		for (j = 0; j < n; ++j)
			isl_int_set(path->eq[k][1 + nparam + 2 * d + j],
				    steps->row[j][i]);
	}

	k = isl_basic_map_alloc_equality(path);
	if (k < 0)
		goto error;
	isl_seq_clr(path->eq[k], 1 + isl_basic_map_total_dim(path));
	isl_int_set_si(path->eq[k][1 + param], -1);
	for (j = 0; j < n; ++j)
		isl_int_set_si(path->eq[k][1 + nparam + 2 * d + j], 1);

	for (i = 0; i < n; ++i) {
		k = isl_basic_map_alloc_inequality(path);
		if (k < 0)
			goto error;
		isl_seq_clr(path->ineq[k], 1 + isl_basic_map_total_dim(path));
		isl_int_set_si(path->ineq[k][1 + nparam + 2 * d + i], 1);
	}

	k = isl_basic_map_alloc_inequality(path);
	if (k < 0)
		goto error;
	isl_seq_clr(path->ineq[k], 1 + isl_basic_map_total_dim(path));
	isl_int_set_si(path->ineq[k][1 + param], 1);
	isl_int_set_si(path->ineq[k][0], -1);

	isl_dim_free(dim);

	path = isl_basic_map_simplify(path);
	path = isl_basic_map_finalize(path);
	return isl_map_from_basic_map(path);
error:
	isl_dim_free(dim);
	isl_basic_map_free(path);
	return NULL;
}

/* Check whether "path" is acyclic.
 * That is, check whether
 *
 *	{ d | d = y - x and (x,y) in path }
 *
 * does not contain the origin.
 */
static int is_acyclic(__isl_take isl_map *path)
{
	int i;
	int acyclic;
	unsigned dim;
	struct isl_set *delta;

	delta = isl_map_deltas(path);
	dim = isl_set_dim(delta, isl_dim_set);
	for (i = 0; i < dim; ++i)
		delta = isl_set_fix_si(delta, isl_dim_set, i, 0);

	acyclic = isl_set_is_empty(delta);
	isl_set_free(delta);

	return acyclic;
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
	__isl_take isl_map *path, unsigned param, int project)
{
	isl_map *test;
	int exact;

	if (project) {
		project = is_acyclic(path);
		if (project < 0)
			goto error;
	} else
		isl_map_free(path);

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
	struct isl_mat *steps = NULL;
	struct isl_set *domain = NULL;
	struct isl_set *range = NULL;
	struct isl_map *app = NULL;
	struct isl_map *path = NULL;
	int ok;

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

	domain = isl_map_domain(isl_map_copy(map));
	domain = isl_set_coalesce(domain);
	range = isl_map_range(isl_map_copy(map));
	range = isl_set_coalesce(range);
	app = isl_map_from_domain_and_range(isl_set_copy(domain),
					    isl_set_copy(range));

	steps = extract_steps(map, &ok);
	if (!ok)
		goto not_exact;

	path = path_along_steps(isl_map_get_dim(map), steps, param);
	app = isl_map_intersect(app, isl_map_copy(path));

	if (exact &&
	    (*exact = check_exactness(isl_map_copy(map), isl_map_copy(app),
				  isl_map_copy(path), param, project)) < 0)
		goto error;

	if (0) {
not_exact:
		if (exact)
			*exact = 0;
	}
	isl_set_free(domain);
	isl_set_free(range);
	isl_mat_free(steps);
	isl_map_free(path);
	isl_map_free(map);
	return app;
error:
	isl_set_free(domain);
	isl_set_free(range);
	isl_mat_free(steps);
	isl_map_free(path);
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
