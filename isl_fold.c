/*
 * Copyright 2010      INRIA Saclay
 *
 * Use of this software is governed by the GNU LGPLv2.1 license
 *
 * Written by Sven Verdoolaege, INRIA Saclay - Ile-de-France,
 * Parc Club Orsay Universite, ZAC des vignes, 4 rue Jacques Monod,
 * 91893 Orsay, France 
 */

#include <isl_polynomial_private.h>
#include <isl_point_private.h>
#include <isl_dim_private.h>
#include <isl_map_private.h>

int isl_qpolynomial_fold_involves_dims(__isl_keep isl_qpolynomial_fold *fold,
	enum isl_dim_type type, unsigned first, unsigned n)
{
	int i;

	if (!fold)
		return -1;
	if (fold->n == 0 || n == 0)
		return 0;

	for (i = 0; i < fold->n; ++i) {
		int involves = isl_qpolynomial_involves_dims(fold->qp[i],
							    type, first, n);
		if (involves < 0 || involves)
			return involves;
	}
	return 0;
}

__isl_give isl_qpolynomial_fold *isl_qpolynomial_fold_drop_dims(
	__isl_take isl_qpolynomial_fold *fold,
	enum isl_dim_type type, unsigned first, unsigned n)
{
	int i;

	if (!fold)
		return NULL;
	if (n == 0)
		return fold;

	fold = isl_qpolynomial_fold_cow(fold);
	if (!fold)
		return NULL;
	fold->dim = isl_dim_drop(fold->dim, type, first, n);
	if (!fold->dim)
		goto error;

	for (i = 0; i < fold->n; ++i) {
		fold->qp[i] = isl_qpolynomial_drop_dims(fold->qp[i],
							    type, first, n);
		if (!fold->qp[i])
			goto error;
	}

	return fold;
error:
	isl_qpolynomial_fold_free(fold);
	return NULL;
}

#undef PW
#define PW isl_pw_qpolynomial_fold
#undef EL
#define EL isl_qpolynomial_fold
#undef IS_ZERO
#define IS_ZERO is_empty
#undef FIELD
#define FIELD fold
#undef ADD
#define ADD fold

#include <isl_pw_templ.c>

static __isl_give isl_qpolynomial_fold *qpolynomial_fold_alloc(
	enum isl_fold type, __isl_take isl_dim *dim, int n)
{
	isl_qpolynomial_fold *fold;

	if (!dim)
		goto error;

	isl_assert(dim->ctx, n >= 0, goto error);
	fold = isl_alloc(dim->ctx, struct isl_qpolynomial_fold,
			sizeof(struct isl_qpolynomial_fold) +
			(n - 1) * sizeof(struct isl_qpolynomial *));
	if (!fold)
		goto error;

	fold->ref = 1;
	fold->size = n;
	fold->n = 0;
	fold->type = type;
	fold->dim = dim;

	return fold;
error:
	isl_dim_free(dim);
	return NULL;
}

__isl_give isl_qpolynomial_fold *isl_qpolynomial_fold_empty(enum isl_fold type,
	__isl_take isl_dim *dim)
{
	return qpolynomial_fold_alloc(type, dim, 0);
}

__isl_give isl_qpolynomial_fold *isl_qpolynomial_fold_alloc(
	enum isl_fold type, __isl_take isl_qpolynomial *qp)
{
	isl_qpolynomial_fold *fold;

	if (!qp)
		return NULL;

	fold = qpolynomial_fold_alloc(type, isl_dim_copy(qp->dim), 1);
	if (!fold)
		goto error;

	fold->qp[0] = qp;
	fold->n++;

	return fold;
error:
	isl_qpolynomial_fold_free(fold);
	isl_qpolynomial_free(qp);
	return NULL;
}

__isl_give isl_qpolynomial_fold *isl_qpolynomial_fold_copy(
	__isl_keep isl_qpolynomial_fold *fold)
{
	if (!fold)
		return NULL;

	fold->ref++;
	return fold;
}

__isl_give isl_qpolynomial_fold *isl_qpolynomial_fold_dup(
	__isl_keep isl_qpolynomial_fold *fold)
{
	int i;
	isl_qpolynomial_fold *dup;

	if (!fold)
		return NULL;
	dup = qpolynomial_fold_alloc(fold->type,
					isl_dim_copy(fold->dim), fold->n);
	if (!dup)
		return NULL;
	
	for (i = 0; i < fold->n; ++i) {
		dup->qp[i] = isl_qpolynomial_copy(fold->qp[i]);
		if (!dup->qp[i])
			goto error;
	}

	return dup;
error:
	isl_qpolynomial_fold_free(dup);
	return NULL;
}

__isl_give isl_qpolynomial_fold *isl_qpolynomial_fold_cow(
	__isl_take isl_qpolynomial_fold *fold)
{
	if (!fold)
		return NULL;

	if (fold->ref == 1)
		return fold;
	fold->ref--;
	return isl_qpolynomial_fold_dup(fold);
}

void isl_qpolynomial_fold_free(__isl_take isl_qpolynomial_fold *fold)
{
	int i;

	if (!fold)
		return;
	if (--fold->ref > 0)
		return;

	for (i = 0; i < fold->n; ++i)
		isl_qpolynomial_free(fold->qp[i]);
	isl_dim_free(fold->dim);
	free(fold);
}

int isl_qpolynomial_fold_is_empty(__isl_keep isl_qpolynomial_fold *fold)
{
	if (!fold)
		return -1;

	return fold->n == 0;
}

__isl_give isl_qpolynomial_fold *isl_qpolynomial_fold_fold(
	__isl_take isl_qpolynomial_fold *fold1,
	__isl_take isl_qpolynomial_fold *fold2)
{
	int i;
	struct isl_qpolynomial_fold *res = NULL;

	if (!fold1 || !fold2)
		goto error;

	isl_assert(fold1->dim->ctx, fold1->type == fold2->type, goto error);
	isl_assert(fold1->dim->ctx, isl_dim_equal(fold1->dim, fold2->dim),
			goto error);

	if (isl_qpolynomial_fold_is_empty(fold1)) {
		isl_qpolynomial_fold_free(fold1);
		return fold2;
	}

	if (isl_qpolynomial_fold_is_empty(fold2)) {
		isl_qpolynomial_fold_free(fold2);
		return fold1;
	}

	res = qpolynomial_fold_alloc(fold1->type, isl_dim_copy(fold1->dim),
					fold1->n + fold2->n);
	if (!res)
		goto error;

	for (i = 0; i < fold1->n; ++i) {
		res->qp[res->n] = isl_qpolynomial_copy(fold1->qp[i]);
		if (!res->qp[res->n])
			goto error;
		res->n++;
	}

	for (i = 0; i < fold2->n; ++i) {
		res->qp[res->n] = isl_qpolynomial_copy(fold2->qp[i]);
		if (!res->qp[res->n])
			goto error;
		res->n++;
	}

	isl_qpolynomial_fold_free(fold1);
	isl_qpolynomial_fold_free(fold2);

	return res;
error:
	isl_qpolynomial_fold_free(res);
	isl_qpolynomial_fold_free(fold1);
	isl_qpolynomial_fold_free(fold2);
	return NULL;
}

__isl_give isl_pw_qpolynomial_fold *isl_pw_qpolynomial_fold_from_pw_qpolynomial(
	enum isl_fold type, __isl_take isl_pw_qpolynomial *pwqp)
{
	int i;
	isl_pw_qpolynomial_fold *pwf;

	if (!pwqp)
		return NULL;
	
	pwf = isl_pw_qpolynomial_fold_alloc_(isl_dim_copy(pwqp->dim), pwqp->n);

	for (i = 0; i < pwqp->n; ++i)
		pwf = isl_pw_qpolynomial_fold_add_piece(pwf,
			isl_set_copy(pwqp->p[i].set),
			isl_qpolynomial_fold_alloc(type,
				isl_qpolynomial_copy(pwqp->p[i].qp)));

	isl_pw_qpolynomial_free(pwqp);

	return pwf;
}

int isl_qpolynomial_fold_is_equal(__isl_keep isl_qpolynomial_fold *fold1,
	__isl_keep isl_qpolynomial_fold *fold2)
{
	int i;

	if (!fold1 || !fold2)
		return -1;

	if (fold1->n != fold2->n)
		return 0;

	/* We probably want to sort the qps first... */
	for (i = 0; i < fold1->n; ++i) {
		int eq = isl_qpolynomial_is_equal(fold1->qp[i], fold2->qp[i]);
		if (eq < 0 || !eq)
			return eq;
	}

	return 1;
}

__isl_give isl_qpolynomial *isl_qpolynomial_fold_eval(
	__isl_take isl_qpolynomial_fold *fold, __isl_take isl_point *pnt)
{
	isl_qpolynomial *qp;

	if (!fold || !pnt)
		goto error;
	isl_assert(pnt->dim->ctx, isl_dim_equal(pnt->dim, fold->dim), goto error);
	isl_assert(pnt->dim->ctx,
		fold->type == isl_fold_max || fold->type == isl_fold_min,
		goto error);

	if (fold->n == 0)
		qp = isl_qpolynomial_zero(isl_dim_copy(fold->dim));
	else {
		int i;
		qp = isl_qpolynomial_eval(isl_qpolynomial_copy(fold->qp[0]),
						isl_point_copy(pnt));
		for (i = 1; i < fold->n; ++i) {
			isl_qpolynomial *qp_i;
			qp_i = isl_qpolynomial_eval(
					    isl_qpolynomial_copy(fold->qp[i]),
					    isl_point_copy(pnt));
			if (fold->type == isl_fold_max)
				qp = isl_qpolynomial_max_cst(qp, qp_i);
			else
				qp = isl_qpolynomial_min_cst(qp, qp_i);
		}
	}
	isl_qpolynomial_fold_free(fold);
	isl_point_free(pnt);

	return qp;
error:
	isl_qpolynomial_fold_free(fold);
	isl_point_free(pnt);
	return NULL;
}
