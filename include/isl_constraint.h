#ifndef ISL_CONSTRAINT_H
#define ISL_CONSTRAINT_H

#include "isl_set.h"

#if defined(__cplusplus)
extern "C" {
#endif

struct isl_basic_set_constraint {
	struct isl_basic_set	*bset;
	isl_int			**line;
};

struct isl_basic_set *isl_basic_set_constraint_set(
	struct isl_basic_set_constraint constraint);

struct isl_basic_set_constraint isl_basic_set_constraint_invalid();
struct isl_basic_set_constraint isl_basic_set_first_constraint(
	struct isl_basic_set *bset);
struct isl_basic_set_constraint isl_basic_set_constraint_next(
	struct isl_basic_set_constraint constraint);
int isl_basic_set_constraint_is_valid(
	struct isl_basic_set_constraint constraint);
int isl_basic_set_constraint_is_equal(
	struct isl_basic_set_constraint constraint1,
	struct isl_basic_set_constraint constraint2);

int isl_basic_set_has_defining_equality(
	struct isl_basic_set *bset, int pos,
	struct isl_basic_set_constraint *constraint);
int isl_basic_set_has_defining_inequalities(
	struct isl_basic_set *bset, int pos,
	struct isl_basic_set_constraint *lower,
	struct isl_basic_set_constraint *upper);

int isl_basic_set_constraint_nparam(
	struct isl_basic_set_constraint constraint);
int isl_basic_set_constraint_dim(
	struct isl_basic_set_constraint constraint);
int isl_basic_set_constraint_n_div(
	struct isl_basic_set_constraint constraint);

void isl_basic_set_constraint_get_constant(
	struct isl_basic_set_constraint constraint, isl_int *v);
void isl_basic_set_constraint_get_dim(
	struct isl_basic_set_constraint constraint, int pos, isl_int *v);
void isl_basic_set_constraint_get_param(
	struct isl_basic_set_constraint constraint, int pos, isl_int *v);
void isl_basic_set_constraint_get_div(
	struct isl_basic_set_constraint constraint, int pos, isl_int *v);
void isl_basic_set_constraint_set_dim(
	struct isl_basic_set_constraint constraint, int pos, isl_int v);
void isl_basic_set_constraint_set_param(
	struct isl_basic_set_constraint constraint, int pos, isl_int v);

void isl_basic_set_constraint_clear(struct isl_basic_set_constraint constraint);

int isl_basic_set_constraint_is_equality(
	struct isl_basic_set_constraint constraint);
int isl_basic_set_constraint_is_dim_lower_bound(
	struct isl_basic_set_constraint constraint, int pos);
int isl_basic_set_constraint_is_dim_upper_bound(
	struct isl_basic_set_constraint constraint, int pos);

struct isl_basic_set *isl_basic_set_from_constraint(
	struct isl_basic_set_constraint constraint);

#if defined(__cplusplus)
}
#endif

#endif
