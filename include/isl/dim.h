/*
 * Copyright 2008-2009 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv2.1 license
 *
 * Written by Sven Verdoolaege, K.U.Leuven, Departement
 * Computerwetenschappen, Celestijnenlaan 200A, B-3001 Leuven, Belgium
 */

#ifndef ISL_DIM_H
#define ISL_DIM_H

#include <isl/ctx.h>
#include <isl/printer.h>

#if defined(__cplusplus)
extern "C" {
#endif

struct isl_dim;
typedef struct isl_dim isl_dim;

enum isl_dim_type {
	isl_dim_cst,
	isl_dim_param,
	isl_dim_in,
	isl_dim_out,
	isl_dim_set = isl_dim_out,
	isl_dim_div,
	isl_dim_all
};

isl_ctx *isl_dim_get_ctx(__isl_keep isl_dim *dim);
__isl_give isl_dim *isl_dim_alloc(isl_ctx *ctx,
			unsigned nparam, unsigned n_in, unsigned n_out);
__isl_give isl_dim *isl_dim_set_alloc(isl_ctx *ctx,
			unsigned nparam, unsigned dim);
__isl_give isl_dim *isl_dim_copy(__isl_keep isl_dim *dim);
struct isl_dim *isl_dim_cow(struct isl_dim *dim);
void isl_dim_free(__isl_take isl_dim *dim);

__isl_give isl_dim *isl_dim_set_tuple_name(__isl_take isl_dim *dim,
	enum isl_dim_type type, const char *s);
const char *isl_dim_get_tuple_name(__isl_keep isl_dim *dim,
				 enum isl_dim_type type);

__isl_give isl_dim *isl_dim_set_name(__isl_take isl_dim *dim,
				 enum isl_dim_type type, unsigned pos,
				 __isl_keep const char *name);
__isl_keep const char *isl_dim_get_name(__isl_keep isl_dim *dim,
				 enum isl_dim_type type, unsigned pos);

struct isl_dim *isl_dim_extend(struct isl_dim *dim,
			unsigned nparam, unsigned n_in, unsigned n_out);
__isl_give isl_dim *isl_dim_add(__isl_take isl_dim *dim, enum isl_dim_type type,
		unsigned n);
__isl_give isl_dim *isl_dim_move(__isl_take isl_dim *dim,
	enum isl_dim_type dst_type, unsigned dst_pos,
	enum isl_dim_type src_type, unsigned src_pos, unsigned n);
__isl_give isl_dim *isl_dim_insert(__isl_take isl_dim *dim,
	enum isl_dim_type type, unsigned pos, unsigned n);
__isl_give isl_dim *isl_dim_join(__isl_take isl_dim *left,
	__isl_take isl_dim *right);
struct isl_dim *isl_dim_product(struct isl_dim *left, struct isl_dim *right);
__isl_give isl_dim *isl_dim_range_product(__isl_take isl_dim *left,
	__isl_take isl_dim *right);
__isl_give isl_dim *isl_dim_map_from_set(__isl_take isl_dim *dim);
__isl_give isl_dim *isl_dim_reverse(__isl_take isl_dim *dim);
__isl_give isl_dim *isl_dim_drop(__isl_take isl_dim *dim,
	enum isl_dim_type type, unsigned first, unsigned num);
struct isl_dim *isl_dim_drop_inputs(struct isl_dim *dim,
		unsigned first, unsigned n);
struct isl_dim *isl_dim_drop_outputs(struct isl_dim *dim,
		unsigned first, unsigned n);
__isl_give isl_dim *isl_dim_domain(__isl_take isl_dim *dim);
__isl_give isl_dim *isl_dim_from_domain(__isl_take isl_dim *dim);
__isl_give isl_dim *isl_dim_range(__isl_take isl_dim *dim);
__isl_give isl_dim *isl_dim_from_range(__isl_take isl_dim *dim);
struct isl_dim *isl_dim_underlying(struct isl_dim *dim, unsigned n_div);

__isl_give isl_dim *isl_dim_align_params(__isl_take isl_dim *dim1,
	__isl_take isl_dim *dim2);

int isl_dim_is_wrapping(__isl_keep isl_dim *dim);
__isl_give isl_dim *isl_dim_wrap(__isl_take isl_dim *dim);
__isl_give isl_dim *isl_dim_unwrap(__isl_take isl_dim *dim);

int isl_dim_can_zip(__isl_keep isl_dim *dim);
__isl_give isl_dim *isl_dim_zip(__isl_take isl_dim *dim);

int isl_dim_equal(struct isl_dim *dim1, struct isl_dim *dim2);
int isl_dim_match(struct isl_dim *dim1, enum isl_dim_type dim1_type,
		struct isl_dim *dim2, enum isl_dim_type dim2_type);
int isl_dim_tuple_match(__isl_keep isl_dim *dim1, enum isl_dim_type dim1_type,
			__isl_keep isl_dim *dim2, enum isl_dim_type dim2_type);
int isl_dim_compatible(struct isl_dim *dim1, struct isl_dim *dim2);
unsigned isl_dim_size(__isl_keep isl_dim *dim, enum isl_dim_type type);
unsigned isl_dim_total(struct isl_dim *dim);

__isl_give isl_printer *isl_printer_print_dim(__isl_take isl_printer *p,
	__isl_keep isl_dim *dim);
void isl_dim_dump(__isl_keep isl_dim *dim);

#if defined(__cplusplus)
}
#endif

#endif
