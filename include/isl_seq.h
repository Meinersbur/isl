#ifndef ISL_SEQ_H
#define ISL_SEQ_H

#include <sys/types.h>
#include <isl_int.h>

/* Some common operations on sequences of isl_int's */

void isl_seq_clr(isl_int *p, unsigned len);
void isl_seq_neg(isl_int *dat, isl_int *src, unsigned len);
void isl_seq_cpy(isl_int *dst, isl_int *src, unsigned len);
void isl_seq_swp_or_cpy(isl_int *dst, isl_int *src, unsigned len);
void isl_seq_scale(isl_int *dst, isl_int *src, isl_int f, unsigned len);
void isl_seq_scale_down(isl_int *dst, isl_int *src, isl_int f, unsigned len);
void isl_seq_elim(isl_int *dst, isl_int *src, unsigned pos, unsigned len,
		  isl_int *m);
void isl_seq_gcd(isl_int *p, unsigned len, isl_int *gcd);
void isl_seq_inner_product(isl_int *p1, isl_int *p2, unsigned len,
			   isl_int *prod);
int isl_seq_first_non_zero(isl_int *p, unsigned len);
int isl_seq_abs_min_non_zero(isl_int *p, unsigned len);
int isl_seq_eq(isl_int *p1, isl_int *p2, unsigned len);
int isl_seq_is_neg(isl_int *p1, isl_int *p2, unsigned len);

u_int32_t isl_seq_hash(isl_int *p, unsigned len, unsigned bits);

#endif
