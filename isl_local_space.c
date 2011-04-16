/*
 * Copyright 2011      INRIA Saclay
 *
 * Use of this software is governed by the GNU LGPLv2.1 license
 *
 * Written by Sven Verdoolaege, INRIA Saclay - Ile-de-France,
 * Parc Club Orsay Universite, ZAC des vignes, 4 rue Jacques Monod,
 * 91893 Orsay, France
 */

#include <isl_map_private.h>
#include <isl_local_space_private.h>
#include <isl_dim_private.h>
#include <isl_mat_private.h>
#include <isl/seq.h>

isl_ctx *isl_local_space_get_ctx(__isl_keep isl_local_space *ls)
{
	return ls ? ls->dim->ctx : NULL;
}

__isl_give isl_local_space *isl_local_space_alloc_div(__isl_take isl_dim *dim,
	__isl_take isl_mat *div)
{
	isl_ctx *ctx;
	isl_local_space *ls = NULL;

	if (!dim)
		goto error;

	ctx = isl_dim_get_ctx(dim);
	ls = isl_calloc_type(ctx, struct isl_local_space);
	if (!ls)
		goto error;

	ls->ref = 1;
	ls->dim = dim;
	ls->div = div;

	return ls;
error:
	isl_dim_free(dim);
	isl_local_space_free(ls);
	return NULL;
}

__isl_give isl_local_space *isl_local_space_alloc(__isl_take isl_dim *dim,
	unsigned n_div)
{
	isl_ctx *ctx;
	isl_mat *div;
	unsigned total;

	if (!dim)
		return NULL;

	total = isl_dim_total(dim);

	ctx = isl_dim_get_ctx(dim);
	div = isl_mat_alloc(ctx, n_div, 1 + 1 + total + n_div);
	return isl_local_space_alloc_div(dim, div);
}

__isl_give isl_local_space *isl_local_space_from_dim(__isl_take isl_dim *dim)
{
	return isl_local_space_alloc(dim, 0);
}

__isl_give isl_local_space *isl_local_space_copy(__isl_keep isl_local_space *ls)
{
	if (!ls)
		return NULL;

	ls->ref++;
	return ls;
}

__isl_give isl_local_space *isl_local_space_dup(__isl_keep isl_local_space *ls)
{
	isl_local_space *dup;

	if (!ls)
		return NULL;

	return isl_local_space_alloc_div(isl_dim_copy(ls->dim),
					 isl_mat_copy(ls->div));

}

__isl_give isl_local_space *isl_local_space_cow(__isl_take isl_local_space *ls)
{
	if (!ls)
		return NULL;

	if (ls->ref == 1)
		return ls;
	ls->ref--;
	return isl_local_space_dup(ls);
}

void *isl_local_space_free(__isl_take isl_local_space *ls)
{
	if (!ls)
		return NULL;

	if (--ls->ref > 0)
		return NULL;

	isl_dim_free(ls->dim);
	isl_mat_free(ls->div);

	free(ls);

	return NULL;
}

int isl_local_space_dim(__isl_keep isl_local_space *ls,
	enum isl_dim_type type)
{
	if (!ls)
		return 0;
	if (type == isl_dim_div)
		return ls->div->n_row;
	if (type == isl_dim_all)
		return isl_dim_size(ls->dim, isl_dim_all) + ls->div->n_row;
	return isl_dim_size(ls->dim, type);
}

unsigned isl_local_space_offset(__isl_keep isl_local_space *ls,
	enum isl_dim_type type)
{
	isl_dim *dim;

	if (!ls)
		return 0;

	dim = ls->dim;
	switch (type) {
	case isl_dim_cst:	return 0;
	case isl_dim_param:	return 1;
	case isl_dim_in:	return 1 + dim->nparam;
	case isl_dim_out:	return 1 + dim->nparam + dim->n_in;
	case isl_dim_div:	return 1 + dim->nparam + dim->n_in + dim->n_out;
	default:		return 0;
	}
}

const char *isl_local_space_get_dim_name(__isl_keep isl_local_space *ls,
	enum isl_dim_type type, unsigned pos)
{
	return ls ? isl_dim_get_name(ls->dim, type, pos) : NULL;
}

__isl_give isl_div *isl_local_space_get_div(__isl_keep isl_local_space *ls,
	int pos)
{
	isl_basic_map *bmap;

	if (!ls)
		return NULL;

	if (pos < 0 || pos >= ls->div->n_row)
		isl_die(isl_local_space_get_ctx(ls), isl_error_invalid,
			"index out of bounds", return NULL);

	bmap = isl_basic_map_from_local_space(isl_local_space_copy(ls));
	return isl_basic_map_div(bmap, pos);
}

__isl_give isl_dim *isl_local_space_get_dim(__isl_keep isl_local_space *ls)
{
	if (!ls)
		return NULL;

	return isl_dim_copy(ls->dim);
}

__isl_give isl_local_space *isl_local_space_add_div(
	__isl_take isl_local_space *ls, __isl_take isl_vec *div)
{
	ls = isl_local_space_cow(ls);
	if (!ls || !div)
		goto error;

	if (ls->div->n_col != div->size)
		isl_die(isl_local_space_get_ctx(ls), isl_error_invalid,
			"incompatible dimensions", goto error);

	ls->div = isl_mat_add_zero_cols(ls->div, 1);
	ls->div = isl_mat_add_rows(ls->div, 1);
	if (!ls->div)
		goto error;

	isl_seq_cpy(ls->div->row[ls->div->n_row - 1], div->el, div->size);
	isl_int_set_si(ls->div->row[ls->div->n_row - 1][div->size], 0);

	isl_vec_free(div);
	return ls;
error:
	isl_local_space_free(ls);
	isl_vec_free(div);
	return NULL;
}

__isl_give isl_local_space *isl_local_space_replace_divs(
	__isl_take isl_local_space *ls, __isl_take isl_mat *div)
{
	ls = isl_local_space_cow(ls);

	if (!ls || !div)
		goto error;

	isl_mat_free(ls->div);
	ls->div = div;
	return ls;
error:
	isl_mat_free(div);
	isl_local_space_free(ls);
	return NULL;
}
