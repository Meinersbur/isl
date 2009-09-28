#include "isl_set.h"

int main(int argc, char **argv)
{
	struct isl_ctx *ctx = isl_ctx_alloc();
	struct isl_basic_set *bset;

	bset = isl_basic_set_read_from_file(ctx, stdin, 0, ISL_FORMAT_POLYLIB);
	bset = isl_basic_set_detect_equalities(bset);
	isl_basic_set_print(bset, stdout, 0, "", "", ISL_FORMAT_POLYLIB);
	isl_basic_set_free(bset);
	isl_ctx_free(ctx);

	return 0;
}
