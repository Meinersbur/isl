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

#include <isl_ctx.h>

#if defined(__cplusplus)
extern "C" {
#endif

struct isl_name;
struct isl_dim {
	int ref;

	struct isl_ctx *ctx;

	unsigned nparam;
	unsigned n_in;		/* zero for sets */
	unsigned n_out;		/* dim for sets */

	unsigned n_name;
	struct isl_name **names;
};
typedef struct isl_dim isl_dim;

enum isl_dim_type {
	isl_dim_param,
	isl_dim_in,
	isl_dim_out,
	isl_dim_set = isl_dim_out,
	isl_dim_div,
	isl_dim_all
};

__isl_give isl_dim *isl_dim_alloc(isl_ctx *ctx,
			unsigned nparam, unsigned n_in, unsigned n_out);
__isl_give isl_dim *isl_dim_set_alloc(isl_ctx *ctx,
			unsigned nparam, unsigned dim);
__isl_give isl_dim *isl_dim_copy(__isl_keep isl_dim *dim);
struct isl_dim *isl_dim_cow(struct isl_dim *dim);
void isl_dim_free(__isl_take isl_dim *dim);

__isl_give isl_dim *isl_dim_set_name(__isl_take isl_dim *dim,
				 enum isl_dim_type type, unsigned pos,
				 __isl_keep const char *name);
__isl_keep const char *isl_dim_get_name(__isl_keep isl_dim *dim,
				 enum isl_dim_type type, unsigned pos);

struct isl_dim *isl_dim_extend(struct isl_dim *dim,
			unsigned nparam, unsigned n_in, unsigned n_out);
struct isl_dim *isl_dim_add(struct isl_dim *dim, enum isl_dim_type type,
		unsigned n);
__isl_give isl_dim *isl_dim_move(__isl_take isl_dim *dim,
	enum isl_dim_type dst_type, unsigned dst_pos,
	enum isl_dim_type src_type, unsigned src_pos, unsigned n);
__isl_give isl_dim *isl_dim_insert(__isl_take isl_dim *dim,
	enum isl_dim_type type, unsigned pos, unsigned n);
struct isl_dim *isl_dim_join(struct isl_dim *left, struct isl_dim *right);
struct isl_dim *isl_dim_product(struct isl_dim *left, struct isl_dim *right);
struct isl_dim *isl_dim_map(struct isl_dim *dim);
struct isl_dim *isl_dim_reverse(struct isl_dim *dim);
struct isl_dim *isl_dim_drop(struct isl_dim *dim, enum isl_dim_type type,
		unsigned first, unsigned num);
struct isl_dim *isl_dim_drop_inputs(struct isl_dim *dim,
		unsigned first, unsigned n);
struct isl_dim *isl_dim_drop_outputs(struct isl_dim *dim,
		unsigned first, unsigned n);
struct isl_dim *isl_dim_domain(struct isl_dim *dim);
struct isl_dim *isl_dim_range(struct isl_dim *dim);
struct isl_dim *isl_dim_underlying(struct isl_dim *dim, unsigned n_div);

int isl_dim_equal(struct isl_dim *dim1, struct isl_dim *dim2);
int isl_dim_match(struct isl_dim *dim1, enum isl_dim_type dim1_type,
		struct isl_dim *dim2, enum isl_dim_type dim2_type);
int isl_dim_compatible(struct isl_dim *dim1, struct isl_dim *dim2);
unsigned isl_dim_size(__isl_keep isl_dim *dim, enum isl_dim_type type);
unsigned isl_dim_total(struct isl_dim *dim);

#if defined(__cplusplus)
}
#endif

#endif
