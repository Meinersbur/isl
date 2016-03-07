/*
 * Copyright 2011      INRIA Saclay
 * Copyright 2012      Ecole Normale Superieure
 *
 * Use of this software is governed by the MIT license
 *
 * Written by Sven Verdoolaege, INRIA Saclay - Ile-de-France,
 * Parc Club Orsay Universite, ZAC des vignes, 4 rue Jacques Monod,
 * 91893 Orsay, France
 * and Ecole Normale Superieure, 45 rue d'Ulm, 75230 Paris, France
 */

#include <isl_pw_macro.h>

/* Given a function "cmp" that returns the set of elements where
 * "el1" is "better" than "el2", return the intersection of this
 * set with "dom1" and "dom2".
 */
static __isl_give isl_set *FN(PW,shared_and_better)(__isl_keep isl_set *dom1,
	__isl_keep isl_set *dom2, __isl_keep EL *el1, __isl_keep EL *el2,
	__isl_give isl_set *(*cmp)(__isl_take EL *el1, __isl_take EL *el2))
{
	isl_set *common;
	isl_set *better;
	int is_empty;

	common = isl_set_intersect(isl_set_copy(dom1), isl_set_copy(dom2));
	is_empty = isl_set_plain_is_empty(common);
	if (is_empty >= 0 && is_empty)
		return common;
	if (is_empty < 0)
		return isl_set_free(common);
	better = cmp(FN(EL,copy)(el1), FN(EL,copy)(el2));
	better = isl_set_intersect(common, better);

	return better;
}

/* Given a function "cmp" that returns the set of elements where
 * "el1" is "better" than "el2", return a piecewise
 * expression defined on the union of the definition domains
 * of "pw1" and "pw2" that maps to the "best" of "pw1" and
 * "pw2" on each cell.  If only one of the two input functions
 * is defined on a given cell, then it is considered the best.
 */
static __isl_give PW *FN(PW,union_opt_cmp)(
	__isl_take PW *pw1, __isl_take PW *pw2,
	__isl_give isl_set *(*cmp)(__isl_take EL *el1, __isl_take EL *el2))
{
	int i, j, n;
	PW *res = NULL;
	isl_ctx *ctx;
	isl_set *set = NULL;

	if (!pw1 || !pw2)
		goto error;

	ctx = isl_space_get_ctx(pw1->dim);
	if (!isl_space_is_equal(pw1->dim, pw2->dim))
		isl_die(ctx, isl_error_invalid,
			"arguments should live in the same space", goto error);

	if (FN(PW,is_empty)(pw1)) {
		FN(PW,free)(pw1);
		return pw2;
	}

	if (FN(PW,is_empty)(pw2)) {
		FN(PW,free)(pw2);
		return pw1;
	}

	n = 2 * (pw1->n + 1) * (pw2->n + 1);
	res = FN(PW,alloc_size)(isl_space_copy(pw1->dim), n);

	for (i = 0; i < pw1->n; ++i) {
		set = isl_set_copy(pw1->p[i].set);
		for (j = 0; j < pw2->n; ++j) {
			isl_set *better;
			int is_empty;

			better = FN(PW,shared_and_better)(pw2->p[j].set,
					pw1->p[i].set, pw2->p[j].FIELD,
					pw1->p[i].FIELD, cmp);
			is_empty = isl_set_plain_is_empty(better);
			if (is_empty < 0 || is_empty) {
				isl_set_free(better);
				if (is_empty < 0)
					goto error;
				continue;
			}
			set = isl_set_subtract(set, isl_set_copy(better));

			res = FN(PW,add_piece)(res, better,
					FN(EL,copy)(pw2->p[j].FIELD));
		}
		res = FN(PW,add_piece)(res, set, FN(EL,copy)(pw1->p[i].FIELD));
	}

	for (j = 0; j < pw2->n; ++j) {
		set = isl_set_copy(pw2->p[j].set);
		for (i = 0; i < pw1->n; ++i)
			set = isl_set_subtract(set,
					isl_set_copy(pw1->p[i].set));
		res = FN(PW,add_piece)(res, set, FN(EL,copy)(pw2->p[j].FIELD));
	}

	FN(PW,free)(pw1);
	FN(PW,free)(pw2);

	return res;
error:
	FN(PW,free)(pw1);
	FN(PW,free)(pw2);
	isl_set_free(set);
	return FN(PW,free)(res);
}
