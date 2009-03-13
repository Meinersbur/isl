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

struct isl_vec *isl_vec_dup(struct isl_ctx *ctx, struct isl_vec *vec)
{
	struct isl_vec *vec2;

	if (!vec)
		return NULL;
	vec2 = isl_vec_alloc(ctx, vec->size);
	isl_seq_cpy(vec2->block.data, vec->block.data, vec->size);
	return vec2;
}

struct isl_vec *isl_vec_cow(struct isl_ctx *ctx, struct isl_vec *vec)
{
	struct isl_vec *vec2;
	if (!vec)
		return NULL;

	if (vec->ref == 1)
		return vec;

	vec2 = isl_vec_dup(ctx, vec);
	isl_vec_free(ctx, vec);
	return vec2;
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

	if (!vec) {
		fprintf(out, "%*snull vec\n", indent, "");
		return;
	}

	fprintf(out, "%*s[", indent, "");
	for (i = 0; i < vec->size; ++i) {
		if (i)
		    fprintf(out, ",");
		isl_int_print(out, vec->block.data[i], 0);
	}
	fprintf(out, "]\n");
}

void isl_vec_lcm(struct isl_ctx *ctx, struct isl_vec *vec, isl_int *lcm)
{
	isl_seq_lcm(vec->block.data, vec->size, lcm);
}
