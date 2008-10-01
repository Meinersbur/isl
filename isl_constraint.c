#include <isl_constraint.h>
#include "isl_map_private.h"

struct isl_basic_set *isl_basic_set_constraint_set(
	struct isl_basic_set_constraint constraint)
{
	return constraint.bset;
}

struct isl_basic_set_constraint isl_basic_set_constraint_invalid()
{
	struct isl_basic_set_constraint c;
	c.bset = NULL;
	c.line = NULL;
	return c;
}

struct isl_basic_set_constraint isl_basic_set_first_constraint(
	struct isl_basic_set *bset)
{
	struct isl_basic_set_constraint c;

	if (!bset)
		return isl_basic_set_constraint_invalid();

	if (bset->n_eq > 0) {
		c.bset = bset;
		c.line = &bset->eq[0];
		return c;
	}

	if (bset->n_ineq > 0) {
		c.bset = bset;
		c.line = &bset->ineq[0];
		return c;
	}

	return isl_basic_set_constraint_invalid();
}

struct isl_basic_set_constraint isl_basic_set_constraint_next(
	struct isl_basic_set_constraint constraint)
{
	struct isl_basic_set_constraint c = constraint;

	c.line++;
	if (c.line >= c.bset->eq + c.bset->n_eq && c.line < c.bset->ineq)
		c.line = c.bset->ineq;
	if (c.line >= c.bset->ineq + c.bset->n_ineq)
		return isl_basic_set_constraint_invalid();
	return c;
}

int isl_basic_set_constraint_is_valid(
	struct isl_basic_set_constraint constraint)
{
	return constraint.bset != NULL && constraint.line != NULL;
}

int isl_basic_set_constraint_is_equal(
	struct isl_basic_set_constraint constraint1,
	struct isl_basic_set_constraint constraint2)
{
	return constraint1.bset == constraint2.bset &&
	       constraint1.line == constraint2.line;
}

int isl_basic_set_constraint_nparam(
	struct isl_basic_set_constraint constraint)
{
	if (!isl_basic_set_constraint_is_valid(constraint))
		return -1;
	return constraint.bset->nparam;
}

int isl_basic_set_constraint_dim(
	struct isl_basic_set_constraint constraint)
{
	if (!isl_basic_set_constraint_is_valid(constraint))
		return -1;
	return constraint.bset->dim;
}

int isl_basic_set_constraint_n_div(
	struct isl_basic_set_constraint constraint)
{
	if (!isl_basic_set_constraint_is_valid(constraint))
		return -1;
	return constraint.bset->n_div;
}

void isl_basic_set_constraint_get_constant(
	struct isl_basic_set_constraint constraint, isl_int *v)
{
	if (!isl_basic_set_constraint_is_valid(constraint))
		return;
	isl_int_set(*v, constraint.line[0][0]);
}

void isl_basic_set_constraint_get_dim(
	struct isl_basic_set_constraint constraint, int pos, isl_int *v)
{
	if (!isl_basic_set_constraint_is_valid(constraint))
		return;
	isl_assert(constraint.bset->ctx, pos < constraint.bset->dim, return);
	isl_int_set(*v, constraint.line[0][1 + constraint.bset->nparam + pos]);
}

void isl_basic_set_constraint_get_div(
	struct isl_basic_set_constraint constraint, int pos, isl_int *v)
{
	if (!isl_basic_set_constraint_is_valid(constraint))
		return;
	isl_assert(constraint.bset->ctx, pos < constraint.bset->n_div, return);
	isl_int_set(*v, constraint.line[0][1 + constraint.bset->nparam +
						constraint.bset->dim + pos]);
}

void isl_basic_set_constraint_get_param(
	struct isl_basic_set_constraint constraint, int pos, isl_int *v)
{
	if (!isl_basic_set_constraint_is_valid(constraint))
		return;
	isl_assert(constraint.bset->ctx, pos < constraint.bset->nparam, return);
	isl_int_set(*v, constraint.line[0][1 + pos]);
}

void isl_basic_set_constraint_set_dim(
	struct isl_basic_set_constraint constraint, int pos, isl_int v)
{
	if (!isl_basic_set_constraint_is_valid(constraint))
		return;
	isl_assert(constraint.bset->ctx, constraint.bset->ref == 1, return);
	isl_assert(constraint.bset->ctx, pos < constraint.bset->dim, return);
	isl_int_set(constraint.line[0][1 + constraint.bset->nparam + pos], v);
}

void isl_basic_set_constraint_set_param(
	struct isl_basic_set_constraint constraint, int pos, isl_int v)
{
	if (!isl_basic_set_constraint_is_valid(constraint))
		return;
	isl_assert(constraint.bset->ctx, constraint.bset->ref == 1, return);
	isl_assert(constraint.bset->ctx, pos < constraint.bset->nparam, return);
	isl_int_set(constraint.line[0][1 + pos], v);
}

void isl_basic_set_constraint_clear(struct isl_basic_set_constraint constraint)
{
	struct isl_basic_set *bset = constraint.bset;
	unsigned total;

	if (!isl_basic_set_constraint_is_valid(constraint))
		return;
	total = bset->nparam + bset->dim + bset->n_div;
	isl_seq_clr(constraint.line[0], 1 + total);
}

int isl_basic_set_constraint_is_equality(
	struct isl_basic_set_constraint constraint)
{
	if (!isl_basic_set_constraint_is_valid(constraint))
		return -1;
	return constraint.line < constraint.bset->eq + constraint.bset->n_eq;
}

int isl_basic_set_constraint_is_dim_lower_bound(
	struct isl_basic_set_constraint constraint, int pos)
{
	if (!isl_basic_set_constraint_is_valid(constraint))
		return -1;
	isl_assert(constraint.bset->ctx, pos < constraint.bset->dim, return -1);
	return isl_int_is_pos(constraint.line[0][1+constraint.bset->nparam+pos]);
}

int isl_basic_set_constraint_is_dim_upper_bound(
	struct isl_basic_set_constraint constraint, int pos)
{
	if (!isl_basic_set_constraint_is_valid(constraint))
		return -1;
	isl_assert(constraint.bset->ctx, pos < constraint.bset->dim, return -1);
	return isl_int_is_neg(constraint.line[0][1+constraint.bset->nparam+pos]);
}


struct isl_basic_set *isl_basic_set_from_constraint(
	struct isl_basic_set_constraint constraint)
{
	int k;
	struct isl_basic_set *bset;
	isl_int *c;
	unsigned total;

	if (!isl_basic_set_constraint_is_valid(constraint))
		return NULL;

	bset = isl_basic_set_universe(constraint.bset->ctx,
				constraint.bset->nparam, constraint.bset->dim);
	bset = isl_basic_set_align_divs(bset, constraint.bset);
	bset = isl_basic_set_extend(bset, bset->nparam, bset->dim, 0, 1, 1);
	if (isl_basic_set_constraint_is_equality(constraint)) {
		k = isl_basic_set_alloc_equality(bset);
		if (k < 0)
			goto error;
		c = bset->eq[k];
	}
	else {
		k = isl_basic_set_alloc_inequality(bset);
		if (k < 0)
			goto error;
		c = bset->ineq[k];
	}
	total = bset->nparam + bset->dim + bset->n_div;
	isl_seq_cpy(c, constraint.line[0], 1 + total);
	return bset;
error:
	isl_basic_set_free(bset);
	return NULL;
}

int isl_basic_set_has_defining_equality(
	struct isl_basic_set *bset, int pos,
	struct isl_basic_set_constraint *constraint)
{
	int i;

	if (!bset)
		return -1;
	isl_assert(bset->ctx, pos < bset->dim, return -1);
	for (i = 0; i < bset->n_eq; ++i)
		if (!isl_int_is_zero(bset->eq[i][1 + bset->nparam + pos]) &&
		    isl_seq_first_non_zero(bset->eq[i]+1+bset->nparam+pos+1,
					   bset->dim-pos-1) == -1) {
			constraint->bset = bset;
			constraint->line = &bset->eq[i];
			return 1;
		}
	return 0;
}

int isl_basic_set_has_defining_inequalities(
	struct isl_basic_set *bset, int pos,
	struct isl_basic_set_constraint *lower,
	struct isl_basic_set_constraint *upper)
{
	int i, j;
	unsigned total;
	isl_int m;

	if (!bset)
		return -1;
	isl_assert(bset->ctx, pos < bset->dim, return -1);
	total = bset->nparam + bset->dim + bset->n_div;
	isl_int_init(m);
	for (i = 0; i < bset->n_ineq; ++i) {
		if (isl_int_is_zero(bset->ineq[i][1 + bset->nparam + pos]))
			continue;
		if (isl_int_is_one(bset->ineq[i][1 + bset->nparam + pos]))
			continue;
		if (isl_int_is_negone(bset->ineq[i][1 + bset->nparam + pos]))
			continue;
		if (isl_seq_first_non_zero(bset->ineq[i]+1+bset->nparam+pos+1,
						bset->dim-pos-1) != -1)
			continue;
		for (j = i + i; j < bset->n_ineq; ++j) {
			if (!isl_seq_is_neg(bset->ineq[i]+1, bset->ineq[j]+1,
					    total))
				continue;
			isl_int_add(m, bset->ineq[i][0], bset->ineq[j][0]);
			if (isl_int_abs_ge(m, bset->ineq[i][1+bset->nparam+pos]))
				continue;

			lower->bset = bset;
			upper->bset = bset;
			if (isl_int_is_pos(bset->ineq[i][1+bset->nparam+pos])) {
				lower->line = &bset->ineq[i];
				upper->line = &bset->ineq[j];
			} else {
				lower->line = &bset->ineq[j];
				upper->line = &bset->ineq[i];
			}
			isl_int_clear(m);
			return 1;
		}
	}
	isl_int_clear(m);
	return 0;
}
