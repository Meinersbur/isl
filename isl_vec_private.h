#ifndef ISL_VEC_PRIVATE_H
#define ISL_VEC_PRIVATE_H

#include <isl_blk.h>
#include <isl/vec.h>

struct isl_vec {
	int ref;

	struct isl_ctx *ctx;

	unsigned size;
	isl_int *el;

	struct isl_blk block;
};

__isl_give isl_vec *isl_vec_cow(__isl_take isl_vec *vec);

#endif
