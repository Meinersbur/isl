/*
 * Copyright 2008-2009 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv2.1 license
 *
 * Written by Sven Verdoolaege, K.U.Leuven, Departement
 * Computerwetenschappen, Celestijnenlaan 200A, B-3001 Leuven, Belgium
 */

#ifndef ISL_MAT_H
#define ISL_MAT_H

#include <stdio.h>

#include <isl_int.h>
#include <isl_ctx.h>
#include <isl_blk.h>
#include <isl_vec.h>

#if defined(__cplusplus)
extern "C" {
#endif

struct isl_mat {
	int ref;

	struct isl_ctx *ctx;

#define ISL_MAT_BORROWED		(1 << 0)
	unsigned flags;

	unsigned n_row;
	unsigned n_col;

	isl_int **row;

	/* actual size of the rows in memory; n_col <= max_col */
	unsigned max_col;

	struct isl_blk block;
};
typedef struct isl_mat	isl_mat;

struct isl_mat *isl_mat_alloc(struct isl_ctx *ctx,
	unsigned n_row, unsigned n_col);
struct isl_mat *isl_mat_dup(struct isl_mat *mat);
struct isl_mat *isl_mat_extend(struct isl_mat *mat,
	unsigned n_row, unsigned n_col);
struct isl_mat *isl_mat_identity(struct isl_ctx *ctx, unsigned n_row);
struct isl_mat *isl_mat_copy(struct isl_mat *mat);
struct isl_mat *isl_mat_cow(struct isl_mat *mat);
void isl_mat_free(struct isl_mat *mat);

struct isl_mat *isl_mat_sub_alloc(struct isl_ctx *ctx, isl_int **row,
	unsigned first_row, unsigned n_row, unsigned first_col, unsigned n_col);
void isl_mat_sub_copy(struct isl_ctx *ctx, isl_int **dst, isl_int **src,
	unsigned n_row, unsigned dst_col, unsigned src_col, unsigned n_col);
void isl_mat_sub_neg(struct isl_ctx *ctx, isl_int **dst, isl_int **src,
	unsigned n_row, unsigned dst_col, unsigned src_col, unsigned n_col);

struct isl_mat *isl_mat_swap_cols(struct isl_mat *mat, unsigned i, unsigned j);
struct isl_mat *isl_mat_swap_rows(struct isl_mat *mat, unsigned i, unsigned j);

struct isl_vec *isl_mat_vec_product(struct isl_mat *mat, struct isl_vec *vec);
struct isl_vec *isl_vec_mat_product(struct isl_vec *vec, struct isl_mat *mat);
__isl_give isl_vec *isl_mat_vec_inverse_product(__isl_take isl_mat *mat,
						__isl_take isl_vec *vec);
struct isl_mat *isl_mat_aff_direct_sum(struct isl_mat *left,
					struct isl_mat *right);
__isl_give isl_mat *isl_mat_diagonal(__isl_take isl_mat *mat1,
	__isl_take isl_mat *mat2);
struct isl_mat *isl_mat_left_hermite(struct isl_mat *M,
	int neg, struct isl_mat **U, struct isl_mat **Q);
struct isl_mat *isl_mat_lin_to_aff(struct isl_mat *mat);
struct isl_mat *isl_mat_inverse_product(struct isl_mat *left,
	struct isl_mat *right);
struct isl_mat *isl_mat_product(struct isl_mat *left, struct isl_mat *right);
struct isl_mat *isl_mat_transpose(struct isl_mat *mat);
struct isl_mat *isl_mat_right_inverse(struct isl_mat *mat);
struct isl_mat *isl_mat_right_kernel(struct isl_mat *mat);

__isl_give isl_mat *isl_mat_normalize(__isl_take isl_mat *mat);

struct isl_mat *isl_mat_drop_cols(struct isl_mat *mat,
				unsigned col, unsigned n);
struct isl_mat *isl_mat_drop_rows(struct isl_mat *mat,
				unsigned row, unsigned n);
__isl_give isl_mat *isl_mat_insert_cols(__isl_take isl_mat *mat,
				unsigned col, unsigned n);
__isl_give isl_mat *isl_mat_insert_rows(__isl_take isl_mat *mat,
				unsigned row, unsigned n);
__isl_give isl_mat *isl_mat_move_cols(__isl_take isl_mat *mat,
	unsigned dst_col, unsigned src_col, unsigned n);

void isl_mat_col_mul(struct isl_mat *mat, int dst_col, isl_int f, int src_col);
void isl_mat_col_submul(struct isl_mat *mat,
			int dst_col, isl_int f, int src_col);

struct isl_mat *isl_mat_unimodular_complete(struct isl_mat *M, int row);

__isl_give isl_mat *isl_mat_from_row_vec(__isl_take isl_vec *vec);
__isl_give isl_mat *isl_mat_concat(__isl_take isl_mat *top,
	__isl_take isl_mat *bot);
__isl_give isl_mat *isl_mat_vec_concat(__isl_take isl_mat *top,
	__isl_take isl_vec *bot);

int isl_mat_is_equal(__isl_keep isl_mat *mat1, __isl_keep isl_mat *mat2);

void isl_mat_dump(struct isl_mat *mat, FILE *out, int indent);

#if defined(__cplusplus)
}
#endif

#endif
