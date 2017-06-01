/*
 * Copyright 2012      Ecole Normale Superieure
 *
 * Use of this software is governed by the MIT license
 *
 * Written by Sven Verdoolaege,
 * Ecole Normale Superieure, 45 rue d'Ulm, 75230 Paris, France
 */

#include <isl_pw_macro.h>

static __isl_give PW *FN(PW,align_params_pw_multi_aff_and)(__isl_take PW *pw,
	__isl_take isl_multi_aff *ma,
	__isl_give PW *(*fn)(__isl_take PW *pw, __isl_take isl_multi_aff *ma))
{
	isl_ctx *ctx;
	isl_bool equal_params;
	isl_space *ma_space;

	ma_space = isl_multi_aff_get_space(ma);
	if (!pw || !ma || !ma_space)
		goto error;
	equal_params = isl_space_has_equal_params(pw->dim, ma_space);
	if (equal_params < 0)
		goto error;
	if (equal_params) {
		isl_space_free(ma_space);
		return fn(pw, ma);
	}
	ctx = FN(PW,get_ctx)(pw);
	if (FN(PW,check_named_params)(pw) < 0)
		goto error;
	if (!isl_space_has_named_params(ma_space))
		isl_die(ctx, isl_error_invalid,
			"unaligned unnamed parameters", goto error);
	pw = FN(PW,align_params)(pw, ma_space);
	ma = isl_multi_aff_align_params(ma, FN(PW,get_space)(pw));
	return fn(pw, ma);
error:
	isl_space_free(ma_space);
	FN(PW,free)(pw);
	isl_multi_aff_free(ma);
	return NULL;
}

static __isl_give PW *FN(PW,align_params_pw_pw_multi_aff_and)(__isl_take PW *pw,
	__isl_take isl_pw_multi_aff *pma,
	__isl_give PW *(*fn)(__isl_take PW *pw,
		__isl_take isl_pw_multi_aff *ma))
{
	isl_bool equal_params;
	isl_space *pma_space;

	pma_space = isl_pw_multi_aff_get_space(pma);
	if (!pw || !pma || !pma_space)
		goto error;
	equal_params = isl_space_has_equal_params(pw->dim, pma_space);
	if (equal_params < 0)
		goto error;
	if (equal_params) {
		isl_space_free(pma_space);
		return fn(pw, pma);
	}
	if (FN(PW,check_named_params)(pw) < 0 ||
	    isl_pw_multi_aff_check_named_params(pma) < 0)
		goto error;
	pw = FN(PW,align_params)(pw, pma_space);
	pma = isl_pw_multi_aff_align_params(pma, FN(PW,get_space)(pw));
	return fn(pw, pma);
error:
	isl_space_free(pma_space);
	FN(PW,free)(pw);
	isl_pw_multi_aff_free(pma);
	return NULL;
}

/* Compute the pullback of "pw" by the function represented by "ma".
 * In other words, plug in "ma" in "pw".
 */
static __isl_give PW *FN(PW,pullback_multi_aff_aligned)(__isl_take PW *pw,
	__isl_take isl_multi_aff *ma)
{
	int i;
	isl_space *space = NULL;

	ma = isl_multi_aff_align_divs(ma);
	pw = FN(PW,cow)(pw);
	if (!pw || !ma)
		goto error;

	space = isl_space_join(isl_multi_aff_get_space(ma),
				FN(PW,get_space)(pw));

	for (i = 0; i < pw->n; ++i) {
		pw->p[i].set = isl_set_preimage_multi_aff(pw->p[i].set,
						    isl_multi_aff_copy(ma));
		if (!pw->p[i].set)
			goto error;
		pw->p[i].FIELD = FN(EL,pullback_multi_aff)(pw->p[i].FIELD,
						    isl_multi_aff_copy(ma));
		if (!pw->p[i].FIELD)
			goto error;
	}

	pw = FN(PW,reset_space)(pw, space);
	isl_multi_aff_free(ma);
	return pw;
error:
	isl_space_free(space);
	isl_multi_aff_free(ma);
	FN(PW,free)(pw);
	return NULL;
}

__isl_give PW *FN(PW,pullback_multi_aff)(__isl_take PW *pw,
	__isl_take isl_multi_aff *ma)
{
	return FN(PW,align_params_pw_multi_aff_and)(pw, ma,
					&FN(PW,pullback_multi_aff_aligned));
}

/* Compute the pullback of "pw" by the function represented by "pma".
 * In other words, plug in "pma" in "pw".
 */
static __isl_give PW *FN(PW,pullback_pw_multi_aff_aligned)(__isl_take PW *pw,
	__isl_take isl_pw_multi_aff *pma)
{
	int i;
	PW *res;

	if (!pma)
		goto error;

	if (pma->n == 0) {
		isl_space *space;
		space = isl_space_join(isl_pw_multi_aff_get_space(pma),
					FN(PW,get_space)(pw));
		isl_pw_multi_aff_free(pma);
		res = FN(PW,empty)(space);
		FN(PW,free)(pw);
		return res;
	}

	res = FN(PW,pullback_multi_aff)(FN(PW,copy)(pw),
					isl_multi_aff_copy(pma->p[0].maff));
	res = FN(PW,intersect_domain)(res, isl_set_copy(pma->p[0].set));

	for (i = 1; i < pma->n; ++i) {
		PW *res_i;

		res_i = FN(PW,pullback_multi_aff)(FN(PW,copy)(pw),
					isl_multi_aff_copy(pma->p[i].maff));
		res_i = FN(PW,intersect_domain)(res_i,
					isl_set_copy(pma->p[i].set));
		res = FN(PW,add_disjoint)(res, res_i);
	}

	isl_pw_multi_aff_free(pma);
	FN(PW,free)(pw);
	return res;
error:
	isl_pw_multi_aff_free(pma);
	FN(PW,free)(pw);
	return NULL;
}

__isl_give PW *FN(PW,pullback_pw_multi_aff)(__isl_take PW *pw,
	__isl_take isl_pw_multi_aff *pma)
{
	return FN(PW,align_params_pw_pw_multi_aff_and)(pw, pma,
					&FN(PW,pullback_pw_multi_aff_aligned));
}
