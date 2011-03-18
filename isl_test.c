/*
 * Copyright 2008-2009 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv2.1 license
 *
 * Written by Sven Verdoolaege, K.U.Leuven, Departement
 * Computerwetenschappen, Celestijnenlaan 200A, B-3001 Leuven, Belgium
 */

#include <assert.h>
#include <stdio.h>
#include <limits.h>
#include <isl_ctx_private.h>
#include <isl_map_private.h>
#include <isl/set.h>
#include <isl/flow.h>
#include <isl/constraint.h>
#include <isl/polynomial.h>
#include <isl/union_map.h>
#include <isl_factorization.h>

static char *srcdir;

void test_parse_map(isl_ctx *ctx, const char *str)
{
	isl_map *map;

	map = isl_map_read_from_str(ctx, str, -1);
	assert(map);
	isl_map_free(map);
}

void test_parse_map_equal(isl_ctx *ctx, const char *str, const char *str2)
{
	isl_map *map, *map2;

	map = isl_map_read_from_str(ctx, str, -1);
	map2 = isl_map_read_from_str(ctx, str2, -1);
	assert(map && map2 && isl_map_is_equal(map, map2));
	isl_map_free(map);
	isl_map_free(map2);
}

void test_parse_pwqp(isl_ctx *ctx, const char *str)
{
	isl_pw_qpolynomial *pwqp;

	pwqp = isl_pw_qpolynomial_read_from_str(ctx, str);
	assert(pwqp);
	isl_pw_qpolynomial_free(pwqp);
}

void test_parse(struct isl_ctx *ctx)
{
	isl_map *map, *map2;
	const char *str, *str2;

	str = "{ [i] -> [-i] }";
	map = isl_map_read_from_str(ctx, str, -1);
	assert(map);
	isl_map_free(map);

	str = "{ A[i] -> L[([i/3])] }";
	map = isl_map_read_from_str(ctx, str, -1);
	assert(map);
	isl_map_free(map);

	test_parse_map(ctx, "{[[s] -> A[i]] -> [[s+1] -> A[i]]}");
	test_parse_map(ctx, "{ [p1, y1, y2] -> [2, y1, y2] : "
				"p1 = 1 && (y1 <= y2 || y2 = 0) }");

	str = "{ [x,y]  : [([x/2]+y)/3] >= 1 }";
	str2 = "{ [x, y] : 2y >= 6 - x }";
	test_parse_map_equal(ctx, str, str2);

	test_parse_map_equal(ctx, "{ [x,y] : x <= min(y, 2*y+3) }",
				  "{ [x,y] : x <= y, 2*y + 3 }");
	str = "{ [x, y] : (y <= x and y >= -3) or (2y <= -3 + x and y <= -4) }";
	test_parse_map_equal(ctx, "{ [x,y] : x >= min(y, 2*y+3) }", str);

	str = "{[new,old] -> [new+1-2*[(new+1)/2],old+1-2*[(old+1)/2]]}";
	map = isl_map_read_from_str(ctx, str, -1);
	str = "{ [new, old] -> [o0, o1] : "
	       "exists (e0 = [(-1 - new + o0)/2], e1 = [(-1 - old + o1)/2]: "
	       "2e0 = -1 - new + o0 and 2e1 = -1 - old + o1 and o0 >= 0 and "
	       "o0 <= 1 and o1 >= 0 and o1 <= 1) }";
	map2 = isl_map_read_from_str(ctx, str, -1);
	assert(isl_map_is_equal(map, map2));
	isl_map_free(map);
	isl_map_free(map2);

	str = "{[new,old] -> [new+1-2*[(new+1)/2],old+1-2*[(old+1)/2]]}";
	map = isl_map_read_from_str(ctx, str, -1);
	str = "{[new,old] -> [(new+1)%2,(old+1)%2]}";
	map2 = isl_map_read_from_str(ctx, str, -1);
	assert(isl_map_is_equal(map, map2));
	isl_map_free(map);
	isl_map_free(map2);

	str = "[n] -> { [c1] : c1>=0 and c1<=floord(n-4,3) }";
	str2 = "[n] -> { [c1] : c1 >= 0 and 3c1 <= -4 + n }";
	test_parse_map_equal(ctx, str, str2);

	test_parse_pwqp(ctx, "{ [i] -> i + [ (i + [i/3])/2 ] }");
}

void test_read(struct isl_ctx *ctx)
{
	char filename[PATH_MAX];
	FILE *input;
	int n;
	struct isl_basic_set *bset1, *bset2;
	const char *str = "{[y]: Exists ( alpha : 2alpha = y)}";

	n = snprintf(filename, sizeof(filename),
			"%s/test_inputs/set.omega", srcdir);
	assert(n < sizeof(filename));
	input = fopen(filename, "r");
	assert(input);

	bset1 = isl_basic_set_read_from_file(ctx, input, 0);
	bset2 = isl_basic_set_read_from_str(ctx, str, 0);

	assert(isl_basic_set_is_equal(bset1, bset2) == 1);

	isl_basic_set_free(bset1);
	isl_basic_set_free(bset2);

	fclose(input);
}

void test_bounded(struct isl_ctx *ctx)
{
	isl_set *set;
	int bounded;

	set = isl_set_read_from_str(ctx, "[n] -> {[i] : 0 <= i <= n }", -1);
	bounded = isl_set_is_bounded(set);
	assert(bounded);
	isl_set_free(set);

	set = isl_set_read_from_str(ctx, "{[n, i] : 0 <= i <= n }", -1);
	bounded = isl_set_is_bounded(set);
	assert(!bounded);
	isl_set_free(set);

	set = isl_set_read_from_str(ctx, "[n] -> {[i] : i <= n }", -1);
	bounded = isl_set_is_bounded(set);
	assert(!bounded);
	isl_set_free(set);
}

/* Construct the basic set { [i] : 5 <= i <= N } */
void test_construction(struct isl_ctx *ctx)
{
	isl_int v;
	struct isl_dim *dim;
	struct isl_basic_set *bset;
	struct isl_constraint *c;

	isl_int_init(v);

	dim = isl_dim_set_alloc(ctx, 1, 1);
	bset = isl_basic_set_universe(dim);

	c = isl_inequality_alloc(isl_basic_set_get_dim(bset));
	isl_int_set_si(v, -1);
	isl_constraint_set_coefficient(c, isl_dim_set, 0, v);
	isl_int_set_si(v, 1);
	isl_constraint_set_coefficient(c, isl_dim_param, 0, v);
	bset = isl_basic_set_add_constraint(bset, c);

	c = isl_inequality_alloc(isl_basic_set_get_dim(bset));
	isl_int_set_si(v, 1);
	isl_constraint_set_coefficient(c, isl_dim_set, 0, v);
	isl_int_set_si(v, -5);
	isl_constraint_set_constant(c, v);
	bset = isl_basic_set_add_constraint(bset, c);

	isl_basic_set_free(bset);

	isl_int_clear(v);
}

void test_dim(struct isl_ctx *ctx)
{
	const char *str;
	isl_map *map1, *map2;

	map1 = isl_map_read_from_str(ctx,
	    "[n] -> { [i] -> [j] : exists (a = [i/10] : i - 10a <= n ) }", -1);
	map1 = isl_map_add_dims(map1, isl_dim_in, 1);
	map2 = isl_map_read_from_str(ctx,
	    "[n] -> { [i,k] -> [j] : exists (a = [i/10] : i - 10a <= n ) }", -1);
	assert(isl_map_is_equal(map1, map2));
	isl_map_free(map2);

	map1 = isl_map_project_out(map1, isl_dim_in, 0, 1);
	map2 = isl_map_read_from_str(ctx, "[n] -> { [i] -> [j] : n >= 0 }", -1);
	assert(isl_map_is_equal(map1, map2));

	isl_map_free(map1);
	isl_map_free(map2);

	str = "[n] -> { [i] -> [] : exists a : 0 <= i <= n and i = 2 a }";
	map1 = isl_map_read_from_str(ctx, str, -1);
	str = "{ [i] -> [j] : exists a : 0 <= i <= j and i = 2 a }";
	map2 = isl_map_read_from_str(ctx, str, -1);
	map1 = isl_map_move_dims(map1, isl_dim_out, 0, isl_dim_param, 0, 1);
	assert(isl_map_is_equal(map1, map2));

	isl_map_free(map1);
	isl_map_free(map2);
}

void test_div(struct isl_ctx *ctx)
{
	isl_int v;
	int pos;
	struct isl_dim *dim;
	struct isl_div *div;
	struct isl_basic_set *bset;
	struct isl_constraint *c;

	isl_int_init(v);

	/* test 1 */
	dim = isl_dim_set_alloc(ctx, 0, 1);
	bset = isl_basic_set_universe(dim);

	c = isl_equality_alloc(isl_basic_set_get_dim(bset));
	isl_int_set_si(v, -1);
	isl_constraint_set_constant(c, v);
	isl_int_set_si(v, 1);
	isl_constraint_set_coefficient(c, isl_dim_set, 0, v);
	div = isl_div_alloc(isl_basic_set_get_dim(bset));
	c = isl_constraint_add_div(c, div, &pos);
	isl_int_set_si(v, 3);
	isl_constraint_set_coefficient(c, isl_dim_div, pos, v);
	bset = isl_basic_set_add_constraint(bset, c);

	c = isl_equality_alloc(isl_basic_set_get_dim(bset));
	isl_int_set_si(v, 1);
	isl_constraint_set_constant(c, v);
	isl_int_set_si(v, -1);
	isl_constraint_set_coefficient(c, isl_dim_set, 0, v);
	div = isl_div_alloc(isl_basic_set_get_dim(bset));
	c = isl_constraint_add_div(c, div, &pos);
	isl_int_set_si(v, 3);
	isl_constraint_set_coefficient(c, isl_dim_div, pos, v);
	bset = isl_basic_set_add_constraint(bset, c);

	assert(bset && bset->n_div == 1);
	isl_basic_set_free(bset);

	/* test 2 */
	dim = isl_dim_set_alloc(ctx, 0, 1);
	bset = isl_basic_set_universe(dim);

	c = isl_equality_alloc(isl_basic_set_get_dim(bset));
	isl_int_set_si(v, 1);
	isl_constraint_set_constant(c, v);
	isl_int_set_si(v, -1);
	isl_constraint_set_coefficient(c, isl_dim_set, 0, v);
	div = isl_div_alloc(isl_basic_set_get_dim(bset));
	c = isl_constraint_add_div(c, div, &pos);
	isl_int_set_si(v, 3);
	isl_constraint_set_coefficient(c, isl_dim_div, pos, v);
	bset = isl_basic_set_add_constraint(bset, c);

	c = isl_equality_alloc(isl_basic_set_get_dim(bset));
	isl_int_set_si(v, -1);
	isl_constraint_set_constant(c, v);
	isl_int_set_si(v, 1);
	isl_constraint_set_coefficient(c, isl_dim_set, 0, v);
	div = isl_div_alloc(isl_basic_set_get_dim(bset));
	c = isl_constraint_add_div(c, div, &pos);
	isl_int_set_si(v, 3);
	isl_constraint_set_coefficient(c, isl_dim_div, pos, v);
	bset = isl_basic_set_add_constraint(bset, c);

	assert(bset && bset->n_div == 1);
	isl_basic_set_free(bset);

	/* test 3 */
	dim = isl_dim_set_alloc(ctx, 0, 1);
	bset = isl_basic_set_universe(dim);

	c = isl_equality_alloc(isl_basic_set_get_dim(bset));
	isl_int_set_si(v, 1);
	isl_constraint_set_constant(c, v);
	isl_int_set_si(v, -1);
	isl_constraint_set_coefficient(c, isl_dim_set, 0, v);
	div = isl_div_alloc(isl_basic_set_get_dim(bset));
	c = isl_constraint_add_div(c, div, &pos);
	isl_int_set_si(v, 3);
	isl_constraint_set_coefficient(c, isl_dim_div, pos, v);
	bset = isl_basic_set_add_constraint(bset, c);

	c = isl_equality_alloc(isl_basic_set_get_dim(bset));
	isl_int_set_si(v, -3);
	isl_constraint_set_constant(c, v);
	isl_int_set_si(v, 1);
	isl_constraint_set_coefficient(c, isl_dim_set, 0, v);
	div = isl_div_alloc(isl_basic_set_get_dim(bset));
	c = isl_constraint_add_div(c, div, &pos);
	isl_int_set_si(v, 4);
	isl_constraint_set_coefficient(c, isl_dim_div, pos, v);
	bset = isl_basic_set_add_constraint(bset, c);

	assert(bset && bset->n_div == 1);
	isl_basic_set_free(bset);

	/* test 4 */
	dim = isl_dim_set_alloc(ctx, 0, 1);
	bset = isl_basic_set_universe(dim);

	c = isl_equality_alloc(isl_basic_set_get_dim(bset));
	isl_int_set_si(v, 2);
	isl_constraint_set_constant(c, v);
	isl_int_set_si(v, -1);
	isl_constraint_set_coefficient(c, isl_dim_set, 0, v);
	div = isl_div_alloc(isl_basic_set_get_dim(bset));
	c = isl_constraint_add_div(c, div, &pos);
	isl_int_set_si(v, 3);
	isl_constraint_set_coefficient(c, isl_dim_div, pos, v);
	bset = isl_basic_set_add_constraint(bset, c);

	c = isl_equality_alloc(isl_basic_set_get_dim(bset));
	isl_int_set_si(v, -1);
	isl_constraint_set_constant(c, v);
	isl_int_set_si(v, 1);
	isl_constraint_set_coefficient(c, isl_dim_set, 0, v);
	div = isl_div_alloc(isl_basic_set_get_dim(bset));
	c = isl_constraint_add_div(c, div, &pos);
	isl_int_set_si(v, 6);
	isl_constraint_set_coefficient(c, isl_dim_div, pos, v);
	bset = isl_basic_set_add_constraint(bset, c);

	assert(isl_basic_set_is_empty(bset));
	isl_basic_set_free(bset);

	/* test 5 */
	dim = isl_dim_set_alloc(ctx, 0, 2);
	bset = isl_basic_set_universe(dim);

	c = isl_equality_alloc(isl_basic_set_get_dim(bset));
	isl_int_set_si(v, -1);
	isl_constraint_set_coefficient(c, isl_dim_set, 0, v);
	div = isl_div_alloc(isl_basic_set_get_dim(bset));
	c = isl_constraint_add_div(c, div, &pos);
	isl_int_set_si(v, 3);
	isl_constraint_set_coefficient(c, isl_dim_div, pos, v);
	bset = isl_basic_set_add_constraint(bset, c);

	c = isl_equality_alloc(isl_basic_set_get_dim(bset));
	isl_int_set_si(v, 1);
	isl_constraint_set_coefficient(c, isl_dim_set, 0, v);
	isl_int_set_si(v, -3);
	isl_constraint_set_coefficient(c, isl_dim_set, 1, v);
	bset = isl_basic_set_add_constraint(bset, c);

	assert(bset && bset->n_div == 0);
	isl_basic_set_free(bset);

	/* test 6 */
	dim = isl_dim_set_alloc(ctx, 0, 2);
	bset = isl_basic_set_universe(dim);

	c = isl_equality_alloc(isl_basic_set_get_dim(bset));
	isl_int_set_si(v, -1);
	isl_constraint_set_coefficient(c, isl_dim_set, 0, v);
	div = isl_div_alloc(isl_basic_set_get_dim(bset));
	c = isl_constraint_add_div(c, div, &pos);
	isl_int_set_si(v, 6);
	isl_constraint_set_coefficient(c, isl_dim_div, pos, v);
	bset = isl_basic_set_add_constraint(bset, c);

	c = isl_equality_alloc(isl_basic_set_get_dim(bset));
	isl_int_set_si(v, 1);
	isl_constraint_set_coefficient(c, isl_dim_set, 0, v);
	isl_int_set_si(v, -3);
	isl_constraint_set_coefficient(c, isl_dim_set, 1, v);
	bset = isl_basic_set_add_constraint(bset, c);

	assert(bset && bset->n_div == 1);
	isl_basic_set_free(bset);

	/* test 7 */
	/* This test is a bit tricky.  We set up an equality
	 *		a + 3b + 3c = 6 e0
	 * Normalization of divs creates _two_ divs
	 *		a = 3 e0
	 *		c - b - e0 = 2 e1
	 * Afterwards e0 is removed again because it has coefficient -1
	 * and we end up with the original equality and div again.
	 * Perhaps we can avoid the introduction of this temporary div.
	 */
	dim = isl_dim_set_alloc(ctx, 0, 3);
	bset = isl_basic_set_universe(dim);

	c = isl_equality_alloc(isl_basic_set_get_dim(bset));
	isl_int_set_si(v, -1);
	isl_constraint_set_coefficient(c, isl_dim_set, 0, v);
	isl_int_set_si(v, -3);
	isl_constraint_set_coefficient(c, isl_dim_set, 1, v);
	isl_int_set_si(v, -3);
	isl_constraint_set_coefficient(c, isl_dim_set, 2, v);
	div = isl_div_alloc(isl_basic_set_get_dim(bset));
	c = isl_constraint_add_div(c, div, &pos);
	isl_int_set_si(v, 6);
	isl_constraint_set_coefficient(c, isl_dim_div, pos, v);
	bset = isl_basic_set_add_constraint(bset, c);

	/* Test disabled for now */
	/*
	assert(bset && bset->n_div == 1);
	*/
	isl_basic_set_free(bset);

	/* test 8 */
	dim = isl_dim_set_alloc(ctx, 0, 4);
	bset = isl_basic_set_universe(dim);

	c = isl_equality_alloc(isl_basic_set_get_dim(bset));
	isl_int_set_si(v, -1);
	isl_constraint_set_coefficient(c, isl_dim_set, 0, v);
	isl_int_set_si(v, -3);
	isl_constraint_set_coefficient(c, isl_dim_set, 1, v);
	isl_int_set_si(v, -3);
	isl_constraint_set_coefficient(c, isl_dim_set, 3, v);
	div = isl_div_alloc(isl_basic_set_get_dim(bset));
	c = isl_constraint_add_div(c, div, &pos);
	isl_int_set_si(v, 6);
	isl_constraint_set_coefficient(c, isl_dim_div, pos, v);
	bset = isl_basic_set_add_constraint(bset, c);

	c = isl_equality_alloc(isl_basic_set_get_dim(bset));
	isl_int_set_si(v, -1);
	isl_constraint_set_coefficient(c, isl_dim_set, 0, v);
	isl_int_set_si(v, 1);
	isl_constraint_set_coefficient(c, isl_dim_set, 2, v);
	isl_int_set_si(v, 1);
	isl_constraint_set_constant(c, v);
	bset = isl_basic_set_add_constraint(bset, c);

	/* Test disabled for now */
	/*
	assert(bset && bset->n_div == 1);
	*/
	isl_basic_set_free(bset);

	/* test 9 */
	dim = isl_dim_set_alloc(ctx, 0, 2);
	bset = isl_basic_set_universe(dim);

	c = isl_equality_alloc(isl_basic_set_get_dim(bset));
	isl_int_set_si(v, 1);
	isl_constraint_set_coefficient(c, isl_dim_set, 0, v);
	isl_int_set_si(v, -1);
	isl_constraint_set_coefficient(c, isl_dim_set, 1, v);
	div = isl_div_alloc(isl_basic_set_get_dim(bset));
	c = isl_constraint_add_div(c, div, &pos);
	isl_int_set_si(v, -2);
	isl_constraint_set_coefficient(c, isl_dim_div, pos, v);
	bset = isl_basic_set_add_constraint(bset, c);

	c = isl_equality_alloc(isl_basic_set_get_dim(bset));
	isl_int_set_si(v, -1);
	isl_constraint_set_coefficient(c, isl_dim_set, 0, v);
	div = isl_div_alloc(isl_basic_set_get_dim(bset));
	c = isl_constraint_add_div(c, div, &pos);
	isl_int_set_si(v, 3);
	isl_constraint_set_coefficient(c, isl_dim_div, pos, v);
	isl_int_set_si(v, 2);
	isl_constraint_set_constant(c, v);
	bset = isl_basic_set_add_constraint(bset, c);

	bset = isl_basic_set_fix_si(bset, isl_dim_set, 0, 2);

	assert(!isl_basic_set_is_empty(bset));

	isl_basic_set_free(bset);

	/* test 10 */
	dim = isl_dim_set_alloc(ctx, 0, 2);
	bset = isl_basic_set_universe(dim);

	c = isl_equality_alloc(isl_basic_set_get_dim(bset));
	isl_int_set_si(v, 1);
	isl_constraint_set_coefficient(c, isl_dim_set, 0, v);
	div = isl_div_alloc(isl_basic_set_get_dim(bset));
	c = isl_constraint_add_div(c, div, &pos);
	isl_int_set_si(v, -2);
	isl_constraint_set_coefficient(c, isl_dim_div, pos, v);
	bset = isl_basic_set_add_constraint(bset, c);

	bset = isl_basic_set_fix_si(bset, isl_dim_set, 0, 2);

	isl_basic_set_free(bset);

	isl_int_clear(v);
}

void test_application_case(struct isl_ctx *ctx, const char *name)
{
	char filename[PATH_MAX];
	FILE *input;
	int n;
	struct isl_basic_set *bset1, *bset2;
	struct isl_basic_map *bmap;

	n = snprintf(filename, sizeof(filename),
			"%s/test_inputs/%s.omega", srcdir, name);
	assert(n < sizeof(filename));
	input = fopen(filename, "r");
	assert(input);

	bset1 = isl_basic_set_read_from_file(ctx, input, 0);
	bmap = isl_basic_map_read_from_file(ctx, input, 0);

	bset1 = isl_basic_set_apply(bset1, bmap);

	bset2 = isl_basic_set_read_from_file(ctx, input, 0);

	assert(isl_basic_set_is_equal(bset1, bset2) == 1);

	isl_basic_set_free(bset1);
	isl_basic_set_free(bset2);

	fclose(input);
}

void test_application(struct isl_ctx *ctx)
{
	test_application_case(ctx, "application");
	test_application_case(ctx, "application2");
}

void test_affine_hull_case(struct isl_ctx *ctx, const char *name)
{
	char filename[PATH_MAX];
	FILE *input;
	int n;
	struct isl_basic_set *bset1, *bset2;

	n = snprintf(filename, sizeof(filename),
			"%s/test_inputs/%s.polylib", srcdir, name);
	assert(n < sizeof(filename));
	input = fopen(filename, "r");
	assert(input);

	bset1 = isl_basic_set_read_from_file(ctx, input, 0);
	bset2 = isl_basic_set_read_from_file(ctx, input, 0);

	bset1 = isl_basic_set_affine_hull(bset1);

	assert(isl_basic_set_is_equal(bset1, bset2) == 1);

	isl_basic_set_free(bset1);
	isl_basic_set_free(bset2);

	fclose(input);
}

void test_affine_hull(struct isl_ctx *ctx)
{
	test_affine_hull_case(ctx, "affine2");
	test_affine_hull_case(ctx, "affine");
	test_affine_hull_case(ctx, "affine3");
}

void test_convex_hull_case(struct isl_ctx *ctx, const char *name)
{
	char filename[PATH_MAX];
	FILE *input;
	int n;
	struct isl_basic_set *bset1, *bset2;
	struct isl_set *set;

	n = snprintf(filename, sizeof(filename),
			"%s/test_inputs/%s.polylib", srcdir, name);
	assert(n < sizeof(filename));
	input = fopen(filename, "r");
	assert(input);

	bset1 = isl_basic_set_read_from_file(ctx, input, 0);
	bset2 = isl_basic_set_read_from_file(ctx, input, 0);

	set = isl_basic_set_union(bset1, bset2);
	bset1 = isl_set_convex_hull(set);

	bset2 = isl_basic_set_read_from_file(ctx, input, 0);

	assert(isl_basic_set_is_equal(bset1, bset2) == 1);

	isl_basic_set_free(bset1);
	isl_basic_set_free(bset2);

	fclose(input);
}

void test_convex_hull_algo(struct isl_ctx *ctx, int convex)
{
	const char *str1, *str2;
	isl_set *set1, *set2;
	int orig_convex = ctx->opt->convex;
	ctx->opt->convex = convex;

	test_convex_hull_case(ctx, "convex0");
	test_convex_hull_case(ctx, "convex1");
	test_convex_hull_case(ctx, "convex2");
	test_convex_hull_case(ctx, "convex3");
	test_convex_hull_case(ctx, "convex4");
	test_convex_hull_case(ctx, "convex5");
	test_convex_hull_case(ctx, "convex6");
	test_convex_hull_case(ctx, "convex7");
	test_convex_hull_case(ctx, "convex8");
	test_convex_hull_case(ctx, "convex9");
	test_convex_hull_case(ctx, "convex10");
	test_convex_hull_case(ctx, "convex11");
	test_convex_hull_case(ctx, "convex12");
	test_convex_hull_case(ctx, "convex13");
	test_convex_hull_case(ctx, "convex14");
	test_convex_hull_case(ctx, "convex15");

	str1 = "{ [i0, i1, i2] : (i2 = 1 and i0 = 0 and i1 >= 0) or "
	       "(i0 = 1 and i1 = 0 and i2 = 1) or "
	       "(i0 = 0 and i1 = 0 and i2 = 0) }";
	str2 = "{ [i0, i1, i2] : i0 >= 0 and i2 >= i0 and i2 <= 1 and i1 >= 0 }";
	set1 = isl_set_read_from_str(ctx, str1, -1);
	set2 = isl_set_read_from_str(ctx, str2, -1);
	set1 = isl_set_from_basic_set(isl_set_convex_hull(set1));
	assert(isl_set_is_equal(set1, set2));
	isl_set_free(set1);
	isl_set_free(set2);

	ctx->opt->convex = orig_convex;
}

void test_convex_hull(struct isl_ctx *ctx)
{
	test_convex_hull_algo(ctx, ISL_CONVEX_HULL_FM);
	test_convex_hull_algo(ctx, ISL_CONVEX_HULL_WRAP);
}

void test_gist_case(struct isl_ctx *ctx, const char *name)
{
	char filename[PATH_MAX];
	FILE *input;
	int n;
	struct isl_basic_set *bset1, *bset2;

	n = snprintf(filename, sizeof(filename),
			"%s/test_inputs/%s.polylib", srcdir, name);
	assert(n < sizeof(filename));
	input = fopen(filename, "r");
	assert(input);

	bset1 = isl_basic_set_read_from_file(ctx, input, 0);
	bset2 = isl_basic_set_read_from_file(ctx, input, 0);

	bset1 = isl_basic_set_gist(bset1, bset2);

	bset2 = isl_basic_set_read_from_file(ctx, input, 0);

	assert(isl_basic_set_is_equal(bset1, bset2) == 1);

	isl_basic_set_free(bset1);
	isl_basic_set_free(bset2);

	fclose(input);
}

void test_gist(struct isl_ctx *ctx)
{
	const char *str;
	isl_basic_set *bset1, *bset2;

	test_gist_case(ctx, "gist1");

	str = "[p0, p2, p3, p5, p6, p10] -> { [] : "
	    "exists (e0 = [(15 + p0 + 15p6 + 15p10)/16], e1 = [(p5)/8], "
	    "e2 = [(p6)/128], e3 = [(8p2 - p5)/128], "
	    "e4 = [(128p3 - p6)/4096]: 8e1 = p5 and 128e2 = p6 and "
	    "128e3 = 8p2 - p5 and 4096e4 = 128p3 - p6 and p2 >= 0 and "
	    "16e0 >= 16 + 16p6 + 15p10 and  p2 <= 15 and p3 >= 0 and "
	    "p3 <= 31 and  p6 >= 128p3 and p5 >= 8p2 and p10 >= 0 and "
	    "16e0 <= 15 + p0 + 15p6 + 15p10 and 16e0 >= p0 + 15p6 + 15p10 and "
	    "p10 <= 15 and p10 <= -1 + p0 - p6) }";
	bset1 = isl_basic_set_read_from_str(ctx, str, -1);
	str = "[p0, p2, p3, p5, p6, p10] -> { [] : exists (e0 = [(p5)/8], "
	    "e1 = [(p6)/128], e2 = [(8p2 - p5)/128], "
	    "e3 = [(128p3 - p6)/4096]: 8e0 = p5 and 128e1 = p6 and "
	    "128e2 = 8p2 - p5 and 4096e3 = 128p3 - p6 and p5 >= -7 and "
	    "p2 >= 0 and 8p2 <= -1 + p0 and p2 <= 15 and p3 >= 0 and "
	    "p3 <= 31 and 128p3 <= -1 + p0 and p6 >= -127 and "
	    "p5 <= -1 + p0 and p6 <= -1 + p0 and p6 >= 128p3 and "
	    "p0 >= 1 and p5 >= 8p2 and p10 >= 0 and p10 <= 15 ) }";
	bset2 = isl_basic_set_read_from_str(ctx, str, -1);
	bset1 = isl_basic_set_gist(bset1, bset2);
	assert(bset1 && bset1->n_div == 0);
	isl_basic_set_free(bset1);
}

void test_coalesce_set(isl_ctx *ctx, const char *str, int check_one)
{
	isl_set *set, *set2;

	set = isl_set_read_from_str(ctx, str, -1);
	set = isl_set_coalesce(set);
	set2 = isl_set_read_from_str(ctx, str, -1);
	assert(isl_set_is_equal(set, set2));
	if (check_one)
		assert(set && set->n == 1);
	isl_set_free(set);
	isl_set_free(set2);
}

void test_coalesce(struct isl_ctx *ctx)
{
	const char *str;
	struct isl_set *set, *set2;
	struct isl_map *map, *map2;

	set = isl_set_read_from_str(ctx,
		"{[x,y]: x >= 0 & x <= 10 & y >= 0 & y <= 10 or "
		       "y >= x & x >= 2 & 5 >= y }", -1);
	set = isl_set_coalesce(set);
	assert(set && set->n == 1);
	isl_set_free(set);

	set = isl_set_read_from_str(ctx,
		"{[x,y]: y >= 0 & 2x + y <= 30 & y <= 10 & x >= 0 or "
		       "x + y >= 10 & y <= x & x + y <= 20 & y >= 0}", -1);
	set = isl_set_coalesce(set);
	assert(set && set->n == 1);
	isl_set_free(set);

	set = isl_set_read_from_str(ctx,
		"{[x,y]: y >= 0 & 2x + y <= 30 & y <= 10 & x >= 0 or "
		       "x + y >= 10 & y <= x & x + y <= 19 & y >= 0}", -1);
	set = isl_set_coalesce(set);
	assert(set && set->n == 2);
	isl_set_free(set);

	set = isl_set_read_from_str(ctx,
		"{[x,y]: y >= 0 & x <= 5 & y <= x or "
		       "y >= 0 & x >= 6 & x <= 10 & y <= x}", -1);
	set = isl_set_coalesce(set);
	assert(set && set->n == 1);
	isl_set_free(set);

	set = isl_set_read_from_str(ctx,
		"{[x,y]: y >= 0 & x <= 5 & y <= x or "
		       "y >= 0 & x >= 7 & x <= 10 & y <= x}", -1);
	set = isl_set_coalesce(set);
	assert(set && set->n == 2);
	isl_set_free(set);

	set = isl_set_read_from_str(ctx,
		"{[x,y]: y >= 0 & x <= 5 & y <= x or "
		       "y >= 0 & x >= 6 & x <= 10 & y + 1 <= x}", -1);
	set = isl_set_coalesce(set);
	assert(set && set->n == 2);
	isl_set_free(set);

	set = isl_set_read_from_str(ctx,
		"{[x,y]: y >= 0 & x <= 5 & y <= x or "
		       "y >= 0 & x = 6 & y <= 6}", -1);
	set = isl_set_coalesce(set);
	assert(set && set->n == 1);
	isl_set_free(set);

	set = isl_set_read_from_str(ctx,
		"{[x,y]: y >= 0 & x <= 5 & y <= x or "
		       "y >= 0 & x = 7 & y <= 6}", -1);
	set = isl_set_coalesce(set);
	assert(set && set->n == 2);
	isl_set_free(set);

	set = isl_set_read_from_str(ctx,
		"{[x,y]: y >= 0 & x <= 5 & y <= x or "
		       "y >= 0 & x = 6 & y <= 5}", -1);
	set = isl_set_coalesce(set);
	assert(set && set->n == 1);
	set2 = isl_set_read_from_str(ctx,
		"{[x,y]: y >= 0 & x <= 5 & y <= x or "
		       "y >= 0 & x = 6 & y <= 5}", -1);
	assert(isl_set_is_equal(set, set2));
	isl_set_free(set);
	isl_set_free(set2);

	set = isl_set_read_from_str(ctx,
		"{[x,y]: y >= 0 & x <= 5 & y <= x or "
		       "y >= 0 & x = 6 & y <= 7}", -1);
	set = isl_set_coalesce(set);
	assert(set && set->n == 2);
	isl_set_free(set);

	set = isl_set_read_from_str(ctx,
		"[n] -> { [i] : i = 1 and n >= 2 or 2 <= i and i <= n }", -1);
	set = isl_set_coalesce(set);
	assert(set && set->n == 1);
	set2 = isl_set_read_from_str(ctx,
		"[n] -> { [i] : i = 1 and n >= 2 or 2 <= i and i <= n }", -1);
	assert(isl_set_is_equal(set, set2));
	isl_set_free(set);
	isl_set_free(set2);

	set = isl_set_read_from_str(ctx,
		"{[x,y] : x >= 0 and y >= 0 or 0 <= y and y <= 5 and x = -1}", -1);
	set = isl_set_coalesce(set);
	set2 = isl_set_read_from_str(ctx,
		"{[x,y] : x >= 0 and y >= 0 or 0 <= y and y <= 5 and x = -1}", -1);
	assert(isl_set_is_equal(set, set2));
	isl_set_free(set);
	isl_set_free(set2);

	set = isl_set_read_from_str(ctx,
		"[n] -> { [i] : 1 <= i and i <= n - 1 or "
				"2 <= i and i <= n }", -1);
	set = isl_set_coalesce(set);
	assert(set && set->n == 1);
	set2 = isl_set_read_from_str(ctx,
		"[n] -> { [i] : 1 <= i and i <= n - 1 or "
				"2 <= i and i <= n }", -1);
	assert(isl_set_is_equal(set, set2));
	isl_set_free(set);
	isl_set_free(set2);

	map = isl_map_read_from_str(ctx,
		"[n] -> { [i0] -> [o0] : exists (e0 = [(i0)/4], e1 = [(o0)/4], "
		"e2 = [(n)/2], e3 = [(-2 + i0)/4], e4 = [(-2 + o0)/4], "
		"e5 = [(-2n + i0)/4]: 2e2 = n and 4e3 = -2 + i0 and "
		"4e4 = -2 + o0 and i0 >= 8 + 2n and o0 >= 2 + i0 and "
		"o0 <= 56 + 2n and o0 <= -12 + 4n and i0 <= 57 + 2n and "
		"i0 <= -11 + 4n and o0 >= 6 + 2n and 4e0 <= i0 and "
		"4e0 >= -3 + i0 and 4e1 <= o0 and 4e1 >= -3 + o0 and "
		"4e5 <= -2n + i0 and 4e5 >= -3 - 2n + i0);"
		"[i0] -> [o0] : exists (e0 = [(i0)/4], e1 = [(o0)/4], "
		"e2 = [(n)/2], e3 = [(-2 + i0)/4], e4 = [(-2 + o0)/4], "
		"e5 = [(-2n + i0)/4]: 2e2 = n and 4e3 = -2 + i0 and "
		"4e4 = -2 + o0 and 2e0 >= 3 + n and e0 <= -4 + n and "
		"2e0 <= 27 + n and e1 <= -4 + n and 2e1 <= 27 + n and "
		"2e1 >= 2 + n and e1 >= 1 + e0 and i0 >= 7 + 2n and "
		"i0 <= -11 + 4n and i0 <= 57 + 2n and 4e0 <= -2 + i0 and "
		"4e0 >= -3 + i0 and o0 >= 6 + 2n and o0 <= -11 + 4n and "
		"o0 <= 57 + 2n and 4e1 <= -2 + o0 and 4e1 >= -3 + o0 and "
		"4e5 <= -2n + i0 and 4e5 >= -3 - 2n + i0 ) }", -1);
	map = isl_map_coalesce(map);
	map2 = isl_map_read_from_str(ctx,
		"[n] -> { [i0] -> [o0] : exists (e0 = [(i0)/4], e1 = [(o0)/4], "
		"e2 = [(n)/2], e3 = [(-2 + i0)/4], e4 = [(-2 + o0)/4], "
		"e5 = [(-2n + i0)/4]: 2e2 = n and 4e3 = -2 + i0 and "
		"4e4 = -2 + o0 and i0 >= 8 + 2n and o0 >= 2 + i0 and "
		"o0 <= 56 + 2n and o0 <= -12 + 4n and i0 <= 57 + 2n and "
		"i0 <= -11 + 4n and o0 >= 6 + 2n and 4e0 <= i0 and "
		"4e0 >= -3 + i0 and 4e1 <= o0 and 4e1 >= -3 + o0 and "
		"4e5 <= -2n + i0 and 4e5 >= -3 - 2n + i0);"
		"[i0] -> [o0] : exists (e0 = [(i0)/4], e1 = [(o0)/4], "
		"e2 = [(n)/2], e3 = [(-2 + i0)/4], e4 = [(-2 + o0)/4], "
		"e5 = [(-2n + i0)/4]: 2e2 = n and 4e3 = -2 + i0 and "
		"4e4 = -2 + o0 and 2e0 >= 3 + n and e0 <= -4 + n and "
		"2e0 <= 27 + n and e1 <= -4 + n and 2e1 <= 27 + n and "
		"2e1 >= 2 + n and e1 >= 1 + e0 and i0 >= 7 + 2n and "
		"i0 <= -11 + 4n and i0 <= 57 + 2n and 4e0 <= -2 + i0 and "
		"4e0 >= -3 + i0 and o0 >= 6 + 2n and o0 <= -11 + 4n and "
		"o0 <= 57 + 2n and 4e1 <= -2 + o0 and 4e1 >= -3 + o0 and "
		"4e5 <= -2n + i0 and 4e5 >= -3 - 2n + i0 ) }", -1);
	assert(isl_map_is_equal(map, map2));
	isl_map_free(map);
	isl_map_free(map2);

	str = "[n, m] -> { [] -> [o0, o2, o3] : (o3 = 1 and o0 >= 1 + m and "
	      "o0 <= n + m and o2 <= m and o0 >= 2 + n and o2 >= 3) or "
	      "(o0 >= 2 + n and o0 >= 1 + m and o0 <= n + m and n >= 1 and "
	      "o3 <= -1 + o2 and o3 >= 1 - m + o2 and o3 >= 2 and o3 <= n) }";
	map = isl_map_read_from_str(ctx, str, -1);
	map = isl_map_coalesce(map);
	map2 = isl_map_read_from_str(ctx, str, -1);
	assert(isl_map_is_equal(map, map2));
	isl_map_free(map);
	isl_map_free(map2);

	str = "[M, N] -> { [i0, i1, i2, i3, i4, i5, i6] -> "
	  "[o0, o1, o2, o3, o4, o5, o6] : "
	  "(o6 <= -4 + 2M - 2N + i0 + i1 - i2 + i6 - o0 - o1 + o2 and "
	  "o3 <= -2 + i3 and o6 >= 2 + i0 + i3 + i6 - o0 - o3 and "
	  "o6 >= 2 - M + N + i3 + i4 + i6 - o3 - o4 and o0 <= -1 + i0 and "
	  "o4 >= 4 - 3M + 3N - i0 - i1 + i2 + 2i3 + i4 + o0 + o1 - o2 - 2o3 "
	  "and o6 <= -3 + 2M - 2N + i3 + i4 - i5 + i6 - o3 - o4 + o5 and "
	  "2o6 <= -5 + 5M - 5N + 2i0 + i1 - i2 - i5 + 2i6 - 2o0 - o1 + o2 + o5 "
	  "and o6 >= 2i0 + i1 + i6 - 2o0 - o1 and "
	  "3o6 <= -5 + 4M - 4N + 2i0 + i1 - i2 + 2i3 + i4 - i5 + 3i6 "
	  "- 2o0 - o1 + o2 - 2o3 - o4 + o5) or "
	  "(N >= 2 and o3 <= -1 + i3 and o0 <= -1 + i0 and "
	  "o6 >= i3 + i6 - o3 and M >= 0 and "
	  "2o6 >= 1 + i0 + i3 + 2i6 - o0 - o3 and "
	  "o6 >= 1 - M + i0 + i6 - o0 and N >= 2M and o6 >= i0 + i6 - o0) }";
	map = isl_map_read_from_str(ctx, str, -1);
	map = isl_map_coalesce(map);
	map2 = isl_map_read_from_str(ctx, str, -1);
	assert(isl_map_is_equal(map, map2));
	isl_map_free(map);
	isl_map_free(map2);

	str = "[M, N] -> { [] -> [o0] : (o0 = 0 and M >= 1 and N >= 2) or "
		"(o0 = 0 and M >= 1 and N >= 2M and N >= 2 + M) or "
		"(o0 = 0 and M >= 2 and N >= 3) or "
		"(M = 0 and o0 = 0 and N >= 3) }";
	map = isl_map_read_from_str(ctx, str, -1);
	map = isl_map_coalesce(map);
	map2 = isl_map_read_from_str(ctx, str, -1);
	assert(isl_map_is_equal(map, map2));
	isl_map_free(map);
	isl_map_free(map2);

	str = "{ [i0, i1, i2, i3] : (i1 = 10i0 and i0 >= 1 and 10i0 <= 100 and "
		"i3 <= 9 + 10 i2 and i3 >= 1 + 10i2 and i3 >= 0) or "
		"(i1 <= 9 + 10i0 and i1 >= 1 + 10i0 and i2 >= 0 and "
		"i0 >= 0 and i1 <= 100 and i3 <= 9 + 10i2 and i3 >= 1 + 10i2) }";
	map = isl_map_read_from_str(ctx, str, -1);
	map = isl_map_coalesce(map);
	map2 = isl_map_read_from_str(ctx, str, -1);
	assert(isl_map_is_equal(map, map2));
	isl_map_free(map);
	isl_map_free(map2);

	test_coalesce_set(ctx,
		"[M] -> { [i1] : (i1 >= 2 and i1 <= M) or "
				"(i1 = M and M >= 1) }", 0);
	test_coalesce_set(ctx,
		"{[x,y] : x,y >= 0; [x,y] : 10 <= x <= 20 and y >= -1 }", 0);
	test_coalesce_set(ctx,
		"{ [x, y] : (x >= 1 and y >= 1 and x <= 2 and y <= 2) or "
		"(y = 3 and x = 1) }", 1);
	test_coalesce_set(ctx,
		"[M] -> { [i0, i1, i2, i3, i4] : (i1 >= 3 and i4 >= 2 + i2 and "
		"i2 >= 2 and i0 >= 2 and i3 >= 1 + i2 and i0 <= M and "
		"i1 <= M and i3 <= M and i4 <= M) or "
		"(i1 >= 2 and i4 >= 1 + i2 and i2 >= 2 and i0 >= 2 and "
		"i3 >= 1 + i2 and i0 <= M and i1 <= -1 + M and i3 <= M and "
		"i4 <= -1 + M) }", 1);
	test_coalesce_set(ctx,
		"{ [x, y] : (x >= 0 and y >= 0 and x <= 10 and y <= 10) or "
		"(x >= 1 and y >= 1 and x <= 11 and y <= 11) }", 1);
	test_coalesce_set(ctx,
		"{[x,y,z] : y + 2 >= 0 and x - y + 1 >= 0 and "
			"-x - y + 1 >= 0 and -3 <= z <= 3;"
		"[x,y,z] : -x+z + 20 >= 0 and -x-z + 20 >= 0 and "
			"x-z + 20 >= 0 and x+z + 20 >= 0 and -10 <= y <= 0}", 1);
	test_coalesce_set(ctx,
		"{[x,y] : 0 <= x,y <= 10; [5,y]: 4 <=y <= 11}", 1);
	test_coalesce_set(ctx, "{[x,0] : x >= 0; [x,1] : x <= 20}", 0);
	test_coalesce_set(ctx,
		"{[x,0,0] : -5 <= x <= 5; [0,y,1] : -5 <= y <= 5 }", 1);
	test_coalesce_set(ctx, "{ [x, 1 - x] : 0 <= x <= 1; [0,0] }", 1);
	test_coalesce_set(ctx, "{ [0,0]; [i,i] : 1 <= i <= 10 }", 1);
	test_coalesce_set(ctx, "{ [0,0]; [i,j] : 1 <= i,j <= 10 }", 0);
	test_coalesce_set(ctx, "{ [0,0]; [i,2i] : 1 <= i <= 10 }", 1);
	test_coalesce_set(ctx, "{ [0,0]; [i,2i] : 2 <= i <= 10 }", 0);
	test_coalesce_set(ctx, "{ [1,0]; [i,2i] : 1 <= i <= 10 }", 0);
	test_coalesce_set(ctx, "{ [0,1]; [i,2i] : 1 <= i <= 10 }", 0);
}

void test_closure(struct isl_ctx *ctx)
{
	const char *str;
	isl_set *dom;
	isl_map *up, *right;
	isl_map *map, *map2;
	int exact;

	/* COCOA example 1 */
	map = isl_map_read_from_str(ctx,
		"[n] -> { [i,j] -> [i2,j2] : i2 = i + 1 and j2 = j + 1 and "
			"1 <= i and i < n and 1 <= j and j < n or "
			"i2 = i + 1 and j2 = j - 1 and "
			"1 <= i and i < n and 2 <= j and j <= n }", -1);
	map = isl_map_power(map, &exact);
	assert(exact);
	isl_map_free(map);

	/* COCOA example 1 */
	map = isl_map_read_from_str(ctx,
		"[n] -> { [i,j] -> [i2,j2] : i2 = i + 1 and j2 = j + 1 and "
			"1 <= i and i < n and 1 <= j and j < n or "
			"i2 = i + 1 and j2 = j - 1 and "
			"1 <= i and i < n and 2 <= j and j <= n }", -1);
	map = isl_map_transitive_closure(map, &exact);
	assert(exact);
	map2 = isl_map_read_from_str(ctx,
		"[n] -> { [i,j] -> [i2,j2] : exists (k1,k2,k : "
			"1 <= i and i < n and 1 <= j and j <= n and "
			"2 <= i2 and i2 <= n and 1 <= j2 and j2 <= n and "
			"i2 = i + k1 + k2 and j2 = j + k1 - k2 and "
			"k1 >= 0 and k2 >= 0 and k1 + k2 = k and k >= 1 )}", -1);
	assert(isl_map_is_equal(map, map2));
	isl_map_free(map2);
	isl_map_free(map);

	map = isl_map_read_from_str(ctx,
		"[n] -> { [x] -> [y] : y = x + 1 and 0 <= x and x <= n and "
				     " 0 <= y and y <= n }", -1);
	map = isl_map_transitive_closure(map, &exact);
	map2 = isl_map_read_from_str(ctx,
		"[n] -> { [x] -> [y] : y > x and 0 <= x and x <= n and "
				     " 0 <= y and y <= n }", -1);
	assert(isl_map_is_equal(map, map2));
	isl_map_free(map2);
	isl_map_free(map);

	/* COCOA example 2 */
	map = isl_map_read_from_str(ctx,
		"[n] -> { [i,j] -> [i2,j2] : i2 = i + 2 and j2 = j + 2 and "
			"1 <= i and i < n - 1 and 1 <= j and j < n - 1 or "
			"i2 = i + 2 and j2 = j - 2 and "
			"1 <= i and i < n - 1 and 3 <= j and j <= n }", -1);
	map = isl_map_transitive_closure(map, &exact);
	assert(exact);
	map2 = isl_map_read_from_str(ctx,
		"[n] -> { [i,j] -> [i2,j2] : exists (k1,k2,k : "
			"1 <= i and i < n - 1 and 1 <= j and j <= n and "
			"3 <= i2 and i2 <= n and 1 <= j2 and j2 <= n and "
			"i2 = i + 2 k1 + 2 k2 and j2 = j + 2 k1 - 2 k2 and "
			"k1 >= 0 and k2 >= 0 and k1 + k2 = k and k >= 1) }", -1);
	assert(isl_map_is_equal(map, map2));
	isl_map_free(map);
	isl_map_free(map2);

	/* COCOA Fig.2 left */
	map = isl_map_read_from_str(ctx,
		"[n] -> { [i,j] -> [i2,j2] : i2 = i + 2 and j2 = j and "
			"i <= 2 j - 3 and i <= n - 2 and j <= 2 i - 1 and "
			"j <= n or "
			"i2 = i and j2 = j + 2 and i <= 2 j - 1 and i <= n and "
			"j <= 2 i - 3 and j <= n - 2 or "
			"i2 = i + 1 and j2 = j + 1 and i <= 2 j - 1 and "
			"i <= n - 1 and j <= 2 i - 1 and j <= n - 1 }", -1);
	map = isl_map_transitive_closure(map, &exact);
	assert(exact);
	isl_map_free(map);

	/* COCOA Fig.2 right */
	map = isl_map_read_from_str(ctx,
		"[n] -> { [i,j] -> [i2,j2] : i2 = i + 3 and j2 = j and "
			"i <= 2 j - 4 and i <= n - 3 and j <= 2 i - 1 and "
			"j <= n or "
			"i2 = i and j2 = j + 3 and i <= 2 j - 1 and i <= n and "
			"j <= 2 i - 4 and j <= n - 3 or "
			"i2 = i + 1 and j2 = j + 1 and i <= 2 j - 1 and "
			"i <= n - 1 and j <= 2 i - 1 and j <= n - 1 }", -1);
	map = isl_map_power(map, &exact);
	assert(exact);
	isl_map_free(map);

	/* COCOA Fig.2 right */
	map = isl_map_read_from_str(ctx,
		"[n] -> { [i,j] -> [i2,j2] : i2 = i + 3 and j2 = j and "
			"i <= 2 j - 4 and i <= n - 3 and j <= 2 i - 1 and "
			"j <= n or "
			"i2 = i and j2 = j + 3 and i <= 2 j - 1 and i <= n and "
			"j <= 2 i - 4 and j <= n - 3 or "
			"i2 = i + 1 and j2 = j + 1 and i <= 2 j - 1 and "
			"i <= n - 1 and j <= 2 i - 1 and j <= n - 1 }", -1);
	map = isl_map_transitive_closure(map, &exact);
	assert(exact);
	map2 = isl_map_read_from_str(ctx,
		"[n] -> { [i,j] -> [i2,j2] : exists (k1,k2,k3,k : "
			"i <= 2 j - 1 and i <= n and j <= 2 i - 1 and "
			"j <= n and 3 + i + 2 j <= 3 n and "
			"3 + 2 i + j <= 3n and i2 <= 2 j2 -1 and i2 <= n and "
			"i2 <= 3 j2 - 4 and j2 <= 2 i2 -1 and j2 <= n and "
			"13 + 4 j2 <= 11 i2 and i2 = i + 3 k1 + k3 and "
			"j2 = j + 3 k2 + k3 and k1 >= 0 and k2 >= 0 and "
			"k3 >= 0 and k1 + k2 + k3 = k and k > 0) }", -1);
	assert(isl_map_is_equal(map, map2));
	isl_map_free(map2);
	isl_map_free(map);

	/* COCOA Fig.1 right */
	dom = isl_set_read_from_str(ctx,
		"{ [x,y] : x >= 0 and -2 x + 3 y >= 0 and x <= 3 and "
			"2 x - 3 y + 3 >= 0 }", -1);
	right = isl_map_read_from_str(ctx,
		"{ [x,y] -> [x2,y2] : x2 = x + 1 and y2 = y }", -1);
	up = isl_map_read_from_str(ctx,
		"{ [x,y] -> [x2,y2] : x2 = x and y2 = y + 1 }", -1);
	right = isl_map_intersect_domain(right, isl_set_copy(dom));
	right = isl_map_intersect_range(right, isl_set_copy(dom));
	up = isl_map_intersect_domain(up, isl_set_copy(dom));
	up = isl_map_intersect_range(up, dom);
	map = isl_map_union(up, right);
	map = isl_map_transitive_closure(map, &exact);
	assert(exact);
	map2 = isl_map_read_from_str(ctx,
		"{ [0,0] -> [0,1]; [0,0] -> [1,1]; [0,1] -> [1,1]; "
		"  [2,2] -> [3,2]; [2,2] -> [3,3]; [3,2] -> [3,3] }", -1);
	assert(isl_map_is_equal(map, map2));
	isl_map_free(map2);
	isl_map_free(map);

	/* COCOA Theorem 1 counter example */
	map = isl_map_read_from_str(ctx,
		"{ [i,j] -> [i2,j2] : i = 0 and 0 <= j and j <= 1 and "
			"i2 = 1 and j2 = j or "
			"i = 0 and j = 0 and i2 = 0 and j2 = 1 }", -1);
	map = isl_map_transitive_closure(map, &exact);
	assert(exact);
	isl_map_free(map);

	map = isl_map_read_from_str(ctx,
		"[m,n] -> { [i,j] -> [i2,j2] : i2 = i and j2 = j + 2 and "
			"1 <= i,i2 <= n and 1 <= j,j2 <= m or "
			"i2 = i + 1 and 3 <= j2 - j <= 4 and "
			"1 <= i,i2 <= n and 1 <= j,j2 <= m }", -1);
	map = isl_map_transitive_closure(map, &exact);
	assert(exact);
	isl_map_free(map);

	/* Kelly et al 1996, fig 12 */
	map = isl_map_read_from_str(ctx,
		"[n] -> { [i,j] -> [i2,j2] : i2 = i and j2 = j + 1 and "
			"1 <= i,j,j+1 <= n or "
			"j = n and j2 = 1 and i2 = i + 1 and "
			"1 <= i,i+1 <= n }", -1);
	map = isl_map_transitive_closure(map, &exact);
	assert(exact);
	map2 = isl_map_read_from_str(ctx,
		"[n] -> { [i,j] -> [i2,j2] : 1 <= j < j2 <= n and "
			"1 <= i <= n and i = i2 or "
			"1 <= i < i2 <= n and 1 <= j <= n and "
			"1 <= j2 <= n }", -1);
	assert(isl_map_is_equal(map, map2));
	isl_map_free(map2);
	isl_map_free(map);

	/* Omega's closure4 */
	map = isl_map_read_from_str(ctx,
		"[m,n] -> { [x,y] -> [x2,y2] : x2 = x and y2 = y + 1 and "
			"1 <= x,y <= 10 or "
			"x2 = x + 1 and y2 = y and "
			"1 <= x <= 20 && 5 <= y <= 15 }", -1);
	map = isl_map_transitive_closure(map, &exact);
	assert(exact);
	isl_map_free(map);

	map = isl_map_read_from_str(ctx,
		"[n] -> { [x] -> [y]: 1 <= n <= y - x <= 10 }", -1);
	map = isl_map_transitive_closure(map, &exact);
	assert(!exact);
	map2 = isl_map_read_from_str(ctx,
		"[n] -> { [x] -> [y] : 1 <= n <= 10 and y >= n + x }", -1);
	assert(isl_map_is_equal(map, map2));
	isl_map_free(map);
	isl_map_free(map2);

	str = "[n, m] -> { [i0, i1, i2, i3] -> [o0, o1, o2, o3] : "
	    "i3 = 1 and o0 = i0 and o1 = -1 + i1 and o2 = -1 + i2 and "
	    "o3 = -2 + i2 and i1 <= -1 + i0 and i1 >= 1 - m + i0 and "
	    "i1 >= 2 and i1 <= n and i2 >= 3 and i2 <= 1 + n and i2 <= m }";
	map = isl_map_read_from_str(ctx, str, -1);
	map = isl_map_transitive_closure(map, &exact);
	assert(exact);
	map2 = isl_map_read_from_str(ctx, str, -1);
	assert(isl_map_is_equal(map, map2));
	isl_map_free(map);
	isl_map_free(map2);

	str = "{[0] -> [1]; [2] -> [3]}";
	map = isl_map_read_from_str(ctx, str, -1);
	map = isl_map_transitive_closure(map, &exact);
	assert(exact);
	map2 = isl_map_read_from_str(ctx, str, -1);
	assert(isl_map_is_equal(map, map2));
	isl_map_free(map);
	isl_map_free(map2);

	str = "[n] -> { [[i0, i1, 1, 0, i0] -> [i5, 1]] -> "
	    "[[i0, -1 + i1, 2, 0, i0] -> [-1 + i5, 2]] : "
	    "exists (e0 = [(3 - n)/3]: i5 >= 2 and i1 >= 2 and "
	    "3i0 <= -1 + n and i1 <= -1 + n and i5 <= -1 + n and "
	    "3e0 >= 1 - n and 3e0 <= 2 - n and 3i0 >= -2 + n); "
	    "[[i0, i1, 2, 0, i0] -> [i5, 1]] -> "
	    "[[i0, i1, 1, 0, i0] -> [-1 + i5, 2]] : "
	    "exists (e0 = [(3 - n)/3]: i5 >= 2 and i1 >= 1 and "
	    "3i0 <= -1 + n and i1 <= -1 + n and i5 <= -1 + n and "
	    "3e0 >= 1 - n and 3e0 <= 2 - n and 3i0 >= -2 + n); "
	    "[[i0, i1, 1, 0, i0] -> [i5, 2]] -> "
	    "[[i0, -1 + i1, 2, 0, i0] -> [i5, 1]] : "
	    "exists (e0 = [(3 - n)/3]: i1 >= 2 and i5 >= 1 and "
	    "3i0 <= -1 + n and i1 <= -1 + n and i5 <= -1 + n and "
	    "3e0 >= 1 - n and 3e0 <= 2 - n and 3i0 >= -2 + n); "
	    "[[i0, i1, 2, 0, i0] -> [i5, 2]] -> "
	    "[[i0, i1, 1, 0, i0] -> [i5, 1]] : "
	    "exists (e0 = [(3 - n)/3]: i5 >= 1 and i1 >= 1 and "
	    "3i0 <= -1 + n and i1 <= -1 + n and i5 <= -1 + n and "
	    "3e0 >= 1 - n and 3e0 <= 2 - n and 3i0 >= -2 + n) }";
	map = isl_map_read_from_str(ctx, str, -1);
	map = isl_map_transitive_closure(map, NULL);
	assert(map);
	isl_map_free(map);
}

void test_lex(struct isl_ctx *ctx)
{
	isl_dim *dim;
	isl_map *map;

	dim = isl_dim_alloc(ctx, 0, 0, 0);
	map = isl_map_lex_le(dim);
	assert(!isl_map_is_empty(map));
	isl_map_free(map);
}

void test_lexmin(struct isl_ctx *ctx)
{
	const char *str;
	isl_map *map, *map2;
	isl_set *set;
	isl_set *set2;

	str = "[p0, p1] -> { [] -> [] : "
	    "exists (e0 = [(2p1)/3], e1, e2, e3 = [(3 - p1 + 3e0)/3], "
	    "e4 = [(p1)/3], e5 = [(p1 + 3e4)/3]: "
	    "3e0 >= -2 + 2p1 and 3e0 >= p1 and 3e3 >= 1 - p1 + 3e0 and "
	    "3e0 <= 2p1 and 3e3 >= -2 + p1 and 3e3 <= -1 + p1 and p1 >= 3 and "
	    "3e5 >= -2 + 2p1 and 3e5 >= p1 and 3e5 <= -1 + p1 + 3e4 and "
	    "3e4 <= p1 and 3e4 >= -2 + p1 and e3 <= -1 + e0 and "
	    "3e4 >= 6 - p1 + 3e1 and 3e1 >= p1 and 3e5 >= -2 + p1 + 3e4 and "
	    "2e4 >= 3 - p1 + 2e1 and e4 <= e1 and 3e3 <= 2 - p1 + 3e0 and "
	    "e5 >= 1 + e1 and 3e4 >= 6 - 2p1 + 3e1 and "
	    "p0 >= 2 and p1 >= p0 and 3e2 >= p1 and 3e4 >= 6 - p1 + 3e2 and "
	    "e2 <= e1 and e3 >= 1 and e4 <= e2) }";
	map = isl_map_read_from_str(ctx, str, -1);
	map = isl_map_lexmin(map);
	isl_map_free(map);

	str = "[C] -> { [obj,a,b,c] : obj <= 38 a + 7 b + 10 c and "
	    "a + b <= 1 and c <= 10 b and c <= C and a,b,c,C >= 0 }";
	set = isl_set_read_from_str(ctx, str, -1);
	set = isl_set_lexmax(set);
	str = "[C] -> { [obj,a,b,c] : C = 8 }";
	set2 = isl_set_read_from_str(ctx, str, -1);
	set = isl_set_intersect(set, set2);
	assert(!isl_set_is_empty(set));
	isl_set_free(set);

	str = "{ [x] -> [y] : x <= y <= 10; [x] -> [5] : -8 <= x <= 8 }";
	map = isl_map_read_from_str(ctx, str, -1);
	map = isl_map_lexmin(map);
	str = "{ [x] -> [5] : 6 <= x <= 8; "
		"[x] -> [x] : x <= 5 or (9 <= x <= 10) }";
	map2 = isl_map_read_from_str(ctx, str, -1);
	assert(isl_map_is_equal(map, map2));
	isl_map_free(map);
	isl_map_free(map2);

	str = "{ [x] -> [y] : 4y = x or 4y = -1 + x or 4y = -2 + x }";
	map = isl_map_read_from_str(ctx, str, -1);
	map2 = isl_map_copy(map);
	map = isl_map_lexmin(map);
	assert(isl_map_is_equal(map, map2));
	isl_map_free(map);
	isl_map_free(map2);

	str = "{ [x] -> [y] : x = 4y; [x] -> [y] : x = 2y }";
	map = isl_map_read_from_str(ctx, str, -1);
	map = isl_map_lexmin(map);
	str = "{ [x] -> [y] : (4y = x and x >= 0) or "
		"(exists (e0 = [(x)/4], e1 = [(-2 + x)/4]: 2y = x and "
		"4e1 = -2 + x and 4e0 <= -1 + x and 4e0 >= -3 + x)) or "
		"(exists (e0 = [(x)/4]: 2y = x and 4e0 = x and x <= -4)) }";
	map2 = isl_map_read_from_str(ctx, str, -1);
	assert(isl_map_is_equal(map, map2));
	isl_map_free(map);
	isl_map_free(map2);
}

struct must_may {
	isl_map *must;
	isl_map *may;
};

static int collect_must_may(__isl_take isl_map *dep, int must,
	void *dep_user, void *user)
{
	struct must_may *mm = (struct must_may *)user;

	if (must)
		mm->must = isl_map_union(mm->must, dep);
	else
		mm->may = isl_map_union(mm->may, dep);

	return 0;
}

static int common_space(void *first, void *second)
{
	int depth = *(int *)first;
	return 2 * depth;
}

static int map_is_equal(__isl_keep isl_map *map, const char *str)
{
	isl_map *map2;
	int equal;

	if (!map)
		return -1;

	map2 = isl_map_read_from_str(map->ctx, str, -1);
	equal = isl_map_is_equal(map, map2);
	isl_map_free(map2);

	return equal;
}

void test_dep(struct isl_ctx *ctx)
{
	const char *str;
	isl_dim *dim;
	isl_map *map;
	isl_access_info *ai;
	isl_flow *flow;
	int depth;
	struct must_may mm;

	depth = 3;

	str = "{ [2,i,0] -> [i] : 0 <= i <= 10 }";
	map = isl_map_read_from_str(ctx, str, -1);
	ai = isl_access_info_alloc(map, &depth, &common_space, 2);

	str = "{ [0,i,0] -> [i] : 0 <= i <= 10 }";
	map = isl_map_read_from_str(ctx, str, -1);
	ai = isl_access_info_add_source(ai, map, 1, &depth);

	str = "{ [1,i,0] -> [5] : 0 <= i <= 10 }";
	map = isl_map_read_from_str(ctx, str, -1);
	ai = isl_access_info_add_source(ai, map, 1, &depth);

	flow = isl_access_info_compute_flow(ai);
	dim = isl_dim_alloc(ctx, 0, 3, 3);
	mm.must = isl_map_empty(isl_dim_copy(dim));
	mm.may = isl_map_empty(dim);

	isl_flow_foreach(flow, collect_must_may, &mm);

	str = "{ [0,i,0] -> [2,i,0] : (0 <= i <= 4) or (6 <= i <= 10); "
	      "  [1,10,0] -> [2,5,0] }";
	assert(map_is_equal(mm.must, str));
	str = "{ [i,j,k] -> [l,m,n] : 1 = 0 }";
	assert(map_is_equal(mm.may, str));

	isl_map_free(mm.must);
	isl_map_free(mm.may);
	isl_flow_free(flow);


	str = "{ [2,i,0] -> [i] : 0 <= i <= 10 }";
	map = isl_map_read_from_str(ctx, str, -1);
	ai = isl_access_info_alloc(map, &depth, &common_space, 2);

	str = "{ [0,i,0] -> [i] : 0 <= i <= 10 }";
	map = isl_map_read_from_str(ctx, str, -1);
	ai = isl_access_info_add_source(ai, map, 1, &depth);

	str = "{ [1,i,0] -> [5] : 0 <= i <= 10 }";
	map = isl_map_read_from_str(ctx, str, -1);
	ai = isl_access_info_add_source(ai, map, 0, &depth);

	flow = isl_access_info_compute_flow(ai);
	dim = isl_dim_alloc(ctx, 0, 3, 3);
	mm.must = isl_map_empty(isl_dim_copy(dim));
	mm.may = isl_map_empty(dim);

	isl_flow_foreach(flow, collect_must_may, &mm);

	str = "{ [0,i,0] -> [2,i,0] : (0 <= i <= 4) or (6 <= i <= 10) }";
	assert(map_is_equal(mm.must, str));
	str = "{ [0,5,0] -> [2,5,0]; [1,i,0] -> [2,5,0] : 0 <= i <= 10 }";
	assert(map_is_equal(mm.may, str));

	isl_map_free(mm.must);
	isl_map_free(mm.may);
	isl_flow_free(flow);


	str = "{ [2,i,0] -> [i] : 0 <= i <= 10 }";
	map = isl_map_read_from_str(ctx, str, -1);
	ai = isl_access_info_alloc(map, &depth, &common_space, 2);

	str = "{ [0,i,0] -> [i] : 0 <= i <= 10 }";
	map = isl_map_read_from_str(ctx, str, -1);
	ai = isl_access_info_add_source(ai, map, 0, &depth);

	str = "{ [1,i,0] -> [5] : 0 <= i <= 10 }";
	map = isl_map_read_from_str(ctx, str, -1);
	ai = isl_access_info_add_source(ai, map, 0, &depth);

	flow = isl_access_info_compute_flow(ai);
	dim = isl_dim_alloc(ctx, 0, 3, 3);
	mm.must = isl_map_empty(isl_dim_copy(dim));
	mm.may = isl_map_empty(dim);

	isl_flow_foreach(flow, collect_must_may, &mm);

	str = "{ [0,i,0] -> [2,i,0] : 0 <= i <= 10; "
	      "  [1,i,0] -> [2,5,0] : 0 <= i <= 10 }";
	assert(map_is_equal(mm.may, str));
	str = "{ [i,j,k] -> [l,m,n] : 1 = 0 }";
	assert(map_is_equal(mm.must, str));

	isl_map_free(mm.must);
	isl_map_free(mm.may);
	isl_flow_free(flow);


	str = "{ [0,i,2] -> [i] : 0 <= i <= 10 }";
	map = isl_map_read_from_str(ctx, str, -1);
	ai = isl_access_info_alloc(map, &depth, &common_space, 2);

	str = "{ [0,i,0] -> [i] : 0 <= i <= 10 }";
	map = isl_map_read_from_str(ctx, str, -1);
	ai = isl_access_info_add_source(ai, map, 0, &depth);

	str = "{ [0,i,1] -> [5] : 0 <= i <= 10 }";
	map = isl_map_read_from_str(ctx, str, -1);
	ai = isl_access_info_add_source(ai, map, 0, &depth);

	flow = isl_access_info_compute_flow(ai);
	dim = isl_dim_alloc(ctx, 0, 3, 3);
	mm.must = isl_map_empty(isl_dim_copy(dim));
	mm.may = isl_map_empty(dim);

	isl_flow_foreach(flow, collect_must_may, &mm);

	str = "{ [0,i,0] -> [0,i,2] : 0 <= i <= 10; "
	      "  [0,i,1] -> [0,5,2] : 0 <= i <= 5 }";
	assert(map_is_equal(mm.may, str));
	str = "{ [i,j,k] -> [l,m,n] : 1 = 0 }";
	assert(map_is_equal(mm.must, str));

	isl_map_free(mm.must);
	isl_map_free(mm.may);
	isl_flow_free(flow);


	str = "{ [0,i,1] -> [i] : 0 <= i <= 10 }";
	map = isl_map_read_from_str(ctx, str, -1);
	ai = isl_access_info_alloc(map, &depth, &common_space, 2);

	str = "{ [0,i,0] -> [i] : 0 <= i <= 10 }";
	map = isl_map_read_from_str(ctx, str, -1);
	ai = isl_access_info_add_source(ai, map, 0, &depth);

	str = "{ [0,i,2] -> [5] : 0 <= i <= 10 }";
	map = isl_map_read_from_str(ctx, str, -1);
	ai = isl_access_info_add_source(ai, map, 0, &depth);

	flow = isl_access_info_compute_flow(ai);
	dim = isl_dim_alloc(ctx, 0, 3, 3);
	mm.must = isl_map_empty(isl_dim_copy(dim));
	mm.may = isl_map_empty(dim);

	isl_flow_foreach(flow, collect_must_may, &mm);

	str = "{ [0,i,0] -> [0,i,1] : 0 <= i <= 10; "
	      "  [0,i,2] -> [0,5,1] : 0 <= i <= 4 }";
	assert(map_is_equal(mm.may, str));
	str = "{ [i,j,k] -> [l,m,n] : 1 = 0 }";
	assert(map_is_equal(mm.must, str));

	isl_map_free(mm.must);
	isl_map_free(mm.may);
	isl_flow_free(flow);


	depth = 5;

	str = "{ [1,i,0,0,0] -> [i,j] : 0 <= i <= 10 and 0 <= j <= 10 }";
	map = isl_map_read_from_str(ctx, str, -1);
	ai = isl_access_info_alloc(map, &depth, &common_space, 1);

	str = "{ [0,i,0,j,0] -> [i,j] : 0 <= i <= 10 and 0 <= j <= 10 }";
	map = isl_map_read_from_str(ctx, str, -1);
	ai = isl_access_info_add_source(ai, map, 1, &depth);

	flow = isl_access_info_compute_flow(ai);
	dim = isl_dim_alloc(ctx, 0, 5, 5);
	mm.must = isl_map_empty(isl_dim_copy(dim));
	mm.may = isl_map_empty(dim);

	isl_flow_foreach(flow, collect_must_may, &mm);

	str = "{ [0,i,0,j,0] -> [1,i,0,0,0] : 0 <= i,j <= 10 }";
	assert(map_is_equal(mm.must, str));
	str = "{ [0,0,0,0,0] -> [0,0,0,0,0] : 1 = 0 }";
	assert(map_is_equal(mm.may, str));

	isl_map_free(mm.must);
	isl_map_free(mm.may);
	isl_flow_free(flow);
}

void test_sv(struct isl_ctx *ctx)
{
	const char *str;
	isl_map *map;

	str = "[N] -> { [i] -> [f] : 0 <= i <= N and 0 <= i - 10 f <= 9 }";
	map = isl_map_read_from_str(ctx, str, -1);
	assert(isl_map_is_single_valued(map));
	isl_map_free(map);

	str = "[N] -> { [i] -> [f] : 0 <= i <= N and 0 <= i - 10 f <= 10 }";
	map = isl_map_read_from_str(ctx, str, -1);
	assert(!isl_map_is_single_valued(map));
	isl_map_free(map);
}

void test_bijective_case(struct isl_ctx *ctx, const char *str, int bijective)
{
	isl_map *map;

	map = isl_map_read_from_str(ctx, str, -1);
	if (bijective)
		assert(isl_map_is_bijective(map));
	else
		assert(!isl_map_is_bijective(map));
	isl_map_free(map);
}

void test_bijective(struct isl_ctx *ctx)
{
	test_bijective_case(ctx, "[N,M]->{[i,j] -> [i]}", 0);
	test_bijective_case(ctx, "[N,M]->{[i,j] -> [i] : j=i}", 1);
	test_bijective_case(ctx, "[N,M]->{[i,j] -> [i] : j=0}", 1);
	test_bijective_case(ctx, "[N,M]->{[i,j] -> [i] : j=N}", 1);
	test_bijective_case(ctx, "[N,M]->{[i,j] -> [j,i]}", 1);
	test_bijective_case(ctx, "[N,M]->{[i,j] -> [i+j]}", 0);
	test_bijective_case(ctx, "[N,M]->{[i,j] -> []}", 0);
	test_bijective_case(ctx, "[N,M]->{[i,j] -> [i,j,N]}", 1);
	test_bijective_case(ctx, "[N,M]->{[i,j] -> [2i]}", 0);
	test_bijective_case(ctx, "[N,M]->{[i,j] -> [i,i]}", 0);
	test_bijective_case(ctx, "[N,M]->{[i,j] -> [2i,i]}", 0);
	test_bijective_case(ctx, "[N,M]->{[i,j] -> [2i,j]}", 1);
	test_bijective_case(ctx, "[N,M]->{[i,j] -> [x,y] : 2x=i & y =j}", 1);
}

void test_pwqp(struct isl_ctx *ctx)
{
	const char *str;
	isl_set *set;
	isl_pw_qpolynomial *pwqp1, *pwqp2;

	str = "{ [i,j,k] -> 1 + 9 * [i/5] + 7 * [j/11] + 4 * [k/13] }";
	pwqp1 = isl_pw_qpolynomial_read_from_str(ctx, str);

	pwqp1 = isl_pw_qpolynomial_move_dims(pwqp1, isl_dim_param, 0,
						isl_dim_set, 1, 1);

	str = "[j] -> { [i,k] -> 1 + 9 * [i/5] + 7 * [j/11] + 4 * [k/13] }";
	pwqp2 = isl_pw_qpolynomial_read_from_str(ctx, str);

	pwqp1 = isl_pw_qpolynomial_sub(pwqp1, pwqp2);

	assert(isl_pw_qpolynomial_is_zero(pwqp1));

	isl_pw_qpolynomial_free(pwqp1);

	str = "{ [i] -> i }";
	pwqp1 = isl_pw_qpolynomial_read_from_str(ctx, str);
	str = "{ [k] : exists a : k = 2a }";
	set = isl_set_read_from_str(ctx, str, 0);
	pwqp1 = isl_pw_qpolynomial_gist(pwqp1, set);
	str = "{ [i] -> i }";
	pwqp2 = isl_pw_qpolynomial_read_from_str(ctx, str);

	pwqp1 = isl_pw_qpolynomial_sub(pwqp1, pwqp2);

	assert(isl_pw_qpolynomial_is_zero(pwqp1));

	isl_pw_qpolynomial_free(pwqp1);

	str = "{ [i] -> i + [ (i + [i/3])/2 ] }";
	pwqp1 = isl_pw_qpolynomial_read_from_str(ctx, str);
	str = "{ [10] }";
	set = isl_set_read_from_str(ctx, str, 0);
	pwqp1 = isl_pw_qpolynomial_gist(pwqp1, set);
	str = "{ [i] -> 16 }";
	pwqp2 = isl_pw_qpolynomial_read_from_str(ctx, str);

	pwqp1 = isl_pw_qpolynomial_sub(pwqp1, pwqp2);

	assert(isl_pw_qpolynomial_is_zero(pwqp1));

	isl_pw_qpolynomial_free(pwqp1);

	str = "{ [i] -> ([(i)/2]) }";
	pwqp1 = isl_pw_qpolynomial_read_from_str(ctx, str);
	str = "{ [k] : exists a : k = 2a+1 }";
	set = isl_set_read_from_str(ctx, str, 0);
	pwqp1 = isl_pw_qpolynomial_gist(pwqp1, set);
	str = "{ [i] -> -1/2 + 1/2 * i }";
	pwqp2 = isl_pw_qpolynomial_read_from_str(ctx, str);

	pwqp1 = isl_pw_qpolynomial_sub(pwqp1, pwqp2);

	assert(isl_pw_qpolynomial_is_zero(pwqp1));

	isl_pw_qpolynomial_free(pwqp1);

	str = "{ [i] -> ([([i/2] + [i/2])/5]) }";
	pwqp1 = isl_pw_qpolynomial_read_from_str(ctx, str);
	str = "{ [i] -> ([(2 * [i/2])/5]) }";
	pwqp2 = isl_pw_qpolynomial_read_from_str(ctx, str);

	pwqp1 = isl_pw_qpolynomial_sub(pwqp1, pwqp2);

	assert(isl_pw_qpolynomial_is_zero(pwqp1));

	isl_pw_qpolynomial_free(pwqp1);

	str = "{ [x] -> ([x/2] + [(x+1)/2]) }";
	pwqp1 = isl_pw_qpolynomial_read_from_str(ctx, str);
	str = "{ [x] -> x }";
	pwqp2 = isl_pw_qpolynomial_read_from_str(ctx, str);

	pwqp1 = isl_pw_qpolynomial_sub(pwqp1, pwqp2);

	assert(isl_pw_qpolynomial_is_zero(pwqp1));

	isl_pw_qpolynomial_free(pwqp1);
}

void test_split_periods(isl_ctx *ctx)
{
	const char *str;
	isl_pw_qpolynomial *pwqp;

	str = "{ [U,V] -> 1/3 * U + 2/3 * V - [(U + 2V)/3] + [U/2] : "
		"U + 2V + 3 >= 0 and - U -2V  >= 0 and - U + 10 >= 0 and "
		"U  >= 0; [U,V] -> U^2 : U >= 100 }";
	pwqp = isl_pw_qpolynomial_read_from_str(ctx, str);

	pwqp = isl_pw_qpolynomial_split_periods(pwqp, 2);
	assert(pwqp);

	isl_pw_qpolynomial_free(pwqp);
}

void test_union(isl_ctx *ctx)
{
	const char *str;
	isl_union_set *uset1, *uset2;
	isl_union_map *umap1, *umap2;

	str = "{ [i] : 0 <= i <= 1 }";
	uset1 = isl_union_set_read_from_str(ctx, str);
	str = "{ [1] -> [0] }";
	umap1 = isl_union_map_read_from_str(ctx, str);

	umap2 = isl_union_set_lex_gt_union_set(isl_union_set_copy(uset1), uset1);
	assert(isl_union_map_is_equal(umap1, umap2));

	isl_union_map_free(umap1);
	isl_union_map_free(umap2);

	str = "{ A[i] -> B[i]; B[i] -> C[i]; A[0] -> C[1] }";
	umap1 = isl_union_map_read_from_str(ctx, str);
	str = "{ A[i]; B[i] }";
	uset1 = isl_union_set_read_from_str(ctx, str);

	uset2 = isl_union_map_domain(umap1);

	assert(isl_union_set_is_equal(uset1, uset2));

	isl_union_set_free(uset1);
	isl_union_set_free(uset2);
}

void test_bound(isl_ctx *ctx)
{
	const char *str;
	isl_pw_qpolynomial *pwqp;
	isl_pw_qpolynomial_fold *pwf;

	str = "{ [[a, b, c, d] -> [e]] -> 0 }";
	pwqp = isl_pw_qpolynomial_read_from_str(ctx, str);
	pwf = isl_pw_qpolynomial_bound(pwqp, isl_fold_max, NULL);
	assert(isl_pw_qpolynomial_fold_dim(pwf, isl_dim_set) == 4);
	isl_pw_qpolynomial_fold_free(pwf);

	str = "{ [[x]->[x]] -> 1 : exists a : x = 2 a }";
	pwqp = isl_pw_qpolynomial_read_from_str(ctx, str);
	pwf = isl_pw_qpolynomial_bound(pwqp, isl_fold_max, NULL);
	assert(isl_pw_qpolynomial_fold_dim(pwf, isl_dim_set) == 1);
	isl_pw_qpolynomial_fold_free(pwf);
}

void test_lift(isl_ctx *ctx)
{
	const char *str;
	isl_basic_map *bmap;
	isl_basic_set *bset;

	str = "{ [i0] : exists e0 : i0 = 4e0 }";
	bset = isl_basic_set_read_from_str(ctx, str, 0);
	bset = isl_basic_set_lift(bset);
	bmap = isl_basic_map_from_range(bset);
	bset = isl_basic_map_domain(bmap);
	isl_basic_set_free(bset);
}

void test_subset(isl_ctx *ctx)
{
	const char *str;
	isl_set *set1, *set2;

	str = "{ [112, 0] }";
	set1 = isl_set_read_from_str(ctx, str, 0);
	str = "{ [i0, i1] : exists (e0 = [(i0 - i1)/16], e1: "
		"16e0 <= i0 - i1 and 16e0 >= -15 + i0 - i1 and "
		"16e1 <= i1 and 16e0 >= -i1 and 16e1 >= -i0 + i1) }";
	set2 = isl_set_read_from_str(ctx, str, 0);
	assert(isl_set_is_subset(set1, set2));
	isl_set_free(set1);
	isl_set_free(set2);
}

void test_factorize(isl_ctx *ctx)
{
	const char *str;
	isl_basic_set *bset;
	isl_factorizer *f;

	str = "{ [i0, i1, i2, i3, i4, i5, i6, i7] : 3i5 <= 2 - 2i0 and "
	    "i0 >= -2 and i6 >= 1 + i3 and i7 >= 0 and 3i5 >= -2i0 and "
	    "2i4 <= i2 and i6 >= 1 + 2i0 + 3i1 and i4 <= -1 and "
	    "i6 >= 1 + 2i0 + 3i5 and i6 <= 2 + 2i0 + 3i5 and "
	    "3i5 <= 2 - 2i0 - i2 + 3i4 and i6 <= 2 + 2i0 + 3i1 and "
	    "i0 <= -1 and i7 <= i2 + i3 - 3i4 - i6 and "
	    "3i5 >= -2i0 - i2 + 3i4 }";
	bset = isl_basic_set_read_from_str(ctx, str, 0);
	f = isl_basic_set_factorizer(bset);
	assert(f);
	isl_basic_set_free(bset);
	isl_factorizer_free(f);

	str = "{ [i0, i1, i2, i3, i4, i5, i6, i7, i8, i9, i10, i11, i12] : "
	    "i12 <= 2 + i0 - i11 and 2i8 >= -i4 and i11 >= i1 and "
	    "3i5 <= -i2 and 2i11 >= -i4 - 2i7 and i11 <= 3 + i0 + 3i9 and "
	    "i11 <= -i4 - 2i7 and i12 >= -i10 and i2 >= -2 and "
	    "i11 >= i1 + 3i10 and i11 >= 1 + i0 + 3i9 and "
	    "i11 <= 1 - i4 - 2i8 and 6i6 <= 6 - i2 and 3i6 >= 1 - i2 and "
	    "i11 <= 2 + i1 and i12 <= i4 + i11 and i12 >= i0 - i11 and "
	    "3i5 >= -2 - i2 and i12 >= -1 + i4 + i11 and 3i3 <= 3 - i2 and "
	    "9i6 <= 11 - i2 + 6i5 and 3i3 >= 1 - i2 and "
	    "9i6 <= 5 - i2 + 6i3 and i12 <= -1 and i2 <= 0 }";
	bset = isl_basic_set_read_from_str(ctx, str, 0);
	f = isl_basic_set_factorizer(bset);
	assert(f);
	isl_basic_set_free(bset);
	isl_factorizer_free(f);
}

int main()
{
	struct isl_ctx *ctx;

	srcdir = getenv("srcdir");
	assert(srcdir);

	ctx = isl_ctx_alloc();
	test_factorize(ctx);
	test_subset(ctx);
	test_lift(ctx);
	test_bound(ctx);
	test_union(ctx);
	test_split_periods(ctx);
	test_parse(ctx);
	test_pwqp(ctx);
	test_lex(ctx);
	test_sv(ctx);
	test_bijective(ctx);
	test_dep(ctx);
	test_read(ctx);
	test_bounded(ctx);
	test_construction(ctx);
	test_dim(ctx);
	test_div(ctx);
	test_application(ctx);
	test_affine_hull(ctx);
	test_convex_hull(ctx);
	test_gist(ctx);
	test_coalesce(ctx);
	test_closure(ctx);
	test_lexmin(ctx);
	isl_ctx_free(ctx);
	return 0;
}
