#ifndef ISL_POLYNOMIAL_H
#define ISL_POLYNOMIAL_H

#include <isl_ctx.h>
#include <isl_dim.h>
#include <isl_div.h>
#include <isl_set.h>
#include <isl_point.h>
#include <isl_printer.h>

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

int isl_qpolynomial_is_equal(__isl_keep isl_qpolynomial *qp1,
	__isl_keep isl_qpolynomial *qp2);
int isl_qpolynomial_is_cst(__isl_keep isl_qpolynomial *qp,
	isl_int *n, isl_int *d);
void isl_qpolynomial_get_den(__isl_keep isl_qpolynomial *qp, isl_int *d);

__isl_give isl_qpolynomial *isl_qpolynomial_neg(__isl_take isl_qpolynomial *qp);
__isl_give isl_qpolynomial *isl_qpolynomial_add(__isl_take isl_qpolynomial *qp1,
	__isl_take isl_qpolynomial *qp2);
__isl_give isl_qpolynomial *isl_qpolynomial_mul(__isl_take isl_qpolynomial *qp1,
	__isl_take isl_qpolynomial *qp2);

__isl_give isl_qpolynomial *isl_qpolynomial_drop_dims(
	__isl_take isl_qpolynomial *qp,
	enum isl_dim_type type, unsigned first, unsigned n);

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

__isl_give isl_qpolynomial *isl_qpolynomial_eval(
	__isl_take isl_qpolynomial *qp, __isl_take isl_point *pnt);

__isl_give isl_printer *isl_printer_print_qpolynomial(
	__isl_take isl_printer *p, __isl_keep isl_qpolynomial *qp);
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

int isl_pw_qpolynomial_is_zero(__isl_keep isl_pw_qpolynomial *pwqp);

__isl_give isl_dim *isl_pw_qpolynomial_get_dim(
	__isl_keep isl_pw_qpolynomial *pwqp);
unsigned isl_pw_qpolynomial_dim(__isl_keep isl_pw_qpolynomial *pwqp,
	enum isl_dim_type type);
int isl_pw_qpolynomial_involves_dims(__isl_keep isl_pw_qpolynomial *pwqp,
	enum isl_dim_type type, unsigned first, unsigned n);

__isl_give isl_set *isl_pw_qpolynomial_domain(__isl_take isl_pw_qpolynomial *pwqp);
__isl_give isl_pw_qpolynomial *isl_pw_qpolynomial_intersect_domain(
	__isl_take isl_pw_qpolynomial *pwpq, __isl_take isl_set *set);

__isl_give isl_pw_qpolynomial *isl_pw_qpolynomial_drop_dims(
	__isl_take isl_pw_qpolynomial *pwqp,
	enum isl_dim_type type, unsigned first, unsigned n);
__isl_give isl_pw_qpolynomial *isl_pw_qpolynomial_split_dims(
	__isl_take isl_pw_qpolynomial *pwqp,
	enum isl_dim_type type, unsigned first, unsigned n);

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

__isl_give isl_pw_qpolynomial *isl_pw_qpolynomial_add_dims(
	__isl_take isl_pw_qpolynomial *pwqp,
	enum isl_dim_type type, unsigned n);
__isl_give isl_pw_qpolynomial *isl_pw_qpolynomial_move(
	__isl_take isl_pw_qpolynomial *pwqp,
	enum isl_dim_type dst_type, unsigned dst_pos,
	enum isl_dim_type src_type, unsigned src_pos, unsigned n);

__isl_give isl_pw_qpolynomial *isl_pw_qpolynomial_fix_dim(
	__isl_take isl_pw_qpolynomial *pwqp,
	enum isl_dim_type type, unsigned n, isl_int v);

__isl_give isl_qpolynomial *isl_pw_qpolynomial_eval(
	__isl_take isl_pw_qpolynomial *pwqp, __isl_take isl_point *pnt);

__isl_give isl_qpolynomial *isl_pw_qpolynomial_max(
	__isl_take isl_pw_qpolynomial *pwqp);
__isl_give isl_qpolynomial *isl_pw_qpolynomial_min(
	__isl_take isl_pw_qpolynomial *pwqp);

int isl_pw_qpolynomial_foreach_piece(__isl_keep isl_pw_qpolynomial *pwqp,
	int (*fn)(__isl_take isl_set *set, __isl_take isl_qpolynomial *qp,
		    void *user), void *user);
int isl_pw_qpolynomial_foreach_lifted_piece(__isl_keep isl_pw_qpolynomial *pwqp,
	int (*fn)(__isl_take isl_set *set, __isl_take isl_qpolynomial *qp,
		    void *user), void *user);

__isl_give isl_printer *isl_printer_print_pw_qpolynomial(
	__isl_take isl_printer *p, __isl_keep isl_pw_qpolynomial *pwqp);
void isl_pw_qpolynomial_print(__isl_keep isl_pw_qpolynomial *pwqp, FILE *out,
	unsigned output_format);

__isl_give isl_pw_qpolynomial *isl_pw_qpolynomial_coalesce(
	__isl_take isl_pw_qpolynomial *pwqp);
__isl_give isl_pw_qpolynomial *isl_pw_qpolynomial_gist(
	__isl_take isl_pw_qpolynomial *pwqp, __isl_take isl_set *context);

enum isl_fold {
	isl_fold_min,
	isl_fold_max,
	isl_fold_list
};

struct isl_qpolynomial_fold;
typedef struct isl_qpolynomial_fold isl_qpolynomial_fold;

__isl_give isl_qpolynomial_fold *isl_qpolynomial_fold_empty(enum isl_fold type,
	__isl_take isl_dim *dim);
__isl_give isl_qpolynomial_fold *isl_qpolynomial_fold_alloc(
	enum isl_fold type, __isl_take isl_qpolynomial *qp);
__isl_give isl_qpolynomial_fold *isl_qpolynomial_fold_copy(
	__isl_keep isl_qpolynomial_fold *fold);
void isl_qpolynomial_fold_free(__isl_take isl_qpolynomial_fold *fold);

int isl_qpolynomial_fold_is_empty(__isl_keep isl_qpolynomial_fold *fold);
int isl_qpolynomial_fold_is_equal(__isl_keep isl_qpolynomial_fold *fold1,
	__isl_keep isl_qpolynomial_fold *fold2);

__isl_give isl_qpolynomial_fold *isl_qpolynomial_fold_fold(
	__isl_take isl_qpolynomial_fold *fold1,
	__isl_take isl_qpolynomial_fold *fold2);

__isl_give isl_qpolynomial *isl_qpolynomial_fold_eval(
	__isl_take isl_qpolynomial_fold *fold, __isl_take isl_point *pnt);

void isl_qpolynomial_fold_print(__isl_keep isl_qpolynomial_fold *fold, FILE *out,
	unsigned output_format);

struct isl_pw_qpolynomial_fold;
typedef struct isl_pw_qpolynomial_fold isl_pw_qpolynomial_fold;

isl_ctx *isl_pw_qpolynomial_fold_get_ctx(__isl_keep isl_pw_qpolynomial_fold *pwf);

__isl_give isl_pw_qpolynomial_fold *isl_pw_qpolynomial_fold_from_pw_qpolynomial(
	enum isl_fold type, __isl_take isl_pw_qpolynomial *pwqp);

__isl_give isl_pw_qpolynomial_fold *isl_pw_qpolynomial_fold_alloc(
	__isl_take isl_set *set, __isl_take isl_qpolynomial_fold *fold);
__isl_give isl_pw_qpolynomial_fold *isl_pw_qpolynomial_fold_copy(
	__isl_keep isl_pw_qpolynomial_fold *pwf);
void isl_pw_qpolynomial_fold_free(__isl_take isl_pw_qpolynomial_fold *pwf);

unsigned isl_pw_qpolynomial_fold_dim(__isl_keep isl_pw_qpolynomial_fold *pwf,
	enum isl_dim_type type);

size_t isl_pw_qpolynomial_fold_size(__isl_keep isl_pw_qpolynomial_fold *pwf);

__isl_give isl_pw_qpolynomial_fold *isl_pw_qpolynomial_fold_zero(
	__isl_take isl_dim *dim);

__isl_give isl_set *isl_pw_qpolynomial_fold_domain(
	__isl_take isl_pw_qpolynomial_fold *pwf);
__isl_give isl_pw_qpolynomial_fold *isl_pw_qpolynomial_fold_intersect_domain(
	__isl_take isl_pw_qpolynomial_fold *pwf, __isl_take isl_set *set);

__isl_give isl_pw_qpolynomial_fold *isl_pw_qpolynomial_fold_add(
	__isl_take isl_pw_qpolynomial_fold *pwf1,
	__isl_take isl_pw_qpolynomial_fold *pwf2);
__isl_give isl_pw_qpolynomial_fold *isl_pw_qpolynomial_fold_add_disjoint(
	__isl_take isl_pw_qpolynomial_fold *pwf1,
	__isl_take isl_pw_qpolynomial_fold *pwf2);

__isl_give isl_qpolynomial *isl_pw_qpolynomial_fold_eval(
	__isl_take isl_pw_qpolynomial_fold *pwf, __isl_take isl_point *pnt);

__isl_give isl_printer *isl_printer_print_pw_qpolynomial_fold(
	__isl_take isl_printer *p, __isl_keep isl_pw_qpolynomial_fold *pwf);
void isl_pw_qpolynomial_fold_print(__isl_keep isl_pw_qpolynomial_fold *pwf,
	FILE *out, unsigned output_format);

__isl_give isl_pw_qpolynomial_fold *isl_pw_qpolynomial_fold_coalesce(
	__isl_take isl_pw_qpolynomial_fold *pwf);
__isl_give isl_pw_qpolynomial_fold *isl_pw_qpolynomial_fold_gist(
	__isl_take isl_pw_qpolynomial_fold *pwf, __isl_take isl_set *context);

#if defined(__cplusplus)
}
#endif

#endif
