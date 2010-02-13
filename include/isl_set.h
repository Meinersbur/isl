/*
 * Copyright 2008-2009 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv2.1 license
 *
 * Written by Sven Verdoolaege, K.U.Leuven, Departement
 * Computerwetenschappen, Celestijnenlaan 200A, B-3001 Leuven, Belgium
 */

#ifndef ISL_SET_H
#define ISL_SET_H

#include "isl_map.h"
#include "isl_list.h"

#if defined(__cplusplus)
extern "C" {
#endif

/* A "basic set" is a basic map with a zero-dimensional
 * domain.
 */
struct isl_basic_set {
	int ref;
#define ISL_BASIC_SET_FINAL		(1 << 0)
#define ISL_BASIC_SET_EMPTY		(1 << 1)
#define ISL_BASIC_SET_NO_IMPLICIT	(1 << 2)
#define ISL_BASIC_SET_NO_REDUNDANT	(1 << 3)
#define ISL_BASIC_SET_RATIONAL		(1 << 4)
#define ISL_BASIC_SET_NORMALIZED	(1 << 5)
#define ISL_BASIC_SET_NORMALIZED_DIVS	(1 << 6)
#define ISL_BASIC_SET_ALL_EQUALITIES	(1 << 7)
	unsigned flags;

	struct isl_ctx *ctx;

	struct isl_dim *dim;
	unsigned extra;

	unsigned n_eq;
	unsigned n_ineq;

	size_t c_size;
	isl_int **eq;
	isl_int **ineq;

	unsigned n_div;

	isl_int **div;

	struct isl_vec *sample;

	struct isl_blk block;
	struct isl_blk block2;
};

/* A "set" is a (possibly disjoint) union of basic sets.
 *
 * See the documentation of isl_map.
 */
struct isl_set {
	int ref;
#define ISL_SET_DISJOINT		(1 << 0)
#define ISL_SET_NORMALIZED		(1 << 1)
	unsigned flags;

	struct isl_ctx *ctx;

	struct isl_dim *dim;

	int n;

	size_t size;
	struct isl_basic_set *p[1];
};

unsigned isl_basic_set_n_dim(const struct isl_basic_set *bset);
unsigned isl_basic_set_n_param(const struct isl_basic_set *bset);
unsigned isl_basic_set_total_dim(const struct isl_basic_set *bset);
unsigned isl_basic_set_dim(const struct isl_basic_set *bset,
				enum isl_dim_type type);

unsigned isl_set_n_dim(const struct isl_set *set);
unsigned isl_set_n_param(const struct isl_set *set);
unsigned isl_set_dim(const struct isl_set *set, enum isl_dim_type type);

struct isl_dim *isl_basic_set_get_dim(struct isl_basic_set *bset);
struct isl_dim *isl_set_get_dim(struct isl_set *set);

struct isl_basic_set *isl_basic_set_alloc(struct isl_ctx *ctx,
		unsigned nparam, unsigned dim, unsigned extra,
		unsigned n_eq, unsigned n_ineq);
struct isl_basic_set *isl_basic_set_extend(struct isl_basic_set *base,
		unsigned nparam, unsigned dim, unsigned extra,
		unsigned n_eq, unsigned n_ineq);
struct isl_basic_set *isl_basic_set_extend_constraints(
		struct isl_basic_set *base, unsigned n_eq, unsigned n_ineq);
struct isl_basic_set *isl_basic_set_finalize(struct isl_basic_set *bset);
void isl_basic_set_free(__isl_take isl_basic_set *bset);
__isl_give isl_basic_set *isl_basic_set_copy(__isl_keep isl_basic_set *bset);
struct isl_basic_set *isl_basic_set_dup(struct isl_basic_set *bset);
__isl_give isl_basic_set *isl_basic_set_empty(__isl_take isl_dim *dim);
struct isl_basic_set *isl_basic_set_empty_like(struct isl_basic_set *bset);
__isl_give isl_basic_set *isl_basic_set_universe(__isl_take isl_dim *dim);
struct isl_basic_set *isl_basic_set_universe_like(struct isl_basic_set *bset);
__isl_give isl_basic_set *isl_basic_set_universe_like_set(
	__isl_keep isl_set *model);
struct isl_basic_set *isl_basic_set_interval(struct isl_ctx *ctx,
	isl_int min, isl_int max);
struct isl_basic_set *isl_basic_set_positive_orthant(struct isl_dim *dims);
void isl_basic_set_dump(__isl_keep isl_basic_set *bset,
				FILE *out, int indent);
struct isl_basic_set *isl_basic_set_swap_vars(
		struct isl_basic_set *bset, unsigned n);
__isl_give isl_basic_set *isl_basic_set_intersect(
		__isl_take isl_basic_set *bset1,
		__isl_take isl_basic_set *bset2);
__isl_give isl_basic_set *isl_basic_set_apply(
		__isl_take isl_basic_set *bset,
		__isl_take isl_basic_map *bmap);
__isl_give isl_basic_set *isl_basic_set_affine_hull(
		__isl_take isl_basic_set *bset);
__isl_give isl_basic_set *isl_basic_set_remove(__isl_take isl_basic_set *bset,
	enum isl_dim_type type, unsigned first, unsigned n);
struct isl_basic_set *isl_basic_set_simplify(struct isl_basic_set *bset);
__isl_give isl_basic_set *isl_basic_set_detect_equalities(
						__isl_take isl_basic_set *bset);
struct isl_basic_set *isl_basic_set_product(struct isl_basic_set_list *list);

__isl_give isl_basic_set *isl_basic_set_read_from_file(isl_ctx *ctx,
		FILE *input, int nparam);
__isl_give isl_basic_set *isl_basic_set_read_from_str(isl_ctx *ctx,
		const char *str, int nparam);
__isl_give isl_set *isl_set_read_from_file(isl_ctx *ctx,
		FILE *input, int nparam);
__isl_give isl_set *isl_set_read_from_str(isl_ctx *ctx,
		const char *str, int nparam);
#define ISL_FORMAT_ISL			0
#define ISL_FORMAT_POLYLIB		1
#define ISL_FORMAT_POLYLIB_CONSTRAINTS	2
void isl_basic_set_print(__isl_keep isl_basic_set *bset, FILE *out, int indent,
	const char *prefix, const char *suffix, unsigned output_format);
void isl_set_print(__isl_keep struct isl_set *set, FILE *out, int indent,
	unsigned output_format);
__isl_give isl_basic_set *isl_basic_set_fix(__isl_take isl_basic_set *bset,
		enum isl_dim_type type, unsigned pos, isl_int value);
struct isl_basic_set *isl_basic_set_fix_si(struct isl_basic_set *bset,
		enum isl_dim_type type, unsigned pos, int value);
__isl_give isl_set *isl_set_fix_si(__isl_take isl_set *set,
		enum isl_dim_type type, unsigned pos, int value);

struct isl_basic_set *isl_basic_set_from_underlying_set(
	struct isl_basic_set *bset, struct isl_basic_set *like);
struct isl_set *isl_set_from_underlying_set(
	struct isl_set *set, struct isl_basic_set *like);
struct isl_set *isl_set_to_underlying_set(struct isl_set *set);

int isl_basic_set_is_equal(
		struct isl_basic_set *bset1, struct isl_basic_set *bset2);

__isl_give isl_set *isl_basic_set_partial_lexmin(
		__isl_take isl_basic_set *bset, __isl_take isl_basic_set *dom,
		__isl_give isl_set **empty);
__isl_give isl_set *isl_basic_set_partial_lexmax(
		__isl_take isl_basic_set *bset, __isl_take isl_basic_set *dom,
		__isl_give isl_set **empty);
__isl_give isl_set *isl_set_partial_lexmin(
		__isl_take isl_set *set, __isl_take isl_set *dom,
		__isl_give isl_set **empty);
__isl_give isl_set *isl_set_partial_lexmax(
		__isl_take isl_set *set, __isl_take isl_set *dom,
		__isl_give isl_set **empty);
__isl_give isl_set *isl_basic_set_lexmin(__isl_take isl_basic_set *bset);
__isl_give isl_set *isl_basic_set_lexmax(__isl_take isl_basic_set *bset);
__isl_give isl_set *isl_set_lexmin(__isl_take isl_set *set);
__isl_give isl_set *isl_set_lexmax(__isl_take isl_set *set);
__isl_give isl_set *isl_basic_set_union(
		__isl_take isl_basic_set *bset1,
		__isl_take isl_basic_set *bset2);

int isl_basic_set_compare_at(struct isl_basic_set *bset1,
	struct isl_basic_set *bset2, int pos);
int isl_set_follows_at(__isl_keep isl_set *set1,
	__isl_keep isl_set *set2, int pos);

int isl_basic_set_is_universe(__isl_keep isl_basic_set *bset);
int isl_basic_set_fast_is_empty(__isl_keep isl_basic_set *bset);
int isl_basic_set_is_empty(__isl_keep isl_basic_set *bset);

struct isl_set *isl_set_alloc(struct isl_ctx *ctx,
		unsigned nparam, unsigned dim, int n, unsigned flags);
struct isl_set *isl_set_extend(struct isl_set *base,
		unsigned nparam, unsigned dim);
__isl_give isl_set *isl_set_empty(__isl_take isl_dim *dim);
struct isl_set *isl_set_empty_like(struct isl_set *set);
__isl_give isl_set *isl_set_universe(__isl_take isl_dim *dim);
__isl_give isl_set *isl_set_universe_like(__isl_keep isl_set *model);
__isl_give isl_set *isl_set_add_basic_set(__isl_take isl_set *set,
						__isl_take isl_basic_set *bset);
struct isl_set *isl_set_finalize(struct isl_set *set);
__isl_give isl_set *isl_set_copy(__isl_keep isl_set *set);
void isl_set_free(__isl_take isl_set *set);
struct isl_set *isl_set_dup(struct isl_set *set);
__isl_give isl_set *isl_set_from_basic_set(__isl_take isl_basic_set *bset);
__isl_give isl_basic_set *isl_set_sample(__isl_take isl_set *set);
__isl_give isl_set *isl_set_detect_equalities(__isl_take isl_set *set);
__isl_give isl_basic_set *isl_set_affine_hull(__isl_take isl_set *set);
__isl_give isl_basic_set *isl_set_convex_hull(__isl_take isl_set *set);
struct isl_basic_set *isl_set_simple_hull(struct isl_set *set);
struct isl_basic_set *isl_set_bounded_simple_hull(struct isl_set *set);

struct isl_set *isl_set_union_disjoint(
			struct isl_set *set1, struct isl_set *set2);
__isl_give isl_set *isl_set_union(
		__isl_take isl_set *set1,
		__isl_take isl_set *set2);
struct isl_set *isl_set_product(struct isl_set *set1, struct isl_set *set2);
__isl_give isl_set *isl_set_intersect(
		__isl_take isl_set *set1,
		__isl_take isl_set *set2);
__isl_give isl_set *isl_set_subtract(
		__isl_take isl_set *set1,
		__isl_take isl_set *set2);
__isl_give isl_set *isl_set_apply(
		__isl_take isl_set *set,
		__isl_take isl_map *map);
__isl_give isl_set *isl_set_fix(__isl_take isl_set *set,
		enum isl_dim_type type, unsigned pos, isl_int value);
struct isl_set *isl_set_fix_dim_si(struct isl_set *set,
		unsigned dim, int value);
struct isl_set *isl_set_lower_bound_dim(struct isl_set *set,
		unsigned dim, isl_int value);
__isl_give isl_basic_set *isl_basic_set_add(__isl_take isl_basic_set *bset,
		enum isl_dim_type type, unsigned n);
__isl_give isl_set *isl_set_add(__isl_take isl_set *set,
		enum isl_dim_type type, unsigned n);
__isl_give isl_basic_set *isl_basic_set_project_out(
		__isl_take isl_basic_set *bset,
		enum isl_dim_type type, unsigned first, unsigned n);
__isl_give isl_set *isl_set_project_out(__isl_take isl_set *set,
		enum isl_dim_type type, unsigned first, unsigned n);
struct isl_basic_set *isl_basic_set_remove_dims(struct isl_basic_set *bset,
		unsigned first, unsigned n);
struct isl_basic_set *isl_basic_set_remove_divs(struct isl_basic_set *bset);
struct isl_set *isl_set_eliminate_dims(struct isl_set *set,
		unsigned first, unsigned n);
__isl_give isl_set *isl_set_remove(__isl_take isl_set *bset,
	enum isl_dim_type type, unsigned first, unsigned n);
struct isl_set *isl_set_remove_dims(struct isl_set *set,
		unsigned first, unsigned n);
struct isl_set *isl_set_remove_divs(struct isl_set *set);

void isl_set_dump(__isl_keep isl_set *set, FILE *out, int indent);
struct isl_set *isl_set_swap_vars(struct isl_set *set, unsigned n);
int isl_set_fast_is_empty(__isl_keep isl_set *set);
int isl_set_is_empty(__isl_keep isl_set *set);
int isl_set_is_subset(__isl_keep isl_set *set1, __isl_keep isl_set *set2);
int isl_set_is_strict_subset(__isl_keep isl_set *set1, __isl_keep isl_set *set2);
int isl_set_is_equal(__isl_keep isl_set *set1, __isl_keep isl_set *set2);

struct isl_set *isl_basic_set_compute_divs(struct isl_basic_set *bset);
__isl_give isl_set *isl_set_compute_divs(__isl_take isl_set *set);
__isl_give isl_set *isl_set_align_divs(__isl_take isl_set *set);

struct isl_basic_set *isl_set_copy_basic_set(struct isl_set *set);
struct isl_set *isl_set_drop_basic_set(struct isl_set *set,
						struct isl_basic_set *bset);

int isl_basic_set_fast_dim_is_fixed(struct isl_basic_set *bset, unsigned dim,
	isl_int *val);

int isl_set_fast_dim_is_fixed(struct isl_set *set, unsigned dim, isl_int *val);
int isl_set_fast_dim_has_fixed_lower_bound(struct isl_set *set,
	unsigned dim, isl_int *val);

struct isl_basic_set *isl_basic_set_gist(struct isl_basic_set *bset,
						struct isl_basic_set *context);
struct isl_set *isl_set_gist(struct isl_set *set,
	struct isl_basic_set *context);
int isl_basic_set_dim_residue_class(struct isl_basic_set *bset,
	int pos, isl_int *modulo, isl_int *residue);
int isl_set_dim_residue_class(struct isl_set *set,
	int pos, isl_int *modulo, isl_int *residue);

__isl_give isl_set *isl_set_coalesce(__isl_take isl_set *set);

int isl_set_fast_is_equal(__isl_keep isl_set *set1, __isl_keep isl_set *set2);
int isl_set_fast_is_disjoint(__isl_keep isl_set *set1, __isl_keep isl_set *set2);

uint32_t isl_set_get_hash(struct isl_set *set);

int isl_set_dim_is_unique(struct isl_set *set, unsigned dim);

int isl_set_foreach_basic_set(__isl_keep isl_set *set,
	int (*fn)(__isl_take isl_basic_set *bset, void *user), void *user);

int isl_set_size(__isl_keep isl_set *set);

#if defined(__cplusplus)
}
#endif

#endif
