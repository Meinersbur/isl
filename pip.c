#include <assert.h>
#include <string.h>
#include "isl_set.h"
#include "isl_tab.h"
#include "isl_map_private.h"

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

int main(int argc, char **argv)
{
	struct isl_ctx *ctx = isl_ctx_alloc();
	struct isl_basic_set *context, *bset;
	struct isl_set *set;
	struct isl_set *empty;
	int neg_one;
	char s[1024];
	int urs_parms = 0;
	int urs_unknowns = 0;
	int max = 0;
	int n;

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
		if (strncasecmp(s, "Rational", 8) == 0)
			bset = isl_basic_set_set_rational(bset);
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

	if (max)
		set = isl_basic_set_partial_lexmax(bset, context, &empty);
	else
		set = isl_basic_set_partial_lexmin(bset, context, &empty);

	isl_set_dump(set, stdout, 0);
	fprintf(stdout, "no solution:\n");
	isl_set_dump(empty, stdout, 4);

	isl_set_free(set);
	isl_set_free(empty);
	isl_ctx_free(ctx);

	return 0;
}
