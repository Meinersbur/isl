#include <isl_mat.h>

struct isl_mat {
	int ref;

	struct isl_ctx *ctx;

#define ISL_MAT_BORROWED		(1 << 0)
	unsigned flags;

	unsigned n_row;
	unsigned n_col;

	isl_int **row;

	/* actual size of the rows in memory; n_col <= max_col */
	unsigned max_col;

	struct isl_blk block;
};
