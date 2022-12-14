/*
 * Copyright 2011      Sven Verdoolaege
 * Copyright 2012-2013 Ecole Normale Superieure
 *
 * Use of this software is governed by the MIT license
 *
 * Written by Sven Verdoolaege,
 * Ecole Normale Superieure, 45 rue d’Ulm, 75230 Paris, France
 */

#include <isl_multi_macro.h>

/* Intersect the explicit domain of "multi" with "domain".
 *
 * In the case of an isl_multi_union_pw_aff object, the explicit domain
 * is allowed to have only constraints on the parameters, while
 * "domain" contains actual domain elements.  In this case,
 * "domain" is intersected with those parameter constraints and
 * then used as the explicit domain of "multi".
 */
static __isl_give MULTI(BASE) *FN(MULTI(BASE),domain_intersect)(
	__isl_take MULTI(BASE) *multi, __isl_take DOM *domain)
{
	isl_bool is_params;
	DOM *multi_dom;

	if (FN(FN(MULTI(BASE),align_params),DOMBASE)(&multi, &domain) < 0)
		goto error;
	if (FN(MULTI(BASE),check_has_explicit_domain)(multi) < 0)
		goto error;
	is_params = FN(DOM,is_params)(multi->u.dom);
	if (is_params < 0)
		goto error;
	multi_dom = FN(MULTI(BASE),get_explicit_domain)(multi);
	if (!is_params) {
		domain = FN(DOM,intersect)(multi_dom, domain);
	} else {
		isl_set *params;

		params = FN(DOM,params)(multi_dom);
		domain = FN(DOM,intersect_params)(domain, params);
	}
	multi = FN(MULTI(BASE),set_explicit_domain)(multi, domain);
	return multi;
error:
	FN(MULTI(BASE),free)(multi);
	FN(DOM,free)(domain);
	return NULL;
}

/* Intersect the domain of "multi" with "domain".
 *
 * If "multi" has an explicit domain, then only this domain
 * needs to be intersected.
 */
__isl_give MULTI(BASE) *FN(MULTI(BASE),intersect_domain)(
	__isl_take MULTI(BASE) *multi, __isl_take DOM *domain)
{
	if (FN(MULTI(BASE),check_compatible_domain)(multi, domain) < 0)
		domain = FN(DOM,free)(domain);
	if (FN(MULTI(BASE),has_explicit_domain)(multi))
		return FN(MULTI(BASE),domain_intersect)(multi, domain);
	return FN(FN(MULTI(BASE),apply),DOMBASE)(multi, domain,
					&FN(EL,intersect_domain));
}

/* Intersect the parameter domain of the explicit domain of "multi"
 * with "domain".
 */
static __isl_give MULTI(BASE) *FN(MULTI(BASE),domain_intersect_params)(
	__isl_take MULTI(BASE) *multi, __isl_take isl_set *domain)
{
	DOM *multi_dom;

	FN(MULTI(BASE),align_params_set)(&multi, &domain);

	multi_dom = FN(MULTI(BASE),get_explicit_domain)(multi);
	multi_dom = FN(DOM,intersect_params)(multi_dom, domain);
	multi = FN(MULTI(BASE),set_explicit_domain)(multi, multi_dom);

	return multi;
}

/* Intersect the parameter domain of "multi" with "domain".
 *
 * If "multi" has an explicit domain, then only this domain
 * needs to be intersected.
 */
__isl_give MULTI(BASE) *FN(MULTI(BASE),intersect_params)(
	__isl_take MULTI(BASE) *multi, __isl_take isl_set *domain)
{
	if (FN(MULTI(BASE),has_explicit_domain)(multi))
		return FN(MULTI(BASE),domain_intersect_params)(multi, domain);
	return FN(MULTI(BASE),apply_set)(multi, domain,
					&FN(EL,intersect_params));
}
