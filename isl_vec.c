/*
 * Copyright 2008-2009 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv2.1 license
 *
 * Written by Sven Verdoolaege, K.U.Leuven, Departement
 * Computerwetenschappen, Celestijnenlaan 200A, B-3001 Leuven, Belgium
 */

#include "isl_seq.h"
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

	vec->ctx = ctx;
	isl_ctx_ref(ctx);
	vec->ref = 1;
	vec->size = size;
	vec->el = vec->block.data;

	return vec;
error:
	isl_blk_free(ctx, vec->block);
	return NULL;
}

__isl_give isl_vec *isl_vec_extend(__isl_take isl_vec *vec, unsigned size)
{
	if (!vec)
		return NULL;
	if (size <= vec->size)
		return vec;

	vec = isl_vec_cow(vec);
	if (!vec)
		return NULL;

	vec->block = isl_blk_extend(vec->ctx, vec->block, size);
	if (!vec->block.data)
		goto error;

	vec->size = size;
	vec->el = vec->block.data;

	return vec;
error:
	isl_vec_free(vec);
	return NULL;
}

__isl_give isl_vec *isl_vec_zero_extend(__isl_take isl_vec *vec, unsigned size)
{
	int extra;

	if (!vec)
		return NULL;
	if (size <= vec->size)
		return vec;

	vec = isl_vec_cow(vec);
	if (!vec)
		return NULL;

	extra = size - vec->size;
	vec = isl_vec_extend(vec, size);
	if (!vec)
		return NULL;

	isl_seq_clr(vec->el + size - extra, extra);

	return vec;
}

struct isl_vec *isl_vec_copy(struct isl_vec *vec)
{
	if (!vec)
		return NULL;

	vec->ref++;
	return vec;
}

struct isl_vec *isl_vec_dup(struct isl_vec *vec)
{
	struct isl_vec *vec2;

	if (!vec)
		return NULL;
	vec2 = isl_vec_alloc(vec->ctx, vec->size);
	isl_seq_cpy(vec2->el, vec->el, vec->size);
	return vec2;
}

struct isl_vec *isl_vec_cow(struct isl_vec *vec)
{
	struct isl_vec *vec2;
	if (!vec)
		return NULL;

	if (vec->ref == 1)
		return vec;

	vec2 = isl_vec_dup(vec);
	isl_vec_free(vec);
	return vec2;
}

void isl_vec_free(struct isl_vec *vec)
{
	if (!vec)
		return;

	if (--vec->ref > 0)
		return;

	isl_ctx_deref(vec->ctx);
	isl_blk_free(vec->ctx, vec->block);
	free(vec);
}

void isl_vec_dump(struct isl_vec *vec, FILE *out, int indent)
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
		isl_int_print(out, vec->el[i], 0);
	}
	fprintf(out, "]\n");
}

__isl_give isl_vec *isl_vec_clr(__isl_take isl_vec *vec)
{
	vec = isl_vec_cow(vec);
	if (!vec)
		return NULL;
	isl_seq_clr(vec->el, vec->size);
	return vec;
}

void isl_vec_lcm(struct isl_vec *vec, isl_int *lcm)
{
	isl_seq_lcm(vec->block.data, vec->size, lcm);
}

/* Given a rational vector, with the denominator in the first element
 * of the vector, round up all coordinates.
 */
struct isl_vec *isl_vec_ceil(struct isl_vec *vec)
{
	vec = isl_vec_cow(vec);
	if (!vec)
		return NULL;

	isl_seq_cdiv_q(vec->el + 1, vec->el + 1, vec->el[0], vec->size - 1);

	isl_int_set_si(vec->el[0], 1);

	return vec;
}

struct isl_vec *isl_vec_normalize(struct isl_vec *vec)
{
	if (!vec)
		return NULL;
	isl_seq_normalize(vec->ctx, vec->el, vec->size);
	return vec;
}

__isl_give isl_vec *isl_vec_scale(__isl_take isl_vec *vec, isl_int m)
{
	if (isl_int_is_one(m))
		return vec;
	vec = isl_vec_cow(vec);
	if (!vec)
		return NULL;
	isl_seq_scale(vec->el, vec->el, m, vec->size);
	return vec;
}

__isl_give isl_vec *isl_vec_add(__isl_take isl_vec *vec1,
	__isl_take isl_vec *vec2)
{
	vec1 = isl_vec_cow(vec1);
	if (!vec1 || !vec2)
		goto error;

	isl_assert(vec1->ctx, vec1->size == vec2->size, goto error);

	isl_seq_combine(vec1->el, vec1->ctx->one, vec1->el,
			vec1->ctx->one, vec2->el, vec1->size);
	
	isl_vec_free(vec2);
	return vec1;
error:
	isl_vec_free(vec1);
	isl_vec_free(vec2);
	return NULL;
}
