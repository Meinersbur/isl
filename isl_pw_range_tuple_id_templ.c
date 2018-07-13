/*
 * Copyright 2018      Sven Verdoolaege
 *
 * Use of this software is governed by the MIT license
 *
 * Written by Sven Verdoolaege
 */

/* Return the identifier of the (range) tuple of "pw", assuming it has one.
 *
 * Technically, the implementation should use isl_dim_set if "pw"
 * lives in a set space and isl_dim_out if it lives in a map space.
 * Internally, however, it can be assumed that isl_dim_set is equal
 * to isl_dim_out.
 */
__isl_give isl_id *FN(PW,get_range_tuple_id)(__isl_keep PW *pw)
{
	return FN(PW,get_tuple_id)(pw, isl_dim_out);
}
