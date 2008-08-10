#include "isl_blk.h"

struct isl_blk isl_blk_empty()
{
	struct isl_blk block;
	block.size = -1;
	block.data = NULL;
	return block;
}

struct isl_blk isl_blk_alloc(struct isl_ctx *ctx, size_t n)
{
	struct isl_blk block;

	block.data = isl_alloc_array(ctx, isl_int, n);
	if (!block.data)
		block.size = -1;
	else {
		int i;
		block.size = n;
		for (i = 0; i < n; ++i)
			isl_int_init(block.data[i]);
	}

	return block;
}

struct isl_blk isl_blk_extend(struct isl_ctx *ctx, struct isl_blk block,
				size_t new_n)
{
	if (block.size >= new_n)
		return block;
	block.data = isl_realloc_array(ctx, block.data, isl_int, new_n);
	if (!block.data)
		block.size = -1;
	else {
		int i;
		for (i = block.size; i < new_n; ++i)
			isl_int_init(block.data[i]);
		block.size = new_n;
	}

	return block;
}

void isl_blk_free(struct isl_ctx *ctx, struct isl_blk block)
{
	int i;

	for (i = 0; i < block.size; ++i)
		isl_int_clear(block.data[i]);
	free(block.data);
}
