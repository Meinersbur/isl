#ifndef ISL_BLK_H
#define ISL_BLK_H

#include <isl_int.h>
#include <isl_ctx.h>

#if defined(__cplusplus)
extern "C" {
#endif

struct isl_blk {
	size_t size;
	isl_int *data;
};

struct isl_blk isl_blk_alloc(struct isl_ctx *ctx, size_t n);
struct isl_blk isl_blk_empty();
struct isl_blk isl_blk_extend(struct isl_ctx *ctx, struct isl_blk block,
				size_t new_n);
void isl_blk_free(struct isl_ctx *ctx, struct isl_blk block);

#if defined(__cplusplus)
}
#endif

#endif
