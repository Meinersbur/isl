/*
 * Copyright 2014      Ecole Normale Superieure
 *
 * Use of this software is governed by the MIT license
 *
 * Written by Sven Verdoolaege,
 * Ecole Normale Superieure, 45 rue d'Ulm, 75230 Paris, France
 */

#include <isl_multi_macro.h>

/* Apply "fn" to each of the base expressions of "multi".
 */
static __isl_give MULTI(BASE) *FN(MULTI(BASE),un_op)(
	__isl_take MULTI(BASE) *multi, __isl_give EL *(*fn)(__isl_take EL *el))
{
	int i;

	multi = FN(MULTI(BASE),cow)(multi);
	if (!multi)
		return NULL;

	for (i = 0; i < multi->n; ++i) {
		multi->u.p[i] = fn(multi->u.p[i]);
		if (!multi->u.p[i])
			return FN(MULTI(BASE),free)(multi);
	}

	return multi;
}
