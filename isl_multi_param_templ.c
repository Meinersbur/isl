/*
 * Use of this software is governed by the MIT license
 *
 * Written by Sven Verdoolaege
 */

#include <isl_multi_macro.h>

/* Does the multiple expression "multi" depend in any way
 * on the parameter with identifier "id"?
 */
isl_bool FN(MULTI(BASE),involves_param_id)(__isl_keep MULTI(BASE) *multi,
	__isl_keep isl_id *id)
{
	int i;
	int pos;

	if (!multi || !id)
		return isl_bool_error;
	if (multi->n == 0)
		return isl_bool_false;
	pos = FN(MULTI(BASE),find_dim_by_id)(multi, isl_dim_param, id);
	if (pos < 0)
		return isl_bool_false;

	for (i = 0; i < multi->n; ++i) {
		isl_bool involved = FN(EL,involves_param_id)(multi->u.p[i], id);
		if (involved < 0 || involved)
			return involved;
	}

	return isl_bool_false;
}
