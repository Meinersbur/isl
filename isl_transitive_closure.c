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
#include <isl_lp.h>
 
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
 *	{ (x) -> (x + d) | \exists k_i >= 1, \delta_i \in \Delta_i :
 *				d = (\sum_i k_i \delta_i, \sum_i k_i) and
 *				x in dom R and x + d in ran R }
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

	isl_dim_free(dim);
	app = set_path_length(app, 0, 1);
	return app;
error:
	isl_dim_free(dim);
	isl_map_free(app);
	return NULL;
}

/* Call construct_component and, if "project" is set, project out
 * the final coordinates.
 */
static __isl_give isl_map *construct_projected_component(
	__isl_take isl_dim *dim,
	__isl_keep isl_map *map, int *exact, int project)
{
	isl_map *app;
	unsigned d;

	if (!dim)
		return NULL;
	d = isl_dim_size(dim, isl_dim_in);

	app = construct_component(dim, map, exact, project);
	if (project) {
		app = isl_map_project_out(app, isl_dim_in, d - 1, 1);
		app = isl_map_project_out(app, isl_dim_out, d - 1, 1);
	}
	return app;
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
 * construct a map that is an overapproximation of the map
 * that takes an element from the dom R \times Z to an
 * element from ran R \times Z, such that the first n coordinates of the
 * difference between them is a sum of differences between images
 * and pre-images in one of the R_i and such that the last coordinate
 * is equal to the number of steps taken.
 * If "project" is set, then these final coordinates are not included,
 * i.e., a relation of type Z^n -> Z^n is returned.
 * That is, let
 *
 *	\Delta_i = { y - x | (x, y) in R_i }
 *
 * then the constructed map is an overapproximation of
 *
 *	{ (x) -> (x + d) | \exists k_i >= 0, \delta_i \in \Delta_i :
 *				d = (\sum_i k_i \delta_i, \sum_i k_i) and
 *				x in dom R and x + d in ran R }
 *
 * or
 *
 *	{ (x) -> (x + d) | \exists k_i >= 0, \delta_i \in \Delta_i :
 *				d = (\sum_i k_i \delta_i) and
 *				x in dom R and x + d in ran R }
 *
 * if "project" is set.
 *
 * We first split the map into strongly connected components, perform
 * the above on each component and then join the results in the correct
 * order, at each join also taking in the union of both arguments
 * to allow for paths that do not go through one of the two arguments.
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
		return construct_projected_component(dim, map, exact, project);

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
	if (project)
		path = isl_map_empty(isl_map_get_dim(map));
	else
		path = isl_map_empty(isl_dim_copy(dim));
	while (n) {
		struct isl_map *comp;
		isl_map *path_comp, *path_comb;
		comp = isl_map_alloc_dim(isl_map_get_dim(map), n, 0);
		while (s->order[i] != -1) {
			comp = isl_map_add_basic_map(comp,
				    isl_basic_map_copy(map->p[s->order[i]]));
			--n;
			++i;
		}
		path_comp = construct_projected_component(isl_dim_copy(dim),
						comp, exact, project);
		path_comb = isl_map_apply_range(isl_map_copy(path),
						isl_map_copy(path_comp));
		path = isl_map_union(path, path_comp);
		path = isl_map_union(path, path_comb);
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
 * or
 *
 *	{ (x) -> (x + d) | \exists k_i >= 0, \delta_i \in \Delta_i :
 *				d = \sum_i k_i \delta_i and \sum_i k_i > 0 }
 *
 * if "project" is set.
 *
 * If "project" is not set, then
 * we first construct an extended mapping with an extra coordinate
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

	if (project) {
		isl_dim_free(dim);
	} else {
		diff = equate_parameter_to_length(dim, param);
		app = isl_map_intersect(app, diff);
		app = isl_map_project_out(app, isl_dim_in, d, 1);
		app = isl_map_project_out(app, isl_dim_out, d, 1);
	}

	return app;
}

/* Compute the positive powers of "map", or an overapproximation.
 * The power is given by parameter "param".  If the result is exact,
 * then *exact is set to 1.
 *
 * If project is set, then we are actually interested in the transitive
 * closure, so we can use a more relaxed exactness check.
 * The lengths of the paths are also projected out instead of being
 * equated to "param" (which is then ignored in this case).
 */
static __isl_give isl_map *map_power(__isl_take isl_map *map, unsigned param,
	int *exact, int project)
{
	struct isl_map *app = NULL;

	if (exact)
		*exact = 1;

	map = isl_map_coalesce(map);
	if (!map)
		return NULL;

	if (isl_map_fast_is_empty(map))
		return map;

	isl_assert(map->ctx, project || param < isl_map_dim(map, isl_dim_param),
		goto error);
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

/* Check whether equality i of bset is a pure stride constraint
 * on a single dimensions, i.e., of the form
 *
 *	v = k e
 *
 * with k a constant and e an existentially quantified variable.
 */
static int is_eq_stride(__isl_keep isl_basic_set *bset, int i)
{
	int k;
	unsigned nparam;
	unsigned d;
	unsigned n_div;
	int pos1;
	int pos2;

	if (!bset)
		return -1;

	if (!isl_int_is_zero(bset->eq[i][0]))
		return 0;

	nparam = isl_basic_set_dim(bset, isl_dim_param);
	d = isl_basic_set_dim(bset, isl_dim_set);
	n_div = isl_basic_set_dim(bset, isl_dim_div);

	if (isl_seq_first_non_zero(bset->eq[i] + 1, nparam) != -1)
		return 0;
	pos1 = isl_seq_first_non_zero(bset->eq[i] + 1 + nparam, d);
	if (pos1 == -1)
		return 0;
	if (isl_seq_first_non_zero(bset->eq[i] + 1 + nparam + pos1 + 1, 
					d - pos1 - 1) != -1)
		return 0;

	pos2 = isl_seq_first_non_zero(bset->eq[i] + 1 + nparam + d, n_div);
	if (pos2 == -1)
		return 0;
	if (isl_seq_first_non_zero(bset->eq[i] + 1 + nparam + d  + pos2 + 1,
				   n_div - pos2 - 1) != -1)
		return 0;
	if (!isl_int_is_one(bset->eq[i][1 + nparam + pos1]) &&
	    !isl_int_is_negone(bset->eq[i][1 + nparam + pos1]))
		return 0;

	return 1;
}

/* Given a map, compute the smallest superset of this map that is of the form
 *
 *	{ i -> j : L <= j - i <= U and exists a_p: j_p - i_p = M_p a_p }
 *
 * (where p ranges over the (non-parametric) dimensions),
 * compute the transitive closure of this map, i.e.,
 *
 *	{ i -> j : exists k > 0:
 *		k L <= j - i <= k U and exists a: j_p - i_p = M_p a_p }
 *
 * and intersect domain and range of this transitive closure with
 * domain and range of the original map.
 *
 * If with_id is set, then try to include as much of the identity mapping
 * as possible, by computing
 *
 *	{ i -> j : exists k >= 0:
 *		k L <= j - i <= k U and exists a: j_p - i_p = M_p a_p }
 *
 * instead (i.e., allow k = 0) and by intersecting domain and range
 * with the union of the domain and the range of the original map.
 *
 * In practice, we compute the difference set
 *
 *	delta  = { j - i | i -> j in map },
 *
 * look for stride constraint on the individual dimensions and compute
 * (constant) lower and upper bounds for each individual dimension,
 * adding a constraint for each bound not equal to infinity.
 */
static __isl_give isl_map *box_closure(__isl_take isl_map *map, int with_id)
{
	int i;
	int k;
	unsigned d;
	unsigned nparam;
	unsigned total;
	isl_dim *dim;
	isl_set *delta;
	isl_set *domain = NULL;
	isl_set *range = NULL;
	isl_map *app = NULL;
	isl_basic_set *aff = NULL;
	isl_basic_map *bmap = NULL;
	isl_vec *obj = NULL;
	isl_int opt;

	isl_int_init(opt);

	delta = isl_map_deltas(isl_map_copy(map));

	aff = isl_set_affine_hull(isl_set_copy(delta));
	if (!aff)
		goto error;
	dim = isl_map_get_dim(map);
	d = isl_dim_size(dim, isl_dim_in);
	nparam = isl_dim_size(dim, isl_dim_param);
	total = isl_dim_total(dim);
	bmap = isl_basic_map_alloc_dim(dim,
					aff->n_div + 1, aff->n_div, 2 * d + 1);
	for (i = 0; i < aff->n_div + 1; ++i) {
		k = isl_basic_map_alloc_div(bmap);
		if (k < 0)
			goto error;
		isl_int_set_si(bmap->div[k][0], 0);
	}
	for (i = 0; i < aff->n_eq; ++i) {
		if (!is_eq_stride(aff, i))
			continue;
		k = isl_basic_map_alloc_equality(bmap);
		if (k < 0)
			goto error;
		isl_seq_clr(bmap->eq[k], 1 + nparam);
		isl_seq_cpy(bmap->eq[k] + 1 + nparam + d,
				aff->eq[i] + 1 + nparam, d);
		isl_seq_neg(bmap->eq[k] + 1 + nparam,
				aff->eq[i] + 1 + nparam, d);
		isl_seq_cpy(bmap->eq[k] + 1 + nparam + 2 * d,
				aff->eq[i] + 1 + nparam + d, aff->n_div);
		isl_int_set_si(bmap->eq[k][1 + total + aff->n_div], 0);
	}
	obj = isl_vec_alloc(map->ctx, 1 + nparam + d);
	if (!obj)
		goto error;
	isl_seq_clr(obj->el, 1 + nparam + d);
	for (i = 0; i < d; ++ i) {
		enum isl_lp_result res;

		isl_int_set_si(obj->el[1 + nparam + i], 1);

		res = isl_set_solve_lp(delta, 0, obj->el, map->ctx->one, &opt,
					NULL, NULL);
		if (res == isl_lp_error)
			goto error;
		if (res == isl_lp_ok) {
			k = isl_basic_map_alloc_inequality(bmap);
			if (k < 0)
				goto error;
			isl_seq_clr(bmap->ineq[k],
					1 + nparam + 2 * d + bmap->n_div);
			isl_int_set_si(bmap->ineq[k][1 + nparam + i], -1);
			isl_int_set_si(bmap->ineq[k][1 + nparam + d + i], 1);
			isl_int_neg(bmap->ineq[k][1 + nparam + 2 * d + aff->n_div], opt);
		}

		res = isl_set_solve_lp(delta, 1, obj->el, map->ctx->one, &opt,
					NULL, NULL);
		if (res == isl_lp_error)
			goto error;
		if (res == isl_lp_ok) {
			k = isl_basic_map_alloc_inequality(bmap);
			if (k < 0)
				goto error;
			isl_seq_clr(bmap->ineq[k],
					1 + nparam + 2 * d + bmap->n_div);
			isl_int_set_si(bmap->ineq[k][1 + nparam + i], 1);
			isl_int_set_si(bmap->ineq[k][1 + nparam + d + i], -1);
			isl_int_set(bmap->ineq[k][1 + nparam + 2 * d + aff->n_div], opt);
		}

		isl_int_set_si(obj->el[1 + nparam + i], 0);
	}
	k = isl_basic_map_alloc_inequality(bmap);
	if (k < 0)
		goto error;
	isl_seq_clr(bmap->ineq[k],
			1 + nparam + 2 * d + bmap->n_div);
	if (!with_id)
		isl_int_set_si(bmap->ineq[k][0], -1);
	isl_int_set_si(bmap->ineq[k][1 + nparam + 2 * d + aff->n_div], 1);

	domain = isl_map_domain(isl_map_copy(map));
	domain = isl_set_coalesce(domain);
	range = isl_map_range(isl_map_copy(map));
	range = isl_set_coalesce(range);
	if (with_id) {
		domain = isl_set_union(domain, range);
		domain = isl_set_coalesce(domain);
		range = isl_set_copy(domain);
	}
	app = isl_map_from_domain_and_range(domain, range);

	isl_vec_free(obj);
	isl_basic_set_free(aff);
	isl_map_free(map);
	bmap = isl_basic_map_finalize(bmap);
	isl_set_free(delta);
	isl_int_clear(opt);

	map = isl_map_from_basic_map(bmap);
	map = isl_map_intersect(map, app);

	return map;
error:
	isl_vec_free(obj);
	isl_basic_map_free(bmap);
	isl_basic_set_free(aff);
	isl_map_free(map);
	isl_set_free(delta);
	isl_int_clear(opt);
	return NULL;
}

/* Check whether app is the transitive closure of map.
 * In particular, check that app is acyclic and, if so,
 * check that
 *
 *	app \subset (map \cup (map \circ app))
 */
static int check_exactness_omega(__isl_keep isl_map *map,
	__isl_keep isl_map *app)
{
	isl_set *delta;
	int i;
	int is_empty, is_exact;
	unsigned d;
	isl_map *test;

	delta = isl_map_deltas(isl_map_copy(app));
	d = isl_set_dim(delta, isl_dim_set);
	for (i = 0; i < d; ++i)
		delta = isl_set_fix_si(delta, isl_dim_set, i, 0);
	is_empty = isl_set_is_empty(delta);
	isl_set_free(delta);
	if (is_empty < 0)
		return -1;
	if (!is_empty)
		return 0;

	test = isl_map_apply_range(isl_map_copy(app), isl_map_copy(map));
	test = isl_map_union(test, isl_map_copy(map));
	is_exact = isl_map_is_subset(app, test);
	isl_map_free(test);

	return is_exact;
}

/* Check if basic map M_i can be combined with all the other
 * basic maps such that
 *
 *	(\cup_j M_j)^+
 *
 * can be computed as
 *
 *	M_i \cup (\cup_{j \ne i} M_i^* \circ M_j \circ M_i^*)^+
 *
 * In particular, check if we can compute a compact representation
 * of
 *
 *		M_i^* \circ M_j \circ M_i^*
 *
 * for each j != i.
 * Let M_i^? be an extension of M_i^+ that allows paths
 * of length zero, i.e., the result of box_closure(., 1).
 * The criterion, as proposed by Kelly et al., is that
 * id = M_i^? - M_i^+ can be represented as a basic map
 * and that
 *
 *	id \circ M_j \circ id = M_j
 *
 * for each j != i.
 *
 * If this function returns 1, then tc and qc are set to
 * M_i^+ and M_i^?, respectively.
 */
static int can_be_split_off(__isl_keep isl_map *map, int i,
	__isl_give isl_map **tc, __isl_give isl_map **qc)
{
	isl_map *map_i, *id;
	int j = -1;

	map_i = isl_map_from_basic_map(isl_basic_map_copy(map->p[i]));
	*tc = box_closure(isl_map_copy(map_i), 0);
	*qc = box_closure(map_i, 1);
	id = isl_map_subtract(isl_map_copy(*qc), isl_map_copy(*tc));

	if (!id || !*qc)
		goto error;
	if (id->n != 1 || (*qc)->n != 1)
		goto done;

	for (j = 0; j < map->n; ++j) {
		isl_map *map_j, *test;
		int is_ok;

		if (i == j)
			continue;
		map_j = isl_map_from_basic_map(
					isl_basic_map_copy(map->p[j]));
		test = isl_map_apply_range(isl_map_copy(id),
						isl_map_copy(map_j));
		test = isl_map_apply_range(test, isl_map_copy(id));
		is_ok = isl_map_is_equal(test, map_j);
		isl_map_free(map_j);
		isl_map_free(test);
		if (is_ok < 0)
			goto error;
		if (!is_ok)
			break;
	}

done:
	isl_map_free(id);
	if (j == map->n)
		return 1;

	isl_map_free(*qc);
	isl_map_free(*tc);
	*qc = NULL;
	*tc = NULL;

	return 0;
error:
	isl_map_free(id);
	isl_map_free(*qc);
	isl_map_free(*tc);
	*qc = NULL;
	*tc = NULL;
	return -1;
}

/* Compute an overapproximation of the transitive closure of "map"
 * using a variation of the algorithm from
 * "Transitive Closure of Infinite Graphs and its Applications"
 * by Kelly et al.
 *
 * We first check whether we can can split of any basic map M_i and
 * compute
 *
 *	(\cup_j M_j)^+
 *
 * as
 *
 *	M_i \cup (\cup_{j \ne i} M_i^* \circ M_j \circ M_i^*)^+
 *
 * using a recursive call on the remaining map.
 *
 * If not, we simply call box_closure on the whole map.
 */
static __isl_give isl_map *compute_closure_omega(__isl_take isl_map *map)
{
	int i, j;

	if (!map)
		return NULL;
	if (map->n == 1)
		return box_closure(map, 0);

	map = isl_map_cow(map);
	if (!map)
		goto error;

	for (i = 0; i < map->n; ++i) {
		int ok;
		isl_map *qc, *tc;
		ok = can_be_split_off(map, i, &tc, &qc);
		if (ok < 0)
			goto error;
		if (!ok)
			continue;

		isl_basic_map_free(map->p[i]);
		if (i != map->n - 1)
			map->p[i] = map->p[map->n - 1];
		map->n--;

		map = isl_map_apply_range(isl_map_copy(qc), map);
		map = isl_map_apply_range(map, qc);

		return isl_map_union(tc, compute_closure_omega(map));
	}

	return box_closure(map, 0);
error:
	isl_map_free(map);
	return NULL;
}

/* Compute an overapproximation of the transitive closure of "map"
 * using a variation of the algorithm from
 * "Transitive Closure of Infinite Graphs and its Applications"
 * by Kelly et al. and check whether the result is definitely exact.
 */
static __isl_give isl_map *transitive_closure_omega(__isl_take isl_map *map,
	int *exact)
{
	isl_map *app;

	app = compute_closure_omega(isl_map_copy(map));

	if (exact)
		*exact = check_exactness_omega(map, app);

	isl_map_free(map);
	return app;
}

/* Compute the transitive closure  of "map", or an overapproximation.
 * If the result is exact, then *exact is set to 1.
 * Simply use map_power to compute the powers of map, but tell
 * it to project out the lengths of the paths instead of equating
 * the length to a parameter.
 */
__isl_give isl_map *isl_map_transitive_closure(__isl_take isl_map *map,
	int *exact)
{
	unsigned param;

	if (!map)
		goto error;

	if (map->ctx->opt->closure == ISL_CLOSURE_OMEGA)
		return transitive_closure_omega(map, exact);

	param = isl_map_dim(map, isl_dim_param);
	map = map_power(map, param, exact, 1);

	return map;
error:
	isl_map_free(map);
	return NULL;
}
