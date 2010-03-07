#ifndef ISL_POLYNOMIAL_H
#define ISL_POLYNOMIAL_H

#include <isl_ctx.h>
#include <isl_dim.h>
#include <isl_div.h>
#include <isl_set.h>
#include <isl_point.h>

#if defined(__cplusplus)
extern "C" {
#endif

struct isl_qpolynomial;
typedef struct isl_qpolynomial isl_qpolynomial;

__isl_give isl_qpolynomial *isl_qpolynomial_zero(__isl_take isl_dim *dim);
__isl_give isl_qpolynomial *isl_qpolynomial_infty(__isl_take isl_dim *dim);
__isl_give isl_qpolynomial *isl_qpolynomial_nan(__isl_take isl_dim *dim);
__isl_give isl_qpolynomial *isl_qpolynomial_rat_cst(__isl_take isl_dim *dim,
	const isl_int n, const isl_int d);
__isl_give isl_qpolynomial *isl_qpolynomial_div(__isl_take isl_div *div);
__isl_give isl_qpolynomial *isl_qpolynomial_var(__isl_take isl_dim *dim,
	enum isl_dim_type type, unsigned pos);
__isl_give isl_qpolynomial *isl_qpolynomial_copy(__isl_keep isl_qpolynomial *qp);
void isl_qpolynomial_free(__isl_take isl_qpolynomial *qp);

int isl_qpolynomial_is_cst(__isl_keep isl_qpolynomial *qp,
	isl_int *n, isl_int *d);

__isl_give isl_qpolynomial *isl_qpolynomial_neg(__isl_take isl_qpolynomial *qp);
__isl_give isl_qpolynomial *isl_qpolynomial_add(__isl_take isl_qpolynomial *qp1,
	__isl_take isl_qpolynomial *qp2);
__isl_give isl_qpolynomial *isl_qpolynomial_mul(__isl_take isl_qpolynomial *qp1,
	__isl_take isl_qpolynomial *qp2);

struct isl_term;
typedef struct isl_term isl_term;

isl_ctx *isl_term_get_ctx(__isl_keep isl_term *term);

void isl_term_free(__isl_take isl_term *term);

unsigned isl_term_dim(__isl_keep isl_term *term, enum isl_dim_type type);
void isl_term_get_num(__isl_keep isl_term *term, isl_int *n);
void isl_term_get_den(__isl_keep isl_term *term, isl_int *d);
int isl_term_get_exp(__isl_keep isl_term *term,
	enum isl_dim_type type, unsigned pos);
__isl_give isl_div *isl_term_get_div(__isl_keep isl_term *term, unsigned pos);

int isl_qpolynomial_foreach_term(__isl_keep isl_qpolynomial *qp,
	int (*fn)(__isl_take isl_term *term, void *user), void *user);

void isl_qpolynomial_print(__isl_keep isl_qpolynomial *qp, FILE *out,
	unsigned output_format);

struct isl_pw_qpolynomial;
typedef struct isl_pw_qpolynomial isl_pw_qpolynomial;

__isl_give isl_pw_qpolynomial *isl_pw_qpolynomial_zero(__isl_take isl_dim *dim);
__isl_give isl_pw_qpolynomial *isl_pw_qpolynomial_alloc(__isl_take isl_set *set,
	__isl_take isl_qpolynomial *qp);
__isl_give isl_pw_qpolynomial *isl_pw_qpolynomial_copy(
	__isl_keep isl_pw_qpolynomial *pwqp);
void isl_pw_qpolynomial_free(__isl_take isl_pw_qpolynomial *pwqp);

__isl_give isl_dim *isl_pw_qpolynomial_get_dim(
	__isl_keep isl_pw_qpolynomial *pwqp);
unsigned isl_pw_qpolynomial_dim(__isl_keep isl_pw_qpolynomial *pwqp,
	enum isl_dim_type type);

__isl_give isl_pw_qpolynomial *isl_pw_qpolynomial_add(
	__isl_take isl_pw_qpolynomial *pwqp1,
	__isl_take isl_pw_qpolynomial *pwqp2);
__isl_give isl_pw_qpolynomial *isl_pw_qpolynomial_sub(
	__isl_take isl_pw_qpolynomial *pwqp1,
	__isl_take isl_pw_qpolynomial *pwqp2);
__isl_give isl_pw_qpolynomial *isl_pw_qpolynomial_add_disjoint(
	__isl_take isl_pw_qpolynomial *pwqp1,
	__isl_take isl_pw_qpolynomial *pwqp2);
__isl_give isl_pw_qpolynomial *isl_pw_qpolynomial_neg(
	__isl_take isl_pw_qpolynomial *pwqp);
__isl_give isl_pw_qpolynomial *isl_pw_qpolynomial_mul(
	__isl_take isl_pw_qpolynomial *pwqp1,
	__isl_take isl_pw_qpolynomial *pwqp2);

__isl_give isl_pw_qpolynomial *isl_pw_qpolynomial_move(
	__isl_take isl_pw_qpolynomial *pwqp,
	enum isl_dim_type dst_type, unsigned dst_pos,
	enum isl_dim_type src_type, unsigned src_pos, unsigned n);

__isl_give isl_qpolynomial *isl_pw_qpolynomial_eval(
	__isl_take isl_pw_qpolynomial *pwqp, __isl_take isl_point *pnt);

int isl_pw_qpolynomial_foreach_piece(__isl_keep isl_pw_qpolynomial *pwqp,
	int (*fn)(__isl_take isl_set *set, __isl_take isl_qpolynomial *qp,
		    void *user), void *user);
int isl_pw_qpolynomial_foreach_lifted_piece(__isl_keep isl_pw_qpolynomial *pwqp,
	int (*fn)(__isl_take isl_set *set, __isl_take isl_qpolynomial *qp,
		    void *user), void *user);

void isl_pw_qpolynomial_print(__isl_keep isl_pw_qpolynomial *pwqp, FILE *out,
	unsigned output_format);

#if defined(__cplusplus)
}
#endif

#endif
