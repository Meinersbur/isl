/*
 * Copyright 2010      INRIA Saclay
 *
 * Use of this software is governed by the GNU LGPLv2.1 license
 *
 * Written by Sven Verdoolaege, INRIA Saclay - Ile-de-France,
 * Parc Club Orsay Universite, ZAC des vignes, 4 rue Jacques Monod,
 * 91893 Orsay, France 
 */

#include <isl_bound.h>
#include <isl_range.h>
#include <isl_polynomial_private.h>

static int compressed_guarded_poly_bound(__isl_take isl_basic_set *bset,
	__isl_take isl_qpolynomial *poly, void *user)
{
	struct isl_bound *bound = (struct isl_bound *)user;
	return isl_qpolynomial_bound_on_domain_range(bset, poly, bound);
}

static int guarded_poly_bound(__isl_take isl_basic_set *bset,
	__isl_take isl_qpolynomial *poly, void *user)
{
	struct isl_bound *bound = (struct isl_bound *)user;
	isl_pw_qpolynomial_fold *top_pwf;
	isl_pw_qpolynomial_fold *top_pwf_tight;
	isl_dim *dim;
	isl_morph *morph;
	unsigned orig_nvar, final_nvar;
	int r;

	bset = isl_basic_set_detect_equalities(bset);

	if (!bset)
		goto error;

	if (bset->n_eq == 0)
		return compressed_guarded_poly_bound(bset, poly, user);

	orig_nvar = isl_basic_set_dim(bset, isl_dim_set);

	morph = isl_basic_set_full_compression(bset);

	bset = isl_morph_basic_set(isl_morph_copy(morph), bset);
	poly = isl_qpolynomial_morph(poly, isl_morph_copy(morph));

	final_nvar = isl_basic_set_dim(bset, isl_dim_set);

	dim = isl_morph_get_ran_dim(morph);
	dim = isl_dim_drop(dim, isl_dim_set, 0, isl_dim_size(dim, isl_dim_set));

	top_pwf = bound->pwf;
	top_pwf_tight = bound->pwf_tight;

	bound->pwf = isl_pw_qpolynomial_fold_zero(isl_dim_copy(dim));
	bound->pwf_tight = isl_pw_qpolynomial_fold_zero(dim);

	r = compressed_guarded_poly_bound(bset, poly, user);

	morph = isl_morph_remove_dom_dims(morph, isl_dim_set, 0, orig_nvar);
	morph = isl_morph_remove_ran_dims(morph, isl_dim_set, 0, final_nvar);
	morph = isl_morph_inverse(morph);

	bound->pwf = isl_pw_qpolynomial_fold_morph(bound->pwf,
							isl_morph_copy(morph));
	bound->pwf_tight = isl_pw_qpolynomial_fold_morph(bound->pwf_tight, morph);

	bound->pwf = isl_pw_qpolynomial_fold_add(top_pwf, bound->pwf);
	bound->pwf_tight = isl_pw_qpolynomial_fold_add(top_pwf_tight,
							bound->pwf_tight);

	return r;
error:
	isl_basic_set_free(bset);
	isl_qpolynomial_free(poly);
	return -1;
}

static int basic_guarded_bound(__isl_take isl_basic_set *bset, void *user)
{
	struct isl_bound *bound = (struct isl_bound *)user;
	int r;

	r = isl_qpolynomial_as_polynomial_on_domain(bound->qp, bset,
						    &guarded_poly_bound, user);
	isl_basic_set_free(bset);
	return r;
}

static int guarded_bound(__isl_take isl_set *set,
	__isl_take isl_qpolynomial *qp, void *user)
{
	struct isl_bound *bound = (struct isl_bound *)user;

	if (!set || !qp)
		goto error;

	set = isl_set_make_disjoint(set);

	bound->qp = qp;

	if (isl_set_foreach_basic_set(set, &basic_guarded_bound, bound) < 0)
		goto error;

	isl_set_free(set);
	isl_qpolynomial_free(qp);

	return 0;
error:
	isl_set_free(set);
	isl_qpolynomial_free(qp);
	return -1;
}

__isl_give isl_pw_qpolynomial_fold *isl_pw_qpolynomial_bound(
	__isl_take isl_pw_qpolynomial *pwqp, enum isl_fold type, int *tight)
{
	isl_dim *dim;
	unsigned nvar;
	struct isl_bound bound;
	int covers;

	if (!pwqp)
		return NULL;

	dim = isl_pw_qpolynomial_get_dim(pwqp);
	nvar = isl_dim_size(dim, isl_dim_set);

	if (isl_pw_qpolynomial_is_zero(pwqp)) {
		isl_pw_qpolynomial_free(pwqp);
		dim = isl_dim_drop(dim, isl_dim_set, 0, nvar);
		if (tight)
			*tight = 1;
		return isl_pw_qpolynomial_fold_zero(dim);
	}

	if (nvar == 0) {
		isl_dim_free(dim);
		if (tight)
			*tight = 1;
		return isl_pw_qpolynomial_fold_from_pw_qpolynomial(type, pwqp);
	}

	dim = isl_dim_drop(dim, isl_dim_set, 0, nvar);

	bound.pwf = isl_pw_qpolynomial_fold_zero(isl_dim_copy(dim));
	bound.pwf_tight = isl_pw_qpolynomial_fold_zero(isl_dim_copy(dim));
	bound.type = type;
	bound.check_tight = !!tight;

	if (isl_pw_qpolynomial_foreach_lifted_piece(pwqp, guarded_bound, &bound))
		goto error;

	covers = isl_pw_qpolynomial_fold_covers(bound.pwf_tight, bound.pwf);
	if (covers < 0)
		goto error;

	if (tight)
		*tight = covers;

	isl_dim_free(dim);
	isl_pw_qpolynomial_free(pwqp);

	if (covers) {
		isl_pw_qpolynomial_fold_free(bound.pwf);
		return bound.pwf_tight;
	}

	bound.pwf = isl_pw_qpolynomial_fold_add(bound.pwf, bound.pwf_tight);

	return bound.pwf;
error:
	isl_pw_qpolynomial_fold_free(bound.pwf_tight);
	isl_pw_qpolynomial_fold_free(bound.pwf);
	isl_dim_free(dim);
	isl_pw_qpolynomial_free(pwqp);
	return NULL;
}
