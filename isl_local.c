/*
 * Copyright 2014      Ecole Normale Superieure
 *
 * Use of this software is governed by the MIT license
 *
 * Written by Sven Verdoolaege,
 * Ecole Normale Superieure, 45 rue d'Ulm, 75230 Paris, France
 */

#include <isl_mat_private.h>

/* Given a matrix "div" representing local variables,
 * does the variable at position "pos" have an explicit representation?
 */
isl_bool isl_local_div_is_known(__isl_keep isl_mat *div, int pos)
{
	if (!div)
		return isl_bool_error;
	if (pos < 0 || pos >= div->n_row)
		isl_die(isl_mat_get_ctx(div), isl_error_invalid,
			"position out of bounds", return isl_bool_error);
	return !isl_int_is_zero(div->row[pos][0]);
}
