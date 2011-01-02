/*
 * Copyright 2008-2009 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv2.1 license
 *
 * Written by Sven Verdoolaege, K.U.Leuven, Departement
 * Computerwetenschappen, Celestijnenlaan 200A, B-3001 Leuven, Belgium
 */

#ifndef ISL_CONSTRAINT_H
#define ISL_CONSTRAINT_H

#include <isl/div.h>
#include <isl/set.h>

#if defined(__cplusplus)
extern "C" {
#endif

struct isl_constraint {
	int ref;
	struct isl_ctx *ctx;

	struct isl_basic_map	*bmap;
	isl_int			**line;
};
typedef struct isl_constraint isl_constraint;

__isl_give isl_constraint *isl_equality_alloc(__isl_take isl_dim *dim);
__isl_give isl_constraint *isl_inequality_alloc(__isl_take isl_dim *dim);
struct isl_constraint *isl_basic_set_constraint(struct isl_basic_set *bset,
	isl_int **line);

struct isl_constraint *isl_constraint_cow(struct isl_constraint *c);
struct isl_constraint *isl_constraint_copy(struct isl_constraint *c);
void isl_constraint_free(struct isl_constraint *c);

__isl_give isl_constraint *isl_basic_map_first_constraint(
	__isl_take isl_basic_map *bmap);
__isl_give isl_constraint *isl_basic_set_first_constraint(
	__isl_take isl_basic_set *bset);
struct isl_constraint *isl_constraint_next(struct isl_constraint *c);
int isl_basic_map_foreach_constraint(__isl_keep isl_basic_map *bmap,
	int (*fn)(__isl_take isl_constraint *c, void *user), void *user);
int isl_basic_set_foreach_constraint(__isl_keep isl_basic_set *bset,
	int (*fn)(__isl_take isl_constraint *c, void *user), void *user);
int isl_constraint_is_equal(struct isl_constraint *constraint1,
			    struct isl_constraint *constraint2);

int isl_basic_set_foreach_bound_pair(__isl_keep isl_basic_set *bset,
	enum isl_dim_type type, unsigned pos,
	int (*fn)(__isl_take isl_constraint *lower,
		  __isl_take isl_constraint *upper,
		  __isl_take isl_basic_set *bset, void *user), void *user);

__isl_give isl_basic_map *isl_basic_map_add_constraint(
	__isl_take isl_basic_map *bmap, __isl_take isl_constraint *constraint);
__isl_give isl_basic_set *isl_basic_set_add_constraint(
	__isl_take isl_basic_set *bset, __isl_take isl_constraint *constraint);

int isl_basic_map_has_defining_equality(
	__isl_keep isl_basic_map *bmap, enum isl_dim_type type, int pos,
	__isl_give isl_constraint **c);
int isl_basic_set_has_defining_equality(
	struct isl_basic_set *bset, enum isl_dim_type type, int pos,
	struct isl_constraint **constraint);
int isl_basic_set_has_defining_inequalities(
	struct isl_basic_set *bset, enum isl_dim_type type, int pos,
	struct isl_constraint **lower,
	struct isl_constraint **upper);

int isl_constraint_dim(struct isl_constraint *constraint,
	enum isl_dim_type type);

const char *isl_constraint_get_dim_name(__isl_keep isl_constraint *constraint,
	enum isl_dim_type type, unsigned pos);
void isl_constraint_get_constant(__isl_keep isl_constraint *constraint,
	isl_int *v);
void isl_constraint_get_coefficient(__isl_keep isl_constraint *constraint,
	enum isl_dim_type type, int pos, isl_int *v);
void isl_constraint_set_constant(__isl_keep isl_constraint *constraint, isl_int v);
void isl_constraint_set_constant_si(__isl_keep isl_constraint *constraint,
	int v);
void isl_constraint_set_coefficient(__isl_keep isl_constraint *constraint,
	enum isl_dim_type type, int pos, isl_int v);
void isl_constraint_set_coefficient_si(__isl_keep isl_constraint *constraint,
	enum isl_dim_type type, int pos, int v);

__isl_give isl_div *isl_constraint_div(__isl_keep isl_constraint *constraint,
	int pos);
struct isl_constraint *isl_constraint_add_div(struct isl_constraint *constraint,
	struct isl_div *div, int *pos);

void isl_constraint_clear(struct isl_constraint *constraint);
struct isl_constraint *isl_constraint_negate(struct isl_constraint *constraint);

int isl_constraint_is_equality(__isl_keep isl_constraint *constraint);
int isl_constraint_is_div_constraint(__isl_keep isl_constraint *constraint);

__isl_give isl_basic_map *isl_basic_map_from_constraint(
	__isl_take isl_constraint *constraint);
struct isl_basic_set *isl_basic_set_from_constraint(
	struct isl_constraint *constraint);

#if defined(__cplusplus)
}
#endif

#endif
