#ifndef ISL_BOUND_H
#define ISL_BOUND_H

#include <isl_polynomial.h>

struct isl_bound {
	/* input */
	int check_tight;
	enum isl_fold type;
	isl_basic_set *bset;
	isl_qpolynomial_fold *fold;

	/* output */
	isl_pw_qpolynomial_fold *pwf;
	isl_pw_qpolynomial_fold *pwf_tight;
};

__isl_give isl_pw_qpolynomial_fold *isl_pw_qpolynomial_bound(
	__isl_take isl_pw_qpolynomial *pwqp, enum isl_fold type, int *tight);

#endif
