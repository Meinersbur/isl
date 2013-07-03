/*
 * Copyright 2012      Ecole Normale Superieure
 *
 * Use of this software is governed by the MIT license
 *
 * Written by Sven Verdoolaege,
 * Ecole Normale Superieure, 45 rue d’Ulm, 75230 Paris, France
 */

#include <isl/ilp.h>
#include <isl_ast_build_expr.h>
#include <isl_ast_private.h>
#include <isl_ast_build_private.h>

/* Compute the "opposite" of the (numerator of the) argument of a div
 * with denonimator "d".
 *
 * In particular, compute
 *
 *	-aff + (d - 1)
 */
static __isl_give isl_aff *oppose_div_arg(__isl_take isl_aff *aff,
	__isl_take isl_val *d)
{
	aff = isl_aff_neg(aff);
	aff = isl_aff_add_constant_val(aff, d);
	aff = isl_aff_add_constant_si(aff, -1);

	return aff;
}

/* Create an isl_ast_expr evaluating the div at position "pos" in "ls".
 * The result is simplified in terms of build->domain.
 *
 * *change_sign is set by this function if the sign of
 * the expression has changed.
 * "ls" is known to be non-NULL.
 *
 * Let the div be of the form floor(e/d).
 * If the ast_build_prefer_pdiv option is set then we check if "e"
 * is non-negative, so that we can generate
 *
 *	(pdiv_q, expr(e), expr(d))
 *
 * instead of
 *
 *	(fdiv_q, expr(e), expr(d))
 *
 * If the ast_build_prefer_pdiv option is set and
 * if "e" is not non-negative, then we check if "-e + d - 1" is non-negative.
 * If so, we can rewrite
 *
 *	floor(e/d) = -ceil(-e/d) = -floor((-e + d - 1)/d)
 *
 * and still use pdiv_q.
 */
static __isl_give isl_ast_expr *var_div(int *change_sign,
	__isl_keep isl_local_space *ls,
	int pos, __isl_keep isl_ast_build *build)
{
	isl_ctx *ctx = isl_local_space_get_ctx(ls);
	isl_aff *aff;
	isl_ast_expr *num, *den;
	isl_val *d;
	enum isl_ast_op_type type;

	aff = isl_local_space_get_div(ls, pos);
	d = isl_aff_get_denominator_val(aff);
	aff = isl_aff_scale_val(aff, isl_val_copy(d));
	den = isl_ast_expr_from_val(isl_val_copy(d));

	type = isl_ast_op_fdiv_q;
	if (isl_options_get_ast_build_prefer_pdiv(ctx)) {
		int non_neg = isl_ast_build_aff_is_nonneg(build, aff);
		if (non_neg >= 0 && !non_neg) {
			isl_aff *opp = oppose_div_arg(isl_aff_copy(aff),
							isl_val_copy(d));
			non_neg = isl_ast_build_aff_is_nonneg(build, opp);
			if (non_neg >= 0 && non_neg) {
				*change_sign = 1;
				isl_aff_free(aff);
				aff = opp;
			} else
				isl_aff_free(opp);
		}
		if (non_neg < 0)
			aff = isl_aff_free(aff);
		else if (non_neg)
			type = isl_ast_op_pdiv_q;
	}

	isl_val_free(d);
	num = isl_ast_expr_from_aff(aff, build);
	return isl_ast_expr_alloc_binary(type, num, den);
}

/* Create an isl_ast_expr evaluating the specified dimension of "ls".
 * The result is simplified in terms of build->domain.
 *
 * *change_sign is set by this function if the sign of
 * the expression has changed.
 *
 * The isl_ast_expr is constructed based on the type of the dimension.
 * - divs are constructed by var_div
 * - set variables are constructed from the iterator isl_ids in "build"
 * - parameters are constructed from the isl_ids in "ls"
 */
static __isl_give isl_ast_expr *var(int *change_sign,
	__isl_keep isl_local_space *ls,
	enum isl_dim_type type, int pos, __isl_keep isl_ast_build *build)
{
	isl_ctx *ctx = isl_local_space_get_ctx(ls);
	isl_id *id;

	if (type == isl_dim_div)
		return var_div(change_sign, ls, pos, build);

	if (type == isl_dim_set) {
		id = isl_ast_build_get_iterator_id(build, pos);
		return isl_ast_expr_from_id(id);
	}

	if (!isl_local_space_has_dim_id(ls, type, pos))
		isl_die(ctx, isl_error_internal, "unnamed dimension",
			return NULL);
	id = isl_local_space_get_dim_id(ls, type, pos);
	return isl_ast_expr_from_id(id);
}

/* Does "expr" represent the zero integer?
 */
static int ast_expr_is_zero(__isl_keep isl_ast_expr *expr)
{
	if (!expr)
		return -1;
	if (expr->type != isl_ast_expr_int)
		return 0;
	return isl_val_is_zero(expr->u.v);
}

/* Create an expression representing the sum of "expr1" and "expr2",
 * provided neither of the two expressions is identically zero.
 */
static __isl_give isl_ast_expr *ast_expr_add(__isl_take isl_ast_expr *expr1,
	__isl_take isl_ast_expr *expr2)
{
	if (!expr1 || !expr2)
		goto error;

	if (ast_expr_is_zero(expr1)) {
		isl_ast_expr_free(expr1);
		return expr2;
	}

	if (ast_expr_is_zero(expr2)) {
		isl_ast_expr_free(expr2);
		return expr1;
	}

	return isl_ast_expr_add(expr1, expr2);
error:
	isl_ast_expr_free(expr1);
	isl_ast_expr_free(expr2);
	return NULL;
}

/* Subtract expr2 from expr1.
 *
 * If expr2 is zero, we simply return expr1.
 * If expr1 is zero, we return
 *
 *	(isl_ast_op_minus, expr2)
 *
 * Otherwise, we return
 *
 *	(isl_ast_op_sub, expr1, expr2)
 */
static __isl_give isl_ast_expr *ast_expr_sub(__isl_take isl_ast_expr *expr1,
	__isl_take isl_ast_expr *expr2)
{
	if (!expr1 || !expr2)
		goto error;

	if (ast_expr_is_zero(expr2)) {
		isl_ast_expr_free(expr2);
		return expr1;
	}

	if (ast_expr_is_zero(expr1)) {
		isl_ast_expr_free(expr1);
		return isl_ast_expr_neg(expr2);
	}

	return isl_ast_expr_sub(expr1, expr2);
error:
	isl_ast_expr_free(expr1);
	isl_ast_expr_free(expr2);
	return NULL;
}

/* Return an isl_ast_expr that represents
 *
 *	v * (aff mod d)
 *
 * v is assumed to be non-negative.
 * The result is simplified in terms of build->domain.
 */
static __isl_give isl_ast_expr *isl_ast_expr_mod(__isl_keep isl_val *v,
	__isl_keep isl_aff *aff, __isl_keep isl_val *d,
	__isl_keep isl_ast_build *build)
{
	isl_ctx *ctx;
	isl_ast_expr *expr;
	isl_ast_expr *c;

	if (!aff)
		return NULL;

	ctx = isl_aff_get_ctx(aff);
	expr = isl_ast_expr_from_aff(isl_aff_copy(aff), build);

	c = isl_ast_expr_from_val(isl_val_copy(d));
	expr = isl_ast_expr_alloc_binary(isl_ast_op_pdiv_r, expr, c);

	if (!isl_val_is_one(v)) {
		c = isl_ast_expr_from_val(isl_val_copy(v));
		expr = isl_ast_expr_mul(c, expr);
	}

	return expr;
}

/* Create an isl_ast_expr that scales "expr" by "v".
 *
 * If v is 1, we simply return expr.
 * If v is -1, we return
 *
 *	(isl_ast_op_minus, expr)
 *
 * Otherwise, we return
 *
 *	(isl_ast_op_mul, expr(v), expr)
 */
static __isl_give isl_ast_expr *scale(__isl_take isl_ast_expr *expr,
	__isl_take isl_val *v)
{
	isl_ast_expr *c;

	if (!expr || !v)
		goto error;
	if (isl_val_is_one(v)) {
		isl_val_free(v);
		return expr;
	}

	if (isl_val_is_negone(v)) {
		isl_val_free(v);
		expr = isl_ast_expr_neg(expr);
	} else {
		c = isl_ast_expr_from_val(v);
		expr = isl_ast_expr_mul(c, expr);
	}

	return expr;
error:
	isl_val_free(v);
	isl_ast_expr_free(expr);
	return NULL;
}

/* Add an expression for "*v" times the specified dimension of "ls"
 * to expr.
 *
 * Let e be the expression for the specified dimension,
 * multiplied by the absolute value of "*v".
 * If "*v" is negative, we create
 *
 *	(isl_ast_op_sub, expr, e)
 *
 * except when expr is trivially zero, in which case we create
 *
 *	(isl_ast_op_minus, e)
 *
 * instead.
 *
 * If "*v" is positive, we simply create
 *
 *	(isl_ast_op_add, expr, e)
 *
 */
static __isl_give isl_ast_expr *isl_ast_expr_add_term(
	__isl_take isl_ast_expr *expr,
	__isl_keep isl_local_space *ls, enum isl_dim_type type, int pos,
	__isl_take isl_val *v, __isl_keep isl_ast_build *build)
{
	isl_ast_expr *term;
	int change_sign;

	if (!expr)
		return NULL;

	change_sign = 0;
	term = var(&change_sign, ls, type, pos, build);
	if (change_sign)
		v = isl_val_neg(v);

	if (isl_val_is_neg(v) && !ast_expr_is_zero(expr)) {
		v = isl_val_neg(v);
		term = scale(term, v);
		return ast_expr_sub(expr, term);
	} else {
		term = scale(term, v);
		return ast_expr_add(expr, term);
	}
}

/* Add an expression for "v" to expr.
 */
static __isl_give isl_ast_expr *isl_ast_expr_add_int(
	__isl_take isl_ast_expr *expr, __isl_take isl_val *v)
{
	isl_ctx *ctx;
	isl_ast_expr *expr_int;

	if (!expr || !v)
		goto error;

	if (isl_val_is_zero(v)) {
		isl_val_free(v);
		return expr;
	}

	ctx = isl_ast_expr_get_ctx(expr);
	if (isl_val_is_neg(v) && !ast_expr_is_zero(expr)) {
		v = isl_val_neg(v);
		expr_int = isl_ast_expr_from_val(v);
		return ast_expr_sub(expr, expr_int);
	} else {
		expr_int = isl_ast_expr_from_val(v);
		return ast_expr_add(expr, expr_int);
	}
error:
	isl_ast_expr_free(expr);
	isl_val_free(v);
	return NULL;
}

/* Check if "aff" involves any (implicit) modulo computations based
 * on div "j".
 * If so, remove them from aff and add expressions corresponding
 * to those modulo computations to *pos and/or *neg.
 * "v" is the coefficient of div "j".
 *
 * In particular, check if (v * div_j) / d is of the form
 *
 *	(f * m * floor(a / m)) / d
 *
 * and, if so, rewrite it as
 *
 *	(f * (a - (a mod m))) / d = (f * a) / d - (f * (a mod m)) / d
 *
 * and extract out -f * (a mod m).
 * In particular, if f > 0, we add (f * (a mod m)) to *neg.
 * If f < 0, we add ((-f) * (a mod m)) to *pos.
 *
 * Note that in order to represent "a mod m" as
 *
 *	(isl_ast_op_pdiv_r, a, m)
 *
 * we need to make sure that a is non-negative.
 * If not, we check if "-a + m - 1" is non-negative.
 * If so, we can rewrite
 *
 *	floor(a/m) = -ceil(-a/m) = -floor((-a + m - 1)/m)
 *
 * and still extract a modulo.
 *
 * The caller is responsible for dividing *neg and/or *pos by d.
 */
static __isl_give isl_aff *extract_modulo(__isl_take isl_aff *aff,
	__isl_keep isl_ast_expr **pos, __isl_keep isl_ast_expr **neg,
	__isl_keep isl_ast_build *build, int j, __isl_take isl_val *v)
{
	isl_ast_expr *expr;
	isl_aff *div;
	int s;
	int mod;
	isl_val *d;

	div = isl_aff_get_div(aff, j);
	d = isl_aff_get_denominator_val(div);
	mod = isl_val_is_divisible_by(v, d);
	if (mod) {
		div = isl_aff_scale_val(div, isl_val_copy(d));
		mod = isl_ast_build_aff_is_nonneg(build, div);
		if (mod >= 0 && !mod) {
			isl_aff *opp = oppose_div_arg(isl_aff_copy(div),
							isl_val_copy(d));
			mod = isl_ast_build_aff_is_nonneg(build, opp);
			if (mod >= 0 && mod) {
				isl_aff_free(div);
				div = opp;
				v = isl_val_neg(v);
			} else
				isl_aff_free(opp);
		}
	}
	if (mod < 0) {
		isl_aff_free(div);
		isl_val_free(d);
		isl_val_free(v);
		return isl_aff_free(aff);
	} else if (!mod) {
		isl_aff_free(div);
		isl_val_free(d);
		isl_val_free(v);
		return aff;
	}
	v = isl_val_div(v, isl_val_copy(d));
	s = isl_val_sgn(v);
	v = isl_val_abs(v);
	expr = isl_ast_expr_mod(v, div, d, build);
	isl_val_free(d);
	if (s > 0)
		*neg = ast_expr_add(*neg, expr);
	else
		*pos = ast_expr_add(*pos, expr);
	aff = isl_aff_set_coefficient_si(aff, isl_dim_div, j, 0);
	if (s < 0)
		v = isl_val_neg(v);
	div = isl_aff_scale_val(div, v);
	d = isl_aff_get_denominator_val(aff);
	div = isl_aff_scale_down_val(div, d);
	aff = isl_aff_add(aff, div);

	return aff;
}

/* Check if "aff" involves any (implicit) modulo computations.
 * If so, remove them from aff and add expressions corresponding
 * to those modulo computations to *pos and/or *neg.
 * We only do this if the option ast_build_prefer_pdiv is set.
 *
 * "aff" is assumed to be an integer affine expression.
 *
 * A modulo expression is of the form
 *
 *	a mod m = a - m * floor(a / m)
 *
 * To detect them in aff, we look for terms of the form
 *
 *	f * m * floor(a / m)
 *
 * rewrite them as
 *
 *	f * (a - (a mod m)) = f * a - f * (a mod m)
 *
 * and extract out -f * (a mod m).
 * In particular, if f > 0, we add (f * (a mod m)) to *neg.
 * If f < 0, we add ((-f) * (a mod m)) to *pos.
 */
static __isl_give isl_aff *extract_modulos(__isl_take isl_aff *aff,
	__isl_keep isl_ast_expr **pos, __isl_keep isl_ast_expr **neg,
	__isl_keep isl_ast_build *build)
{
	isl_ctx *ctx;
	int j, n;

	if (!aff)
		return NULL;

	ctx = isl_aff_get_ctx(aff);
	if (!isl_options_get_ast_build_prefer_pdiv(ctx))
		return aff;

	n = isl_aff_dim(aff, isl_dim_div);
	for (j = 0; j < n; ++j) {
		isl_val *v;

		v = isl_aff_get_coefficient_val(aff, isl_dim_div, j);
		if (!v)
			return isl_aff_free(aff);
		if (isl_val_is_zero(v) ||
		    isl_val_is_one(v) || isl_val_is_negone(v)) {
			isl_val_free(v);
			continue;
		}
		aff = extract_modulo(aff, pos, neg, build, j, v);
		if (!aff)
			break;
	}

	return aff;
}

/* Construct an isl_ast_expr that evaluates the affine expression "aff",
 * The result is simplified in terms of build->domain.
 *
 * We first extract hidden modulo computations from the affine expression
 * and then add terms for each variable with a non-zero coefficient.
 * Finally, if the affine expression has a non-trivial denominator,
 * we divide the resulting isl_ast_expr by this denominator.
 */
__isl_give isl_ast_expr *isl_ast_expr_from_aff(__isl_take isl_aff *aff,
	__isl_keep isl_ast_build *build)
{
	int i, j;
	int n;
	isl_val *v, *d;
	isl_ctx *ctx = isl_aff_get_ctx(aff);
	isl_ast_expr *expr, *expr_neg;
	enum isl_dim_type t[] = { isl_dim_param, isl_dim_in, isl_dim_div };
	enum isl_dim_type l[] = { isl_dim_param, isl_dim_set, isl_dim_div };
	isl_local_space *ls;

	if (!aff)
		return NULL;

	expr = isl_ast_expr_alloc_int_si(ctx, 0);
	expr_neg = isl_ast_expr_alloc_int_si(ctx, 0);

	d = isl_aff_get_denominator_val(aff);
	aff = isl_aff_scale_val(aff, isl_val_copy(d));

	aff = extract_modulos(aff, &expr, &expr_neg, build);
	expr = ast_expr_sub(expr, expr_neg);

	ls = isl_aff_get_domain_local_space(aff);

	for (i = 0; i < 3; ++i) {
		n = isl_aff_dim(aff, t[i]);
		for (j = 0; j < n; ++j) {
			v = isl_aff_get_coefficient_val(aff, t[i], j);
			if (!v)
				expr = isl_ast_expr_free(expr);
			if (isl_val_is_zero(v)) {
				isl_val_free(v);
				continue;
			}
			expr = isl_ast_expr_add_term(expr,
							ls, l[i], j, v, build);
		}
	}

	v = isl_aff_get_constant_val(aff);
	expr = isl_ast_expr_add_int(expr, v);

	if (!isl_val_is_one(d))
		expr = isl_ast_expr_div(expr, isl_ast_expr_from_val(d));
	else
		isl_val_free(d);

	isl_local_space_free(ls);
	isl_aff_free(aff);
	return expr;
}

/* Add terms to "expr" for each variable in "aff" with a coefficient
 * with sign equal to "sign".
 * The result is simplified in terms of build->domain.
 */
static __isl_give isl_ast_expr *add_signed_terms(__isl_take isl_ast_expr *expr,
	__isl_keep isl_aff *aff, int sign, __isl_keep isl_ast_build *build)
{
	int i, j;
	isl_val *v;
	enum isl_dim_type t[] = { isl_dim_param, isl_dim_in, isl_dim_div };
	enum isl_dim_type l[] = { isl_dim_param, isl_dim_set, isl_dim_div };
	isl_local_space *ls;

	ls = isl_aff_get_domain_local_space(aff);

	for (i = 0; i < 3; ++i) {
		int n = isl_aff_dim(aff, t[i]);
		for (j = 0; j < n; ++j) {
			v = isl_aff_get_coefficient_val(aff, t[i], j);
			if (sign * isl_val_sgn(v) <= 0) {
				isl_val_free(v);
				continue;
			}
			v = isl_val_abs(v);
			expr = isl_ast_expr_add_term(expr,
						ls, l[i], j, v, build);
		}
	}

	isl_local_space_free(ls);

	return expr;
}

/* Should the constant term "v" be considered positive?
 *
 * A positive constant will be added to "pos" by the caller,
 * while a negative constant will be added to "neg".
 * If either "pos" or "neg" is exactly zero, then we prefer
 * to add the constant "v" to that side, irrespective of the sign of "v".
 * This results in slightly shorter expressions and may reduce the risk
 * of overflows.
 */
static int constant_is_considered_positive(__isl_keep isl_val *v,
	__isl_keep isl_ast_expr *pos, __isl_keep isl_ast_expr *neg)
{
	if (ast_expr_is_zero(pos))
		return 1;
	if (ast_expr_is_zero(neg))
		return 0;
	return isl_val_is_pos(v);
}

/* Construct an isl_ast_expr that evaluates the condition "constraint",
 * The result is simplified in terms of build->domain.
 *
 * Let the constraint by either "a >= 0" or "a == 0".
 * We first extract hidden modulo computations from "a"
 * and then collect all the terms with a positive coefficient in cons_pos
 * and the terms with a negative coefficient in cons_neg.
 *
 * The result is then of the form
 *
 *	(isl_ast_op_ge, expr(pos), expr(-neg)))
 *
 * or
 *
 *	(isl_ast_op_eq, expr(pos), expr(-neg)))
 *
 * However, if the first expression is an integer constant (and the second
 * is not), then we swap the two expressions.  This ensures that we construct,
 * e.g., "i <= 5" rather than "5 >= i".
 *
 * Furthermore, is there are no terms with positive coefficients (or no terms
 * with negative coefficients), then the constant term is added to "pos"
 * (or "neg"), ignoring the sign of the constant term.
 */
static __isl_give isl_ast_expr *isl_ast_expr_from_constraint(
	__isl_take isl_constraint *constraint, __isl_keep isl_ast_build *build)
{
	isl_ctx *ctx;
	isl_ast_expr *expr_pos;
	isl_ast_expr *expr_neg;
	isl_ast_expr *expr;
	isl_aff *aff;
	isl_val *v;
	int eq;
	enum isl_ast_op_type type;

	if (!constraint)
		return NULL;

	aff = isl_constraint_get_aff(constraint);

	ctx = isl_constraint_get_ctx(constraint);
	expr_pos = isl_ast_expr_alloc_int_si(ctx, 0);
	expr_neg = isl_ast_expr_alloc_int_si(ctx, 0);

	aff = extract_modulos(aff, &expr_pos, &expr_neg, build);

	expr_pos = add_signed_terms(expr_pos, aff, 1, build);
	expr_neg = add_signed_terms(expr_neg, aff, -1, build);

	v = isl_aff_get_constant_val(aff);
	if (constant_is_considered_positive(v, expr_pos, expr_neg)) {
		expr_pos = isl_ast_expr_add_int(expr_pos, v);
	} else {
		v = isl_val_neg(v);
		expr_neg = isl_ast_expr_add_int(expr_neg, v);
	}

	eq = isl_constraint_is_equality(constraint);

	if (isl_ast_expr_get_type(expr_pos) == isl_ast_expr_int &&
	    isl_ast_expr_get_type(expr_neg) != isl_ast_expr_int) {
		type = eq ? isl_ast_op_eq : isl_ast_op_le;
		expr = isl_ast_expr_alloc_binary(type, expr_neg, expr_pos);
	} else {
		type = eq ? isl_ast_op_eq : isl_ast_op_ge;
		expr = isl_ast_expr_alloc_binary(type, expr_pos, expr_neg);
	}

	isl_constraint_free(constraint);
	isl_aff_free(aff);
	return expr;
}

struct isl_expr_from_basic_data {
	isl_ast_build *build;
	int first;
	isl_ast_expr *res;
};

/* Construct an isl_ast_expr that evaluates the condition "c",
 * except if it is a div constraint, and add it to the data->res.
 * The result is simplified in terms of data->build->domain.
 */
static int expr_from_basic_set(__isl_take isl_constraint *c, void *user)
{
	struct isl_expr_from_basic_data *data = user;
	isl_ast_expr *expr;

	if (isl_constraint_is_div_constraint(c)) {
		isl_constraint_free(c);
		return 0;
	}

	expr = isl_ast_expr_from_constraint(c, data->build);
	if (data->first)
		data->res = expr;
	else
		data->res = isl_ast_expr_and(data->res, expr);

	data->first = 0;

	if (!data->res)
		return -1;
	return 0;
}

/* Construct an isl_ast_expr that evaluates the conditions defining "bset".
 * The result is simplified in terms of build->domain.
 *
 * We filter out the div constraints during printing, so we do not know
 * in advance how many constraints are going to be printed.
 *
 * If it turns out that there was no constraint, then we contruct
 * the expression "1", i.e., "true".
 */
__isl_give isl_ast_expr *isl_ast_build_expr_from_basic_set(
	 __isl_keep isl_ast_build *build, __isl_take isl_basic_set *bset)
{
	struct isl_expr_from_basic_data data = { build, 1, NULL };

	if (isl_basic_set_foreach_constraint(bset,
					    &expr_from_basic_set, &data) < 0) {
		data.res = isl_ast_expr_free(data.res);
	} else if (data.res == NULL) {
		isl_ctx *ctx = isl_basic_set_get_ctx(bset);
		data.res = isl_ast_expr_alloc_int_si(ctx, 1);
	}

	isl_basic_set_free(bset);
	return data.res;
}

struct isl_expr_from_set_data {
	isl_ast_build *build;
	int first;
	isl_ast_expr *res;
};

/* Construct an isl_ast_expr that evaluates the conditions defining "bset"
 * and add it to data->res.
 * The result is simplified in terms of data->build->domain.
 */
static int expr_from_set(__isl_take isl_basic_set *bset, void *user)
{
	struct isl_expr_from_set_data *data = user;
	isl_ast_expr *expr;

	expr = isl_ast_build_expr_from_basic_set(data->build, bset);
	if (data->first)
		data->res = expr;
	else
		data->res = isl_ast_expr_or(data->res, expr);

	data->first = 0;

	if (!data->res)
		return -1;
	return 0;
}

/* Construct an isl_ast_expr that evaluates the conditions defining "set".
 * The result is simplified in terms of build->domain.
 */
__isl_give isl_ast_expr *isl_ast_build_expr_from_set(
	__isl_keep isl_ast_build *build, __isl_take isl_set *set)
{
	struct isl_expr_from_set_data data = { build, 1, NULL };

	if (isl_set_foreach_basic_set(set, &expr_from_set, &data) < 0)
		data.res = isl_ast_expr_free(data.res);

	isl_set_free(set);
	return data.res;
}

struct isl_from_pw_aff_data {
	isl_ast_build *build;
	int n;
	isl_ast_expr **next;
	isl_set *dom;
};

/* This function is called during the construction of an isl_ast_expr
 * that evaluates an isl_pw_aff.
 * Adjust data->next to take into account this piece.
 *
 * data->n is the number of pairs of set and aff to go.
 * data->dom is the domain of the entire isl_pw_aff.
 *
 * If this is the last pair, then data->next is set to evaluate aff
 * and the domain is ignored.
 * Otherwise, data->next is set to a select operation that selects
 * an isl_ast_expr correponding to "aff" on "set" and to an expression
 * that will be filled in by later calls otherwise.
 */
static int ast_expr_from_pw_aff(__isl_take isl_set *set,
	__isl_take isl_aff *aff, void *user)
{
	struct isl_from_pw_aff_data *data = user;
	isl_ctx *ctx;

	ctx = isl_set_get_ctx(set);
	data->n--;
	if (data->n == 0) {
		*data->next = isl_ast_expr_from_aff(aff, data->build);
		isl_set_free(set);
		if (!*data->next)
			return -1;
	} else {
		isl_ast_expr *ternary, *arg;

		ternary = isl_ast_expr_alloc_op(ctx, isl_ast_op_select, 3);
		set = isl_set_gist(set, isl_set_copy(data->dom));
		arg = isl_ast_build_expr_from_set(data->build, set);
		ternary = isl_ast_expr_set_op_arg(ternary, 0, arg);
		arg = isl_ast_expr_from_aff(aff, data->build);
		ternary = isl_ast_expr_set_op_arg(ternary, 1, arg);
		if (!ternary)
			return -1;

		*data->next = ternary;
		data->next = &ternary->u.op.args[2];
	}

	return 0;
}

/* Construct an isl_ast_expr that evaluates "pa".
 * The result is simplified in terms of build->domain.
 *
 * The domain of "pa" lives in the internal schedule space.
 */
__isl_give isl_ast_expr *isl_ast_build_expr_from_pw_aff_internal(
	__isl_keep isl_ast_build *build, __isl_take isl_pw_aff *pa)
{
	struct isl_from_pw_aff_data data;
	isl_ast_expr *res = NULL;

	if (!pa)
		return NULL;

	data.build = build;
	data.n = isl_pw_aff_n_piece(pa);
	data.next = &res;
	data.dom = isl_pw_aff_domain(isl_pw_aff_copy(pa));

	if (isl_pw_aff_foreach_piece(pa, &ast_expr_from_pw_aff, &data) < 0)
		res = isl_ast_expr_free(res);
	else if (!res)
		isl_die(isl_pw_aff_get_ctx(pa), isl_error_invalid,
			"cannot handle void expression", res = NULL);

	isl_pw_aff_free(pa);
	isl_set_free(data.dom);
	return res;
}

/* Construct an isl_ast_expr that evaluates "pa".
 * The result is simplified in terms of build->domain.
 *
 * The domain of "pa" lives in the external schedule space.
 */
__isl_give isl_ast_expr *isl_ast_build_expr_from_pw_aff(
	__isl_keep isl_ast_build *build, __isl_take isl_pw_aff *pa)
{
	isl_ast_expr *expr;

	if (isl_ast_build_need_schedule_map(build)) {
		isl_multi_aff *ma;
		ma = isl_ast_build_get_schedule_map_multi_aff(build);
		pa = isl_pw_aff_pullback_multi_aff(pa, ma);
	}
	expr = isl_ast_build_expr_from_pw_aff_internal(build, pa);
	return expr;
}

/* Set the ids of the input dimensions of "pma" to the iterator ids
 * of "build".
 *
 * The domain of "pma" is assumed to live in the internal schedule domain.
 */
static __isl_give isl_pw_multi_aff *set_iterator_names(
	__isl_keep isl_ast_build *build, __isl_take isl_pw_multi_aff *pma)
{
	int i, n;

	n = isl_pw_multi_aff_dim(pma, isl_dim_in);
	for (i = 0; i < n; ++i) {
		isl_id *id;

		id = isl_ast_build_get_iterator_id(build, i);
		pma = isl_pw_multi_aff_set_dim_id(pma, isl_dim_in, i, id);
	}

	return pma;
}

/* Construct an isl_ast_expr that calls the domain element specified by "pma".
 * The name of the function is obtained from the output tuple name.
 * The arguments are given by the piecewise affine expressions.
 *
 * The domain of "pma" is assumed to live in the internal schedule domain.
 */
static __isl_give isl_ast_expr *isl_ast_build_call_from_pw_multi_aff_internal(
	__isl_keep isl_ast_build *build, __isl_take isl_pw_multi_aff *pma)
{
	int i, n;
	isl_ctx *ctx;
	isl_id *id;
	isl_ast_expr *expr;

	pma = set_iterator_names(build, pma);
	if (!build || !pma)
		return isl_pw_multi_aff_free(pma);

	ctx = isl_ast_build_get_ctx(build);
	n = isl_pw_multi_aff_dim(pma, isl_dim_out);
	expr = isl_ast_expr_alloc_op(ctx, isl_ast_op_call, 1 + n);

	if (isl_pw_multi_aff_has_tuple_id(pma, isl_dim_out))
		id = isl_pw_multi_aff_get_tuple_id(pma, isl_dim_out);
	else
		id = isl_id_alloc(ctx, "", NULL);

	expr = isl_ast_expr_set_op_arg(expr, 0, isl_ast_expr_from_id(id));
	for (i = 0; i < n; ++i) {
		isl_pw_aff *pa;
		isl_ast_expr *arg;

		pa = isl_pw_multi_aff_get_pw_aff(pma, i);
		arg = isl_ast_build_expr_from_pw_aff_internal(build, pa);
		expr = isl_ast_expr_set_op_arg(expr, 1 + i, arg);
	}

	isl_pw_multi_aff_free(pma);
	return expr;
}

/* Construct an isl_ast_expr that calls the domain element specified by "pma".
 * The name of the function is obtained from the output tuple name.
 * The arguments are given by the piecewise affine expressions.
 *
 * The domain of "pma" is assumed to live in the external schedule domain.
 */
__isl_give isl_ast_expr *isl_ast_build_call_from_pw_multi_aff(
	__isl_keep isl_ast_build *build, __isl_take isl_pw_multi_aff *pma)
{
	int is_domain;
	isl_ast_expr *expr;
	isl_space *space_build, *space_pma;

	space_build = isl_ast_build_get_space(build, 0);
	space_pma = isl_pw_multi_aff_get_space(pma);
	is_domain = isl_space_tuple_match(space_build, isl_dim_set,
					space_pma, isl_dim_in);
	isl_space_free(space_build);
	isl_space_free(space_pma);
	if (is_domain < 0)
		return isl_pw_multi_aff_free(pma);
	if (!is_domain)
		isl_die(isl_ast_build_get_ctx(build), isl_error_invalid,
			"spaces don't match",
			return isl_pw_multi_aff_free(pma));

	if (isl_ast_build_need_schedule_map(build)) {
		isl_multi_aff *ma;
		ma = isl_ast_build_get_schedule_map_multi_aff(build);
		pma = isl_pw_multi_aff_pullback_multi_aff(pma, ma);
	}

	expr = isl_ast_build_call_from_pw_multi_aff_internal(build, pma);
	return expr;
}

/* Construct an isl_ast_expr that calls the domain element
 * specified by "executed".
 *
 * "executed" is assumed to be single-valued, with a domain that lives
 * in the internal schedule space.
 */
__isl_give isl_ast_node *isl_ast_build_call_from_executed(
	__isl_keep isl_ast_build *build, __isl_take isl_map *executed)
{
	isl_pw_multi_aff *iteration;
	isl_ast_expr *expr;

	iteration = isl_pw_multi_aff_from_map(executed);
	iteration = isl_ast_build_compute_gist_pw_multi_aff(build, iteration);
	iteration = isl_pw_multi_aff_intersect_domain(iteration,
					isl_ast_build_get_domain(build));
	expr = isl_ast_build_call_from_pw_multi_aff_internal(build, iteration);
	return isl_ast_node_alloc_user(expr);
}
