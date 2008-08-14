#include <ctype.h>
#include <stdio.h>
#include <isl_set.h>
#include "isl_map_private.h"

static char *next_line(FILE *input, char *line, unsigned len)
{
	char *p;

	do {
		if (!(p = fgets(line, len, input)))
			return NULL;
		while (isspace(*p) && *p != '\n')
			++p;
	} while (*p == '#' || *p == '\n');

	return p;
}

struct isl_basic_set *isl_basic_set_read_from_file(struct isl_ctx *ctx,
		FILE *input, unsigned input_format)
{
	struct isl_basic_set *bset = NULL;
	int i, j;
	unsigned n_row, n_col;
	unsigned dim;
	char line[1024];
	char val[1024];
	char *p;

	isl_assert(ctx, input_format == ISL_FORMAT_POLYLIB, return NULL);
	isl_assert(ctx, next_line(input, line, sizeof(line)), return NULL);
	isl_assert(ctx, sscanf(line, "%u %u", &n_row, &n_col) == 2, return NULL);
	isl_assert(ctx, n_col >= 2, return NULL);
	dim = n_col - 2;
	bset = isl_basic_set_alloc(ctx, 0, dim, 0, n_row, n_row);
	if (!bset)
		return NULL;
	for (i = 0; i < n_row; ++i) {
		int type;
		int offset;
		int n;
		int k;
		isl_int *c;

		p = next_line(input, line, sizeof(line));
		isl_assert(ctx, p, goto error);
		n = sscanf(p, "%u%n", &type, &offset);
		isl_assert(ctx, n != 0, goto error);
		p += offset;
		isl_assert(ctx, type == 0 || type == 1, goto error);
		if (type == 0) {
			k = isl_basic_set_alloc_equality(ctx, bset);
			c = bset->eq[k];
		} else {
			k = isl_basic_set_alloc_inequality(ctx, bset);
			c = bset->ineq[k];
		}
		isl_assert(ctx, k >= 0, goto error);
		for (j = 0; j < dim; ++j) {
			n = sscanf(p, "%s%n", val, &offset);
			isl_assert(ctx, n != 0, goto error);
			isl_int_read(c[1+j], val);
			p += offset;
		}
		n = sscanf(p, "%s%n", val, &offset);
		isl_assert(ctx, n != 0, goto error);
		isl_int_read(c[0], val);
	}
	bset = isl_basic_set_simplify(ctx, bset);
	bset = isl_basic_set_finalize(ctx, bset);
	return bset;
error:
	isl_basic_set_free(ctx, bset);
	return NULL;
}
