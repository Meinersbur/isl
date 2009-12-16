/*
 * Copyright 2008-2009 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv2.1 license
 *
 * Written by Sven Verdoolaege, K.U.Leuven, Departement
 * Computerwetenschappen, Celestijnenlaan 200A, B-3001 Leuven, Belgium
 */

#include <assert.h>
#include <string.h>
#include "isl_set.h"
#include "isl_tab.h"
#include "isl_map_private.h"
#include "isl_sample.h"
#include "isl_scan.h"
#include "isl_seq.h"
#include "isl_ilp.h"

/* The input of this program is the same as that of the "example" program
 * from the PipLib distribution, except that the "big parameter column"
 * should always be -1.
 *
 * Context constraints in PolyLib format
 * -1
 * Problem constraints in PolyLib format
 * Optional list of options
 *
 * The options are
 *	Maximize	compute maximum instead of minimum
 *	Rational	compute rational optimum instead of integer optimum
 *	Urs_parms	don't assume parameters are non-negative
 *	Urs_unknowns	don't assume unknowns are non-negative
 */

struct pip_options {
	struct isl_options	*isl;
	unsigned		 verify;
};

struct isl_arg pip_options_arg[] = {
ISL_ARG_CHILD(struct pip_options, isl, "isl", isl_options_arg)
ISL_ARG_BOOL(struct pip_options, verify, 'T', "verify", 0)
};

ISL_ARG_DEF(pip_options, struct pip_options, pip_options_arg)

static struct isl_basic_set *to_parameter_domain(struct isl_basic_set *context)
{
	struct isl_dim *param_dim;
	struct isl_basic_set *model;

	param_dim = isl_dim_set_alloc(context->ctx,
					isl_basic_set_n_dim(context), 0);
	model = isl_basic_set_empty(param_dim);
	context = isl_basic_set_from_underlying_set(context, model);

	return context;
}

static isl_basic_set *construct_cube(isl_basic_set *context)
{
	int i;
	unsigned dim;
	int range;
	isl_int m, M;
	struct isl_basic_set *cube;
	struct isl_basic_set *interval;
	struct isl_basic_set_list *list;

	dim = isl_basic_set_total_dim(context);
	if (dim == 0) {
		struct isl_dim *dims;
		dims = isl_dim_set_alloc(context->ctx, 0, 0);
		return isl_basic_set_universe(dims);
	}

	if (dim >= 8)
		range = 4;
	else if (dim >= 5)
		range = 6;
	else
		range = 30;

	isl_int_init(m);
	isl_int_init(M);

	isl_int_set_si(m, -range);
	isl_int_set_si(M, range);

	interval = isl_basic_set_interval(context->ctx, m, M);
	list = isl_basic_set_list_alloc(context->ctx, dim);
	for (i = 0; i < dim; ++i)
		list = isl_basic_set_list_add(list, isl_basic_set_copy(interval));
	isl_basic_set_free(interval);
	cube = isl_basic_set_product(list);

	isl_int_clear(m);
	isl_int_clear(M);

	return cube;
}

isl_basic_set *plug_in_parameters(isl_basic_set *bset, struct isl_vec *params)
{
	int i;

	for (i = 0; i < params->size - 1; ++i)
		bset = isl_basic_set_fix(bset,
					 isl_dim_param, i, params->el[1 + i]);

	bset = isl_basic_set_remove(bset, isl_dim_param, 0, params->size - 1);

	isl_vec_free(params);

	return bset;
}

isl_set *set_plug_in_parameters(isl_set *set, struct isl_vec *params)
{
	int i;

	for (i = 0; i < params->size - 1; ++i)
		set = isl_set_fix(set, isl_dim_param, i, params->el[1 + i]);

	set = isl_set_remove(set, isl_dim_param, 0, params->size - 1);

	isl_vec_free(params);

	return set;
}

/* Compute the lexicographically minimal (or maximal if max is set)
 * element of bset for the given values of the parameters, by
 * successively solving an ilp problem in each direction.
 */
struct isl_vec *opt_at(struct isl_basic_set *bset,
	struct isl_vec *params, int max)
{
	unsigned dim;
	struct isl_vec *opt;
	struct isl_vec *obj;
	int i;

	dim = isl_basic_set_dim(bset, isl_dim_set);

	bset = plug_in_parameters(bset, params);

	if (isl_basic_set_fast_is_empty(bset)) {
		opt = isl_vec_alloc(bset->ctx, 0);
		isl_basic_set_free(bset);
		return opt;
	}

	opt = isl_vec_alloc(bset->ctx, 1 + dim);
	assert(opt);

	obj = isl_vec_alloc(bset->ctx, 1 + dim);
	assert(obj);

	isl_int_set_si(opt->el[0], 1);
	isl_int_set_si(obj->el[0], 0);

	for (i = 0; i < dim; ++i) {
		enum isl_lp_result res;

		isl_seq_clr(obj->el + 1, dim);
		isl_int_set_si(obj->el[1 + i], 1);
		res = isl_basic_set_solve_ilp(bset, max, obj->el,
						&opt->el[1 + i], NULL);
		if (res == isl_lp_empty)
			goto empty;
		assert(res == isl_lp_ok);
		bset = isl_basic_set_fix(bset, isl_dim_set, i, opt->el[1 + i]);
	}

	isl_basic_set_free(bset);
	isl_vec_free(obj);

	return opt;
empty:
	isl_vec_free(opt);
	opt = isl_vec_alloc(bset->ctx, 0);
	isl_basic_set_free(bset);
	isl_vec_free(obj);

	return opt;
}

struct isl_scan_pip {
	struct isl_scan_callback callback;
	isl_basic_set *bset;
	isl_set *sol;
	isl_set *empty;
	int stride;
	int n;
	int max;
};

/* Check if the "manually" computed optimum of bset at the "sample"
 * values of the parameters agrees with the solution of pilp problem
 * represented by the pair (sol, empty).
 * In particular, if there is no solution for this value of the parameters,
 * then it should be an element of the parameter domain "empty".
 * Otherwise, the optimal solution, should be equal to the result of
 * plugging in the value of the parameters in "sol".
 */
static int scan_one(struct isl_scan_callback *callback,
	__isl_take isl_vec *sample)
{
	struct isl_scan_pip *sp = (struct isl_scan_pip *)callback;
	struct isl_vec *opt;

	opt = opt_at(isl_basic_set_copy(sp->bset), isl_vec_copy(sample), sp->max);
	assert(opt);

	if (opt->size == 0) {
		isl_set *sample_set;
		sample_set = isl_set_from_basic_set(
				isl_basic_set_from_vec(sample));
		assert(isl_set_is_subset(sample_set, sp->empty));
		isl_set_free(sample_set);
		isl_vec_free(opt);
	} else {
		isl_set *sol;
		isl_set *opt_set;
		opt_set = isl_set_from_basic_set(isl_basic_set_from_vec(opt));
		sol = set_plug_in_parameters(isl_set_copy(sp->sol), sample);
		assert(isl_set_is_equal(opt_set, sol));
		isl_set_free(sol);
		isl_set_free(opt_set);
	}

	if (!(sp->n++ % sp->stride)) {
		printf("o");
		fflush(stdout);
	}

	return 0;
}

struct isl_scan_count {
	struct isl_scan_callback callback;
	int n;
};

static int count_one(struct isl_scan_callback *callback,
	__isl_take isl_vec *sample)
{
	struct isl_scan_count *sc = (struct isl_scan_count *)callback;
	isl_vec_free(sample);

	sc->n++;

	return 0;
}

static void check_solution(isl_basic_set *bset, isl_basic_set *context,
	isl_set *sol, isl_set *empty, int max)
{
	isl_basic_set *cube;
	struct isl_scan_count sc;
	struct isl_scan_pip sp;
	int i;
	int r;

	context = isl_basic_set_underlying_set(context);

	cube = construct_cube(context);
	context = isl_basic_set_intersect(context, cube);

	sc.callback.add = count_one;
	sc.n = 0;

	r = isl_basic_set_scan(isl_basic_set_copy(context), &sc.callback);
	assert (r == 0);

	sp.callback.add = scan_one;
	sp.bset = bset;
	sp.sol = sol;
	sp.empty = empty;
	sp.n = 0;
	sp.stride = sc.n > 70 ? 1 + (sc.n + 1)/70 : 1;
	sp.max = max;

	for (i = 0; i < sc.n; i += sp.stride)
		printf(".");
	printf("\r");
	fflush(stdout);

	r = isl_basic_set_scan(context, &sp.callback);
	assert(r == 0);

	printf("\n");

	isl_basic_set_free(bset);
}

int main(int argc, char **argv)
{
	struct isl_ctx *ctx;
	struct isl_basic_set *context, *bset, *copy, *context_copy;
	struct isl_set *set;
	struct isl_set *empty;
	int neg_one;
	char s[1024];
	int urs_parms = 0;
	int urs_unknowns = 0;
	int max = 0;
	int rational = 0;
	int n;
	struct pip_options *options;

	options = pip_options_new_with_defaults();
	assert(options);
	argc = pip_options_parse(options, argc, argv);

	ctx = isl_ctx_alloc_with_options(options->isl);
	options->isl = NULL;

	context = isl_basic_set_read_from_file(ctx, stdin, 0, ISL_FORMAT_POLYLIB);
	assert(context);
	n = fscanf(stdin, "%d", &neg_one);
	assert(n == 1);
	assert(neg_one == -1);
	bset = isl_basic_set_read_from_file(ctx, stdin,
		isl_basic_set_dim(context, isl_dim_set), ISL_FORMAT_POLYLIB);

	while (fgets(s, sizeof(s), stdin)) {
		if (strncasecmp(s, "Maximize", 8) == 0)
			max = 1;
		if (strncasecmp(s, "Rational", 8) == 0) {
			rational = 1;
			bset = isl_basic_set_set_rational(bset);
		}
		if (strncasecmp(s, "Urs_parms", 9) == 0)
			urs_parms = 1;
		if (strncasecmp(s, "Urs_unknowns", 12) == 0)
			urs_unknowns = 1;
	}
	if (!urs_parms)
		context = isl_basic_set_intersect(context,
		isl_basic_set_positive_orthant(isl_basic_set_get_dim(context)));
	context = to_parameter_domain(context);
	if (!urs_unknowns)
		bset = isl_basic_set_intersect(bset,
		isl_basic_set_positive_orthant(isl_basic_set_get_dim(bset)));

	if (options->verify) {
		copy = isl_basic_set_copy(bset);
		context_copy = isl_basic_set_copy(context);
	}

	if (max)
		set = isl_basic_set_partial_lexmax(bset, context, &empty);
	else
		set = isl_basic_set_partial_lexmin(bset, context, &empty);

	if (options->verify) {
		assert(!rational);
		check_solution(copy, context_copy, set, empty, max);
	} else {
		isl_set_dump(set, stdout, 0);
		fprintf(stdout, "no solution:\n");
		isl_set_dump(empty, stdout, 4);
	}

	isl_set_free(set);
	isl_set_free(empty);
	isl_ctx_free(ctx);
	free(options);

	return 0;
}
