#ifndef ISL_SET_H
#define ISL_SET_H

#include "isl_map.h"

#if defined(__cplusplus)
extern "C" {
#endif

/* A "basic set" is a basic map with a zero-dimensional
 * domain.
 */
struct isl_basic_set {
	int ref;
#define ISL_BASIC_SET_FINAL		(1 << 0)
	unsigned flags;

	unsigned nparam;
	unsigned zero;
	unsigned dim;
	unsigned extra;

	unsigned n_eq;
	unsigned n_ineq;

	size_t c_size;
	isl_int **eq;
	isl_int **ineq;

	unsigned n_div;

	isl_int **div;

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
	unsigned flags;

	unsigned nparam;
	unsigned zero;
	unsigned dim;

	int n;

	size_t size;
	struct isl_basic_set *p[0];
};

struct isl_basic_set *isl_basic_set_alloc(struct isl_ctx *ctx,
		unsigned nparam, unsigned dim, unsigned extra,
		unsigned n_eq, unsigned n_ineq);
struct isl_basic_set *isl_basic_set_extend(struct isl_ctx *ctx,
		struct isl_basic_set *base,
		unsigned nparam, unsigned dim, unsigned extra,
		unsigned n_eq, unsigned n_ineq);
struct isl_basic_set *isl_basic_set_finalize(struct isl_ctx *ctx,
		struct isl_basic_set *bset);
void isl_basic_set_free(struct isl_ctx *ctx, struct isl_basic_set *bset);
struct isl_basic_set *isl_basic_set_copy(struct isl_ctx *ctx,
					struct isl_basic_set *bset);
void isl_basic_set_dump(struct isl_ctx *ctx, struct isl_basic_set *bset,
				FILE *out, int indent);
struct isl_basic_set *isl_basic_set_swap_vars(struct isl_ctx *ctx,
		struct isl_basic_set *bset, unsigned n);
struct isl_basic_set *isl_basic_set_drop_vars(struct isl_ctx *ctx,
		struct isl_basic_set *bset, unsigned first, unsigned n);
struct isl_basic_set *isl_basic_set_intersect(
		struct isl_ctx *ctx, struct isl_basic_set *bset1,
		struct isl_basic_set *bset2);
struct isl_basic_set *isl_basic_set_affine_hull(struct isl_ctx *ctx,
						struct isl_basic_set *bset);
struct isl_basic_set *isl_basic_set_simplify(
		struct isl_ctx *ctx, struct isl_basic_set *bset);

struct isl_set *isl_basic_set_lexmin(struct isl_ctx *ctx,
		struct isl_basic_set *bset);
struct isl_set *isl_basic_set_union(
		struct isl_ctx *ctx, struct isl_basic_set *bset1,
		struct isl_basic_set *bset2);

struct isl_set *isl_set_alloc(struct isl_ctx *ctx,
		unsigned nparam, unsigned dim, int n, unsigned flags);
struct isl_set *isl_set_empty(struct isl_ctx *ctx,
		unsigned nparam, unsigned dim);
struct isl_set *isl_set_add(struct isl_ctx *ctx, struct isl_set *set,
				struct isl_basic_set *bset);
struct isl_set *isl_set_finalize(struct isl_ctx *ctx, struct isl_set *set);
struct isl_set *isl_set_copy(struct isl_ctx *ctx, struct isl_set *set);
void isl_set_free(struct isl_ctx *ctx, struct isl_set *set);
struct isl_set *isl_set_dup(struct isl_ctx *ctx, struct isl_set *set);
struct isl_set *isl_set_from_basic_set(struct isl_ctx *ctx,
				struct isl_basic_set *bset);
struct isl_basic_set *isl_set_affine_hull(struct isl_ctx *ctx,
		struct isl_set *set);

struct isl_set *isl_set_union_disjoint(struct isl_ctx *ctx,
			struct isl_set *set1, struct isl_set *set2);
struct isl_set *isl_set_union(struct isl_ctx *ctx,
			struct isl_set *set1, struct isl_set *set2);
struct isl_set *isl_set_subtract(struct isl_ctx *ctx, struct isl_set *set1,
		struct isl_set *set2);
struct isl_set *isl_set_apply(struct isl_ctx *ctx, struct isl_set *set,
		struct isl_map *map);

void isl_set_dump(struct isl_ctx *ctx, struct isl_set *set, FILE *out,
		  int indent);
struct isl_set *isl_set_swap_vars(struct isl_ctx *ctx,
		struct isl_set *set, unsigned n);
int isl_set_is_empty(struct isl_ctx *ctx, struct isl_set *set);
int isl_set_is_subset(struct isl_ctx *ctx,
		struct isl_set *set1, struct isl_set *set2);
int isl_set_is_equal(struct isl_ctx *ctx,
		struct isl_set *set1, struct isl_set *set2);

struct isl_set *isl_basic_set_compute_divs(struct isl_ctx *ctx,
		struct isl_basic_set *bset);

#if defined(__cplusplus)
}
#endif

#endif
