/*
 * Copyright 2010      INRIA Saclay
 *
 * Use of this software is governed by the GNU LGPLv2.1 license
 *
 * Written by Sven Verdoolaege, INRIA Saclay - Ile-de-France,
 * Parc Club Orsay Universite, ZAC des vignes, 4 rue Jacques Monod,
 * 91893 Orsay, France
 */

#include <isl_ctx_private.h>
#include <isl_dim_private.h>
#include <isl_reordering.h>

__isl_give isl_reordering *isl_reordering_alloc(isl_ctx *ctx, int len)
{
	isl_reordering *exp;

	exp = isl_alloc(ctx, struct isl_reordering,
			sizeof(struct isl_reordering) + (len - 1) * sizeof(int));
	if (!exp)
		return NULL;

	exp->ref = 1;
	exp->len = len;
	exp->dim = NULL;

	return exp;
}

__isl_give isl_reordering *isl_reordering_copy(__isl_keep isl_reordering *exp)
{
	if (!exp)
		return NULL;

	exp->ref++;
	return exp;
}

__isl_give isl_reordering *isl_reordering_dup(__isl_keep isl_reordering *r)
{
	int i;
	isl_reordering *dup;

	if (!r)
		return NULL;

	dup = isl_reordering_alloc(r->dim->ctx, r->len);
	if (!dup)
		return NULL;

	dup->dim = isl_dim_copy(r->dim);
	if (!dup->dim)
		return isl_reordering_free(dup);
	for (i = 0; i < dup->len; ++i)
		dup->pos[i] = r->pos[i];

	return dup;
}

__isl_give isl_reordering *isl_reordering_cow(__isl_take isl_reordering *r)
{
	if (!r)
		return NULL;

	if (r->ref == 1)
		return r;
	r->ref--;
	return isl_reordering_dup(r);
}

void *isl_reordering_free(__isl_take isl_reordering *exp)
{
	if (!exp)
		return NULL;

	if (--exp->ref > 0)
		return NULL;

	isl_dim_free(exp->dim);
	free(exp);
	return NULL;
}

/* Construct a reordering that maps the parameters of "alignee"
 * to the corresponding parameters in a new dimension specification
 * that has the parameters of "aligner" first, followed by
 * any remaining parameters of "alignee" that do not occur in "aligner".
 */
__isl_give isl_reordering *isl_parameter_alignment_reordering(
	__isl_keep isl_dim *alignee, __isl_keep isl_dim *aligner)
{
	int i, j;
	isl_reordering *exp;

	if (!alignee || !aligner)
		return NULL;

	exp = isl_reordering_alloc(alignee->ctx, alignee->nparam);
	if (!exp)
		return NULL;

	exp->dim = isl_dim_copy(aligner);

	for (i = 0; i < alignee->nparam; ++i) {
		const char *name_i;
		name_i = isl_dim_get_name(alignee, isl_dim_param, i);
		if (!name_i)
			isl_die(alignee->ctx, isl_error_invalid,
				"cannot align unnamed parameters", goto error);
		for (j = 0; j < aligner->nparam; ++j) {
			const char *name_j;
			name_j = isl_dim_get_name(aligner, isl_dim_param, j);
			if (name_i == name_j)
				break;
		}
		if (j < aligner->nparam)
			exp->pos[i] = j;
		else {
			int pos;
			pos = isl_dim_size(exp->dim, isl_dim_param);
			exp->dim = isl_dim_add(exp->dim, isl_dim_param, 1);
			exp->dim = isl_dim_set_name(exp->dim,
						isl_dim_param, pos, name_i);
			exp->pos[i] = pos;
		}
	}

	return exp;
error:
	isl_reordering_free(exp);
	return NULL;
}

__isl_give isl_reordering *isl_reordering_extend(__isl_take isl_reordering *exp,
	unsigned extra)
{
	int i;
	isl_reordering *res;
	int offset;

	if (!exp)
		return NULL;
	if (extra == 0)
		return exp;

	offset = isl_dim_total(exp->dim) - exp->len;
	res = isl_reordering_alloc(exp->dim->ctx, exp->len + extra);
	if (!res)
		goto error;
	res->dim = isl_dim_copy(exp->dim);
	for (i = 0; i < exp->len; ++i)
		res->pos[i] = exp->pos[i];
	for (i = exp->len; i < res->len; ++i)
		res->pos[i] = offset + i;

	isl_reordering_free(exp);

	return res;
error:
	isl_reordering_free(exp);
	return NULL;
}

__isl_give isl_reordering *isl_reordering_extend_dim(
	__isl_take isl_reordering *exp, __isl_take isl_dim *dim)
{
	isl_reordering *res;

	if (!exp || !dim)
		goto error;

	res = isl_reordering_extend(isl_reordering_copy(exp),
					    isl_dim_total(dim) - exp->len);
	res = isl_reordering_cow(res);
	if (!res)
		goto error;
	isl_dim_free(res->dim);
	res->dim = isl_dim_replace(dim, isl_dim_param, exp->dim);

	isl_reordering_free(exp);

	return res;
error:
	isl_reordering_free(exp);
	isl_dim_free(dim);
	return NULL;
}

void isl_reordering_dump(__isl_keep isl_reordering *exp)
{
	int i;

	for (i = 0; i < exp->len; ++i)
		fprintf(stderr, "%d -> %d; ", i, exp->pos[i]);
	fprintf(stderr, "\n");
}
