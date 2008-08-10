#ifndef ISL_VEC_H
#define ISL_VEC_H

#include <stdio.h>

#include <isl_int.h>
#include <isl_ctx.h>
#include <isl_blk.h>

#if defined(__cplusplus)
extern "C" {
#endif

struct isl_vec {
	int ref;

	unsigned size;

	struct isl_blk block;
};

struct isl_vec *isl_vec_alloc(struct isl_ctx *ctx, unsigned size);
struct isl_vec *isl_vec_copy(struct isl_ctx *ctx, struct isl_vec *vec);
void isl_vec_free(struct isl_ctx *ctx, struct isl_vec *vec);

void isl_vec_dump(struct isl_ctx *ctx, struct isl_vec *vec,
				FILE *out, int indent);

#if defined(__cplusplus)
}
#endif

#endif
