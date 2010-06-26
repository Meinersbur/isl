/*
 * Copyright 2008-2009 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv2.1 license
 *
 * Written by Sven Verdoolaege, K.U.Leuven, Departement
 * Computerwetenschappen, Celestijnenlaan 200A, B-3001 Leuven, Belgium
 */

#ifndef ISL_MAP_H
#define ISL_MAP_H

#include <stdio.h>

#include <isl_int.h>
#include <isl_ctx.h>
#include <isl_blk.h>
#include <isl_dim.h>
#include <isl_vec.h>
#include <isl_mat.h>
#include <isl_printer.h>

#if defined(__cplusplus)
extern "C" {
#endif

/* General notes:
 *
 * All structures are reference counted to allow reuse without duplication.
 * A *_copy operation will increase the reference count, while a *_free
 * operation will decrease the reference count and only actually release
 * the structures when the reference count drops to zero.
 *
 * Functions that return an isa structure will in general _destroy_
 * all argument isa structures (the obvious execption begin the _copy
 * functions).  A pointer passed to such a function may therefore
 * never be used after the function call.  If you want to keep a
 * reference to the old structure(s), use the appropriate _copy function.
 */

/* A "basic map" is a relation between two sets of variables,
 * called the "in" and "out" variables.
 *
 * It is implemented as a set with two extra fields:
 * n_in is the number of in variables
 * n_out is the number of out variables
 * n_in + n_out should be equal to set.dim
 */
struct isl_dim;
struct isl_basic_map {
	int ref;
#define ISL_BASIC_MAP_FINAL		(1 << 0)
#define ISL_BASIC_MAP_EMPTY		(1 << 1)
#define ISL_BASIC_MAP_NO_IMPLICIT	(1 << 2)
#define ISL_BASIC_MAP_NO_REDUNDANT	(1 << 3)
#define ISL_BASIC_MAP_RATIONAL		(1 << 4)
#define ISL_BASIC_MAP_NORMALIZED	(1 << 5)
#define ISL_BASIC_MAP_NORMALIZED_DIVS	(1 << 6)
#define ISL_BASIC_MAP_ALL_EQUALITIES	(1 << 7)
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
typedef struct isl_basic_map isl_basic_map;
struct isl_basic_set;
typedef struct isl_basic_set isl_basic_set;

/* A "map" is a (disjoint) union of basic maps.
 *
 * Currently, the isl_set structure is identical to the isl_map structure
 * and the library depends on this correspondence internally.
 * However, users should not depend on this correspondence.
 */
struct isl_map {
	int ref;
#define ISL_MAP_DISJOINT		(1 << 0)
#define ISL_MAP_NORMALIZED		(1 << 1)
	unsigned flags;

	struct isl_ctx *ctx;

	struct isl_dim *dim;

	int n;

	size_t size;
	struct isl_basic_map *p[1];
};
typedef struct isl_map isl_map;
struct isl_set;
typedef struct isl_set isl_set;

unsigned isl_basic_map_n_in(const struct isl_basic_map *bmap);
unsigned isl_basic_map_n_out(const struct isl_basic_map *bmap);
unsigned isl_basic_map_n_param(const struct isl_basic_map *bmap);
unsigned isl_basic_map_n_div(const struct isl_basic_map *bmap);
unsigned isl_basic_map_total_dim(const struct isl_basic_map *bmap);
unsigned isl_basic_map_dim(const struct isl_basic_map *bmap,
				enum isl_dim_type type);

unsigned isl_map_n_in(const struct isl_map *map);
unsigned isl_map_n_out(const struct isl_map *map);
unsigned isl_map_n_param(const struct isl_map *map);
unsigned isl_map_dim(const struct isl_map *map, enum isl_dim_type type);

__isl_give isl_dim *isl_basic_map_get_dim(__isl_keep isl_basic_map *bmap);
__isl_give isl_dim *isl_map_get_dim(__isl_keep isl_map *map);

__isl_give isl_basic_map *isl_basic_map_set_dim_name(
	__isl_take isl_basic_map *bmap,
	enum isl_dim_type type, unsigned pos, const char *s);
__isl_give isl_map *isl_map_set_dim_name(__isl_take isl_map *map,
	enum isl_dim_type type, unsigned pos, const char *s);

int isl_basic_map_is_rational(__isl_keep isl_basic_map *bmap);

struct isl_basic_map *isl_basic_map_alloc(struct isl_ctx *ctx,
		unsigned nparam, unsigned in, unsigned out, unsigned extra,
		unsigned n_eq, unsigned n_ineq);
__isl_give isl_basic_map *isl_basic_map_identity(__isl_take isl_dim *set_dim);
struct isl_basic_map *isl_basic_map_identity_like(struct isl_basic_map *model);
struct isl_basic_map *isl_basic_map_finalize(struct isl_basic_map *bmap);
void isl_basic_map_free(__isl_take isl_basic_map *bmap);
__isl_give isl_basic_map *isl_basic_map_copy(__isl_keep isl_basic_map *bmap);
struct isl_basic_map *isl_basic_map_extend(struct isl_basic_map *base,
		unsigned nparam, unsigned n_in, unsigned n_out, unsigned extra,
		unsigned n_eq, unsigned n_ineq);
struct isl_basic_map *isl_basic_map_extend_constraints(
		struct isl_basic_map *base, unsigned n_eq, unsigned n_ineq);
struct isl_basic_map *isl_basic_map_equal(
		struct isl_dim *dim, unsigned n_equal);
struct isl_basic_map *isl_basic_map_less_at(struct isl_dim *dim, unsigned pos);
struct isl_basic_map *isl_basic_map_more_at(struct isl_dim *dim, unsigned pos);
__isl_give isl_basic_map *isl_basic_map_empty(__isl_take isl_dim *dim);
struct isl_basic_map *isl_basic_map_empty_like(struct isl_basic_map *model);
struct isl_basic_map *isl_basic_map_empty_like_map(struct isl_map *model);
__isl_give isl_basic_map *isl_basic_map_universe(__isl_take isl_dim *dim);
__isl_give isl_basic_map *isl_basic_map_universe_like(
		__isl_keep isl_basic_map *bmap);
__isl_give isl_basic_map *isl_basic_map_remove_redundancies(
	__isl_take isl_basic_map *bmap);
__isl_give isl_basic_map *isl_map_simple_hull(__isl_take isl_map *map);

__isl_give isl_basic_map *isl_basic_map_intersect_domain(
		__isl_take isl_basic_map *bmap,
		__isl_take isl_basic_set *bset);
__isl_give isl_basic_map *isl_basic_map_intersect_range(
		__isl_take isl_basic_map *bmap,
		__isl_take isl_basic_set *bset);
__isl_give isl_basic_map *isl_basic_map_intersect(
		__isl_take isl_basic_map *bmap1,
		__isl_take isl_basic_map *bmap2);
__isl_give isl_map *isl_basic_map_union(
		__isl_take isl_basic_map *bmap1,
		__isl_take isl_basic_map *bmap2);
__isl_give isl_basic_map *isl_basic_map_apply_domain(
		__isl_take isl_basic_map *bmap1,
		__isl_take isl_basic_map *bmap2);
__isl_give isl_basic_map *isl_basic_map_apply_range(
		__isl_take isl_basic_map *bmap1,
		__isl_take isl_basic_map *bmap2);
__isl_give isl_basic_map *isl_basic_map_affine_hull(
		__isl_take isl_basic_map *bmap);
__isl_give isl_basic_map *isl_basic_map_reverse(__isl_take isl_basic_map *bmap);
__isl_give isl_basic_set *isl_basic_map_domain(__isl_take isl_basic_map *bmap);
__isl_give isl_basic_set *isl_basic_map_range(__isl_take isl_basic_map *bmap);
struct isl_basic_map *isl_basic_map_remove(struct isl_basic_map *bmap,
	enum isl_dim_type type, unsigned first, unsigned n);
struct isl_basic_map *isl_basic_map_from_basic_set(struct isl_basic_set *bset,
		struct isl_dim *dim);
struct isl_basic_set *isl_basic_set_from_basic_map(struct isl_basic_map *bmap);
__isl_give isl_basic_map *isl_basic_map_sample(__isl_take isl_basic_map *bmap);
struct isl_basic_map *isl_basic_map_simplify(struct isl_basic_map *bmap);
struct isl_basic_map *isl_basic_map_detect_equalities(
						struct isl_basic_map *bmap);
__isl_give isl_basic_map *isl_basic_map_read_from_file(isl_ctx *ctx,
		FILE *input, int nparam);
__isl_give isl_basic_map *isl_basic_map_read_from_str(isl_ctx *ctx,
		const char *str, int nparam);
__isl_give isl_map *isl_map_read_from_file(struct isl_ctx *ctx,
		FILE *input, int nparam);
__isl_give isl_map *isl_map_read_from_str(isl_ctx *ctx,
		const char *str, int nparam);
void isl_basic_map_print(__isl_keep isl_basic_map *bmap, FILE *out, int indent,
	const char *prefix, const char *suffix, unsigned output_format);
void isl_map_print(__isl_keep isl_map *map, FILE *out, int indent,
	unsigned output_format);
__isl_give isl_printer *isl_printer_print_basic_map(
	__isl_take isl_printer *printer, __isl_keep isl_basic_map *bmap);
__isl_give isl_printer *isl_printer_print_map(__isl_take isl_printer *printer,
	__isl_keep isl_map *map);
struct isl_basic_map *isl_basic_map_fix_si(struct isl_basic_map *bmap,
		enum isl_dim_type type, unsigned pos, int value);
__isl_give isl_basic_map *isl_basic_map_lower_bound_si(
		__isl_take isl_basic_map *bmap,
		enum isl_dim_type type, unsigned pos, int value);

struct isl_basic_map *isl_basic_map_sum(
		struct isl_basic_map *bmap1, struct isl_basic_map *bmap2);
struct isl_basic_map *isl_basic_map_neg(struct isl_basic_map *bmap);
struct isl_basic_map *isl_basic_map_floordiv(struct isl_basic_map *bmap,
		isl_int d);

struct isl_map *isl_map_sum(struct isl_map *map1, struct isl_map *map2);
struct isl_map *isl_map_neg(struct isl_map *map);
struct isl_map *isl_map_floordiv(struct isl_map *map, isl_int d);

int isl_basic_map_is_equal(
		__isl_keep isl_basic_map *bmap1,
		__isl_keep isl_basic_map *bmap2);

__isl_give isl_map *isl_basic_map_partial_lexmax(
		__isl_take isl_basic_map *bmap, __isl_take isl_basic_set *dom,
		__isl_give isl_set **empty);
__isl_give isl_map *isl_basic_map_partial_lexmin(
		__isl_take isl_basic_map *bmap, __isl_take isl_basic_set *dom,
		__isl_give isl_set **empty);
__isl_give isl_map *isl_map_partial_lexmax(
		__isl_take isl_map *map, __isl_take isl_set *dom,
		__isl_give isl_set **empty);
__isl_give isl_map *isl_map_partial_lexmin(
		__isl_take isl_map *map, __isl_take isl_set *dom,
		__isl_give isl_set **empty);
__isl_give isl_map *isl_basic_map_lexmin(__isl_take isl_basic_map *bmap);
__isl_give isl_map *isl_basic_map_lexmax(__isl_take isl_basic_map *bmap);
__isl_give isl_map *isl_map_lexmin(__isl_take isl_map *map);
__isl_give isl_map *isl_map_lexmax(__isl_take isl_map *map);
int isl_basic_map_foreach_lexmin(__isl_keep isl_basic_map *bmap,
	int (*fn)(__isl_take isl_basic_set *dom, __isl_take isl_mat *map,
		  void *user),
	void *user);

void isl_basic_map_dump(__isl_keep isl_basic_map *bmap, FILE *out, int indent);

struct isl_basic_map *isl_map_copy_basic_map(struct isl_map *map);
__isl_give isl_map *isl_map_drop_basic_map(__isl_take isl_map *map,
						__isl_keep isl_basic_map *bmap);

int isl_basic_map_fast_is_fixed(struct isl_basic_map *bmap,
	enum isl_dim_type type, unsigned pos, isl_int *val);

int isl_basic_map_is_universe(__isl_keep isl_basic_map *bmap);
int isl_basic_map_fast_is_empty(__isl_keep isl_basic_map *bmap);
int isl_basic_map_is_empty(__isl_keep isl_basic_map *bmap);
int isl_basic_map_is_subset(__isl_keep isl_basic_map *bmap1,
		__isl_keep isl_basic_map *bmap2);
int isl_basic_map_is_strict_subset(__isl_keep isl_basic_map *bmap1,
		__isl_keep isl_basic_map *bmap2);

struct isl_map *isl_map_alloc(struct isl_ctx *ctx,
		unsigned nparam, unsigned in, unsigned out, int n,
		unsigned flags);
__isl_give isl_map *isl_map_universe(__isl_take isl_dim *dim);
__isl_give isl_map *isl_map_empty(__isl_take isl_dim *dim);
struct isl_map *isl_map_empty_like(struct isl_map *model);
struct isl_map *isl_map_empty_like_basic_map(struct isl_basic_map *model);
struct isl_map *isl_map_dup(struct isl_map *map);
__isl_give isl_map *isl_map_add_basic_map(__isl_take isl_map *map,
						__isl_take isl_basic_map *bmap);
__isl_give isl_map *isl_map_identity(__isl_take isl_dim *set_dim);
struct isl_map *isl_map_identity_like(struct isl_map *model);
struct isl_map *isl_map_identity_like_basic_map(struct isl_basic_map *model);
__isl_give isl_map *isl_map_lex_lt_first(__isl_take isl_dim *dim, unsigned n);
__isl_give isl_map *isl_map_lex_le_first(__isl_take isl_dim *dim, unsigned n);
__isl_give isl_map *isl_map_lex_lt(__isl_take isl_dim *set_dim);
__isl_give isl_map *isl_map_lex_le(__isl_take isl_dim *set_dim);
__isl_give isl_map *isl_map_lex_gt_first(__isl_take isl_dim *dim, unsigned n);
__isl_give isl_map *isl_map_lex_ge_first(__isl_take isl_dim *dim, unsigned n);
__isl_give isl_map *isl_map_lex_gt(__isl_take isl_dim *set_dim);
__isl_give isl_map *isl_map_lex_ge(__isl_take isl_dim *set_dim);
struct isl_map *isl_map_finalize(struct isl_map *map);
void isl_map_free(__isl_take isl_map *map);
__isl_give isl_map *isl_map_copy(__isl_keep isl_map *map);
struct isl_map *isl_map_extend(struct isl_map *base,
		unsigned nparam, unsigned n_in, unsigned n_out);
__isl_give isl_map *isl_map_reverse(__isl_take isl_map *map);
__isl_give isl_map *isl_map_union(
		__isl_take isl_map *map1,
		__isl_take isl_map *map2);
struct isl_map *isl_map_union_disjoint(
			struct isl_map *map1, struct isl_map *map2);
__isl_give isl_map *isl_map_intersect_domain(
		__isl_take isl_map *map,
		__isl_take isl_set *set);
__isl_give isl_map *isl_map_intersect_range(
		__isl_take isl_map *map,
		__isl_take isl_set *set);
__isl_give isl_map *isl_map_apply_domain(
		__isl_take isl_map *map1,
		__isl_take isl_map *map2);
__isl_give isl_map *isl_map_apply_range(
		__isl_take isl_map *map1,
		__isl_take isl_map *map2);
struct isl_map *isl_map_product(struct isl_map *map1, struct isl_map *map2);
__isl_give isl_map *isl_map_intersect(__isl_take isl_map *map1,
				      __isl_take isl_map *map2);
__isl_give isl_map *isl_map_subtract(
		__isl_take isl_map *map1,
		__isl_take isl_map *map2);
struct isl_map *isl_map_fix_input_si(struct isl_map *map,
		unsigned input, int value);
struct isl_map *isl_map_fix_si(struct isl_map *map,
		enum isl_dim_type type, unsigned pos, int value);
__isl_give isl_map *isl_map_lower_bound_si(__isl_take isl_map *map,
		enum isl_dim_type type, unsigned pos, int value);
__isl_give isl_basic_set *isl_basic_map_deltas(__isl_take isl_basic_map *bmap);
__isl_give isl_set *isl_map_deltas(__isl_take isl_map *map);
struct isl_map *isl_map_detect_equalities(struct isl_map *map);
__isl_give isl_basic_map *isl_map_affine_hull(__isl_take isl_map *map);
__isl_give isl_basic_map *isl_map_convex_hull(__isl_take isl_map *map);
__isl_give isl_basic_map *isl_basic_map_add(__isl_take isl_basic_map *bmap,
		enum isl_dim_type type, unsigned n);
__isl_give isl_map *isl_map_add(__isl_take isl_map *map,
		enum isl_dim_type type, unsigned n);
__isl_give isl_map *isl_map_insert(__isl_take isl_map *map,
		enum isl_dim_type type, unsigned pos, unsigned n);
__isl_give isl_map *isl_map_move_dims(__isl_take isl_map *map,
	enum isl_dim_type dst_type, unsigned dst_pos,
	enum isl_dim_type src_type, unsigned src_pos, unsigned n);
__isl_give isl_basic_map *isl_basic_map_project_out(
		__isl_take isl_basic_map *bmap,
		enum isl_dim_type type, unsigned first, unsigned n);
__isl_give isl_map *isl_map_project_out(__isl_take isl_map *map,
		enum isl_dim_type type, unsigned first, unsigned n);
struct isl_map *isl_map_remove(struct isl_map *map,
	enum isl_dim_type type, unsigned first, unsigned n);
struct isl_map *isl_map_remove_inputs(struct isl_map *map,
	unsigned first, unsigned n);

__isl_give isl_set *isl_map_domain(__isl_take isl_map *bmap);
__isl_give isl_set *isl_map_range(__isl_take isl_map *map);
__isl_give isl_map *isl_map_from_basic_map(__isl_take isl_basic_map *bmap);
__isl_give isl_map *isl_map_from_domain(__isl_take isl_set *set);
struct isl_map *isl_map_from_range(struct isl_set *set);
__isl_give isl_map *isl_map_from_domain_and_range(__isl_take isl_set *domain,
	__isl_take isl_set *range);
struct isl_map *isl_map_from_set(struct isl_set *set, struct isl_dim *dim);
struct isl_set *isl_set_from_map(struct isl_map *map);
__isl_give isl_basic_map *isl_map_sample(__isl_take isl_map *map);

int isl_map_fast_is_empty(__isl_keep isl_map *map);
int isl_map_fast_is_universe(__isl_keep isl_map *map);
int isl_map_is_empty(__isl_keep isl_map *map);
int isl_map_is_subset(__isl_keep isl_map *map1, __isl_keep isl_map *map2);
int isl_map_is_strict_subset(__isl_keep isl_map *map1, __isl_keep isl_map *map2);
int isl_map_is_equal(__isl_keep isl_map *map1, __isl_keep isl_map *map2);
int isl_map_is_single_valued(__isl_keep isl_map *map);
int isl_map_is_translation(__isl_keep isl_map *map);

__isl_give isl_map *isl_map_make_disjoint(__isl_take isl_map *map);
__isl_give isl_map *isl_basic_map_compute_divs(__isl_take isl_basic_map *bmap);
__isl_give isl_map *isl_map_compute_divs(__isl_take isl_map *map);
__isl_give isl_map *isl_map_align_divs(__isl_take isl_map *map);

void isl_map_dump(__isl_keep isl_map *map, FILE *out, int indent);

int isl_map_fast_input_is_fixed(struct isl_map *map,
		unsigned in, isl_int *val);
int isl_map_fast_is_fixed(struct isl_map *map,
	enum isl_dim_type type, unsigned pos, isl_int *val);

__isl_give isl_basic_map *isl_basic_map_gist(__isl_take isl_basic_map *bmap,
	__isl_take isl_basic_map *context);
__isl_give isl_map *isl_map_gist(__isl_take isl_map *map,
	__isl_take isl_map *context);
__isl_give isl_map *isl_map_gist_basic_map(__isl_take isl_map *map,
	__isl_take isl_basic_map *context);

__isl_give isl_map *isl_map_coalesce(__isl_take isl_map *map);

int isl_map_fast_is_equal(__isl_keep isl_map *map1, __isl_keep isl_map *map2);

int isl_map_foreach_basic_map(__isl_keep isl_map *map,
	int (*fn)(__isl_take isl_basic_map *bmap, void *user), void *user);

__isl_give isl_map *isl_set_lifting(__isl_take isl_set *set);

__isl_give isl_map *isl_map_power(__isl_take isl_map *map, unsigned param,
	int *exact);
__isl_give isl_map *isl_map_transitive_closure(__isl_take isl_map *map,
	int *exact);

#if defined(__cplusplus)
}
#endif

#endif
