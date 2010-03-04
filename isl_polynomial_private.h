#include <stdio.h>
#include <isl_div.h>
#include <isl_map.h>
#include <isl_mat.h>
#include <isl_polynomial.h>

struct isl_upoly {
	int ref;
	struct isl_ctx *ctx;

	int var;
};

struct isl_upoly_cst {
	struct isl_upoly up;
	isl_int n;
	isl_int d;
};

struct isl_upoly_rec {
	struct isl_upoly up;
	int n;

	size_t size;
	struct isl_upoly *p[1];
};

struct isl_qpolynomial {
	int ref;

	struct isl_dim *dim;
	struct isl_mat *div;
	struct isl_upoly *upoly;
};

struct isl_pw_qpolynomial_piece {
	struct isl_set *set;
	struct isl_qpolynomial *qp;
};

struct isl_pw_qpolynomial {
	int ref;

	struct isl_dim *dim;

	int n;

	size_t size;
	struct isl_pw_qpolynomial_piece p[1];
};

__isl_give struct isl_upoly *isl_upoly_zero(struct isl_ctx *ctx);
__isl_give struct isl_upoly *isl_upoly_copy(__isl_keep struct isl_upoly *up);
__isl_give struct isl_upoly *isl_upoly_cow(__isl_take struct isl_upoly *up);
__isl_give struct isl_upoly *isl_upoly_dup(__isl_keep struct isl_upoly *up);
void isl_upoly_free(__isl_take struct isl_upoly *up);
__isl_give struct isl_upoly *isl_upoly_mul(__isl_take struct isl_upoly *up1,
	__isl_take struct isl_upoly *up2);

int isl_upoly_is_cst(__isl_keep struct isl_upoly *up);
int isl_upoly_is_zero(__isl_keep struct isl_upoly *up);
int isl_upoly_is_one(__isl_keep struct isl_upoly *up);
int isl_upoly_is_negone(__isl_keep struct isl_upoly *up);
__isl_keep struct isl_upoly_cst *isl_upoly_as_cst(__isl_keep struct isl_upoly *up);
__isl_keep struct isl_upoly_rec *isl_upoly_as_rec(__isl_keep struct isl_upoly *up);

__isl_give struct isl_upoly *isl_upoly_sum(__isl_take struct isl_upoly *up1,
	__isl_take struct isl_upoly *up2);
__isl_give struct isl_upoly *isl_upoly_neg(__isl_take struct isl_upoly *up);

__isl_give isl_qpolynomial *isl_qpolynomial_alloc(__isl_take isl_dim *dim,
	unsigned n_div);
__isl_give isl_qpolynomial *isl_qpolynomial_cow(__isl_take isl_qpolynomial *qp);
__isl_give isl_qpolynomial *isl_qpolynomial_dup(__isl_keep isl_qpolynomial *qp);

__isl_give isl_qpolynomial *isl_qpolynomial_sub(__isl_take isl_qpolynomial *qp1,
	__isl_take isl_qpolynomial *qp2);

__isl_give isl_qpolynomial *isl_qpolynomial_cst(__isl_take isl_dim *dim,
	isl_int v);
__isl_give isl_qpolynomial *isl_qpolynomial_pow(__isl_take isl_dim *dim,
	int pos, int power);
__isl_give isl_qpolynomial *isl_qpolynomial_div_pow(__isl_take isl_div *div,
	int power);
int isl_qpolynomial_is_zero(__isl_keep isl_qpolynomial *qp);
int isl_qpolynomial_is_one(__isl_keep isl_qpolynomial *qp);

__isl_give isl_pw_qpolynomial *isl_pw_qpolynomial_add_piece(
	__isl_take isl_pw_qpolynomial *pwqp,
	__isl_take isl_set *set, __isl_take isl_qpolynomial *qp);
int isl_pw_qpolynomial_is_zero(__isl_keep isl_pw_qpolynomial *pwqp);
int isl_pw_qpolynomial_is_one(__isl_keep isl_pw_qpolynomial *pwqp);
