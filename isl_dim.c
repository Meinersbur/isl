#include "isl_dim.h"

struct isl_dim *isl_dim_alloc(struct isl_ctx *ctx,
			unsigned nparam, unsigned n_in, unsigned n_out)
{
	struct isl_dim *dim;

	dim = isl_alloc_type(ctx, struct isl_dim);
	if (!dim)
		return NULL;

	dim->ctx = ctx;
	isl_ctx_ref(ctx);
	dim->ref = 1;
	dim->nparam = nparam;
	dim->n_in = n_in;
	dim->n_out = n_out;

	return dim;
}

struct isl_dim *isl_dim_dup(struct isl_dim *dim)
{
	return isl_dim_alloc(dim->ctx, dim->nparam, dim->n_in, dim->n_out);
}

struct isl_dim *isl_dim_cow(struct isl_dim *dim)
{
	if (!dim)
		return NULL;

	if (dim->ref == 1)
		return dim;
	dim->ref--;
	return isl_dim_dup(dim);
}

struct isl_dim *isl_dim_copy(struct isl_dim *dim)
{
	if (!dim)
		return NULL;

	dim->ref++;
	return dim;
}

void isl_dim_free(struct isl_dim *dim)
{
	if (!dim)
		return;

	if (--dim->ref > 0)
		return;

	isl_ctx_deref(dim->ctx);
	
	free(dim);
}

struct isl_dim *isl_dim_join(struct isl_dim *left, struct isl_dim *right)
{
	struct isl_dim *dim;

	if (!left || !right)
		goto error;

	isl_assert(left->ctx, left->nparam == right->nparam, goto error);
	isl_assert(left->ctx, left->n_out == right->n_in, goto error);

	dim = isl_dim_alloc(left->ctx, left->nparam, left->n_in, right->n_out);
	if (!dim)
		goto error;

	isl_dim_free(left);
	isl_dim_free(right);

	return dim;
error:
	isl_dim_free(left);
	isl_dim_free(right);
	return NULL;
}

struct isl_dim *isl_dim_reverse(struct isl_dim *dim)
{
	unsigned t;

	if (!dim)
		return NULL;
	if (dim->n_in == dim->n_out)
		return dim;

	dim = isl_dim_cow(dim);
	if (!dim)
		return NULL;

	t = dim->n_in;
	dim->n_in = dim->n_out;
	dim->n_out = t;

	return dim;
error:
	isl_dim_free(dim);
	return NULL;
}

unsigned isl_dim_total(struct isl_dim *dim)
{
	return dim->nparam + dim->n_in + dim->n_out;
}

int isl_dim_equal(struct isl_dim *dim1, struct isl_dim *dim2)
{
	return dim1->nparam == dim2->nparam &&
	       dim1->n_in == dim2->n_in &&
	       dim1->n_out == dim2->n_out;
}

int isl_dim_compatible(struct isl_dim *dim1, struct isl_dim *dim2)
{
	return dim1->nparam == dim2->nparam &&
	       dim1->n_in + dim1->n_out == dim2->n_in + dim2->n_out;
}
