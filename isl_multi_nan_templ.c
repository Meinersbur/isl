/*
 * Copyright 2014      Ecole Normale Superieure
 *
 * Use of this software is governed by the MIT license
 *
 * Written by Sven Verdoolaege,
 * Ecole Normale Superieure, 45 rue d'Ulm, 75230 Paris, France
 */

#include <isl_multi_macro.h>

/* Does "multi" involve any NaNs?
 */
isl_bool FN(MULTI(BASE),involves_nan)(__isl_keep MULTI(BASE) *multi)
{
	int i;

	if (!multi)
		return isl_bool_error;
	if (multi->n == 0)
		return isl_bool_false;

	for (i = 0; i < multi->n; ++i) {
		isl_bool has_nan = FN(EL,involves_nan)(multi->u.p[i]);
		if (has_nan < 0 || has_nan)
			return has_nan;
	}

	return isl_bool_false;
}
