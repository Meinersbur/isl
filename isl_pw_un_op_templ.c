/*
 * Copyright 2010      INRIA Saclay
 *
 * Use of this software is governed by the MIT license
 *
 * Written by Sven Verdoolaege, INRIA Saclay - Ile-de-France,
 * Parc Club Orsay Universite, ZAC des vignes, 4 rue Jacques Monod,
 * 91893 Orsay, France
 */

#include <isl_pw_macro.h>

/* Data structure that specifies how isl_pw_*_un_op should
 * modify its input.
 *
 * "fn_base" is applied to each base expression.
 * This function is assumed to have no effect on the default value
 * (i.e., zero for those objects with a default value).
 */
S(PW,un_op_control) {
	__isl_give EL *(*fn_base)(__isl_take EL *el);
};

/* Modify "pw" based on "control".
 */
static __isl_give PW *FN(PW,un_op)(__isl_take PW *pw,
	S(PW,un_op_control) *control)
{
	isl_size n;
	int i;

	n = FN(PW,n_piece)(pw);
	if (n < 0)
		return FN(PW,free)(pw);

	for (i = 0; i < n; ++i) {
		EL *el;

		el = FN(PW,take_base_at)(pw, i);
		el = control->fn_base(el);
		pw = FN(PW,restore_base_at)(pw, i, el);
	}

	return pw;
}
