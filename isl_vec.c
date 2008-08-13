#include "isl_vec.h"

struct isl_vec *isl_vec_alloc(struct isl_ctx *ctx, unsigned size)
{
	struct isl_vec *vec;

	vec = isl_alloc_type(ctx, struct isl_vec);
	if (!vec)
		return NULL;

	vec->block = isl_blk_alloc(ctx, size);
	if (isl_blk_is_error(vec->block))
		goto error;

	vec->ref = 1;
	vec->size = size;

	return vec;
error:
	isl_blk_free(ctx, vec->block);
	return NULL;
}

struct isl_vec *isl_vec_copy(struct isl_ctx *ctx, struct isl_vec *vec)
{
	if (!vec)
		return NULL;

	vec->ref++;
	return vec;
}

void isl_vec_free(struct isl_ctx *ctx, struct isl_vec *vec)
{
	if (!vec)
		return;

	if (--vec->ref > 0)
		return;

	isl_blk_free(ctx, vec->block);
	free(vec);
}

void isl_vec_dump(struct isl_ctx *ctx, struct isl_vec *vec,
				FILE *out, int indent)
{
	int i;
	fprintf(out, "%*s[", indent, "");
	for (i = 0; i < vec->size; ++i) {
		if (i)
		    fprintf(out, ",");
		isl_int_print(out, vec->block.data[i]);
	}
	fprintf(out, "]\n");
}
