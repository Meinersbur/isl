/*
 * Copyright 2010      INRIA Saclay
 *
 * Use of this software is governed by the GNU LGPLv2.1 license
 *
 * Written by Sven Verdoolaege, INRIA Saclay - Ile-de-France,
 * Parc Club Orsay Universite, ZAC des vignes, 4 rue Jacques Monod,
 * 91893 Orsay, France 
 */

#include <isl_union_map_private.h>
#include <isl_polynomial_private.h>
#include <isl_point_private.h>
#include <isl_dim_private.h>
#include <isl_map_private.h>
#include <isl_lp.h>
#include <isl_seq.h>

static __isl_give isl_qpolynomial_fold *qpolynomial_fold_alloc(
	enum isl_fold type, __isl_take isl_dim *dim, int n)
{
	isl_qpolynomial_fold *fold;

	if (!dim)
		goto error;

	isl_assert(dim->ctx, n >= 0, goto error);
	fold = isl_calloc(dim->ctx, struct isl_qpolynomial_fold,
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

static int isl_qpolynomial_cst_sign(__isl_keep isl_qpolynomial *qp)
{
	struct isl_upoly_cst *cst;

	cst = isl_upoly_as_cst(qp->upoly);
	if (!cst)
		return 0;

	return isl_int_sgn(cst->n) < 0 ? -1 : 1;
}

static int isl_qpolynomial_aff_sign(__isl_keep isl_set *set,
	__isl_keep isl_qpolynomial *qp)
{
	enum isl_lp_result res;
	isl_vec *aff;
	isl_int opt;
	int sgn = 0;

	aff = isl_qpolynomial_extract_affine(qp);
	if (!aff)
		return 0;

	isl_int_init(opt);

	res = isl_set_solve_lp(set, 0, aff->el + 1, aff->el[0],
				&opt, NULL, NULL);
	if (res == isl_lp_error)
		goto done;
	if (res == isl_lp_empty ||
	    (res == isl_lp_ok && !isl_int_is_neg(opt))) {
		sgn = 1;
		goto done;
	}

	res = isl_set_solve_lp(set, 1, aff->el + 1, aff->el[0],
				&opt, NULL, NULL);
	if (res == isl_lp_ok && !isl_int_is_pos(opt))
		sgn = -1;

done:
	isl_int_clear(opt);
	isl_vec_free(aff);
	return sgn;
}

/* Determine, if possible, the sign of the quasipolynomial "qp" on
 * the domain "set".
 *
 * If qp is a constant, then the problem is trivial.
 * If qp is linear, then we check if the minimum of the corresponding
 * affine constraint is non-negative or if the maximum is non-positive.
 *
 * Otherwise, we check if the outermost variable "v" has a lower bound "l"
 * in "set".  If so, we write qp(v,v') as
 *
 *	q(v,v') * (v - l) + r(v')
 *
 * if q(v,v') and r(v') have the same known sign, then the original
 * quasipolynomial has the same sign as well.
 *
 * Return
 *	-1 if qp <= 0
 *	 1 if qp >= 0
 *	 0 if unknown
 */
static int isl_qpolynomial_sign(__isl_keep isl_set *set,
	__isl_keep isl_qpolynomial *qp)
{
	int d;
	int i;
	int is;
	struct isl_upoly_rec *rec;
	isl_vec *v;
	isl_int l;
	enum isl_lp_result res;
	int sgn = 0;

	is = isl_qpolynomial_is_cst(qp, NULL, NULL);
	if (is < 0)
		return 0;
	if (is)
		return isl_qpolynomial_cst_sign(qp);

	is = isl_qpolynomial_is_affine(qp);
	if (is < 0)
		return 0;
	if (is)
		return isl_qpolynomial_aff_sign(set, qp);

	if (qp->div->n_row > 0)
		return 0;

	rec = isl_upoly_as_rec(qp->upoly);
	if (!rec)
		return 0;

	d = isl_dim_total(qp->dim);
	v = isl_vec_alloc(set->ctx, 2 + d);
	if (!v)
		return 0;

	isl_seq_clr(v->el + 1, 1 + d);
	isl_int_set_si(v->el[0], 1);
	isl_int_set_si(v->el[2 + qp->upoly->var], 1);

	isl_int_init(l);

	res = isl_set_solve_lp(set, 0, v->el + 1, v->el[0], &l, NULL, NULL);
	if (res == isl_lp_ok) {
		isl_qpolynomial *min;
		isl_qpolynomial *base;
		isl_qpolynomial *r, *q;
		isl_qpolynomial *t;

		min = isl_qpolynomial_cst(isl_dim_copy(qp->dim), l);
		base = isl_qpolynomial_pow(isl_dim_copy(qp->dim),
						qp->upoly->var, 1);

		r = isl_qpolynomial_alloc(isl_dim_copy(qp->dim), 0,
					  isl_upoly_copy(rec->p[rec->n - 1]));
		q = isl_qpolynomial_copy(r);

		for (i = rec->n - 2; i >= 0; --i) {
			r = isl_qpolynomial_mul(r, isl_qpolynomial_copy(min));
			t = isl_qpolynomial_alloc(isl_dim_copy(qp->dim), 0,
						  isl_upoly_copy(rec->p[i]));
			r = isl_qpolynomial_add(r, t);
			if (i == 0)
				break;
			q = isl_qpolynomial_mul(q, isl_qpolynomial_copy(base));
			q = isl_qpolynomial_add(q, isl_qpolynomial_copy(r));
		}

		if (isl_qpolynomial_is_zero(q))
			sgn = isl_qpolynomial_sign(set, r);
		else if (isl_qpolynomial_is_zero(r))
			sgn = isl_qpolynomial_sign(set, q);
		else {
			int sgn_q, sgn_r;
			sgn_r = isl_qpolynomial_sign(set, r);
			sgn_q = isl_qpolynomial_sign(set, q);
			if (sgn_r == sgn_q)
				sgn = sgn_r;
		}

		isl_qpolynomial_free(min);
		isl_qpolynomial_free(base);
		isl_qpolynomial_free(q);
		isl_qpolynomial_free(r);
	}

	isl_int_clear(l);

	isl_vec_free(v);

	return sgn;
}

__isl_give isl_qpolynomial_fold *isl_qpolynomial_fold_fold_on_domain(
	__isl_keep isl_set *set,
	__isl_take isl_qpolynomial_fold *fold1,
	__isl_take isl_qpolynomial_fold *fold2)
{
	int i, j;
	int n1;
	struct isl_qpolynomial_fold *res = NULL;
	int better;

	if (!fold1 || !fold2)
		goto error;

	isl_assert(fold1->dim->ctx, fold1->type == fold2->type, goto error);
	isl_assert(fold1->dim->ctx, isl_dim_equal(fold1->dim, fold2->dim),
			goto error);

	better = fold1->type == isl_fold_max ? -1 : 1;

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
	n1 = res->n;

	for (i = 0; i < fold2->n; ++i) {
		for (j = n1 - 1; j >= 0; --j) {
			isl_qpolynomial *d;
			int sgn;
			d = isl_qpolynomial_sub(
				isl_qpolynomial_copy(res->qp[j]),
				isl_qpolynomial_copy(fold2->qp[i]));
			sgn = isl_qpolynomial_sign(set, d);
			isl_qpolynomial_free(d);
			if (sgn == 0)
				continue;
			if (sgn != better)
				break;
			isl_qpolynomial_free(res->qp[j]);
			if (j != n1 - 1)
				res->qp[j] = res->qp[n1 - 1];
			n1--;
			if (n1 != res->n - 1)
				res->qp[n1] = res->qp[res->n - 1];
			res->n--;
		}
		if (j >= 0)
			continue;
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

#undef PW
#define PW isl_pw_qpolynomial_fold
#undef EL
#define EL isl_qpolynomial_fold
#undef IS_ZERO
#define IS_ZERO is_empty
#undef FIELD
#define FIELD fold
#undef ADD
#define ADD(d,e1,e2)	isl_qpolynomial_fold_fold_on_domain(d,e1,e2) 

#include <isl_pw_templ.c>

#undef UNION
#define UNION isl_union_pw_qpolynomial_fold
#undef PART
#define PART isl_pw_qpolynomial_fold
#undef PARTS
#define PARTS pw_qpolynomial_fold

#include <isl_union_templ.c>

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
	
	dup->n = fold->n;
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

size_t isl_pw_qpolynomial_fold_size(__isl_keep isl_pw_qpolynomial_fold *pwf)
{
	int i;
	size_t n = 0;

	for (i = 0; i < pwf->n; ++i)
		n += pwf->p[i].fold->n;

	return n;
}

__isl_give isl_qpolynomial *isl_qpolynomial_fold_opt_on_domain(
	__isl_take isl_qpolynomial_fold *fold, __isl_take isl_set *set, int max)
{
	int i;
	isl_qpolynomial *opt;

	if (!set || !fold)
		goto error;

	if (fold->n == 0) {
		isl_dim *dim = isl_dim_copy(fold->dim);
		isl_set_free(set);
		isl_qpolynomial_fold_free(fold);
		return isl_qpolynomial_zero(dim);
	}

	opt = isl_qpolynomial_opt_on_domain(isl_qpolynomial_copy(fold->qp[0]),
						isl_set_copy(set), max);
	for (i = 1; i < fold->n; ++i) {
		isl_qpolynomial *opt_i;
		opt_i = isl_qpolynomial_opt_on_domain(
				isl_qpolynomial_copy(fold->qp[i]),
				isl_set_copy(set), max);
		if (max)
			opt = isl_qpolynomial_max_cst(opt, opt_i);
		else
			opt = isl_qpolynomial_min_cst(opt, opt_i);
	}

	isl_set_free(set);
	isl_qpolynomial_fold_free(fold);

	return opt;
error:
	isl_set_free(set);
	isl_qpolynomial_fold_free(fold);
	return NULL;
}

/* Check whether for each quasi-polynomial in "fold2" there is
 * a quasi-polynomial in "fold1" that dominates it on "set".
 */
static int qpolynomial_fold_covers_on_domain(__isl_keep isl_set *set,
	__isl_keep isl_qpolynomial_fold *fold1,
	__isl_keep isl_qpolynomial_fold *fold2)
{
	int i, j;
	int covers;

	if (!set || !fold1 || !fold2)
		return -1;

	covers = fold1->type == isl_fold_max ? 1 : -1;

	for (i = 0; i < fold2->n; ++i) {
		for (j = 0; j < fold1->n; ++j) {
			isl_qpolynomial *d;
			int sgn;

			d = isl_qpolynomial_sub(
				isl_qpolynomial_copy(fold1->qp[j]),
				isl_qpolynomial_copy(fold2->qp[i]));
			sgn = isl_qpolynomial_sign(set, d);
			isl_qpolynomial_free(d);
			if (sgn == covers)
				break;
		}
		if (j >= fold1->n)
			return 0;
	}

	return 1;
}

/* Check whether "pwf1" dominated "pwf2", i.e., the domain of "pwf1" contains
 * that of "pwf2" and on each cell, the corresponding fold from pwf1 dominates
 * that of pwf2.
 */
int isl_pw_qpolynomial_fold_covers(__isl_keep isl_pw_qpolynomial_fold *pwf1,
	__isl_keep isl_pw_qpolynomial_fold *pwf2)
{
	int i, j;
	isl_set *dom1, *dom2;
	int is_subset;

	if (!pwf1 || !pwf2)
		return -1;

	if (pwf2->n == 0)
		return 1;
	if (pwf1->n == 0)
		return 0;

	dom1 = isl_pw_qpolynomial_fold_domain(isl_pw_qpolynomial_fold_copy(pwf1));
	dom2 = isl_pw_qpolynomial_fold_domain(isl_pw_qpolynomial_fold_copy(pwf2));
	is_subset = isl_set_is_subset(dom2, dom1);
	isl_set_free(dom1);
	isl_set_free(dom2);

	if (is_subset < 0 || !is_subset)
		return is_subset;

	for (i = 0; i < pwf2->n; ++i) {
		for (j = 0; j < pwf1->n; ++j) {
			int is_empty;
			isl_set *common;
			int covers;

			common = isl_set_intersect(isl_set_copy(pwf1->p[j].set),
						   isl_set_copy(pwf2->p[i].set));
			is_empty = isl_set_is_empty(common);
			if (is_empty < 0 || is_empty) {
				isl_set_free(common);
				if (is_empty < 0)
					return -1;
				continue;
			}
			covers = qpolynomial_fold_covers_on_domain(common,
					pwf1->p[j].fold, pwf2->p[i].fold);
			isl_set_free(common);
			if (covers < 0 || !covers)
				return covers;
		}
	}

	return 1;
}

__isl_give isl_qpolynomial_fold *isl_qpolynomial_fold_morph(
	__isl_take isl_qpolynomial_fold *fold, __isl_take isl_morph *morph)
{
	int i;
	isl_ctx *ctx;

	if (!fold || !morph)
		goto error;

	ctx = fold->dim->ctx;
	isl_assert(ctx, isl_dim_equal(fold->dim, morph->dom->dim), goto error);

	fold = isl_qpolynomial_fold_cow(fold);
	if (!fold)
		goto error;

	isl_dim_free(fold->dim);
	fold->dim = isl_dim_copy(morph->ran->dim);
	if (!fold->dim)
		goto error;

	for (i = 0; i < fold->n; ++i) {
		fold->qp[i] = isl_qpolynomial_morph(fold->qp[i],
						isl_morph_copy(morph));
		if (!fold->qp[i])
			goto error;
	}

	isl_morph_free(morph);

	return fold;
error:
	isl_qpolynomial_fold_free(fold);
	isl_morph_free(morph);
	return NULL;
}

enum isl_fold isl_qpolynomial_fold_get_type(__isl_keep isl_qpolynomial_fold *fold)
{
	if (!fold)
		return isl_fold_list;
	return fold->type;
}

__isl_give isl_qpolynomial_fold *isl_qpolynomial_fold_lift(
	__isl_take isl_qpolynomial_fold *fold, __isl_take isl_dim *dim)
{
	int i;
	isl_ctx *ctx;

	if (!fold || !dim)
		goto error;

	if (isl_dim_equal(fold->dim, dim)) {
		isl_dim_free(dim);
		return fold;
	}

	fold = isl_qpolynomial_fold_cow(fold);
	if (!fold)
		goto error;

	isl_dim_free(fold->dim);
	fold->dim = isl_dim_copy(dim);
	if (!fold->dim)
		goto error;

	for (i = 0; i < fold->n; ++i) {
		fold->qp[i] = isl_qpolynomial_lift(fold->qp[i],
						isl_dim_copy(dim));
		if (!fold->qp[i])
			goto error;
	}

	isl_dim_free(dim);

	return fold;
error:
	isl_qpolynomial_fold_free(fold);
	isl_dim_free(dim);
	return NULL;
}

int isl_qpolynomial_fold_foreach_qpolynomial(
	__isl_keep isl_qpolynomial_fold *fold,
	int (*fn)(__isl_take isl_qpolynomial *qp, void *user), void *user)
{
	int i;

	if (!fold)
		return -1;

	for (i = 0; i < fold->n; ++i)
		if (fn(isl_qpolynomial_copy(fold->qp[i]), user) < 0)
			return -1;

	return 0;
}

__isl_give isl_qpolynomial_fold *isl_qpolynomial_fold_move_dims(
	__isl_take isl_qpolynomial_fold *fold,
	enum isl_dim_type dst_type, unsigned dst_pos,
	enum isl_dim_type src_type, unsigned src_pos, unsigned n)
{
	int i;

	if (n == 0)
		return fold;

	fold = isl_qpolynomial_fold_cow(fold);
	if (!fold)
		return NULL;

	fold->dim = isl_dim_move(fold->dim, dst_type, dst_pos,
						src_type, src_pos, n);
	if (!fold->dim)
		goto error;

	for (i = 0; i < fold->n; ++i) {
		fold->qp[i] = isl_qpolynomial_move_dims(fold->qp[i],
				dst_type, dst_pos, src_type, src_pos, n);
		if (!fold->qp[i])
			goto error;
	}

	return fold;
error:
	isl_qpolynomial_fold_free(fold);
	return NULL;
}
