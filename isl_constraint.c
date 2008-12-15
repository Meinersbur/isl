#include <isl_constraint.h>
#include "isl_map_private.h"

static unsigned n(struct isl_constraint *c, enum isl_dim_type type)
{
	struct isl_dim *dim = c->bmap->dim;
	switch (type) {
	case isl_dim_param:	return dim->nparam;
	case isl_dim_in:	return dim->n_in;
	case isl_dim_out:	return dim->n_out;
	case isl_dim_div:	return c->bmap->n_div;
	case isl_dim_all:	return isl_basic_map_total_dim(c->bmap);
	}
}

static unsigned offset(struct isl_constraint *c, enum isl_dim_type type)
{
	struct isl_dim *dim = c->bmap->dim;
	switch (type) {
	case isl_dim_param:	return 1;
	case isl_dim_in:	return 1 + dim->nparam;
	case isl_dim_out:	return 1 + dim->nparam + dim->n_in;
	case isl_dim_div:	return 1 + dim->nparam + dim->n_in + dim->n_out;
	}
}

struct isl_constraint *isl_basic_map_constraint(struct isl_basic_map *bmap,
	isl_int **line)
{
	struct isl_constraint *constraint;

	if (!bmap || !line)
		goto error;
	
	constraint = isl_alloc_type(bmap->ctx, struct isl_constraint);
	if (!constraint)
		goto error;

	constraint->ctx = bmap->ctx;
	isl_ctx_ref(constraint->ctx);
	constraint->ref = 1;
	constraint->bmap = bmap;
	constraint->line = line;

	return constraint;
error:
	isl_basic_map_free(bmap);
	return NULL;
}

struct isl_constraint *isl_basic_set_constraint(struct isl_basic_set *bset,
	isl_int **line)
{
	return isl_basic_map_constraint((struct isl_basic_map *)bset, line);
}

struct isl_constraint *isl_equality_alloc(struct isl_dim *dim)
{
	struct isl_basic_map *bmap;

	if (!dim)
		return NULL;

	bmap = isl_basic_map_alloc_dim(dim->ctx, dim, 0, 1, 0);
	if (!bmap)
		return NULL;

	isl_basic_map_alloc_equality(bmap);
	isl_seq_clr(bmap->eq[0], 1 + isl_basic_map_total_dim(bmap));
	return isl_basic_map_constraint(bmap, &bmap->eq[0]);
}

struct isl_constraint *isl_inequality_alloc(struct isl_dim *dim)
{
	struct isl_basic_map *bmap;

	if (!dim)
		return NULL;

	bmap = isl_basic_map_alloc_dim(dim->ctx, dim, 0, 0, 1);
	if (!bmap)
		return NULL;

	isl_basic_map_alloc_inequality(bmap);
	isl_seq_clr(bmap->ineq[0], 1 + isl_basic_map_total_dim(bmap));
	return isl_basic_map_constraint(bmap, &bmap->ineq[0]);
}

struct isl_constraint *isl_constraint_dup(struct isl_constraint *c)
{
	if (!c)
		return NULL;

	return isl_basic_map_constraint(isl_basic_map_copy(c->bmap), c->line);
}

struct isl_constraint *isl_constraint_cow(struct isl_constraint *c)
{
	if (!c)
		return NULL;

	if (c->ref == 1)
		return c;
	c->ref--;
	return isl_constraint_dup(c);
}

struct isl_constraint *isl_constraint_copy(struct isl_constraint *constraint)
{
	if (!constraint)
		return NULL;

	constraint->ref++;
	return constraint;
}

struct isl_constraint *isl_constraint_free(struct isl_constraint *c)
{
	if (!c)
		return;

	if (--c->ref > 0)
		return;

	isl_basic_map_free(c->bmap);
	isl_ctx_deref(c->ctx);
	free(c);
}

struct isl_constraint *isl_basic_set_first_constraint(
	struct isl_basic_set *bset)
{
	struct isl_constraint *c;

	if (!bset)
		return NULL;

	if (bset->n_eq > 0)
		return isl_basic_set_constraint(bset, &bset->eq[0]);

	if (bset->n_ineq > 0)
		return isl_basic_set_constraint(bset, &bset->ineq[0]);

	isl_basic_set_free(bset);
	return NULL;
}

struct isl_constraint *isl_constraint_next(struct isl_constraint *c)
{
	c = isl_constraint_cow(c);
	c->line++;
	if (c->line >= c->bmap->eq + c->bmap->n_eq && c->line < c->bmap->ineq)
		c->line = c->bmap->ineq;
	if (c->line < c->bmap->ineq + c->bmap->n_ineq)
		return c;
	isl_constraint_free(c);
	return NULL;
}

int isl_constraint_is_equal(struct isl_constraint *constraint1,
	struct isl_constraint *constraint2)
{
	if (!constraint1 || !constraint2)
		return 0;
	return constraint1->bmap == constraint2->bmap &&
	       constraint1->line == constraint2->line;
}

struct isl_basic_set *isl_basic_set_add_constraint(
	struct isl_basic_set *bset, struct isl_constraint *constraint)
{
	if (!bset || !constraint)
		goto error;

	isl_assert(constraint->ctx,
		isl_dim_equal(bset->dim, constraint->bmap->dim), goto error);

	bset = isl_basic_set_intersect(bset,
		isl_basic_set_copy((struct isl_basic_set *)constraint->bmap));
	isl_constraint_free(constraint);
	return bset;
error:
	isl_basic_set_free(bset);
	isl_constraint_free(constraint);
	return NULL;
}

int isl_constraint_dim(struct isl_constraint *constraint,
	enum isl_dim_type type)
{
	if (!constraint)
		return -1;
	return n(constraint, type);
}

void isl_constraint_get_constant(struct isl_constraint *constraint, isl_int *v)
{
	if (!constraint)
		return;
	isl_int_set(*v, constraint->line[0][0]);
}

void isl_constraint_get_coefficient(struct isl_constraint *constraint,
	enum isl_dim_type type, int pos, isl_int *v)
{
	if (!constraint)
		return;

	isl_assert(constraint->ctx, pos < n(constraint, type), return);
	isl_int_set(*v, constraint->line[0][offset(constraint, type) + pos]);
}

void isl_constraint_set_constant(struct isl_constraint *constraint, isl_int v)
{
	if (!constraint)
		return;
	isl_int_set(constraint->line[0][0], v);
}

void isl_constraint_set_coefficient(struct isl_constraint *constraint,
	enum isl_dim_type type, int pos, isl_int v)
{
	if (!constraint)
		return;

	isl_assert(constraint->ctx, pos < n(constraint, type), return);
	isl_int_set(constraint->line[0][offset(constraint, type) + pos], v);
}

void isl_constraint_clear(struct isl_constraint *constraint)
{
	struct isl_basic_set *bset;
	unsigned total;

	if (!constraint)
		return;
	total = isl_basic_map_total_dim(constraint->bmap);
	isl_seq_clr(constraint->line[0], 1 + total);
}

struct isl_constraint *isl_constraint_negate(struct isl_constraint *constraint)
{
	unsigned total;

	if (!constraint)
		return NULL;

	isl_assert(constraint->ctx, !isl_constraint_is_equality(constraint),
			goto error);
	isl_assert(constraint->ctx, constraint->bmap->ref == 1, goto error);
	total = isl_basic_map_total_dim(constraint->bmap);
	isl_seq_neg(constraint->line[0], constraint->line[0], 1 + total);
	isl_int_sub_ui(constraint->line[0][0], constraint->line[0][0], 1);
	F_CLR(constraint->bmap, ISL_BASIC_MAP_NORMALIZED);
	return constraint;
error:
	isl_constraint_free(constraint);
	return NULL;
}

int isl_constraint_is_equality(struct isl_constraint *constraint)
{
	if (!constraint)
		return -1;
	return constraint->line < constraint->bmap->eq + constraint->bmap->n_eq;
}


struct isl_basic_set *isl_basic_set_from_constraint(
	struct isl_constraint *constraint)
{
	int k;
	struct isl_basic_set *constraint_bset, *bset;
	isl_int *c;
	unsigned dim;
	unsigned nparam;
	unsigned total;

	if (!constraint)
		return NULL;

	isl_assert(constraint->ctx,n(constraint, isl_dim_in) == 0, goto error);

	constraint_bset = (struct isl_basic_set *)constraint->bmap;
	bset = isl_basic_set_universe_like(constraint_bset);
	bset = isl_basic_set_align_divs(bset, constraint_bset);
	nparam = isl_basic_set_n_param(bset);
	dim = isl_basic_set_n_dim(bset);
	bset = isl_basic_set_extend(bset, nparam, dim, 0, 1, 1);
	if (isl_constraint_is_equality(constraint)) {
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
	total = isl_basic_set_total_dim(bset);
	isl_seq_cpy(c, constraint->line[0], 1 + total);
	isl_constraint_free(constraint);
	return bset;
error:
	isl_constraint_free(constraint);
	isl_basic_set_free(bset);
	return NULL;
}

int isl_basic_set_has_defining_equality(
	struct isl_basic_set *bset, int pos,
	struct isl_constraint **c)
{
	int i;
	unsigned dim, nparam;

	if (!bset)
		return -1;
	nparam = isl_basic_set_n_param(bset);
	dim = isl_basic_set_n_dim(bset);
	isl_assert(bset->ctx, pos < dim, return -1);
	for (i = 0; i < bset->n_eq; ++i)
		if (!isl_int_is_zero(bset->eq[i][1 + nparam + pos]) &&
		    isl_seq_first_non_zero(bset->eq[i]+1+nparam+pos+1,
					   dim-pos-1) == -1) {
			*c= isl_basic_set_constraint(isl_basic_set_copy(bset),
								&bset->eq[i]);
			return 1;
		}
	return 0;
}

int isl_basic_set_has_defining_inequalities(
	struct isl_basic_set *bset, int pos,
	struct isl_constraint **lower,
	struct isl_constraint **upper)
{
	int i, j;
	unsigned dim;
	unsigned nparam;
	unsigned total;
	isl_int m;
	isl_int **lower_line, **upper_line;

	if (!bset)
		return -1;
	nparam = isl_basic_set_n_param(bset);
	dim = isl_basic_set_n_dim(bset);
	total = isl_basic_set_total_dim(bset);
	isl_assert(bset->ctx, pos < dim, return -1);
	isl_int_init(m);
	for (i = 0; i < bset->n_ineq; ++i) {
		if (isl_int_is_zero(bset->ineq[i][1 + nparam + pos]))
			continue;
		if (isl_int_is_one(bset->ineq[i][1 + nparam + pos]))
			continue;
		if (isl_int_is_negone(bset->ineq[i][1 + nparam + pos]))
			continue;
		if (isl_seq_first_non_zero(bset->ineq[i]+1+nparam+pos+1,
						dim-pos-1) != -1)
			continue;
		for (j = i + i; j < bset->n_ineq; ++j) {
			if (!isl_seq_is_neg(bset->ineq[i]+1, bset->ineq[j]+1,
					    total))
				continue;
			isl_int_add(m, bset->ineq[i][0], bset->ineq[j][0]);
			if (isl_int_abs_ge(m, bset->ineq[i][1+nparam+pos]))
				continue;

			if (isl_int_is_pos(bset->ineq[i][1+nparam+pos])) {
				lower_line = &bset->ineq[i];
				upper_line = &bset->ineq[j];
			} else {
				lower_line = &bset->ineq[j];
				upper_line = &bset->ineq[i];
			}
			*lower = isl_basic_set_constraint(
					isl_basic_set_copy(bset), lower_line);
			*upper = isl_basic_set_constraint(
					isl_basic_set_copy(bset), upper_line);
			isl_int_clear(m);
			return 1;
		}
	}
	*lower = NULL;
	*upper = NULL;
	isl_int_clear(m);
	return 0;
}
