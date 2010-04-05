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
 
/* Given a map that represents a path with the length of the path
 * encoded as the difference between the last output coordindate
 * and the last input coordinate, set this length to either
 * exactly "length" (if "exactly" is set) or at least "length"
 * (if "exactly" is not set).
 */
static __isl_give isl_map *set_path_length(__isl_take isl_map *map,
	int exactly, int length)
{
	struct isl_dim *dim;
	struct isl_basic_map *bmap;
	unsigned d;
	unsigned nparam;
	int k;
	isl_int *c;

	if (!map)
		return NULL;

	dim = isl_map_get_dim(map);
	d = isl_dim_size(dim, isl_dim_in);
	nparam = isl_dim_size(dim, isl_dim_param);
	bmap = isl_basic_map_alloc_dim(dim, 0, 1, 1);
	if (exactly) {
		k = isl_basic_map_alloc_equality(bmap);
		c = bmap->eq[k];
	} else {
		k = isl_basic_map_alloc_inequality(bmap);
		c = bmap->ineq[k];
	}
	if (k < 0)
		goto error;
	isl_seq_clr(c, 1 + isl_basic_map_total_dim(bmap));
	isl_int_set_si(c[0], -length);
	isl_int_set_si(c[1 + nparam + d - 1], -1);
	isl_int_set_si(c[1 + nparam + d + d - 1], 1);

	bmap = isl_basic_map_finalize(bmap);
	map = isl_map_intersect(map, isl_map_from_basic_map(bmap));

	return map;
error:
	isl_basic_map_free(bmap);
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
 * In practice, "app" has an extra input and output coordinate
 * to encode the length of the path.  So, we first need to add
 * this coordinate to "map" and set the length of the path to
 * one.
 */
static int check_power_exactness(__isl_take isl_map *map,
	__isl_take isl_map *app)
{
	int exact;
	isl_map *app_1;
	isl_map *app_2;

	map = isl_map_add(map, isl_dim_in, 1);
	map = isl_map_add(map, isl_dim_out, 1);
	map = set_path_length(map, 1, 1);

	app_1 = set_path_length(isl_map_copy(app), 1, 1);

	exact = isl_map_is_subset(app_1, map);
	isl_map_free(app_1);

	if (!exact || exact < 0) {
		isl_map_free(app);
		isl_map_free(map);
		return exact;
	}

	app_1 = set_path_length(isl_map_copy(app), 0, 1);
	app_2 = set_path_length(app, 0, 2);
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
 *
 * Note that "app" has an extra input and output coordinate to encode
 * the length of the part.  If we are only interested in the transitive
 * closure, then we can simply project out these coordinates first.
 */
static int check_exactness(__isl_take isl_map *map, __isl_take isl_map *app,
	int project)
{
	isl_map *test;
	int exact;
	unsigned d;

	if (!project)
		return check_power_exactness(map, app);

	d = isl_map_dim(map, isl_dim_in);
	app = set_path_length(app, 0, 1);
	app = isl_map_project_out(app, isl_dim_in, d, 1);
	app = isl_map_project_out(app, isl_dim_out, d, 1);

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
#define MIXED		3

/* Return PURE_PARAM if only the coefficients of the parameters are non-zero.
 * Return PURE_VAR if only the coefficients of the set variables are non-zero.
 * Return MIXED if only the coefficients of the parameters and the set
 * 	variables are non-zero and if moreover the parametric constant
 * 	can never attain positive values.
 * Return IMPURE otherwise.
 */
static int purity(__isl_keep isl_basic_set *bset, isl_int *c, int *div_purity,
	int eq)
{
	unsigned d;
	unsigned n_div;
	unsigned nparam;
	int k;
	int empty;
	int i;
	int p = 0, v = 0;

	n_div = isl_basic_set_dim(bset, isl_dim_div);
	d = isl_basic_set_dim(bset, isl_dim_set);
	nparam = isl_basic_set_dim(bset, isl_dim_param);

	for (i = 0; i < n_div; ++i) {
		if (isl_int_is_zero(c[1 + nparam + d + i]))
			continue;
		switch (div_purity[i]) {
		case PURE_PARAM: p = 1; break;
		case PURE_VAR: v = 1; break;
		default: return IMPURE;
		}
	}
	if (!p && isl_seq_first_non_zero(c + 1, nparam) == -1)
		return PURE_VAR;
	if (!v && isl_seq_first_non_zero(c + 1 + nparam, d) == -1)
		return PURE_PARAM;
	if (eq)
		return IMPURE;

	bset = isl_basic_set_copy(bset);
	bset = isl_basic_set_cow(bset);
	bset = isl_basic_set_extend_constraints(bset, 0, 1);
	k = isl_basic_set_alloc_inequality(bset);
	if (k < 0)
		goto error;
	isl_seq_clr(bset->ineq[k], 1 + isl_basic_set_total_dim(bset));
	isl_seq_cpy(bset->ineq[k], c, 1 + nparam);
	for (i = 0; i < n_div; ++i) {
		if (div_purity[i] != PURE_PARAM)
			continue;
		isl_int_set(bset->ineq[k][1 + nparam + d + i],
			    c[1 + nparam + d + i]);
	}
	isl_int_sub_ui(bset->ineq[k][0], bset->ineq[k][0], 1);
	empty = isl_basic_set_is_empty(bset);
	isl_basic_set_free(bset);

	return empty < 0 ? -1 : empty ? MIXED : IMPURE;
error:
	isl_basic_set_free(bset);
	return -1;
}

/* Return an array of integers indicating the type of each div in bset.
 * If the div is (recursively) defined in terms of only the parameters,
 * then the type is PURE_PARAM.
 * If the div is (recursively) defined in terms of only the set variables,
 * then the type is PURE_VAR.
 * Otherwise, the type is IMPURE.
 */
static __isl_give int *get_div_purity(__isl_keep isl_basic_set *bset)
{
	int i, j;
	int *div_purity;
	unsigned d;
	unsigned n_div;
	unsigned nparam;

	if (!bset)
		return NULL;

	n_div = isl_basic_set_dim(bset, isl_dim_div);
	d = isl_basic_set_dim(bset, isl_dim_set);
	nparam = isl_basic_set_dim(bset, isl_dim_param);

	div_purity = isl_alloc_array(bset->ctx, int, n_div);
	if (!div_purity)
		return NULL;

	for (i = 0; i < bset->n_div; ++i) {
		int p = 0, v = 0;
		if (isl_int_is_zero(bset->div[i][0])) {
			div_purity[i] = IMPURE;
			continue;
		}
		if (isl_seq_first_non_zero(bset->div[i] + 2, nparam) != -1)
			p = 1;
		if (isl_seq_first_non_zero(bset->div[i] + 2 + nparam, d) != -1)
			v = 1;
		for (j = 0; j < i; ++j) {
			if (isl_int_is_zero(bset->div[i][2 + nparam + d + j]))
				continue;
			switch (div_purity[j]) {
			case PURE_PARAM: p = 1; break;
			case PURE_VAR: v = 1; break;
			default: p = v = 1; break;
			}
		}
		div_purity[i] = v ? p ? IMPURE : PURE_VAR : PURE_PARAM;
	}

	return div_purity;
}

/* Given a path with the as yet unconstrained length at position "pos",
 * check if setting the length to zero results in only the identity
 * mapping.
 */
int empty_path_is_identity(__isl_keep isl_basic_map *path, unsigned pos)
{
	isl_basic_map *test = NULL;
	isl_basic_map *id = NULL;
	int k;
	int is_id;

	test = isl_basic_map_copy(path);
	test = isl_basic_map_extend_constraints(test, 1, 0);
	k = isl_basic_map_alloc_equality(test);
	if (k < 0)
		goto error;
	isl_seq_clr(test->eq[k], 1 + isl_basic_map_total_dim(test));
	isl_int_set_si(test->eq[k][pos], 1);
	id = isl_basic_map_identity(isl_dim_domain(isl_basic_map_get_dim(path)));
	is_id = isl_basic_map_is_subset(test, id);
	isl_basic_map_free(test);
	isl_basic_map_free(id);
	return is_id;
error:
	isl_basic_map_free(test);
	return -1;
}

__isl_give isl_basic_map *add_delta_constraints(__isl_take isl_basic_map *path,
	__isl_keep isl_basic_set *delta, unsigned off, unsigned nparam,
	unsigned d, int *div_purity, int eq)
{
	int i, k;
	int n = eq ? delta->n_eq : delta->n_ineq;
	isl_int **delta_c = eq ? delta->eq : delta->ineq;
	isl_int **path_c = eq ? path->eq : path->ineq;
	unsigned n_div;

	n_div = isl_basic_set_dim(delta, isl_dim_div);

	for (i = 0; i < n; ++i) {
		int p = purity(delta, delta_c[i], div_purity, eq);
		if (p < 0)
			goto error;
		if (p == IMPURE)
			continue;
		if (eq)
			k = isl_basic_map_alloc_equality(path);
		else
			k = isl_basic_map_alloc_inequality(path);
		if (k < 0)
			goto error;
		isl_seq_clr(path_c[k], 1 + isl_basic_map_total_dim(path));
		if (p == PURE_VAR) {
			isl_seq_cpy(path_c[k] + off,
				    delta_c[i] + 1 + nparam, d);
			isl_int_set(path_c[k][off + d], delta_c[i][0]);
		} else if (p == PURE_PARAM) {
			isl_seq_cpy(path_c[k], delta_c[i], 1 + nparam);
		} else {
			isl_seq_cpy(path_c[k] + off,
				    delta_c[i] + 1 + nparam, d);
			isl_seq_cpy(path_c[k], delta_c[i], 1 + nparam);
		}
		isl_seq_cpy(path_c[k] + off - n_div,
			    delta_c[i] + 1 + nparam + d, n_div);
	}

	return path;
error:
	isl_basic_map_free(path);
	return NULL;
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
 *				C x + C'p + c >= 0 and
 *				D x + D'p + d >= 0 }
 *
 * where the constraints C x + C'p + c >= 0 are such that the parametric
 * constant term of each constraint j, "C_j x + C'_j p + c_j",
 * can never attain positive values, then the relation is constructed as
 *
 *	{ [x] -> [y] : exists [f, k] \in Z^{n+1} : y = x + f and
 *			A f + k a >= 0 and B p + b >= 0 and
 *			C f + C'p + c >= 0 and k >= 1 }
 *	union { [x] -> [x] }
 *
 * If the zero-length paths happen to correspond exactly to the identity
 * mapping, then we return
 *
 *	{ [x] -> [y] : exists [f, k] \in Z^{n+1} : y = x + f and
 *			A f + k a >= 0 and B p + b >= 0 and
 *			C f + C'p + c >= 0 and k >= 0 }
 *
 * instead.
 *
 * Existentially quantified variables in \delta are handled by
 * classifying them as independent of the parameters, purely
 * parameter dependent and others.  Constraints containing
 * any of the other existentially quantified variables are removed.
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
	int is_id;
	int *div_purity = NULL;

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

	div_purity = get_div_purity(delta);
	if (!div_purity)
		goto error;

	path = add_delta_constraints(path, delta, off, nparam, d, div_purity, 1);
	path = add_delta_constraints(path, delta, off, nparam, d, div_purity, 0);

	is_id = empty_path_is_identity(path, off + d);
	if (is_id < 0)
		goto error;

	k = isl_basic_map_alloc_inequality(path);
	if (k < 0)
		goto error;
	isl_seq_clr(path->ineq[k], 1 + isl_basic_map_total_dim(path));
	if (!is_id)
		isl_int_set_si(path->ineq[k][0], -1);
	isl_int_set_si(path->ineq[k][off + d], 1);
			
	free(div_purity);
	isl_basic_set_free(delta);
	path = isl_basic_map_finalize(path);
	if (is_id) {
		isl_dim_free(dim);
		return isl_map_from_basic_map(path);
	}
	return isl_basic_map_union(path,
				isl_basic_map_identity(isl_dim_domain(dim)));
error:
	free(div_purity);
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
			path = isl_map_coalesce(path);
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
 * construct a map that is the union of the identity map and
 * an overapproximation of the map
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
 *				x in dom R and x + d in ran R } union
 *	{ (x) -> (x) }
 */
static __isl_give isl_map *construct_component(__isl_take isl_dim *dim,
	__isl_keep isl_map *map, int *exact, int project)
{
	struct isl_set *domain = NULL;
	struct isl_set *range = NULL;
	struct isl_set *overlap;
	struct isl_map *app = NULL;
	struct isl_map *path = NULL;

	domain = isl_map_domain(isl_map_copy(map));
	domain = isl_set_coalesce(domain);
	range = isl_map_range(isl_map_copy(map));
	range = isl_set_coalesce(range);
	overlap = isl_set_intersect(isl_set_copy(domain), isl_set_copy(range));
	if (isl_set_is_empty(overlap) == 1) {
		isl_set_free(domain);
		isl_set_free(range);
		isl_set_free(overlap);
		isl_dim_free(dim);

		map = isl_map_copy(map);
		map = isl_map_add(map, isl_dim_in, 1);
		map = isl_map_add(map, isl_dim_out, 1);
		map = set_path_length(map, 1, 1);
		return map;
	}
	isl_set_free(overlap);
	app = isl_map_from_domain_and_range(domain, range);
	app = isl_map_add(app, isl_dim_in, 1);
	app = isl_map_add(app, isl_dim_out, 1);

	path = construct_extended_path(isl_dim_copy(dim), map,
					exact && *exact ? &project : NULL);
	app = isl_map_intersect(app, path);

	if (exact && *exact &&
	    (*exact = check_exactness(isl_map_copy(map), isl_map_copy(app),
				      project)) < 0)
		goto error;

	return isl_map_union(app, isl_map_identity(isl_dim_domain(dim)));
error:
	isl_dim_free(dim);
	isl_map_free(app);
	return NULL;
}

/* Structure for representing the nodes in the graph being traversed
 * using Tarjan's algorithm.
 * index represents the order in which nodes are visited.
 * min_index is the index of the root of a (sub)component.
 * on_stack indicates whether the node is currently on the stack.
 */
struct basic_map_sort_node {
	int index;
	int min_index;
	int on_stack;
};
/* Structure for representing the graph being traversed
 * using Tarjan's algorithm.
 * len is the number of nodes
 * node is an array of nodes
 * stack contains the nodes on the path from the root to the current node
 * sp is the stack pointer
 * index is the index of the last node visited
 * order contains the elements of the components separated by -1
 * op represents the current position in order
 */
struct basic_map_sort {
	int len;
	struct basic_map_sort_node *node;
	int *stack;
	int sp;
	int index;
	int *order;
	int op;
};

static void basic_map_sort_free(struct basic_map_sort *s)
{
	if (!s)
		return;
	free(s->node);
	free(s->stack);
	free(s->order);
	free(s);
}

static struct basic_map_sort *basic_map_sort_alloc(struct isl_ctx *ctx, int len)
{
	struct basic_map_sort *s;
	int i;

	s = isl_calloc_type(ctx, struct basic_map_sort);
	if (!s)
		return NULL;
	s->len = len;
	s->node = isl_alloc_array(ctx, struct basic_map_sort_node, len);
	if (!s->node)
		goto error;
	for (i = 0; i < len; ++i)
		s->node[i].index = -1;
	s->stack = isl_alloc_array(ctx, int, len);
	if (!s->stack)
		goto error;
	s->order = isl_alloc_array(ctx, int, 2 * len);
	if (!s->order)
		goto error;

	s->sp = 0;
	s->index = 0;
	s->op = 0;

	return s;
error:
	basic_map_sort_free(s);
	return NULL;
}

/* Check whether in the computation of the transitive closure
 * "bmap1" (R_1) should follow (or be part of the same component as)
 * "bmap2" (R_2).
 *
 * That is check whether
 *
 *	R_1 \circ R_2
 *
 * is a subset of
 *
 *	R_2 \circ R_1
 *
 * If so, then there is no reason for R_1 to immediately follow R_2
 * in any path.
 */
static int basic_map_follows(__isl_keep isl_basic_map *bmap1,
	__isl_keep isl_basic_map *bmap2)
{
	struct isl_map *map12 = NULL;
	struct isl_map *map21 = NULL;
	int subset;

	map21 = isl_map_from_basic_map(
			isl_basic_map_apply_range(
				isl_basic_map_copy(bmap2),
				isl_basic_map_copy(bmap1)));
	subset = isl_map_is_empty(map21);
	if (subset < 0)
		goto error;
	if (subset) {
		isl_map_free(map21);
		return 0;
	}

	map12 = isl_map_from_basic_map(
			isl_basic_map_apply_range(
				isl_basic_map_copy(bmap1),
				isl_basic_map_copy(bmap2)));

	subset = isl_map_is_subset(map21, map12);

	isl_map_free(map12);
	isl_map_free(map21);

	return subset < 0 ? -1 : !subset;
error:
	isl_map_free(map21);
	return -1;
}

/* Perform Tarjan's algorithm for computing the strongly connected components
 * in the graph with the disjuncts of "map" as vertices and with an
 * edge between any pair of disjuncts such that the first has
 * to be applied after the second.
 */
static int power_components_tarjan(struct basic_map_sort *s,
	__isl_keep isl_map *map, int i)
{
	int j;

	s->node[i].index = s->index;
	s->node[i].min_index = s->index;
	s->node[i].on_stack = 1;
	s->index++;
	s->stack[s->sp++] = i;

	for (j = s->len - 1; j >= 0; --j) {
		int f;

		if (j == i)
			continue;
		if (s->node[j].index >= 0 &&
			(!s->node[j].on_stack ||
			 s->node[j].index > s->node[i].min_index))
			continue;

		f = basic_map_follows(map->p[i], map->p[j]);
		if (f < 0)
			return -1;
		if (!f)
			continue;

		if (s->node[j].index < 0) {
			power_components_tarjan(s, map, j);
			if (s->node[j].min_index < s->node[i].min_index)
				s->node[i].min_index = s->node[j].min_index;
		} else if (s->node[j].index < s->node[i].min_index)
			s->node[i].min_index = s->node[j].index;
	}

	if (s->node[i].index != s->node[i].min_index)
		return 0;

	do {
		j = s->stack[--s->sp];
		s->node[j].on_stack = 0;
		s->order[s->op++] = j;
	} while (j != i);
	s->order[s->op++] = -1;

	return 0;
}

/* Given a union of basic maps R = \cup_i R_i \subseteq D \times D
 * and a dimension specification (Z^{n+1} -> Z^{n+1}),
 * construct a map that is the union of the identity map and
 * an overapproximation of the map
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
 *				x in dom R and x + d in ran R } union
 *	{ (x) -> (x) }
 *
 * We first split the map into strongly connected components, perform
 * the above on each component and the join the results in the correct
 * order.  The power of each of the components needs to be extended
 * with the identity map because a path in the global result need
 * not go through every component.
 * The final result will then also contain the identity map, but
 * this part will be removed when the length of the path is forced
 * to be strictly positive.
 */
static __isl_give isl_map *construct_power_components(__isl_take isl_dim *dim,
	__isl_keep isl_map *map, int *exact, int project)
{
	int i, n;
	struct isl_map *path = NULL;
	struct basic_map_sort *s = NULL;

	if (!map)
		goto error;
	if (map->n <= 1)
		return construct_component(dim, map, exact, project);

	s = basic_map_sort_alloc(map->ctx, map->n);
	if (!s)
		goto error;
	for (i = map->n - 1; i >= 0; --i) {
		if (s->node[i].index >= 0)
			continue;
		if (power_components_tarjan(s, map, i) < 0)
			goto error;
	}

	i = 0;
	n = map->n;
	path = isl_map_identity(isl_dim_domain(isl_dim_copy(dim)));
	while (n) {
		struct isl_map *comp;
		comp = isl_map_alloc_dim(isl_map_get_dim(map), n, 0);
		while (s->order[i] != -1) {
			comp = isl_map_add_basic_map(comp,
				    isl_basic_map_copy(map->p[s->order[i]]));
			--n;
			++i;
		}
		path = isl_map_apply_range(path,
			    construct_component(isl_dim_copy(dim), comp,
						exact, project));
		isl_map_free(comp);
		++i;
	}

	basic_map_sort_free(s);
	isl_dim_free(dim);

	return path;
error:
	basic_map_sort_free(s);
	isl_dim_free(dim);
	return NULL;
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
	unsigned param, int *exact, int project)
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

	app = construct_power_components(isl_dim_copy(dim), map,
					exact, project);

	diff = equate_parameter_to_length(dim, param);
	app = isl_map_intersect(app, diff);
	app = isl_map_project_out(app, isl_dim_in, d, 1);
	app = isl_map_project_out(app, isl_dim_out, d, 1);

	return app;
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

	app = construct_power(map, param, exact, project);

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
