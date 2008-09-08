#include <assert.h>
#include <stdio.h>
#include <limits.h>
#include <isl_ctx.h>
#include <isl_set.h>

static char *srcdir;

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

	bset1 = isl_basic_set_read_from_file(ctx, input, 0, ISL_FORMAT_OMEGA);
	bmap = isl_basic_map_read_from_file(ctx, input, 0, ISL_FORMAT_OMEGA);

	bset1 = isl_basic_set_apply(ctx, bset1, bmap);

	bset2 = isl_basic_set_read_from_file(ctx, input, 0, ISL_FORMAT_OMEGA);

	assert(isl_basic_set_is_equal(ctx, bset1, bset2) == 1);

	isl_basic_set_free(ctx, bset1);
	isl_basic_set_free(ctx, bset2);

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

	bset1 = isl_basic_set_read_from_file(ctx, input, 0, ISL_FORMAT_POLYLIB);
	bset2 = isl_basic_set_read_from_file(ctx, input, 0, ISL_FORMAT_POLYLIB);

	bset1 = isl_basic_set_affine_hull(ctx, bset1);

	assert(isl_basic_set_is_equal(ctx, bset1, bset2) == 1);

	isl_basic_set_free(ctx, bset1);
	isl_basic_set_free(ctx, bset2);

	fclose(input);
}

void test_affine_hull(struct isl_ctx *ctx)
{
	test_affine_hull_case(ctx, "affine2");
	test_affine_hull_case(ctx, "affine");
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

	bset1 = isl_basic_set_read_from_file(ctx, input, 0, ISL_FORMAT_POLYLIB);
	bset2 = isl_basic_set_read_from_file(ctx, input, 0, ISL_FORMAT_POLYLIB);

	set = isl_basic_set_union(ctx, bset1, bset2);
	bset1 = isl_set_convex_hull(ctx, set);

	bset2 = isl_basic_set_read_from_file(ctx, input, 0, ISL_FORMAT_POLYLIB);

	assert(isl_basic_set_is_equal(ctx, bset1, bset2) == 1);

	isl_basic_set_free(ctx, bset1);
	isl_basic_set_free(ctx, bset2);

	fclose(input);
}

void test_convex_hull(struct isl_ctx *ctx)
{
	test_convex_hull_case(ctx, "convex0");
	test_convex_hull_case(ctx, "convex1");
	test_convex_hull_case(ctx, "convex2");
	test_convex_hull_case(ctx, "convex3");
	test_convex_hull_case(ctx, "convex4");
	test_convex_hull_case(ctx, "convex5");
	test_convex_hull_case(ctx, "convex6");
	test_convex_hull_case(ctx, "convex7");
}

int main()
{
	struct isl_ctx *ctx;

	srcdir = getenv("srcdir");

	ctx = isl_ctx_alloc();
	test_application(ctx);
	test_affine_hull(ctx);
	test_convex_hull(ctx);
	isl_ctx_free(ctx);
	return 0;
}
