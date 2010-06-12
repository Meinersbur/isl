#include <isl_constraint.h>
#include <isl_set.h>
#include <isl_polynomial_private.h>
#include <isl_morph.h>

struct range_data {
	int 		    	*signs;
	int			sign;
	int			test_monotonicity;
	int		    	monotonicity;
	int			exact;
	isl_qpolynomial	    	*qp;
	isl_qpolynomial	    	*poly;
	isl_pw_qpolynomial_fold *pwf;
	isl_pw_qpolynomial_fold *pwf_exact;
};

static int propagate_on_domain(__isl_take isl_basic_set *bset,
	__isl_take isl_qpolynomial *poly, struct range_data *data);

/* Check whether the polynomial "poly" has sign "sign" over "bset",
 * i.e., if sign == 1, check that the lower bound on the polynomial
 * is non-negative and if sign == -1, check that the upper bound on
 * the polynomial is non-positive.
 */
static int has_sign(__isl_keep isl_basic_set *bset,
	__isl_keep isl_qpolynomial *poly, int sign, int *signs)
{
	struct range_data data_m;
	unsigned nvar;
	unsigned nparam;
	isl_dim *dim;
	isl_qpolynomial *opt;
	int r;

	nparam = isl_basic_set_dim(bset, isl_dim_param);
	nvar = isl_basic_set_dim(bset, isl_dim_set);

	bset = isl_basic_set_copy(bset);
	poly = isl_qpolynomial_copy(poly);

	bset = isl_basic_set_move_dims(bset, isl_dim_set, 0,
					isl_dim_param, 0, nparam);
	poly = isl_qpolynomial_move_dims(poly, isl_dim_set, 0,
					isl_dim_param, 0, nparam);

	dim = isl_qpolynomial_get_dim(poly);
	dim = isl_dim_drop(dim, isl_dim_set, 0, isl_dim_size(dim, isl_dim_set));

	data_m.test_monotonicity = 0;
	data_m.signs = signs;
	data_m.pwf = isl_pw_qpolynomial_fold_zero(dim);
	data_m.sign = -sign;
	data_m.exact = 0;
	data_m.pwf_exact = NULL;

	if (propagate_on_domain(bset, poly, &data_m) < 0)
		goto error;

	if (sign > 0)
		opt = isl_pw_qpolynomial_fold_min(data_m.pwf);
	else
		opt = isl_pw_qpolynomial_fold_max(data_m.pwf);

	if (!opt)
		r = -1;
	else if (isl_qpolynomial_is_nan(opt) ||
		 isl_qpolynomial_is_infty(opt) ||
		 isl_qpolynomial_is_neginfty(opt))
		r = 0;
	else
		r = sign * isl_qpolynomial_sgn(opt) >= 0;

	isl_qpolynomial_free(opt);

	return r;
error:
	isl_pw_qpolynomial_fold_free(data_m.pwf);
	return -1;
}

/* Return  1 if poly is monotonically increasing in the last set variable,
 *        -1 if poly is monotonically decreasing in the last set variable,
 *	   0 if no conclusion,
 *	  -2 on error.
 *
 * We simply check the sign of p(x+1)-p(x)
 */
static int monotonicity(__isl_keep isl_basic_set *bset,
	__isl_keep isl_qpolynomial *poly, struct range_data *data)
{
	isl_ctx *ctx;
	isl_dim *dim;
	isl_qpolynomial *sub = NULL;
	isl_qpolynomial *diff = NULL;
	int result = 0;
	int s;
	unsigned nvar;

	ctx = isl_qpolynomial_get_ctx(poly);
	dim = isl_qpolynomial_get_dim(poly);

	nvar = isl_basic_set_dim(bset, isl_dim_set);

	sub = isl_qpolynomial_var(isl_dim_copy(dim), isl_dim_set, nvar - 1);
	sub = isl_qpolynomial_add(sub,
		isl_qpolynomial_rat_cst(dim, ctx->one, ctx->one));

	diff = isl_qpolynomial_substitute(isl_qpolynomial_copy(poly),
			isl_dim_set, nvar - 1, 1, &sub);
	diff = isl_qpolynomial_sub(diff, isl_qpolynomial_copy(poly));

	s = has_sign(bset, diff, 1, data->signs);
	if (s < 0)
		goto error;
	if (s)
		result = 1;
	else {
		s = has_sign(bset, diff, -1, data->signs);
		if (s < 0)
			goto error;
		if (s)
			result = -1;
	}

	isl_qpolynomial_free(diff);
	isl_qpolynomial_free(sub);

	return result;
error:
	isl_qpolynomial_free(diff);
	isl_qpolynomial_free(sub);
	return -2;
}

static __isl_give isl_qpolynomial *bound2poly(__isl_take isl_constraint *bound,
	__isl_take isl_dim *dim, unsigned pos, int sign)
{
	if (!bound) {
		if (sign > 0)
			return isl_qpolynomial_infty(dim);
		else
			return isl_qpolynomial_neginfty(dim);
	}
	isl_dim_free(dim);
	return isl_qpolynomial_from_constraint(bound, isl_dim_set, pos);
}

static int bound_is_integer(__isl_take isl_constraint *bound, unsigned pos)
{
	isl_int c;
	int is_int;

	if (!bound)
		return 1;

	isl_int_init(c);
	isl_constraint_get_coefficient(bound, isl_dim_set, pos, &c);
	is_int = isl_int_is_one(c) || isl_int_is_negone(c);
	isl_int_clear(c);

	return is_int;
}

struct isl_fixed_sign_data {
	int		*signs;
	int		sign;
	isl_qpolynomial	*poly;
};

/* Add term "term" to data->poly if it has sign data->sign.
 * The sign is determined based on the signs of the parameters
 * and variables in data->signs.
 */
static int collect_fixed_sign_terms(__isl_take isl_term *term, void *user)
{
	struct isl_fixed_sign_data *data = (struct isl_fixed_sign_data *)user;
	isl_int n, d;
	int i;
	int sign;
	unsigned nparam;
	unsigned nvar;

	if (!term)
		return -1;

	nparam = isl_term_dim(term, isl_dim_param);
	nvar = isl_term_dim(term, isl_dim_set);

	isl_assert(isl_term_get_ctx(term), isl_term_dim(term, isl_dim_div) == 0,
			return -1);

	isl_int_init(n);
	isl_int_init(d);

	isl_term_get_num(term, &n);
	isl_term_get_den(term, &d);

	sign = isl_int_sgn(n);
	for (i = 0; i < nparam; ++i) {
		if (data->signs[i] > 0)
			continue;
		if (isl_term_get_exp(term, isl_dim_param, i) % 2)
			sign = -sign;
	}
	for (i = 0; i < nvar; ++i) {
		if (data->signs[nparam + i] > 0)
			continue;
		if (isl_term_get_exp(term, isl_dim_set, i) % 2)
			sign = -sign;
	}

	if (sign == data->sign) {
		isl_qpolynomial *t = isl_qpolynomial_from_term(term);

		data->poly = isl_qpolynomial_add(data->poly, t);
	} else
		isl_term_free(term);

	isl_int_clear(n);
	isl_int_clear(d);

	return 0;
}

/* Construct and return a polynomial that consists of the terms
 * in "poly" that have sign "sign".
 */
static __isl_give isl_qpolynomial *fixed_sign_terms(
	__isl_keep isl_qpolynomial *poly, int *signs, int sign)
{
	struct isl_fixed_sign_data data = { signs, sign };
	data.poly = isl_qpolynomial_zero(isl_qpolynomial_get_dim(poly));

	if (isl_qpolynomial_foreach_term(poly, collect_fixed_sign_terms, &data) < 0)
		goto error;

	return data.poly;
error:
	isl_qpolynomial_free(data.poly);
	return NULL;
}

/* Helper function to add a guarded polynomial to either pwf_exact or pwf,
 * depending on whether the result has been determined to be exact.
 */
static int add_guarded_poly(__isl_take isl_basic_set *bset,
	__isl_take isl_qpolynomial *poly, struct range_data *data)
{
	enum isl_fold type = data->sign < 0 ? isl_fold_min : isl_fold_max;
	isl_set *set;
	isl_qpolynomial_fold *fold;
	isl_pw_qpolynomial_fold *pwf;

	fold = isl_qpolynomial_fold_alloc(type, poly);
	set = isl_set_from_basic_set(bset);
	pwf = isl_pw_qpolynomial_fold_alloc(set, fold);
	if (data->exact)
		data->pwf_exact = isl_pw_qpolynomial_fold_add(
						data->pwf_exact, pwf);
	else
		data->pwf = isl_pw_qpolynomial_fold_add(data->pwf, pwf);

	return 0;
}

/* Given a lower and upper bound on the final variable and constraints
 * on the remaining variables where these bounds are active,
 * eliminate the variable from data->poly based on these bounds.
 * If the polynomial has been determined to be monotonic
 * in the variable, then simply plug in the appropriate bound.
 * If the current polynomial is exact and if this bound is integer,
 * then the result is still exact.  In all other cases, the results
 * may be inexact.
 * Otherwise, plug in the largest bound (in absolute value) in
 * the positive terms (if an upper bound is wanted) or the negative terms
 * (if a lower bounded is wanted) and the other bound in the other terms.
 *
 * If all variables have been eliminated, then record the result.
 * Ohterwise, recurse on the next variable.
 */
static int propagate_on_bound_pair(__isl_take isl_constraint *lower,
	__isl_take isl_constraint *upper, __isl_take isl_basic_set *bset,
	void *user)
{
	struct range_data *data = (struct range_data *)user;
	int save_exact = data->exact;
	isl_qpolynomial *poly;
	int r;
	unsigned nvar;

	nvar = isl_basic_set_dim(bset, isl_dim_set);

	if (data->monotonicity) {
		isl_qpolynomial *sub;
		isl_dim *dim = isl_qpolynomial_get_dim(data->poly);
		if (data->monotonicity * data->sign > 0) {
			if (data->exact)
				data->exact = bound_is_integer(upper, nvar);
			sub = bound2poly(upper, dim, nvar, 1);
			isl_constraint_free(lower);
		} else {
			if (data->exact)
				data->exact = bound_is_integer(lower, nvar);
			sub = bound2poly(lower, dim, nvar, -1);
			isl_constraint_free(upper);
		}
		poly = isl_qpolynomial_copy(data->poly);
		poly = isl_qpolynomial_substitute(poly, isl_dim_set, nvar, 1, &sub);
		poly = isl_qpolynomial_drop_dims(poly, isl_dim_set, nvar, 1);

		isl_qpolynomial_free(sub);
	} else {
		isl_qpolynomial *l, *u;
		isl_qpolynomial *pos, *neg;
		isl_dim *dim = isl_qpolynomial_get_dim(data->poly);
		unsigned nparam = isl_basic_set_dim(bset, isl_dim_param);
		int sign = data->sign * data->signs[nparam + nvar];

		data->exact = 0;

		u = bound2poly(upper, isl_dim_copy(dim), nvar, 1);
		l = bound2poly(lower, dim, nvar, -1);

		pos = fixed_sign_terms(data->poly, data->signs, sign);
		neg = fixed_sign_terms(data->poly, data->signs, -sign);

		pos = isl_qpolynomial_substitute(pos, isl_dim_set, nvar, 1, &u);
		neg = isl_qpolynomial_substitute(neg, isl_dim_set, nvar, 1, &l);

		poly = isl_qpolynomial_add(pos, neg);
		poly = isl_qpolynomial_drop_dims(poly, isl_dim_set, nvar, 1);

		isl_qpolynomial_free(u);
		isl_qpolynomial_free(l);
	}

	if (isl_basic_set_dim(bset, isl_dim_set) == 0)
		r = add_guarded_poly(bset, poly, data);
	else
		r = propagate_on_domain(bset, poly, data);

	data->exact = save_exact;

	return r;
}

/* Recursively perform range propagation on the polynomial "poly"
 * defined over the basic set "bset" and collect the results in "data".
 */
static int propagate_on_domain(__isl_take isl_basic_set *bset,
	__isl_take isl_qpolynomial *poly, struct range_data *data)
{
	isl_qpolynomial *save_poly = data->poly;
	int save_monotonicity = data->monotonicity;
	unsigned d;

	if (!bset || !poly)
		goto error;

	d = isl_basic_set_dim(bset, isl_dim_set);
	isl_assert(bset->ctx, d >= 1, goto error);

	if (isl_qpolynomial_is_cst(poly, NULL, NULL)) {
		bset = isl_basic_set_project_out(bset, isl_dim_set, 0, d);
		poly = isl_qpolynomial_drop_dims(poly, isl_dim_set, 0, d);
		return add_guarded_poly(bset, poly, data);
	}

	if (data->test_monotonicity)
		data->monotonicity = monotonicity(bset, poly, data);
	else
		data->monotonicity = 0;
	if (data->monotonicity < -1)
		goto error;

	data->poly = poly;
	if (isl_basic_set_foreach_bound_pair(bset, isl_dim_set, d - 1,
					    &propagate_on_bound_pair, data) < 0)
		goto error;

	isl_basic_set_free(bset);
	isl_qpolynomial_free(poly);
	data->monotonicity = save_monotonicity;
	data->poly = save_poly;

	return 0;
error:
	isl_basic_set_free(bset);
	isl_qpolynomial_free(poly);
	data->monotonicity = save_monotonicity;
	data->poly = save_poly;
	return -1;
}

static int basic_guarded_poly_bound(__isl_take isl_basic_set *bset, void *user)
{
	struct range_data *data = (struct range_data *)user;
	unsigned nparam = isl_basic_set_dim(bset, isl_dim_param);
	unsigned dim = isl_basic_set_dim(bset, isl_dim_set);
	int r;

	data->signs = NULL;

	data->signs = isl_alloc_array(bset->ctx, int,
					isl_basic_set_dim(bset, isl_dim_all));

	if (isl_basic_set_dims_get_sign(bset, isl_dim_set, 0, dim,
					data->signs + nparam) < 0)
		goto error;
	if (isl_basic_set_dims_get_sign(bset, isl_dim_param, 0, nparam,
					data->signs) < 0)
		goto error;

	r = propagate_on_domain(bset, isl_qpolynomial_copy(data->poly), data);

	free(data->signs);

	return r;
error:
	free(data->signs);
	isl_basic_set_free(bset);
	return -1;
}

static int compressed_guarded_poly_bound(__isl_take isl_basic_set *bset,
	__isl_take isl_qpolynomial *poly, void *user)
{
	struct range_data *data = (struct range_data *)user;
	unsigned nparam = isl_basic_set_dim(bset, isl_dim_param);
	unsigned nvar = isl_basic_set_dim(bset, isl_dim_set);
	isl_set *set;

	if (!bset)
		goto error;

	if (nvar == 0)
		return add_guarded_poly(bset, poly, data);

	set = isl_set_from_basic_set(bset);
	set = isl_set_split_dims(set, isl_dim_param, 0, nparam);
	set = isl_set_split_dims(set, isl_dim_set, 0, nvar);

	data->poly = poly;

	if (isl_set_foreach_basic_set(set, &basic_guarded_poly_bound, data) < 0)
		goto error;

	isl_set_free(set);
	isl_qpolynomial_free(poly);

	return 0;
error:
	isl_set_free(set);
	isl_qpolynomial_free(poly);
	return -1;
}

static int guarded_poly_bound(__isl_take isl_basic_set *bset,
	__isl_take isl_qpolynomial *poly, void *user)
{
	struct range_data *data = (struct range_data *)user;
	isl_pw_qpolynomial_fold *top_pwf;
	isl_pw_qpolynomial_fold *top_pwf_exact;
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

	top_pwf = data->pwf;
	top_pwf_exact = data->pwf_exact;

	data->pwf = isl_pw_qpolynomial_fold_zero(isl_dim_copy(dim));
	data->pwf_exact = isl_pw_qpolynomial_fold_zero(dim);

	r = compressed_guarded_poly_bound(bset, poly, user);

	morph = isl_morph_remove_dom_dims(morph, isl_dim_set, 0, orig_nvar);
	morph = isl_morph_remove_ran_dims(morph, isl_dim_set, 0, final_nvar);
	morph = isl_morph_inverse(morph);

	data->pwf = isl_pw_qpolynomial_fold_morph(data->pwf,
							isl_morph_copy(morph));
	data->pwf_exact = isl_pw_qpolynomial_fold_morph(data->pwf_exact, morph);

	data->pwf = isl_pw_qpolynomial_fold_add(top_pwf, data->pwf);
	data->pwf_exact = isl_pw_qpolynomial_fold_add(top_pwf_exact,
							data->pwf_exact);

	return r;
error:
	isl_basic_set_free(bset);
	isl_qpolynomial_free(poly);
	return -1;
}

static int basic_guarded_bound(__isl_take isl_basic_set *bset, void *user)
{
	struct range_data *data = (struct range_data *)user;
	int r;

	r = isl_qpolynomial_as_polynomial_on_domain(data->qp, bset,
						    &guarded_poly_bound, user);
	isl_basic_set_free(bset);
	return r;
}

static int guarded_bound(__isl_take isl_set *set,
	__isl_take isl_qpolynomial *qp, void *user)
{
	struct range_data *data = (struct range_data *)user;

	if (!set || !qp)
		goto error;

	set = isl_set_make_disjoint(set);

	data->qp = qp;

	if (isl_set_foreach_basic_set(set, &basic_guarded_bound, data) < 0)
		goto error;

	isl_set_free(set);
	isl_qpolynomial_free(qp);

	return 0;
error:
	isl_set_free(set);
	isl_qpolynomial_free(qp);
	return -1;
}

/* Compute a lower or upper bound (depending on "type") on the given
 * piecewise step-polynomial using range propagation.
 */
__isl_give isl_pw_qpolynomial_fold *isl_pw_qpolynomial_bound_range(
	__isl_take isl_pw_qpolynomial *pwqp, enum isl_fold type, int *exact)
{
	isl_dim *dim;
	isl_pw_qpolynomial_fold *pwf;
	unsigned nvar;
	unsigned nparam;
	struct range_data data;
	int covers;

	if (!pwqp)
		return NULL;

	dim = isl_pw_qpolynomial_get_dim(pwqp);
	nvar = isl_dim_size(dim, isl_dim_set);

	if (isl_pw_qpolynomial_is_zero(pwqp)) {
		isl_pw_qpolynomial_free(pwqp);
		dim = isl_dim_drop(dim, isl_dim_set, 0, nvar);
		if (exact)
			*exact = 1;
		return isl_pw_qpolynomial_fold_zero(dim);
	}

	if (nvar == 0) {
		isl_dim_free(dim);
		if (exact)
			*exact = 1;
		return isl_pw_qpolynomial_fold_from_pw_qpolynomial(type, pwqp);
	}

	dim = isl_dim_drop(dim, isl_dim_set, 0, nvar);

	nparam = isl_dim_size(dim, isl_dim_param);
	data.pwf = isl_pw_qpolynomial_fold_zero(isl_dim_copy(dim));
	data.pwf_exact = isl_pw_qpolynomial_fold_zero(isl_dim_copy(dim));
	if (type == isl_fold_min)
		data.sign = -1;
	else
		data.sign = 1;
	data.test_monotonicity = 1;
	data.exact = !!exact;

	if (isl_pw_qpolynomial_foreach_lifted_piece(pwqp, guarded_bound, &data))
		goto error;

	covers = isl_pw_qpolynomial_fold_covers(data.pwf_exact, data.pwf);
	if (covers < 0)
		goto error;

	if (exact)
		*exact = covers;

	isl_dim_free(dim);
	isl_pw_qpolynomial_free(pwqp);

	if (covers) {
		isl_pw_qpolynomial_fold_free(data.pwf);
		return data.pwf_exact;
	}

	data.pwf = isl_pw_qpolynomial_fold_add(data.pwf, data.pwf_exact);

	return data.pwf;
error:
	isl_pw_qpolynomial_fold_free(data.pwf);
	isl_dim_free(dim);
	isl_pw_qpolynomial_free(pwqp);
	return NULL;
}
