/*
 * Copyright 2010      INRIA Saclay
 *
 * Use of this software is governed by the GNU LGPLv2.1 license
 *
 * Written by Sven Verdoolaege, INRIA Saclay - Ile-de-France,
 * Parc Club Orsay Universite, ZAC des vignes, 4 rue Jacques Monod,
 * 91893 Orsay, France 
 */

#include <stdlib.h>
#include <isl_seq.h>
#include <isl_polynomial_private.h>
#include <isl_point_private.h>
#include <isl_dim_private.h>
#include <isl_map_private.h>

static unsigned pos(__isl_keep isl_dim *dim, enum isl_dim_type type)
{
	switch (type) {
	case isl_dim_param:	return 0;
	case isl_dim_in:	return dim->nparam;
	case isl_dim_out:	return dim->nparam + dim->n_in;
	default:		return 0;
	}
}

int isl_upoly_is_cst(__isl_keep struct isl_upoly *up)
{
	if (!up)
		return -1;

	return up->var < 0;
}

__isl_keep struct isl_upoly_cst *isl_upoly_as_cst(__isl_keep struct isl_upoly *up)
{
	if (!up)
		return NULL;

	isl_assert(up->ctx, up->var < 0, return NULL);

	return (struct isl_upoly_cst *)up;
}

__isl_keep struct isl_upoly_rec *isl_upoly_as_rec(__isl_keep struct isl_upoly *up)
{
	if (!up)
		return NULL;

	isl_assert(up->ctx, up->var >= 0, return NULL);

	return (struct isl_upoly_rec *)up;
}

int isl_upoly_is_equal(__isl_keep struct isl_upoly *up1,
	__isl_keep struct isl_upoly *up2)
{
	int i;
	struct isl_upoly_rec *rec1, *rec2;

	if (!up1 || !up2)
		return -1;
	if (up1 == up2)
		return 1;
	if (up1->var != up2->var)
		return 0;
	if (isl_upoly_is_cst(up1)) {
		struct isl_upoly_cst *cst1, *cst2;
		cst1 = isl_upoly_as_cst(up1);
		cst2 = isl_upoly_as_cst(up2);
		if (!cst1 || !cst2)
			return -1;
		return isl_int_eq(cst1->n, cst2->n) &&
		       isl_int_eq(cst1->d, cst2->d);
	}

	rec1 = isl_upoly_as_rec(up1);
	rec2 = isl_upoly_as_rec(up2);
	if (!rec1 || !rec2)
		return -1;

	if (rec1->n != rec2->n)
		return 0;

	for (i = 0; i < rec1->n; ++i) {
		int eq = isl_upoly_is_equal(rec1->p[i], rec2->p[i]);
		if (eq < 0 || !eq)
			return eq;
	}

	return 1;
}

int isl_upoly_is_zero(__isl_keep struct isl_upoly *up)
{
	struct isl_upoly_cst *cst;

	if (!up)
		return -1;
	if (!isl_upoly_is_cst(up))
		return 0;

	cst = isl_upoly_as_cst(up);
	if (!cst)
		return -1;

	return isl_int_is_zero(cst->n) && isl_int_is_pos(cst->d);
}

int isl_upoly_sgn(__isl_keep struct isl_upoly *up)
{
	struct isl_upoly_cst *cst;

	if (!up)
		return 0;
	if (!isl_upoly_is_cst(up))
		return 0;

	cst = isl_upoly_as_cst(up);
	if (!cst)
		return 0;

	return isl_int_sgn(cst->n);
}

int isl_upoly_is_nan(__isl_keep struct isl_upoly *up)
{
	struct isl_upoly_cst *cst;

	if (!up)
		return -1;
	if (!isl_upoly_is_cst(up))
		return 0;

	cst = isl_upoly_as_cst(up);
	if (!cst)
		return -1;

	return isl_int_is_zero(cst->n) && isl_int_is_zero(cst->d);
}

int isl_upoly_is_infty(__isl_keep struct isl_upoly *up)
{
	struct isl_upoly_cst *cst;

	if (!up)
		return -1;
	if (!isl_upoly_is_cst(up))
		return 0;

	cst = isl_upoly_as_cst(up);
	if (!cst)
		return -1;

	return isl_int_is_pos(cst->n) && isl_int_is_zero(cst->d);
}

int isl_upoly_is_neginfty(__isl_keep struct isl_upoly *up)
{
	struct isl_upoly_cst *cst;

	if (!up)
		return -1;
	if (!isl_upoly_is_cst(up))
		return 0;

	cst = isl_upoly_as_cst(up);
	if (!cst)
		return -1;

	return isl_int_is_neg(cst->n) && isl_int_is_zero(cst->d);
}

int isl_upoly_is_one(__isl_keep struct isl_upoly *up)
{
	struct isl_upoly_cst *cst;

	if (!up)
		return -1;
	if (!isl_upoly_is_cst(up))
		return 0;

	cst = isl_upoly_as_cst(up);
	if (!cst)
		return -1;

	return isl_int_eq(cst->n, cst->d) && isl_int_is_pos(cst->d);
}

int isl_upoly_is_negone(__isl_keep struct isl_upoly *up)
{
	struct isl_upoly_cst *cst;

	if (!up)
		return -1;
	if (!isl_upoly_is_cst(up))
		return 0;

	cst = isl_upoly_as_cst(up);
	if (!cst)
		return -1;

	return isl_int_is_negone(cst->n) && isl_int_is_one(cst->d);
}

__isl_give struct isl_upoly_cst *isl_upoly_cst_alloc(struct isl_ctx *ctx)
{
	struct isl_upoly_cst *cst;

	cst = isl_alloc_type(ctx, struct isl_upoly_cst);
	if (!cst)
		return NULL;

	cst->up.ref = 1;
	cst->up.ctx = ctx;
	isl_ctx_ref(ctx);
	cst->up.var = -1;

	isl_int_init(cst->n);
	isl_int_init(cst->d);

	return cst;
}

__isl_give struct isl_upoly *isl_upoly_zero(struct isl_ctx *ctx)
{
	struct isl_upoly_cst *cst;

	cst = isl_upoly_cst_alloc(ctx);
	if (!cst)
		return NULL;

	isl_int_set_si(cst->n, 0);
	isl_int_set_si(cst->d, 1);

	return &cst->up;
}

__isl_give struct isl_upoly *isl_upoly_infty(struct isl_ctx *ctx)
{
	struct isl_upoly_cst *cst;

	cst = isl_upoly_cst_alloc(ctx);
	if (!cst)
		return NULL;

	isl_int_set_si(cst->n, 1);
	isl_int_set_si(cst->d, 0);

	return &cst->up;
}

__isl_give struct isl_upoly *isl_upoly_neginfty(struct isl_ctx *ctx)
{
	struct isl_upoly_cst *cst;

	cst = isl_upoly_cst_alloc(ctx);
	if (!cst)
		return NULL;

	isl_int_set_si(cst->n, -1);
	isl_int_set_si(cst->d, 0);

	return &cst->up;
}

__isl_give struct isl_upoly *isl_upoly_nan(struct isl_ctx *ctx)
{
	struct isl_upoly_cst *cst;

	cst = isl_upoly_cst_alloc(ctx);
	if (!cst)
		return NULL;

	isl_int_set_si(cst->n, 0);
	isl_int_set_si(cst->d, 0);

	return &cst->up;
}

__isl_give struct isl_upoly *isl_upoly_rat_cst(struct isl_ctx *ctx,
	isl_int n, isl_int d)
{
	struct isl_upoly_cst *cst;

	cst = isl_upoly_cst_alloc(ctx);
	if (!cst)
		return NULL;

	isl_int_set(cst->n, n);
	isl_int_set(cst->d, d);

	return &cst->up;
}

__isl_give struct isl_upoly_rec *isl_upoly_alloc_rec(struct isl_ctx *ctx,
	int var, int size)
{
	struct isl_upoly_rec *rec;

	isl_assert(ctx, var >= 0, return NULL);
	isl_assert(ctx, size >= 0, return NULL);
	rec = isl_calloc(ctx, struct isl_upoly_rec,
			sizeof(struct isl_upoly_rec) +
			(size - 1) * sizeof(struct isl_upoly *));
	if (!rec)
		return NULL;

	rec->up.ref = 1;
	rec->up.ctx = ctx;
	isl_ctx_ref(ctx);
	rec->up.var = var;

	rec->n = 0;
	rec->size = size;

	return rec;
}

isl_ctx *isl_qpolynomial_get_ctx(__isl_keep isl_qpolynomial *qp)
{
	return qp ? qp->dim->ctx : NULL;
}

__isl_give isl_dim *isl_qpolynomial_get_dim(__isl_keep isl_qpolynomial *qp)
{
	return qp ? isl_dim_copy(qp->dim) : NULL;
}

unsigned isl_qpolynomial_dim(__isl_keep isl_qpolynomial *qp,
	enum isl_dim_type type)
{
	return qp ? isl_dim_size(qp->dim, type) : 0;
}

int isl_qpolynomial_is_zero(__isl_keep isl_qpolynomial *qp)
{
	return qp ? isl_upoly_is_zero(qp->upoly) : -1;
}

int isl_qpolynomial_is_one(__isl_keep isl_qpolynomial *qp)
{
	return qp ? isl_upoly_is_one(qp->upoly) : -1;
}

int isl_qpolynomial_is_nan(__isl_keep isl_qpolynomial *qp)
{
	return qp ? isl_upoly_is_nan(qp->upoly) : -1;
}

int isl_qpolynomial_is_infty(__isl_keep isl_qpolynomial *qp)
{
	return qp ? isl_upoly_is_infty(qp->upoly) : -1;
}

int isl_qpolynomial_is_neginfty(__isl_keep isl_qpolynomial *qp)
{
	return qp ? isl_upoly_is_neginfty(qp->upoly) : -1;
}

int isl_qpolynomial_sgn(__isl_keep isl_qpolynomial *qp)
{
	return qp ? isl_upoly_sgn(qp->upoly) : 0;
}

static void upoly_free_cst(__isl_take struct isl_upoly_cst *cst)
{
	isl_int_clear(cst->n);
	isl_int_clear(cst->d);
}

static void upoly_free_rec(__isl_take struct isl_upoly_rec *rec)
{
	int i;

	for (i = 0; i < rec->n; ++i)
		isl_upoly_free(rec->p[i]);
}

__isl_give struct isl_upoly *isl_upoly_copy(__isl_keep struct isl_upoly *up)
{
	if (!up)
		return NULL;

	up->ref++;
	return up;
}

__isl_give struct isl_upoly *isl_upoly_dup_cst(__isl_keep struct isl_upoly *up)
{
	struct isl_upoly_cst *cst;
	struct isl_upoly_cst *dup;

	cst = isl_upoly_as_cst(up);
	if (!cst)
		return NULL;

	dup = isl_upoly_as_cst(isl_upoly_zero(up->ctx));
	if (!dup)
		return NULL;
	isl_int_set(dup->n, cst->n);
	isl_int_set(dup->d, cst->d);

	return &dup->up;
}

__isl_give struct isl_upoly *isl_upoly_dup_rec(__isl_keep struct isl_upoly *up)
{
	int i;
	struct isl_upoly_rec *rec;
	struct isl_upoly_rec *dup;

	rec = isl_upoly_as_rec(up);
	if (!rec)
		return NULL;

	dup = isl_upoly_alloc_rec(up->ctx, up->var, rec->n);
	if (!dup)
		return NULL;

	for (i = 0; i < rec->n; ++i) {
		dup->p[i] = isl_upoly_copy(rec->p[i]);
		if (!dup->p[i])
			goto error;
		dup->n++;
	}

	return &dup->up;
error:
	isl_upoly_free(&dup->up);
	return NULL;
}

__isl_give struct isl_upoly *isl_upoly_dup(__isl_keep struct isl_upoly *up)
{
	struct isl_upoly *dup;

	if (!up)
		return NULL;

	if (isl_upoly_is_cst(up))
		return isl_upoly_dup_cst(up);
	else
		return isl_upoly_dup_rec(up);
}

__isl_give struct isl_upoly *isl_upoly_cow(__isl_take struct isl_upoly *up)
{
	if (!up)
		return NULL;

	if (up->ref == 1)
		return up;
	up->ref--;
	return isl_upoly_dup(up);
}

void isl_upoly_free(__isl_take struct isl_upoly *up)
{
	if (!up)
		return;

	if (--up->ref > 0)
		return;

	if (up->var < 0)
		upoly_free_cst((struct isl_upoly_cst *)up);
	else
		upoly_free_rec((struct isl_upoly_rec *)up);

	isl_ctx_deref(up->ctx);
	free(up);
}

static void isl_upoly_cst_reduce(__isl_keep struct isl_upoly_cst *cst)
{
	isl_int gcd;

	isl_int_init(gcd);
	isl_int_gcd(gcd, cst->n, cst->d);
	if (!isl_int_is_zero(gcd) && !isl_int_is_one(gcd)) {
		isl_int_divexact(cst->n, cst->n, gcd);
		isl_int_divexact(cst->d, cst->d, gcd);
	}
	isl_int_clear(gcd);
}

__isl_give struct isl_upoly *isl_upoly_sum_cst(__isl_take struct isl_upoly *up1,
	__isl_take struct isl_upoly *up2)
{
	struct isl_upoly_cst *cst1;
	struct isl_upoly_cst *cst2;

	up1 = isl_upoly_cow(up1);
	if (!up1 || !up2)
		goto error;

	cst1 = isl_upoly_as_cst(up1);
	cst2 = isl_upoly_as_cst(up2);

	if (isl_int_eq(cst1->d, cst2->d))
		isl_int_add(cst1->n, cst1->n, cst2->n);
	else {
		isl_int_mul(cst1->n, cst1->n, cst2->d);
		isl_int_addmul(cst1->n, cst2->n, cst1->d);
		isl_int_mul(cst1->d, cst1->d, cst2->d);
	}

	isl_upoly_cst_reduce(cst1);

	isl_upoly_free(up2);
	return up1;
error:
	isl_upoly_free(up1);
	isl_upoly_free(up2);
	return NULL;
}

static __isl_give struct isl_upoly *replace_by_zero(
	__isl_take struct isl_upoly *up)
{
	struct isl_ctx *ctx;

	if (!up)
		return NULL;
	ctx = up->ctx;
	isl_upoly_free(up);
	return isl_upoly_zero(ctx);
}

static __isl_give struct isl_upoly *replace_by_constant_term(
	__isl_take struct isl_upoly *up)
{
	struct isl_upoly_rec *rec;
	struct isl_upoly *cst;

	if (!up)
		return NULL;

	rec = isl_upoly_as_rec(up);
	if (!rec)
		goto error;
	cst = isl_upoly_copy(rec->p[0]);
	isl_upoly_free(up);
	return cst;
error:
	isl_upoly_free(up);
	return NULL;
}

__isl_give struct isl_upoly *isl_upoly_sum(__isl_take struct isl_upoly *up1,
	__isl_take struct isl_upoly *up2)
{
	int i;
	struct isl_upoly_rec *rec1, *rec2;

	if (!up1 || !up2)
		goto error;

	if (isl_upoly_is_nan(up1)) {
		isl_upoly_free(up2);
		return up1;
	}

	if (isl_upoly_is_nan(up2)) {
		isl_upoly_free(up1);
		return up2;
	}

	if (isl_upoly_is_zero(up1)) {
		isl_upoly_free(up1);
		return up2;
	}

	if (isl_upoly_is_zero(up2)) {
		isl_upoly_free(up2);
		return up1;
	}

	if (up1->var < up2->var)
		return isl_upoly_sum(up2, up1);

	if (up2->var < up1->var) {
		struct isl_upoly_rec *rec;
		if (isl_upoly_is_infty(up2) || isl_upoly_is_neginfty(up2)) {
			isl_upoly_free(up1);
			return up2;
		}
		up1 = isl_upoly_cow(up1);
		rec = isl_upoly_as_rec(up1);
		if (!rec)
			goto error;
		rec->p[0] = isl_upoly_sum(rec->p[0], up2);
		if (rec->n == 1)
			up1 = replace_by_constant_term(up1);
		return up1;
	}

	if (isl_upoly_is_cst(up1))
		return isl_upoly_sum_cst(up1, up2);

	rec1 = isl_upoly_as_rec(up1);
	rec2 = isl_upoly_as_rec(up2);
	if (!rec1 || !rec2)
		goto error;

	if (rec1->n < rec2->n)
		return isl_upoly_sum(up2, up1);

	up1 = isl_upoly_cow(up1);
	rec1 = isl_upoly_as_rec(up1);
	if (!rec1)
		goto error;

	for (i = rec2->n - 1; i >= 0; --i) {
		rec1->p[i] = isl_upoly_sum(rec1->p[i],
					    isl_upoly_copy(rec2->p[i]));
		if (!rec1->p[i])
			goto error;
		if (i == rec1->n - 1 && isl_upoly_is_zero(rec1->p[i])) {
			isl_upoly_free(rec1->p[i]);
			rec1->n--;
		}
	}

	if (rec1->n == 0)
		up1 = replace_by_zero(up1);
	else if (rec1->n == 1)
		up1 = replace_by_constant_term(up1);

	isl_upoly_free(up2);

	return up1;
error:
	isl_upoly_free(up1);
	isl_upoly_free(up2);
	return NULL;
}

__isl_give struct isl_upoly *isl_upoly_neg_cst(__isl_take struct isl_upoly *up)
{
	struct isl_upoly_cst *cst;

	if (isl_upoly_is_zero(up))
		return up;

	up = isl_upoly_cow(up);
	if (!up)
		return NULL;

	cst = isl_upoly_as_cst(up);

	isl_int_neg(cst->n, cst->n);

	return up;
}

__isl_give struct isl_upoly *isl_upoly_neg(__isl_take struct isl_upoly *up)
{
	int i;
	struct isl_upoly_rec *rec;

	if (!up)
		return NULL;

	if (isl_upoly_is_cst(up))
		return isl_upoly_neg_cst(up);

	up = isl_upoly_cow(up);
	rec = isl_upoly_as_rec(up);
	if (!rec)
		goto error;

	for (i = 0; i < rec->n; ++i) {
		rec->p[i] = isl_upoly_neg(rec->p[i]);
		if (!rec->p[i])
			goto error;
	}

	return up;
error:
	isl_upoly_free(up);
	return NULL;
}

__isl_give struct isl_upoly *isl_upoly_mul_cst(__isl_take struct isl_upoly *up1,
	__isl_take struct isl_upoly *up2)
{
	struct isl_upoly_cst *cst1;
	struct isl_upoly_cst *cst2;

	up1 = isl_upoly_cow(up1);
	if (!up1 || !up2)
		goto error;

	cst1 = isl_upoly_as_cst(up1);
	cst2 = isl_upoly_as_cst(up2);

	isl_int_mul(cst1->n, cst1->n, cst2->n);
	isl_int_mul(cst1->d, cst1->d, cst2->d);

	isl_upoly_cst_reduce(cst1);

	isl_upoly_free(up2);
	return up1;
error:
	isl_upoly_free(up1);
	isl_upoly_free(up2);
	return NULL;
}

__isl_give struct isl_upoly *isl_upoly_mul_rec(__isl_take struct isl_upoly *up1,
	__isl_take struct isl_upoly *up2)
{
	struct isl_upoly_rec *rec1;
	struct isl_upoly_rec *rec2;
	struct isl_upoly_rec *res;
	int i, j;
	int size;

	rec1 = isl_upoly_as_rec(up1);
	rec2 = isl_upoly_as_rec(up2);
	if (!rec1 || !rec2)
		goto error;
	size = rec1->n + rec2->n - 1;
	res = isl_upoly_alloc_rec(up1->ctx, up1->var, size);
	if (!res)
		goto error;

	for (i = 0; i < rec1->n; ++i) {
		res->p[i] = isl_upoly_mul(isl_upoly_copy(rec2->p[0]),
					    isl_upoly_copy(rec1->p[i]));
		if (!res->p[i])
			goto error;
		res->n++;
	}
	for (; i < size; ++i) {
		res->p[i] = isl_upoly_zero(up1->ctx);
		if (!res->p[i])
			goto error;
		res->n++;
	}
	for (i = 0; i < rec1->n; ++i) {
		for (j = 1; j < rec2->n; ++j) {
			struct isl_upoly *up;
			up = isl_upoly_mul(isl_upoly_copy(rec2->p[j]),
					    isl_upoly_copy(rec1->p[i]));
			res->p[i + j] = isl_upoly_sum(res->p[i + j], up);
			if (!res->p[i + j])
				goto error;
		}
	}

	isl_upoly_free(up1);
	isl_upoly_free(up2);

	return &res->up;
error:
	isl_upoly_free(up1);
	isl_upoly_free(up2);
	isl_upoly_free(&res->up);
	return NULL;
}

__isl_give struct isl_upoly *isl_upoly_mul(__isl_take struct isl_upoly *up1,
	__isl_take struct isl_upoly *up2)
{
	if (!up1 || !up2)
		goto error;

	if (isl_upoly_is_nan(up1)) {
		isl_upoly_free(up2);
		return up1;
	}

	if (isl_upoly_is_nan(up2)) {
		isl_upoly_free(up1);
		return up2;
	}

	if (isl_upoly_is_zero(up1)) {
		isl_upoly_free(up2);
		return up1;
	}

	if (isl_upoly_is_zero(up2)) {
		isl_upoly_free(up1);
		return up2;
	}

	if (isl_upoly_is_one(up1)) {
		isl_upoly_free(up1);
		return up2;
	}

	if (isl_upoly_is_one(up2)) {
		isl_upoly_free(up2);
		return up1;
	}

	if (up1->var < up2->var)
		return isl_upoly_mul(up2, up1);

	if (up2->var < up1->var) {
		int i;
		struct isl_upoly_rec *rec;
		if (isl_upoly_is_infty(up2) || isl_upoly_is_neginfty(up2)) {
			isl_ctx *ctx = up1->ctx;
			isl_upoly_free(up1);
			isl_upoly_free(up2);
			return isl_upoly_nan(ctx);
		}
		up1 = isl_upoly_cow(up1);
		rec = isl_upoly_as_rec(up1);
		if (!rec)
			goto error;

		for (i = 0; i < rec->n; ++i) {
			rec->p[i] = isl_upoly_mul(rec->p[i],
						    isl_upoly_copy(up2));
			if (!rec->p[i])
				goto error;
		}
		isl_upoly_free(up2);
		return up1;
	}

	if (isl_upoly_is_cst(up1))
		return isl_upoly_mul_cst(up1, up2);

	return isl_upoly_mul_rec(up1, up2);
error:
	isl_upoly_free(up1);
	isl_upoly_free(up2);
	return NULL;
}

__isl_give isl_qpolynomial *isl_qpolynomial_alloc(__isl_take isl_dim *dim,
	unsigned n_div, __isl_take struct isl_upoly *up)
{
	struct isl_qpolynomial *qp = NULL;
	unsigned total;

	if (!dim || !up)
		goto error;

	total = isl_dim_total(dim);

	qp = isl_calloc_type(dim->ctx, struct isl_qpolynomial);
	if (!qp)
		goto error;

	qp->ref = 1;
	qp->div = isl_mat_alloc(dim->ctx, n_div, 1 + 1 + total + n_div);
	if (!qp->div)
		goto error;

	qp->dim = dim;
	qp->upoly = up;

	return qp;
error:
	isl_dim_free(dim);
	isl_upoly_free(up);
	isl_qpolynomial_free(qp);
	return NULL;
}

__isl_give isl_qpolynomial *isl_qpolynomial_copy(__isl_keep isl_qpolynomial *qp)
{
	if (!qp)
		return NULL;

	qp->ref++;
	return qp;
}

__isl_give isl_qpolynomial *isl_qpolynomial_dup(__isl_keep isl_qpolynomial *qp)
{
	struct isl_qpolynomial *dup;

	if (!qp)
		return NULL;

	dup = isl_qpolynomial_alloc(isl_dim_copy(qp->dim), qp->div->n_row,
				    isl_upoly_copy(qp->upoly));
	if (!dup)
		return NULL;
	isl_mat_free(dup->div);
	dup->div = isl_mat_copy(qp->div);
	if (!dup->div)
		goto error;

	return dup;
error:
	isl_qpolynomial_free(dup);
	return NULL;
}

__isl_give isl_qpolynomial *isl_qpolynomial_cow(__isl_take isl_qpolynomial *qp)
{
	if (!qp)
		return NULL;

	if (qp->ref == 1)
		return qp;
	qp->ref--;
	return isl_qpolynomial_dup(qp);
}

void isl_qpolynomial_free(__isl_take isl_qpolynomial *qp)
{
	if (!qp)
		return;

	if (--qp->ref > 0)
		return;

	isl_dim_free(qp->dim);
	isl_mat_free(qp->div);
	isl_upoly_free(qp->upoly);

	free(qp);
}

static int compatible_divs(__isl_keep isl_mat *div1, __isl_keep isl_mat *div2)
{
	int n_row, n_col;
	int equal;

	isl_assert(div1->ctx, div1->n_row >= div2->n_row &&
				div1->n_col >= div2->n_col, return -1);

	if (div1->n_row == div2->n_row)
		return isl_mat_is_equal(div1, div2);

	n_row = div1->n_row;
	n_col = div1->n_col;
	div1->n_row = div2->n_row;
	div1->n_col = div2->n_col;

	equal = isl_mat_is_equal(div1, div2);

	div1->n_row = n_row;
	div1->n_col = n_col;

	return equal;
}

static void expand_row(__isl_keep isl_mat *dst, int d,
	__isl_keep isl_mat *src, int s, int *exp)
{
	int i;
	unsigned c = src->n_col - src->n_row;

	isl_seq_cpy(dst->row[d], src->row[s], c);
	isl_seq_clr(dst->row[d] + c, dst->n_col - c);

	for (i = 0; i < s; ++i)
		isl_int_set(dst->row[d][c + exp[i]], src->row[s][c + i]);
}

static int cmp_row(__isl_keep isl_mat *div, int i, int j)
{
	int li, lj;

	li = isl_seq_last_non_zero(div->row[i], div->n_col);
	lj = isl_seq_last_non_zero(div->row[j], div->n_col);

	if (li != lj)
		return li - lj;

	return isl_seq_cmp(div->row[i], div->row[j], div->n_col);
}

struct isl_div_sort_info {
	isl_mat	*div;
	int	 row;
};

static int div_sort_cmp(const void *p1, const void *p2)
{
	const struct isl_div_sort_info *i1, *i2;
	i1 = (const struct isl_div_sort_info *) p1;
	i2 = (const struct isl_div_sort_info *) p2;

	return cmp_row(i1->div, i1->row, i2->row);
}

static __isl_give isl_mat *sort_divs(__isl_take isl_mat *div)
{
	int i;
	struct isl_div_sort_info *array = NULL;
	int *pos = NULL;

	if (!div)
		return NULL;
	if (div->n_row <= 1)
		return div;

	array = isl_alloc_array(div->ctx, struct isl_div_sort_info, div->n_row);
	pos = isl_alloc_array(div->ctx, int, div->n_row);
	if (!array || !pos)
		goto error;

	for (i = 0; i < div->n_row; ++i) {
		array[i].div = div;
		array[i].row = i;
		pos[i] = i;
	}

	qsort(array, div->n_row, sizeof(struct isl_div_sort_info),
		div_sort_cmp);

	for (i = 0; i < div->n_row; ++i) {
		int t;
		if (pos[array[i].row] == i)
			continue;
		div = isl_mat_cow(div);
		div = isl_mat_swap_rows(div, i, pos[array[i].row]);
		t = pos[array[i].row];
		pos[array[i].row] = pos[i];
		pos[i] = t;
	}

	free(array);

	return div;
error:
	free(pos);
	free(array);
	isl_mat_free(div);
	return NULL;
}

static __isl_give isl_mat *merge_divs(__isl_keep isl_mat *div1,
	__isl_keep isl_mat *div2, int *exp1, int *exp2)
{
	int i, j, k;
	isl_mat *div = NULL;
	unsigned d = div1->n_col - div1->n_row;

	div = isl_mat_alloc(div1->ctx, 1 + div1->n_row + div2->n_row,
				d + div1->n_row + div2->n_row);
	if (!div)
		return NULL;

	for (i = 0, j = 0, k = 0; i < div1->n_row && j < div2->n_row; ++k) {
		int cmp;

		expand_row(div, k, div1, i, exp1);
		expand_row(div, k + 1, div2, j, exp2);

		cmp = cmp_row(div, k, k + 1);
		if (cmp == 0) {
			exp1[i++] = k;
			exp2[j++] = k;
		} else if (cmp < 0) {
			exp1[i++] = k;
		} else {
			exp2[j++] = k;
			isl_seq_cpy(div->row[k], div->row[k + 1], div->n_col);
		}
	}
	for (; i < div1->n_row; ++i, ++k) {
		expand_row(div, k, div1, i, exp1);
		exp1[i] = k;
	}
	for (; j < div2->n_row; ++j, ++k) {
		expand_row(div, k, div2, j, exp2);
		exp2[j] = k;
	}

	div->n_row = k;
	div->n_col = d + k;

	return div;
}

static __isl_give struct isl_upoly *expand(__isl_take struct isl_upoly *up,
	int *exp, int first)
{
	int i;
	struct isl_upoly_rec *rec;

	if (isl_upoly_is_cst(up))
		return up;

	if (up->var < first)
		return up;

	if (exp[up->var - first] == up->var - first)
		return up;

	up = isl_upoly_cow(up);
	if (!up)
		goto error;

	up->var = exp[up->var - first] + first;

	rec = isl_upoly_as_rec(up);
	if (!rec)
		goto error;

	for (i = 0; i < rec->n; ++i) {
		rec->p[i] = expand(rec->p[i], exp, first);
		if (!rec->p[i])
			goto error;
	}

	return up;
error:
	isl_upoly_free(up);
	return NULL;
}

static __isl_give isl_qpolynomial *with_merged_divs(
	__isl_give isl_qpolynomial *(*fn)(__isl_take isl_qpolynomial *qp1,
					  __isl_take isl_qpolynomial *qp2),
	__isl_take isl_qpolynomial *qp1, __isl_take isl_qpolynomial *qp2)
{
	int *exp1 = NULL;
	int *exp2 = NULL;
	isl_mat *div = NULL;

	qp1 = isl_qpolynomial_cow(qp1);
	qp2 = isl_qpolynomial_cow(qp2);

	if (!qp1 || !qp2)
		goto error;

	isl_assert(qp1->div->ctx, qp1->div->n_row >= qp2->div->n_row &&
				qp1->div->n_col >= qp2->div->n_col, goto error);

	exp1 = isl_alloc_array(qp1->div->ctx, int, qp1->div->n_row);
	exp2 = isl_alloc_array(qp2->div->ctx, int, qp2->div->n_row);
	if (!exp1 || !exp2)
		goto error;

	div = merge_divs(qp1->div, qp2->div, exp1, exp2);
	if (!div)
		goto error;

	isl_mat_free(qp1->div);
	qp1->div = isl_mat_copy(div);
	isl_mat_free(qp2->div);
	qp2->div = isl_mat_copy(div);

	qp1->upoly = expand(qp1->upoly, exp1, div->n_col - div->n_row - 2);
	qp2->upoly = expand(qp2->upoly, exp2, div->n_col - div->n_row - 2);

	if (!qp1->upoly || !qp2->upoly)
		goto error;

	isl_mat_free(div);
	free(exp1);
	free(exp2);

	return fn(qp1, qp2);
error:
	isl_mat_free(div);
	free(exp1);
	free(exp2);
	isl_qpolynomial_free(qp1);
	isl_qpolynomial_free(qp2);
	return NULL;
}

__isl_give isl_qpolynomial *isl_qpolynomial_add(__isl_take isl_qpolynomial *qp1,
	__isl_take isl_qpolynomial *qp2)
{
	qp1 = isl_qpolynomial_cow(qp1);

	if (!qp1 || !qp2)
		goto error;

	if (qp1->div->n_row < qp2->div->n_row)
		return isl_qpolynomial_add(qp2, qp1);

	isl_assert(qp1->dim->ctx, isl_dim_equal(qp1->dim, qp2->dim), goto error);
	if (!compatible_divs(qp1->div, qp2->div))
		return with_merged_divs(isl_qpolynomial_add, qp1, qp2);

	qp1->upoly = isl_upoly_sum(qp1->upoly, isl_upoly_copy(qp2->upoly));
	if (!qp1->upoly)
		goto error;

	isl_qpolynomial_free(qp2);

	return qp1;
error:
	isl_qpolynomial_free(qp1);
	isl_qpolynomial_free(qp2);
	return NULL;
}

__isl_give isl_qpolynomial *isl_qpolynomial_sub(__isl_take isl_qpolynomial *qp1,
	__isl_take isl_qpolynomial *qp2)
{
	return isl_qpolynomial_add(qp1, isl_qpolynomial_neg(qp2));
}

__isl_give isl_qpolynomial *isl_qpolynomial_neg(__isl_take isl_qpolynomial *qp)
{
	qp = isl_qpolynomial_cow(qp);

	if (!qp)
		return NULL;

	qp->upoly = isl_upoly_neg(qp->upoly);
	if (!qp->upoly)
		goto error;

	return qp;
error:
	isl_qpolynomial_free(qp);
	return NULL;
}

__isl_give isl_qpolynomial *isl_qpolynomial_mul(__isl_take isl_qpolynomial *qp1,
	__isl_take isl_qpolynomial *qp2)
{
	qp1 = isl_qpolynomial_cow(qp1);

	if (!qp1 || !qp2)
		goto error;

	if (qp1->div->n_row < qp2->div->n_row)
		return isl_qpolynomial_mul(qp2, qp1);

	isl_assert(qp1->dim->ctx, isl_dim_equal(qp1->dim, qp2->dim), goto error);
	if (!compatible_divs(qp1->div, qp2->div))
		return with_merged_divs(isl_qpolynomial_mul, qp1, qp2);

	qp1->upoly = isl_upoly_mul(qp1->upoly, isl_upoly_copy(qp2->upoly));
	if (!qp1->upoly)
		goto error;

	isl_qpolynomial_free(qp2);

	return qp1;
error:
	isl_qpolynomial_free(qp1);
	isl_qpolynomial_free(qp2);
	return NULL;
}

__isl_give isl_qpolynomial *isl_qpolynomial_zero(__isl_take isl_dim *dim)
{
	return isl_qpolynomial_alloc(dim, 0, isl_upoly_zero(dim->ctx));
}

__isl_give isl_qpolynomial *isl_qpolynomial_infty(__isl_take isl_dim *dim)
{
	return isl_qpolynomial_alloc(dim, 0, isl_upoly_infty(dim->ctx));
}

__isl_give isl_qpolynomial *isl_qpolynomial_neginfty(__isl_take isl_dim *dim)
{
	return isl_qpolynomial_alloc(dim, 0, isl_upoly_neginfty(dim->ctx));
}

__isl_give isl_qpolynomial *isl_qpolynomial_nan(__isl_take isl_dim *dim)
{
	return isl_qpolynomial_alloc(dim, 0, isl_upoly_nan(dim->ctx));
}

__isl_give isl_qpolynomial *isl_qpolynomial_cst(__isl_take isl_dim *dim,
	isl_int v)
{
	struct isl_qpolynomial *qp;
	struct isl_upoly_cst *cst;

	qp = isl_qpolynomial_alloc(dim, 0, isl_upoly_zero(dim->ctx));
	if (!qp)
		return NULL;

	cst = isl_upoly_as_cst(qp->upoly);
	isl_int_set(cst->n, v);

	return qp;
}

int isl_qpolynomial_is_cst(__isl_keep isl_qpolynomial *qp,
	isl_int *n, isl_int *d)
{
	struct isl_upoly_cst *cst;

	if (!qp)
		return -1;

	if (!isl_upoly_is_cst(qp->upoly))
		return 0;

	cst = isl_upoly_as_cst(qp->upoly);
	if (!cst)
		return -1;

	if (n)
		isl_int_set(*n, cst->n);
	if (d)
		isl_int_set(*d, cst->d);

	return 1;
}

int isl_upoly_is_affine(__isl_keep struct isl_upoly *up)
{
	int is_cst;
	struct isl_upoly_rec *rec;

	if (!up)
		return -1;

	if (up->var < 0)
		return 1;

	rec = isl_upoly_as_rec(up);
	if (!rec)
		return -1;

	if (rec->n > 2)
		return 0;

	isl_assert(up->ctx, rec->n > 1, return -1);

	is_cst = isl_upoly_is_cst(rec->p[1]);
	if (is_cst < 0)
		return -1;
	if (!is_cst)
		return 0;

	return isl_upoly_is_affine(rec->p[0]);
}

int isl_qpolynomial_is_affine(__isl_keep isl_qpolynomial *qp)
{
	if (!qp)
		return -1;

	if (qp->div->n_row > 0)
		return 0;

	return isl_upoly_is_affine(qp->upoly);
}

static void update_coeff(__isl_keep isl_vec *aff,
	__isl_keep struct isl_upoly_cst *cst, int pos)
{
	isl_int gcd;
	isl_int f;

	if (isl_int_is_zero(cst->n))
		return;

	isl_int_init(gcd);
	isl_int_init(f);
	isl_int_gcd(gcd, cst->d, aff->el[0]);
	isl_int_divexact(f, cst->d, gcd);
	isl_int_divexact(gcd, aff->el[0], gcd);
	isl_seq_scale(aff->el, aff->el, f, aff->size);
	isl_int_mul(aff->el[1 + pos], gcd, cst->n);
	isl_int_clear(gcd);
	isl_int_clear(f);
}

int isl_upoly_update_affine(__isl_keep struct isl_upoly *up,
	__isl_keep isl_vec *aff)
{
	struct isl_upoly_cst *cst;
	struct isl_upoly_rec *rec;

	if (!up || !aff)
		return -1;

	if (up->var < 0) {
		struct isl_upoly_cst *cst;

		cst = isl_upoly_as_cst(up);
		if (!cst)
			return -1;
		update_coeff(aff, cst, 0);
		return 0;
	}

	rec = isl_upoly_as_rec(up);
	if (!rec)
		return -1;
	isl_assert(up->ctx, rec->n == 2, return -1);

	cst = isl_upoly_as_cst(rec->p[1]);
	if (!cst)
		return -1;
	update_coeff(aff, cst, 1 + up->var);

	return isl_upoly_update_affine(rec->p[0], aff);
}

__isl_give isl_vec *isl_qpolynomial_extract_affine(
	__isl_keep isl_qpolynomial *qp)
{
	isl_vec *aff;
	unsigned d;

	if (!qp)
		return NULL;

	isl_assert(qp->div->ctx, qp->div->n_row == 0, return NULL);
	d = isl_dim_total(qp->dim);
	aff = isl_vec_alloc(qp->div->ctx, 2 + d);
	if (!aff)
		return NULL;

	isl_seq_clr(aff->el + 1, 1 + d);
	isl_int_set_si(aff->el[0], 1);

	if (isl_upoly_update_affine(qp->upoly, aff) < 0)
		goto error;

	return aff;
error:
	isl_vec_free(aff);
	return NULL;
}

int isl_qpolynomial_is_equal(__isl_keep isl_qpolynomial *qp1,
	__isl_keep isl_qpolynomial *qp2)
{
	if (!qp1 || !qp2)
		return -1;

	return isl_upoly_is_equal(qp1->upoly, qp2->upoly);
}

static void upoly_update_den(__isl_keep struct isl_upoly *up, isl_int *d)
{
	int i;
	struct isl_upoly_rec *rec;

	if (isl_upoly_is_cst(up)) {
		struct isl_upoly_cst *cst;
		cst = isl_upoly_as_cst(up);
		if (!cst)
			return;
		isl_int_lcm(*d, *d, cst->d);
		return;
	}

	rec = isl_upoly_as_rec(up);
	if (!rec)
		return;

	for (i = 0; i < rec->n; ++i)
		upoly_update_den(rec->p[i], d);
}

void isl_qpolynomial_get_den(__isl_keep isl_qpolynomial *qp, isl_int *d)
{
	isl_int_set_si(*d, 1);
	if (!qp)
		return;
	upoly_update_den(qp->upoly, d);
}

__isl_give struct isl_upoly *isl_upoly_pow(isl_ctx *ctx, int pos, int power)
{
	int i;
	struct isl_upoly *up;
	struct isl_upoly_rec *rec;
	struct isl_upoly_cst *cst;

	rec = isl_upoly_alloc_rec(ctx, pos, 1 + power);
	if (!rec)
		return NULL;
	for (i = 0; i < 1 + power; ++i) {
		rec->p[i] = isl_upoly_zero(ctx);
		if (!rec->p[i])
			goto error;
		rec->n++;
	}
	cst = isl_upoly_as_cst(rec->p[power]);
	isl_int_set_si(cst->n, 1);

	return &rec->up;
error:
	isl_upoly_free(&rec->up);
	return NULL;
}

__isl_give isl_qpolynomial *isl_qpolynomial_pow(__isl_take isl_dim *dim,
	int pos, int power)
{
	struct isl_ctx *ctx;

	if (!dim)
		return NULL;

	ctx = dim->ctx;

	return isl_qpolynomial_alloc(dim, 0, isl_upoly_pow(ctx, pos, power));
}

__isl_give isl_qpolynomial *isl_qpolynomial_var(__isl_take isl_dim *dim,
	enum isl_dim_type type, unsigned pos)
{
	if (!dim)
		return NULL;

	isl_assert(dim->ctx, isl_dim_size(dim, isl_dim_in) == 0, goto error);
	isl_assert(dim->ctx, pos < isl_dim_size(dim, type), goto error);

	if (type == isl_dim_set)
		pos += isl_dim_size(dim, isl_dim_param);

	return isl_qpolynomial_pow(dim, pos, 1);
error:
	isl_dim_free(dim);
	return NULL;
}

__isl_give isl_qpolynomial *isl_qpolynomial_div_pow(__isl_take isl_div *div,
	int power)
{
	struct isl_qpolynomial *qp = NULL;
	struct isl_upoly_rec *rec;
	struct isl_upoly_cst *cst;
	int i;
	int pos;

	if (!div)
		return NULL;
	isl_assert(div->ctx, div->bmap->n_div == 1, goto error);

	pos = isl_dim_total(div->bmap->dim);
	rec = isl_upoly_alloc_rec(div->ctx, pos, 1 + power);
	qp = isl_qpolynomial_alloc(isl_basic_map_get_dim(div->bmap), 1,
				   &rec->up);
	if (!qp)
		goto error;

	isl_seq_cpy(qp->div->row[0], div->line[0], qp->div->n_col - 1);
	isl_int_set_si(qp->div->row[0][qp->div->n_col - 1], 0);

	for (i = 0; i < 1 + power; ++i) {
		rec->p[i] = isl_upoly_zero(div->ctx);
		if (!rec->p[i])
			goto error;
		rec->n++;
	}
	cst = isl_upoly_as_cst(rec->p[power]);
	isl_int_set_si(cst->n, 1);

	isl_div_free(div);

	return qp;
error:
	isl_qpolynomial_free(qp);
	isl_div_free(div);
	return NULL;
}

__isl_give isl_qpolynomial *isl_qpolynomial_div(__isl_take isl_div *div)
{
	return isl_qpolynomial_div_pow(div, 1);
}

__isl_give isl_qpolynomial *isl_qpolynomial_rat_cst(__isl_take isl_dim *dim,
	const isl_int n, const isl_int d)
{
	struct isl_qpolynomial *qp;
	struct isl_upoly_cst *cst;

	qp = isl_qpolynomial_alloc(dim, 0, isl_upoly_zero(dim->ctx));
	if (!qp)
		return NULL;

	cst = isl_upoly_as_cst(qp->upoly);
	isl_int_set(cst->n, n);
	isl_int_set(cst->d, d);

	return qp;
}

static int up_set_active(__isl_keep struct isl_upoly *up, int *active, int d)
{
	struct isl_upoly_rec *rec;
	int i;

	if (!up)
		return -1;

	if (isl_upoly_is_cst(up))
		return 0;

	if (up->var < d)
		active[up->var] = 1;

	rec = isl_upoly_as_rec(up);
	for (i = 0; i < rec->n; ++i)
		if (up_set_active(rec->p[i], active, d) < 0)
			return -1;

	return 0;
}

static int set_active(__isl_keep isl_qpolynomial *qp, int *active)
{
	int i, j;
	int d = isl_dim_total(qp->dim);

	if (!qp || !active)
		return -1;

	for (i = 0; i < d; ++i)
		for (j = 0; j < qp->div->n_row; ++j) {
			if (isl_int_is_zero(qp->div->row[j][2 + i]))
				continue;
			active[i] = 1;
			break;
		}

	return up_set_active(qp->upoly, active, d);
}

int isl_qpolynomial_involves_dims(__isl_keep isl_qpolynomial *qp,
	enum isl_dim_type type, unsigned first, unsigned n)
{
	int i;
	int *active = NULL;
	int involves = 0;

	if (!qp)
		return -1;
	if (n == 0)
		return 0;

	isl_assert(qp->dim->ctx, first + n <= isl_dim_size(qp->dim, type),
			return -1);
	isl_assert(qp->dim->ctx, type == isl_dim_param ||
				 type == isl_dim_set, return -1);

	active = isl_calloc_array(set->ctx, int, isl_dim_total(qp->dim));
	if (set_active(qp, active) < 0)
		goto error;

	if (type == isl_dim_set)
		first += isl_dim_size(qp->dim, isl_dim_param);
	for (i = 0; i < n; ++i)
		if (active[first + i]) {
			involves = 1;
			break;
		}

	free(active);

	return involves;
error:
	free(active);
	return -1;
}

__isl_give struct isl_upoly *isl_upoly_drop(__isl_take struct isl_upoly *up,
	unsigned first, unsigned n)
{
	int i;
	struct isl_upoly_rec *rec;

	if (!up)
		return NULL;
	if (n == 0 || up->var < 0 || up->var < first)
		return up;
	if (up->var < first + n) {
		up = replace_by_constant_term(up);
		return isl_upoly_drop(up, first, n);
	}
	up = isl_upoly_cow(up);
	if (!up)
		return NULL;
	up->var -= n;
	rec = isl_upoly_as_rec(up);
	if (!rec)
		goto error;

	for (i = 0; i < rec->n; ++i) {
		rec->p[i] = isl_upoly_drop(rec->p[i], first, n);
		if (!rec->p[i])
			goto error;
	}

	return up;
error:
	isl_upoly_free(up);
	return NULL;
}

__isl_give isl_qpolynomial *isl_qpolynomial_drop_dims(
	__isl_take isl_qpolynomial *qp,
	enum isl_dim_type type, unsigned first, unsigned n)
{
	if (!qp)
		return NULL;
	if (n == 0)
		return qp;

	qp = isl_qpolynomial_cow(qp);
	if (!qp)
		return NULL;

	isl_assert(qp->dim->ctx, first + n <= isl_dim_size(qp->dim, type),
			goto error);
	isl_assert(qp->dim->ctx, type == isl_dim_param ||
				 type == isl_dim_set, goto error);

	qp->dim = isl_dim_drop(qp->dim, type, first, n);
	if (!qp->dim)
		goto error;

	if (type == isl_dim_set)
		first += isl_dim_size(qp->dim, isl_dim_param);

	qp->div = isl_mat_drop_cols(qp->div, 2 + first, n);
	if (!qp->div)
		goto error;

	qp->upoly = isl_upoly_drop(qp->upoly, first, n);
	if (!qp->upoly)
		goto error;

	return qp;
error:
	isl_qpolynomial_free(qp);
	return NULL;
}

#undef PW
#define PW isl_pw_qpolynomial
#undef EL
#define EL isl_qpolynomial
#undef IS_ZERO
#define IS_ZERO is_zero
#undef FIELD
#define FIELD qp
#undef ADD
#define ADD(d,e1,e2)	isl_qpolynomial_add(e1,e2) 

#include <isl_pw_templ.c>

int isl_pw_qpolynomial_is_one(__isl_keep isl_pw_qpolynomial *pwqp)
{
	if (!pwqp)
		return -1;

	if (pwqp->n != -1)
		return 0;

	if (!isl_set_fast_is_universe(pwqp->p[0].set))
		return 0;

	return isl_qpolynomial_is_one(pwqp->p[0].qp);
}

__isl_give isl_pw_qpolynomial *isl_pw_qpolynomial_mul(
	__isl_take isl_pw_qpolynomial *pwqp1,
	__isl_take isl_pw_qpolynomial *pwqp2)
{
	int i, j, n;
	struct isl_pw_qpolynomial *res;
	isl_set *set;

	if (!pwqp1 || !pwqp2)
		goto error;

	isl_assert(pwqp1->dim->ctx, isl_dim_equal(pwqp1->dim, pwqp2->dim),
			goto error);

	if (isl_pw_qpolynomial_is_zero(pwqp1)) {
		isl_pw_qpolynomial_free(pwqp2);
		return pwqp1;
	}

	if (isl_pw_qpolynomial_is_zero(pwqp2)) {
		isl_pw_qpolynomial_free(pwqp1);
		return pwqp2;
	}

	if (isl_pw_qpolynomial_is_one(pwqp1)) {
		isl_pw_qpolynomial_free(pwqp1);
		return pwqp2;
	}

	if (isl_pw_qpolynomial_is_one(pwqp2)) {
		isl_pw_qpolynomial_free(pwqp2);
		return pwqp1;
	}

	n = pwqp1->n * pwqp2->n;
	res = isl_pw_qpolynomial_alloc_(isl_dim_copy(pwqp1->dim), n);

	for (i = 0; i < pwqp1->n; ++i) {
		for (j = 0; j < pwqp2->n; ++j) {
			struct isl_set *common;
			struct isl_qpolynomial *prod;
			common = isl_set_intersect(isl_set_copy(pwqp1->p[i].set),
						isl_set_copy(pwqp2->p[j].set));
			if (isl_set_fast_is_empty(common)) {
				isl_set_free(common);
				continue;
			}

			prod = isl_qpolynomial_mul(
				isl_qpolynomial_copy(pwqp1->p[i].qp),
				isl_qpolynomial_copy(pwqp2->p[j].qp));

			res = isl_pw_qpolynomial_add_piece(res, common, prod);
		}
	}

	isl_pw_qpolynomial_free(pwqp1);
	isl_pw_qpolynomial_free(pwqp2);

	return res;
error:
	isl_pw_qpolynomial_free(pwqp1);
	isl_pw_qpolynomial_free(pwqp2);
	return NULL;
}

__isl_give isl_pw_qpolynomial *isl_pw_qpolynomial_neg(
	__isl_take isl_pw_qpolynomial *pwqp)
{
	int i, j, n;
	struct isl_pw_qpolynomial *res;
	isl_set *set;

	if (!pwqp)
		return NULL;

	if (isl_pw_qpolynomial_is_zero(pwqp))
		return pwqp;

	pwqp = isl_pw_qpolynomial_cow(pwqp);

	for (i = 0; i < pwqp->n; ++i) {
		pwqp->p[i].qp = isl_qpolynomial_neg(pwqp->p[i].qp);
		if (!pwqp->p[i].qp)
			goto error;
	}

	return pwqp;
error:
	isl_pw_qpolynomial_free(pwqp);
	return NULL;
}

__isl_give isl_pw_qpolynomial *isl_pw_qpolynomial_sub(
	__isl_take isl_pw_qpolynomial *pwqp1,
	__isl_take isl_pw_qpolynomial *pwqp2)
{
	return isl_pw_qpolynomial_add(pwqp1, isl_pw_qpolynomial_neg(pwqp2));
}

__isl_give struct isl_upoly *isl_upoly_eval(
	__isl_take struct isl_upoly *up, __isl_take isl_vec *vec)
{
	int i;
	struct isl_upoly_rec *rec;
	struct isl_upoly *res;
	struct isl_upoly *base;

	if (isl_upoly_is_cst(up)) {
		isl_vec_free(vec);
		return up;
	}

	rec = isl_upoly_as_rec(up);
	if (!rec)
		goto error;

	isl_assert(up->ctx, rec->n >= 1, goto error);

	base = isl_upoly_rat_cst(up->ctx, vec->el[1 + up->var], vec->el[0]);

	res = isl_upoly_eval(isl_upoly_copy(rec->p[rec->n - 1]),
				isl_vec_copy(vec));

	for (i = rec->n - 2; i >= 0; --i) {
		res = isl_upoly_mul(res, isl_upoly_copy(base));
		res = isl_upoly_sum(res, 
			    isl_upoly_eval(isl_upoly_copy(rec->p[i]),
							    isl_vec_copy(vec)));
	}

	isl_upoly_free(base);
	isl_upoly_free(up);
	isl_vec_free(vec);
	return res;
error:
	isl_upoly_free(up);
	isl_vec_free(vec);
	return NULL;
}

__isl_give isl_qpolynomial *isl_qpolynomial_eval(
	__isl_take isl_qpolynomial *qp, __isl_take isl_point *pnt)
{
	isl_vec *ext;
	struct isl_upoly *up;
	isl_dim *dim;

	if (!qp || !pnt)
		goto error;
	isl_assert(pnt->dim->ctx, isl_dim_equal(pnt->dim, qp->dim), goto error);

	if (qp->div->n_row == 0)
		ext = isl_vec_copy(pnt->vec);
	else {
		int i;
		unsigned dim = isl_dim_total(qp->dim);
		ext = isl_vec_alloc(qp->dim->ctx, 1 + dim + qp->div->n_row);
		if (!ext)
			goto error;

		isl_seq_cpy(ext->el, pnt->vec->el, pnt->vec->size);
		for (i = 0; i < qp->div->n_row; ++i) {
			isl_seq_inner_product(qp->div->row[i] + 1, ext->el,
						1 + dim + i, &ext->el[1+dim+i]);
			isl_int_fdiv_q(ext->el[1+dim+i], ext->el[1+dim+i],
					qp->div->row[i][0]);
		}
	}

	up = isl_upoly_eval(isl_upoly_copy(qp->upoly), ext);
	if (!up)
		goto error;

	dim = isl_dim_copy(qp->dim);
	isl_qpolynomial_free(qp);
	isl_point_free(pnt);

	return isl_qpolynomial_alloc(dim, 0, up);
error:
	isl_qpolynomial_free(qp);
	isl_point_free(pnt);
	return NULL;
}

int isl_upoly_cmp(__isl_keep struct isl_upoly_cst *cst1,
	__isl_keep struct isl_upoly_cst *cst2)
{
	int cmp;
	isl_int t;
	isl_int_init(t);
	isl_int_mul(t, cst1->n, cst2->d);
	isl_int_submul(t, cst2->n, cst1->d);
	cmp = isl_int_sgn(t);
	isl_int_clear(t);
	return cmp;
}

int isl_qpolynomial_le_cst(__isl_keep isl_qpolynomial *qp1,
	__isl_keep isl_qpolynomial *qp2)
{
	struct isl_upoly_cst *cst1, *cst2;

	if (!qp1 || !qp2)
		return -1;
	isl_assert(qp1->dim->ctx, isl_upoly_is_cst(qp1->upoly), return -1);
	isl_assert(qp2->dim->ctx, isl_upoly_is_cst(qp2->upoly), return -1);
	if (isl_qpolynomial_is_nan(qp1))
		return -1;
	if (isl_qpolynomial_is_nan(qp2))
		return -1;
	cst1 = isl_upoly_as_cst(qp1->upoly);
	cst2 = isl_upoly_as_cst(qp2->upoly);

	return isl_upoly_cmp(cst1, cst2) <= 0;
}

__isl_give isl_qpolynomial *isl_qpolynomial_min_cst(
	__isl_take isl_qpolynomial *qp1, __isl_take isl_qpolynomial *qp2)
{
	struct isl_upoly_cst *cst1, *cst2;
	int cmp;

	if (!qp1 || !qp2)
		goto error;
	isl_assert(qp1->dim->ctx, isl_upoly_is_cst(qp1->upoly), goto error);
	isl_assert(qp2->dim->ctx, isl_upoly_is_cst(qp2->upoly), goto error);
	cst1 = isl_upoly_as_cst(qp1->upoly);
	cst2 = isl_upoly_as_cst(qp2->upoly);
	cmp = isl_upoly_cmp(cst1, cst2);

	if (cmp <= 0) {
		isl_qpolynomial_free(qp2);
	} else {
		isl_qpolynomial_free(qp1);
		qp1 = qp2;
	}
	return qp1;
error:
	isl_qpolynomial_free(qp1);
	isl_qpolynomial_free(qp2);
	return NULL;
}

__isl_give isl_qpolynomial *isl_qpolynomial_max_cst(
	__isl_take isl_qpolynomial *qp1, __isl_take isl_qpolynomial *qp2)
{
	struct isl_upoly_cst *cst1, *cst2;
	int cmp;

	if (!qp1 || !qp2)
		goto error;
	isl_assert(qp1->dim->ctx, isl_upoly_is_cst(qp1->upoly), goto error);
	isl_assert(qp2->dim->ctx, isl_upoly_is_cst(qp2->upoly), goto error);
	cst1 = isl_upoly_as_cst(qp1->upoly);
	cst2 = isl_upoly_as_cst(qp2->upoly);
	cmp = isl_upoly_cmp(cst1, cst2);

	if (cmp >= 0) {
		isl_qpolynomial_free(qp2);
	} else {
		isl_qpolynomial_free(qp1);
		qp1 = qp2;
	}
	return qp1;
error:
	isl_qpolynomial_free(qp1);
	isl_qpolynomial_free(qp2);
	return NULL;
}

__isl_give isl_qpolynomial *isl_qpolynomial_insert_dims(
	__isl_take isl_qpolynomial *qp, enum isl_dim_type type,
	unsigned first, unsigned n)
{
	unsigned total;
	unsigned g_pos;
	int *exp;

	if (n == 0)
		return qp;

	qp = isl_qpolynomial_cow(qp);
	if (!qp)
		return NULL;

	isl_assert(qp->div->ctx, first <= isl_dim_size(qp->dim, type),
		    goto error);

	g_pos = pos(qp->dim, type) + first;

	qp->div = isl_mat_insert_cols(qp->div, 2 + g_pos, n);
	if (!qp->div)
		goto error;

	total = qp->div->n_col - 2;
	if (total > g_pos) {
		int i;
		exp = isl_alloc_array(qp->div->ctx, int, total - g_pos);
		if (!exp)
			goto error;
		for (i = 0; i < total - g_pos; ++i)
			exp[i] = i + n;
		qp->upoly = expand(qp->upoly, exp, g_pos);
		free(exp);
		if (!qp->upoly)
			goto error;
	}

	qp->dim = isl_dim_insert(qp->dim, type, first, n);
	if (!qp->dim)
		goto error;

	return qp;
error:
	isl_qpolynomial_free(qp);
	return NULL;
}

__isl_give isl_qpolynomial *isl_qpolynomial_add_dims(
	__isl_take isl_qpolynomial *qp, enum isl_dim_type type, unsigned n)
{
	unsigned pos;

	pos = isl_qpolynomial_dim(qp, type);

	return isl_qpolynomial_insert_dims(qp, type, pos, n);
}

__isl_give isl_pw_qpolynomial *isl_pw_qpolynomial_add_dims(
	__isl_take isl_pw_qpolynomial *pwqp,
	enum isl_dim_type type, unsigned n)
{
	int i;

	if (n == 0)
		return pwqp;

	pwqp = isl_pw_qpolynomial_cow(pwqp);
	if (!pwqp)
		return NULL;

	pwqp->dim = isl_dim_add(pwqp->dim, type, n);
	if (!pwqp->dim)
		goto error;

	for (i = 0; i < pwqp->n; ++i) {
		pwqp->p[i].set = isl_set_add(pwqp->p[i].set, type, n);
		if (!pwqp->p[i].set)
			goto error;
		pwqp->p[i].qp = isl_qpolynomial_add_dims(pwqp->p[i].qp, type, n);
		if (!pwqp->p[i].qp)
			goto error;
	}

	return pwqp;
error:
	isl_pw_qpolynomial_free(pwqp);
	return NULL;
}

static int *reordering_move(isl_ctx *ctx,
	unsigned len, unsigned dst, unsigned src, unsigned n)
{
	int i;
	int *reordering;

	reordering = isl_alloc_array(ctx, int, len);
	if (!reordering)
		return NULL;

	if (dst <= src) {
		for (i = 0; i < dst; ++i)
			reordering[i] = i;
		for (i = 0; i < n; ++i)
			reordering[src + i] = dst + i;
		for (i = 0; i < src - dst; ++i)
			reordering[dst + i] = dst + n + i;
		for (i = 0; i < len - src - n; ++i)
			reordering[src + n + i] = src + n + i;
	} else {
		for (i = 0; i < src; ++i)
			reordering[i] = i;
		for (i = 0; i < n; ++i)
			reordering[src + i] = dst + i;
		for (i = 0; i < dst - src; ++i)
			reordering[src + n + i] = src + i;
		for (i = 0; i < len - dst - n; ++i)
			reordering[dst + n + i] = dst + n + i;
	}

	return reordering;
}

static __isl_give struct isl_upoly *reorder(__isl_take struct isl_upoly *up,
	int *r)
{
	int i;
	struct isl_upoly_rec *rec;
	struct isl_upoly *base;
	struct isl_upoly *res;

	if (isl_upoly_is_cst(up))
		return up;

	rec = isl_upoly_as_rec(up);
	if (!rec)
		goto error;

	isl_assert(up->ctx, rec->n >= 1, goto error);

	base = isl_upoly_pow(up->ctx, r[up->var], 1);
	res = reorder(isl_upoly_copy(rec->p[rec->n - 1]), r);

	for (i = rec->n - 2; i >= 0; --i) {
		res = isl_upoly_mul(res, isl_upoly_copy(base));
		res = isl_upoly_sum(res, reorder(isl_upoly_copy(rec->p[i]), r));
	}

	isl_upoly_free(base);
	isl_upoly_free(up);

	return res;
error:
	isl_upoly_free(up);
	return NULL;
}

__isl_give isl_qpolynomial *isl_qpolynomial_move_dims(
	__isl_take isl_qpolynomial *qp,
	enum isl_dim_type dst_type, unsigned dst_pos,
	enum isl_dim_type src_type, unsigned src_pos, unsigned n)
{
	unsigned g_dst_pos;
	unsigned g_src_pos;
	int *reordering;

	qp = isl_qpolynomial_cow(qp);
	if (!qp)
		return NULL;

	isl_assert(qp->dim->ctx, src_pos + n <= isl_dim_size(qp->dim, src_type),
		goto error);

	g_dst_pos = pos(qp->dim, dst_type) + dst_pos;
	g_src_pos = pos(qp->dim, src_type) + src_pos;
	if (dst_type > src_type)
		g_dst_pos -= n;

	qp->div = isl_mat_move_cols(qp->div, 2 + g_dst_pos, 2 + g_src_pos, n);
	qp->div = sort_divs(qp->div);
	if (!qp->div)
		goto error;

	reordering = reordering_move(qp->dim->ctx,
				qp->div->n_col - 2, g_dst_pos, g_src_pos, n);
	if (!reordering)
		goto error;

	qp->upoly = reorder(qp->upoly, reordering);
	free(reordering);
	if (!qp->upoly)
		goto error;

	qp->dim = isl_dim_move(qp->dim, dst_type, dst_pos, src_type, src_pos, n);
	if (!qp->dim)
		goto error;

	return qp;
error:
	isl_qpolynomial_free(qp);
	return NULL;
}

__isl_give struct isl_upoly *isl_upoly_from_affine(isl_ctx *ctx, isl_int *f,
	isl_int denom, unsigned len)
{
	int i;
	struct isl_upoly *up;

	isl_assert(ctx, len >= 1, return NULL);

	up = isl_upoly_rat_cst(ctx, f[0], denom);
	for (i = 0; i < len - 1; ++i) {
		struct isl_upoly *t;
		struct isl_upoly *c;

		if (isl_int_is_zero(f[1 + i]))
			continue;

		c = isl_upoly_rat_cst(ctx, f[1 + i], denom);
		t = isl_upoly_pow(ctx, i, 1);
		t = isl_upoly_mul(c, t);
		up = isl_upoly_sum(up, t);
	}

	return up;
}

__isl_give isl_qpolynomial *isl_qpolynomial_from_affine(__isl_take isl_dim *dim,
	isl_int *f, isl_int denom)
{
	struct isl_upoly *up;

	if (!dim)
		return NULL;

	up = isl_upoly_from_affine(dim->ctx, f, denom, 1 + isl_dim_total(dim));

	return isl_qpolynomial_alloc(dim, 0, up);
}

__isl_give isl_qpolynomial *isl_qpolynomial_from_constraint(
	__isl_take isl_constraint *c, enum isl_dim_type type, unsigned pos)
{
	isl_int denom;
	isl_dim *dim;
	struct isl_upoly *up;
	isl_qpolynomial *qp;
	int sgn;

	if (!c)
		return NULL;

	isl_int_init(denom);

	isl_constraint_get_coefficient(c, type, pos, &denom);
	isl_constraint_set_coefficient(c, type, pos, c->ctx->zero);
	sgn = isl_int_sgn(denom);
	isl_int_abs(denom, denom);
	up = isl_upoly_from_affine(c->ctx, c->line[0], denom,
					1 + isl_constraint_dim(c, isl_dim_all));
	if (sgn < 0)
		isl_int_neg(denom, denom);
	isl_constraint_set_coefficient(c, type, pos, denom);

	dim = isl_dim_copy(c->bmap->dim);

	isl_int_clear(denom);
	isl_constraint_free(c);

	qp = isl_qpolynomial_alloc(dim, 0, up);
	if (sgn > 0)
		qp = isl_qpolynomial_neg(qp);
	return qp;
}

__isl_give struct isl_upoly *isl_upoly_subs(__isl_take struct isl_upoly *up,
	unsigned first, unsigned n, __isl_keep struct isl_upoly **subs)
{
	int i;
	struct isl_upoly_rec *rec;
	struct isl_upoly *base, *res;

	if (!up)
		return NULL;

	if (isl_upoly_is_cst(up))
		return up;

	if (up->var < first)
		return up;

	rec = isl_upoly_as_rec(up);
	if (!rec)
		goto error;

	isl_assert(up->ctx, rec->n >= 1, goto error);

	if (up->var >= first + n)
		base = isl_upoly_pow(up->ctx, up->var, 1);
	else
		base = isl_upoly_copy(subs[up->var - first]);

	res = isl_upoly_subs(isl_upoly_copy(rec->p[rec->n - 1]), first, n, subs);
	for (i = rec->n - 2; i >= 0; --i) {
		struct isl_upoly *t;
		t = isl_upoly_subs(isl_upoly_copy(rec->p[i]), first, n, subs);
		res = isl_upoly_mul(res, isl_upoly_copy(base));
		res = isl_upoly_sum(res, t);
	}

	isl_upoly_free(base);
	isl_upoly_free(up);
				
	return res;
error:
	isl_upoly_free(up);
	return NULL;
}	

/* For each 0 <= i < "n", replace variable "first" + i of type "type"
 * in "qp" by subs[i].
 */
__isl_give isl_qpolynomial *isl_qpolynomial_substitute(
	__isl_take isl_qpolynomial *qp,
	enum isl_dim_type type, unsigned first, unsigned n,
	__isl_keep isl_qpolynomial **subs)
{
	int i;
	struct isl_upoly **ups;

	if (n == 0)
		return qp;

	qp = isl_qpolynomial_cow(qp);
	if (!qp)
		return NULL;
	for (i = 0; i < n; ++i)
		if (!subs[i])
			goto error;

	isl_assert(qp->dim->ctx, first + n <= isl_dim_size(qp->dim, type),
			goto error);

	for (i = 0; i < n; ++i)
		isl_assert(qp->dim->ctx, isl_dim_equal(qp->dim, subs[i]->dim),
				goto error);

	isl_assert(qp->dim->ctx, qp->div->n_row == 0, goto error);
	for (i = 0; i < n; ++i)
		isl_assert(qp->dim->ctx, subs[i]->div->n_row == 0, goto error);

	first += pos(qp->dim, type);

	ups = isl_alloc_array(qp->dim->ctx, struct isl_upoly *, n);
	if (!ups)
		goto error;
	for (i = 0; i < n; ++i)
		ups[i] = subs[i]->upoly;

	qp->upoly = isl_upoly_subs(qp->upoly, first, n, ups);

	free(ups);

	if (!qp->upoly)
		goto error;

	return qp;
error:
	isl_qpolynomial_free(qp);
	return NULL;
}

__isl_give isl_basic_set *add_div_constraints(__isl_take isl_basic_set *bset,
	__isl_take isl_mat *div)
{
	int i;
	unsigned total;

	if (!bset || !div)
		goto error;

	bset = isl_basic_set_extend_constraints(bset, 0, 2 * div->n_row);
	if (!bset)
		goto error;
	total = isl_basic_set_total_dim(bset);
	for (i = 0; i < div->n_row; ++i)
		if (isl_basic_set_add_div_constraints_var(bset,
				    total - div->n_row + i, div->row[i]) < 0)
			goto error;

	isl_mat_free(div);
	return bset;
error:
	isl_mat_free(div);
	isl_basic_set_free(bset);
	return NULL;
}

/* Extend "bset" with extra set dimensions for each integer division
 * in "qp" and then call "fn" with the extended bset and the polynomial
 * that results from replacing each of the integer divisions by the
 * corresponding extra set dimension.
 */
int isl_qpolynomial_as_polynomial_on_domain(__isl_keep isl_qpolynomial *qp,
	__isl_keep isl_basic_set *bset,
	int (*fn)(__isl_take isl_basic_set *bset,
		  __isl_take isl_qpolynomial *poly, void *user), void *user)
{
	isl_dim *dim;
	isl_mat *div;
	isl_qpolynomial *poly;

	if (!qp || !bset)
		goto error;
	if (qp->div->n_row == 0)
		return fn(isl_basic_set_copy(bset), isl_qpolynomial_copy(qp),
			  user);

	div = isl_mat_copy(qp->div);
	dim = isl_dim_copy(qp->dim);
	dim = isl_dim_add(dim, isl_dim_set, qp->div->n_row);
	poly = isl_qpolynomial_alloc(dim, 0, isl_upoly_copy(qp->upoly));
	bset = isl_basic_set_copy(bset);
	bset = isl_basic_set_add(bset, isl_dim_set, qp->div->n_row);
	bset = add_div_constraints(bset, div);

	return fn(bset, poly, user);
error:
	return -1;
}

/* Return total degree in variables first (inclusive) up to last (exclusive).
 */
int isl_upoly_degree(__isl_keep struct isl_upoly *up, int first, int last)
{
	int deg = -1;
	int i;
	struct isl_upoly_rec *rec;

	if (!up)
		return -2;
	if (isl_upoly_is_zero(up))
		return -1;
	if (isl_upoly_is_cst(up) || up->var < first)
		return 0;

	rec = isl_upoly_as_rec(up);
	if (!rec)
		return -2;

	for (i = 0; i < rec->n; ++i) {
		int d;

		if (isl_upoly_is_zero(rec->p[i]))
			continue;
		d = isl_upoly_degree(rec->p[i], first, last);
		if (up->var < last)
			d += i;
		if (d > deg)
			deg = d;
	}

	return deg;
}

/* Return total degree in set variables.
 */
int isl_qpolynomial_degree(__isl_keep isl_qpolynomial *poly)
{
	unsigned ovar;
	unsigned nvar;

	if (!poly)
		return -2;

	ovar = isl_dim_offset(poly->dim, isl_dim_set);
	nvar = isl_dim_size(poly->dim, isl_dim_set);
	return isl_upoly_degree(poly->upoly, ovar, ovar + nvar);
}

__isl_give struct isl_upoly *isl_upoly_coeff(__isl_keep struct isl_upoly *up,
	unsigned pos, int deg)
{
	int i;
	struct isl_upoly_rec *rec;

	if (!up)
		return NULL;

	if (isl_upoly_is_cst(up) || up->var < pos) {
		if (deg == 0)
			return isl_upoly_copy(up);
		else
			return isl_upoly_zero(up->ctx);
	}

	rec = isl_upoly_as_rec(up);
	if (!rec)
		return NULL;

	if (up->var == pos) {
		if (deg < rec->n)
			return isl_upoly_copy(rec->p[deg]);
		else
			return isl_upoly_zero(up->ctx);
	}

	up = isl_upoly_copy(up);
	up = isl_upoly_cow(up);
	rec = isl_upoly_as_rec(up);
	if (!rec)
		goto error;

	for (i = 0; i < rec->n; ++i) {
		struct isl_upoly *t;
		t = isl_upoly_coeff(rec->p[i], pos, deg);
		if (!t)
			goto error;
		isl_upoly_free(rec->p[i]);
		rec->p[i] = t;
	}

	return up;
error:
	isl_upoly_free(up);
	return NULL;
}

/* Return coefficient of power "deg" of variable "t_pos" of type "type".
 */
__isl_give isl_qpolynomial *isl_qpolynomial_coeff(
	__isl_keep isl_qpolynomial *qp,
	enum isl_dim_type type, unsigned t_pos, int deg)
{
	unsigned g_pos;
	struct isl_upoly *up;
	isl_qpolynomial *c;

	if (!qp)
		return NULL;

	isl_assert(qp->div->ctx, t_pos < isl_dim_size(qp->dim, type),
			return NULL);

	g_pos = pos(qp->dim, type) + t_pos;
	up = isl_upoly_coeff(qp->upoly, g_pos, deg);

	c = isl_qpolynomial_alloc(isl_dim_copy(qp->dim), qp->div->n_row, up);
	if (!c)
		return NULL;
	isl_mat_free(c->div);
	c->div = isl_mat_copy(qp->div);
	if (!c->div)
		goto error;
	return c;
error:
	isl_qpolynomial_free(c);
	return NULL;
}

/* Homogenize the polynomial in the variables first (inclusive) up to
 * last (exclusive) by inserting powers of variable first.
 * Variable first is assumed not to appear in the input.
 */
__isl_give struct isl_upoly *isl_upoly_homogenize(
	__isl_take struct isl_upoly *up, int deg, int target,
	int first, int last)
{
	int i;
	struct isl_upoly_rec *rec;

	if (!up)
		return NULL;
	if (isl_upoly_is_zero(up))
		return up;
	if (deg == target)
		return up;
	if (isl_upoly_is_cst(up) || up->var < first) {
		struct isl_upoly *hom;

		hom = isl_upoly_pow(up->ctx, first, target - deg);
		if (!hom)
			goto error;
		rec = isl_upoly_as_rec(hom);
		rec->p[target - deg] = isl_upoly_mul(rec->p[target - deg], up);

		return hom;
	}

	up = isl_upoly_cow(up);
	rec = isl_upoly_as_rec(up);
	if (!rec)
		goto error;

	for (i = 0; i < rec->n; ++i) {
		if (isl_upoly_is_zero(rec->p[i]))
			continue;
		rec->p[i] = isl_upoly_homogenize(rec->p[i],
				up->var < last ? deg + i : i, target,
				first, last);
		if (!rec->p[i])
			goto error;
	}

	return up;
error:
	isl_upoly_free(up);
	return NULL;
}

/* Homogenize the polynomial in the set variables by introducing
 * powers of an extra set variable at position 0.
 */
__isl_give isl_qpolynomial *isl_qpolynomial_homogenize(
	__isl_take isl_qpolynomial *poly)
{
	unsigned ovar;
	unsigned nvar;
	int deg = isl_qpolynomial_degree(poly);

	if (deg < -1)
		goto error;

	poly = isl_qpolynomial_insert_dims(poly, isl_dim_set, 0, 1);
	poly = isl_qpolynomial_cow(poly);
	if (!poly)
		goto error;

	ovar = isl_dim_offset(poly->dim, isl_dim_set);
	nvar = isl_dim_size(poly->dim, isl_dim_set);
	poly->upoly = isl_upoly_homogenize(poly->upoly, 0, deg,
						ovar, ovar + nvar);
	if (!poly->upoly)
		goto error;

	return poly;
error:
	isl_qpolynomial_free(poly);
	return NULL;
}

__isl_give isl_term *isl_term_alloc(__isl_take isl_dim *dim,
	__isl_take isl_mat *div)
{
	isl_term *term;
	int n;

	if (!dim || !div)
		goto error;

	n = isl_dim_total(dim) + div->n_row;

	term = isl_calloc(dim->ctx, struct isl_term,
			sizeof(struct isl_term) + (n - 1) * sizeof(int));
	if (!term)
		goto error;

	term->ref = 1;
	term->dim = dim;
	term->div = div;
	isl_int_init(term->n);
	isl_int_init(term->d);
	
	return term;
error:
	isl_dim_free(dim);
	isl_mat_free(div);
	return NULL;
}

__isl_give isl_term *isl_term_copy(__isl_keep isl_term *term)
{
	if (!term)
		return NULL;

	term->ref++;
	return term;
}

__isl_give isl_term *isl_term_dup(__isl_keep isl_term *term)
{
	int i;
	isl_term *dup;
	unsigned total;

	if (term)
		return NULL;

	total = isl_dim_total(term->dim) + term->div->n_row;

	dup = isl_term_alloc(isl_dim_copy(term->dim), isl_mat_copy(term->div));
	if (!dup)
		return NULL;

	isl_int_set(dup->n, term->n);
	isl_int_set(dup->d, term->d);

	for (i = 0; i < total; ++i)
		dup->pow[i] = term->pow[i];

	return dup;
}

__isl_give isl_term *isl_term_cow(__isl_take isl_term *term)
{
	if (!term)
		return NULL;

	if (term->ref == 1)
		return term;
	term->ref--;
	return isl_term_dup(term);
}

void isl_term_free(__isl_take isl_term *term)
{
	if (!term)
		return;

	if (--term->ref > 0)
		return;

	isl_dim_free(term->dim);
	isl_mat_free(term->div);
	isl_int_clear(term->n);
	isl_int_clear(term->d);
	free(term);
}

unsigned isl_term_dim(__isl_keep isl_term *term, enum isl_dim_type type)
{
	if (!term)
		return 0;

	switch (type) {
	case isl_dim_param:
	case isl_dim_in:
	case isl_dim_out:	return isl_dim_size(term->dim, type);
	case isl_dim_div:	return term->div->n_row;
	case isl_dim_all:	return isl_dim_total(term->dim) + term->div->n_row;
	default:		return 0;
	}
}

isl_ctx *isl_term_get_ctx(__isl_keep isl_term *term)
{
	return term ? term->dim->ctx : NULL;
}

void isl_term_get_num(__isl_keep isl_term *term, isl_int *n)
{
	if (!term)
		return;
	isl_int_set(*n, term->n);
}

void isl_term_get_den(__isl_keep isl_term *term, isl_int *d)
{
	if (!term)
		return;
	isl_int_set(*d, term->d);
}

int isl_term_get_exp(__isl_keep isl_term *term,
	enum isl_dim_type type, unsigned pos)
{
	if (!term)
		return -1;

	isl_assert(term->dim->ctx, pos < isl_term_dim(term, type), return -1);

	if (type >= isl_dim_set)
		pos += isl_dim_size(term->dim, isl_dim_param);
	if (type >= isl_dim_div)
		pos += isl_dim_size(term->dim, isl_dim_set);

	return term->pow[pos];
}

__isl_give isl_div *isl_term_get_div(__isl_keep isl_term *term, unsigned pos)
{
	isl_basic_map *bmap;
	unsigned total;
	int k;

	if (!term)
		return NULL;

	isl_assert(term->dim->ctx, pos < isl_term_dim(term, isl_dim_div),
			return NULL);

	total = term->div->n_col - term->div->n_row - 2;
	/* No nested divs for now */
	isl_assert(term->dim->ctx,
		isl_seq_first_non_zero(term->div->row[pos] + 2 + total,
					term->div->n_row) == -1,
		return NULL);

	bmap = isl_basic_map_alloc_dim(isl_dim_copy(term->dim), 1, 0, 0);
	if ((k = isl_basic_map_alloc_div(bmap)) < 0)
		goto error;

	isl_seq_cpy(bmap->div[k], term->div->row[pos], 2 + total);

	return isl_basic_map_div(bmap, k);
error:
	isl_basic_map_free(bmap);
	return NULL;
}

__isl_give isl_term *isl_upoly_foreach_term(__isl_keep struct isl_upoly *up,
	int (*fn)(__isl_take isl_term *term, void *user),
	__isl_take isl_term *term, void *user)
{
	int i;
	struct isl_upoly_rec *rec;

	if (!up || !term)
		goto error;

	if (isl_upoly_is_zero(up))
		return term;

	isl_assert(up->ctx, !isl_upoly_is_nan(up), goto error);
	isl_assert(up->ctx, !isl_upoly_is_infty(up), goto error);
	isl_assert(up->ctx, !isl_upoly_is_neginfty(up), goto error);

	if (isl_upoly_is_cst(up)) {
		struct isl_upoly_cst *cst;
		cst = isl_upoly_as_cst(up);
		if (!cst)
			goto error;
		term = isl_term_cow(term);
		if (!term)
			goto error;
		isl_int_set(term->n, cst->n);
		isl_int_set(term->d, cst->d);
		if (fn(isl_term_copy(term), user) < 0)
			goto error;
		return term;
	}

	rec = isl_upoly_as_rec(up);
	if (!rec)
		goto error;

	for (i = 0; i < rec->n; ++i) {
		term = isl_term_cow(term);
		if (!term)
			goto error;
		term->pow[up->var] = i;
		term = isl_upoly_foreach_term(rec->p[i], fn, term, user);
		if (!term)
			goto error;
	}
	term->pow[up->var] = 0;

	return term;
error:
	isl_term_free(term);
	return NULL;
}

int isl_qpolynomial_foreach_term(__isl_keep isl_qpolynomial *qp,
	int (*fn)(__isl_take isl_term *term, void *user), void *user)
{
	isl_term *term;

	if (!qp)
		return -1;

	term = isl_term_alloc(isl_dim_copy(qp->dim), isl_mat_copy(qp->div));
	if (!term)
		return -1;

	term = isl_upoly_foreach_term(qp->upoly, fn, term, user);

	isl_term_free(term);

	return term ? 0 : -1;
}

__isl_give isl_qpolynomial *isl_qpolynomial_from_term(__isl_take isl_term *term)
{
	struct isl_upoly *up;
	isl_qpolynomial *qp;
	int i, n;

	if (!term)
		return NULL;

	n = isl_dim_total(term->dim) + term->div->n_row;

	up = isl_upoly_rat_cst(term->dim->ctx, term->n, term->d);
	for (i = 0; i < n; ++i) {
		if (!term->pow[i])
			continue;
		up = isl_upoly_mul(up,
			isl_upoly_pow(term->dim->ctx, i, term->pow[i]));
	}

	qp = isl_qpolynomial_alloc(isl_dim_copy(term->dim), term->div->n_row, up);
	if (!qp)
		goto error;
	isl_mat_free(qp->div);
	qp->div = isl_mat_copy(term->div);
	if (!qp->div)
		goto error;

	isl_term_free(term);
	return qp;
error:
	isl_qpolynomial_free(qp);
	isl_term_free(term);
	return NULL;
}

__isl_give isl_qpolynomial *isl_qpolynomial_lift(__isl_take isl_qpolynomial *qp,
	__isl_take isl_dim *dim)
{
	int i;
	int extra;
	unsigned total;

	if (!qp || !dim)
		goto error;

	if (isl_dim_equal(qp->dim, dim)) {
		isl_dim_free(dim);
		return qp;
	}

	qp = isl_qpolynomial_cow(qp);
	if (!qp)
		goto error;

	extra = isl_dim_size(dim, isl_dim_set) -
			isl_dim_size(qp->dim, isl_dim_set);
	total = isl_dim_total(qp->dim);
	if (qp->div->n_row) {
		int *exp;

		exp = isl_alloc_array(qp->div->ctx, int, qp->div->n_row);
		if (!exp)
			goto error;
		for (i = 0; i < qp->div->n_row; ++i)
			exp[i] = extra + i;
		qp->upoly = expand(qp->upoly, exp, total);
		free(exp);
		if (!qp->upoly)
			goto error;
	}
	qp->div = isl_mat_insert_cols(qp->div, 2 + total, extra);
	if (!qp->div)
		goto error;
	for (i = 0; i < qp->div->n_row; ++i)
		isl_seq_clr(qp->div->row[i] + 2 + total, extra);

	isl_dim_free(qp->dim);
	qp->dim = dim;

	return qp;
error:
	isl_dim_free(dim);
	isl_qpolynomial_free(qp);
	return NULL;
}

/* For each parameter or variable that does not appear in qp,
 * first eliminate the variable from all constraints and then set it to zero.
 */
static __isl_give isl_set *fix_inactive(__isl_take isl_set *set,
	__isl_keep isl_qpolynomial *qp)
{
	int *active = NULL;
	int i;
	int d;
	unsigned nparam;
	unsigned nvar;

	if (!set || !qp)
		goto error;

	d = isl_dim_total(set->dim);
	active = isl_calloc_array(set->ctx, int, d);
	if (set_active(qp, active) < 0)
		goto error;

	for (i = 0; i < d; ++i)
		if (!active[i])
			break;

	if (i == d) {
		free(active);
		return set;
	}

	nparam = isl_dim_size(set->dim, isl_dim_param);
	nvar = isl_dim_size(set->dim, isl_dim_set);
	for (i = 0; i < nparam; ++i) {
		if (active[i])
			continue;
		set = isl_set_eliminate(set, isl_dim_param, i, 1);
		set = isl_set_fix_si(set, isl_dim_param, i, 0);
	}
	for (i = 0; i < nvar; ++i) {
		if (active[nparam + i])
			continue;
		set = isl_set_eliminate(set, isl_dim_set, i, 1);
		set = isl_set_fix_si(set, isl_dim_set, i, 0);
	}

	free(active);

	return set;
error:
	free(active);
	isl_set_free(set);
	return NULL;
}

struct isl_opt_data {
	isl_qpolynomial *qp;
	int first;
	isl_qpolynomial *opt;
	int max;
};

static int opt_fn(__isl_take isl_point *pnt, void *user)
{
	struct isl_opt_data *data = (struct isl_opt_data *)user;
	isl_qpolynomial *val;

	val = isl_qpolynomial_eval(isl_qpolynomial_copy(data->qp), pnt);
	if (data->first) {
		data->first = 0;
		data->opt = val;
	} else if (data->max) {
		data->opt = isl_qpolynomial_max_cst(data->opt, val);
	} else {
		data->opt = isl_qpolynomial_min_cst(data->opt, val);
	}

	return 0;
}

__isl_give isl_qpolynomial *isl_qpolynomial_opt_on_domain(
	__isl_take isl_qpolynomial *qp, __isl_take isl_set *set, int max)
{
	struct isl_opt_data data = { NULL, 1, NULL, max };

	if (!set || !qp)
		goto error;

	if (isl_upoly_is_cst(qp->upoly)) {
		isl_set_free(set);
		return qp;
	}

	set = fix_inactive(set, qp);

	data.qp = qp;
	if (isl_set_foreach_point(set, opt_fn, &data) < 0)
		goto error;

	if (data.first)
		data.opt = isl_qpolynomial_zero(isl_qpolynomial_get_dim(qp));

	isl_set_free(set);
	isl_qpolynomial_free(qp);
	return data.opt;
error:
	isl_set_free(set);
	isl_qpolynomial_free(qp);
	isl_qpolynomial_free(data.opt);
	return NULL;
}

__isl_give isl_qpolynomial *isl_qpolynomial_morph(__isl_take isl_qpolynomial *qp,
	__isl_take isl_morph *morph)
{
	int i;
	isl_ctx *ctx;
	struct isl_upoly *up;
	unsigned n_div;
	struct isl_upoly **subs;
	isl_mat *mat;

	qp = isl_qpolynomial_cow(qp);
	if (!qp || !morph)
		goto error;

	ctx = qp->dim->ctx;
	isl_assert(ctx, isl_dim_equal(qp->dim, morph->dom->dim), goto error);

	subs = isl_calloc_array(ctx, struct isl_upoly *, morph->inv->n_row - 1);
	if (!subs)
		goto error;

	for (i = 0; 1 + i < morph->inv->n_row; ++i)
		subs[i] = isl_upoly_from_affine(ctx, morph->inv->row[1 + i],
					morph->inv->row[0][0], morph->inv->n_col);

	qp->upoly = isl_upoly_subs(qp->upoly, 0, morph->inv->n_row - 1, subs);

	for (i = 0; 1 + i < morph->inv->n_row; ++i)
		isl_upoly_free(subs[i]);
	free(subs);

	mat = isl_mat_diagonal(isl_mat_identity(ctx, 1), isl_mat_copy(morph->inv));
	mat = isl_mat_diagonal(mat, isl_mat_identity(ctx, qp->div->n_row));
	qp->div = isl_mat_product(qp->div, mat);
	isl_dim_free(qp->dim);
	qp->dim = isl_dim_copy(morph->ran->dim);

	if (!qp->upoly || !qp->div || !qp->dim)
		goto error;

	isl_morph_free(morph);

	return qp;
error:
	isl_qpolynomial_free(qp);
	isl_morph_free(morph);
	return NULL;
}
