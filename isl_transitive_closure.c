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
 * If strict is non-negative, then at least one step should be taken
 * along the given offset v_strict, i.e., k_strict > 0.
 */
static __isl_give isl_map *path_along_steps(__isl_take isl_dim *dim,
	__isl_keep isl_mat *steps, unsigned param, int strict)
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
		if (i == strict)
			isl_int_set_si(path->ineq[k][0], -1);
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

/* Check whether the overapproximation of the power of "map" is exactly
 * the power of "map".  In particular, for each path of a given length
 * that starts of in domain or range and ends up in the range,
 * check whether there is at least one path of the same length
 * with a valid first segment, i.e., one in "map".
 * If "project" is set, then we drop the condition that the lengths
 * should be the same.
 *
 * "domain" and "range" are the domain and range of "map"
 * "steps" represents the translations of "map"
 * "path" is a path along "steps"
 *
 * "domain", "range", "steps" and "path" have been precomputed by the calling
 * function.
 */
static int check_path_exactness(__isl_take isl_map *map,
	__isl_take isl_set *domain, __isl_take isl_set *range,
	__isl_take isl_map *path, __isl_keep isl_mat *steps, unsigned param,
	int project)
{
	isl_map *test;
	int ok;
	int i;

	if (!map)
		goto error;

	test = isl_map_empty(isl_map_get_dim(map));
	for (i = 0; i < map->n; ++i) {
		struct isl_map *path_i;
		struct isl_set *dom_i;
		path_i = path_along_steps(isl_map_get_dim(map), steps, param, i);
		dom_i = isl_set_from_basic_set(
			isl_basic_map_domain(isl_basic_map_copy(map->p[i])));
		path_i = isl_map_intersect_domain(path_i, dom_i);
		test = isl_map_union(test, path_i);
	}
	isl_map_free(map);
	test = isl_map_intersect_range(test, isl_set_copy(range));

	domain = isl_set_union(domain, isl_set_copy(range));
	path = isl_map_intersect_domain(path, domain);
	path = isl_map_intersect_range(path, range);

	if (project) {
		path = isl_map_project_out(path, isl_dim_param, param, 1);
		test = isl_map_project_out(test, isl_dim_param, param, 1);
	}

	ok = isl_map_is_subset(path, test);

	isl_map_free(path);
	isl_map_free(test);

	return ok;
error:
	isl_map_free(map);
	isl_set_free(domain);
	isl_set_free(range);
	isl_map_free(path);
	return -1;
}

/* Check whether any path of length at least one along "steps" is acyclic.
 * That is, check whether
 *
 *	\sum_i k_i \delta_i = 0
 *	\sum_i k_i >= 1
 *	k_i >= 0
 *
 * with \delta_i the rows of "steps", is infeasible.
 */
static int is_acyclic(__isl_keep isl_mat *steps)
{
	int acyclic;
	int i, j, k;
	struct isl_basic_set *test;

	if (!steps)
		return -1;

	test = isl_basic_set_alloc(steps->ctx, 0, steps->n_row, 0,
					steps->n_col, steps->n_row + 1);

	for (i = 0; i < steps->n_col; ++i) {
		k = isl_basic_set_alloc_equality(test);
		if (k < 0)
			goto error;
		isl_int_set_si(test->eq[k][0], 0);
		for (j = 0; j < steps->n_row; ++j)
			isl_int_set(test->eq[k][1 + j], steps->row[j][i]);
	}
	for (j = 0; j < steps->n_row; ++j) {
		k = isl_basic_set_alloc_inequality(test);
		if (k < 0)
			goto error;
		isl_seq_clr(test->ineq[k], 1 + steps->n_row);
		isl_int_set_si(test->ineq[k][1 + j], 1);
	}

	k = isl_basic_set_alloc_inequality(test);
	if (k < 0)
		goto error;
	isl_int_set_si(test->ineq[k][0], -1);
	for (j = 0; j < steps->n_row; ++j)
		isl_int_set_si(test->ineq[k][1 + j], 1);

	acyclic = isl_basic_set_is_empty(test);

	isl_basic_set_free(test);
	return acyclic;
error:
	isl_basic_set_free(test);
	return -1;
}

/* Check whether the overapproximation of the power of "map" is exactly
 * the power of "map", possibly after projecting out the power (if "project"
 * is set).
 *
 * If "project" is not set, then we simply check for each power if there
 * is a path of the given length with valid initial segment.
 * If "project" is set, then we check if "steps" can only result in acyclic
 * paths.  If so, we only need to check that there is a path of _some_
 * length >= 1.  Otherwise, we perform the standard check, i.e., whether
 * there is a path of the given length.
 */
static int check_exactness(__isl_take isl_map *map, __isl_take isl_set *domain,
	__isl_take isl_set *range, __isl_take isl_map *path,
	__isl_keep isl_mat *steps, unsigned param, int project)
{
	int acyclic;

	if (!project)
		return check_path_exactness(map, domain, range, path, steps,
						param, 0);

	acyclic = is_acyclic(steps);
	if (acyclic < 0)
		goto error;

	return check_path_exactness(map, domain, range, path, steps,
					param, acyclic);
error:
	isl_map_free(map);
	isl_set_free(domain);
	isl_set_free(range);
	isl_map_free(path);
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

	path = path_along_steps(isl_map_get_dim(map), steps, param, -1);
	app = isl_map_intersect(app, isl_map_copy(path));

	if (exact &&
	    (*exact = check_exactness(isl_map_copy(map), isl_set_copy(domain),
				  isl_set_copy(range), isl_map_copy(path),
				  steps, param, project)) < 0)
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
