/*
 * Copyright 2011      INRIA Saclay
 * Copyright 2011      Universiteit Leiden
 *
 * Use of this software is governed by the GNU LGPLv2.1 license
 *
 * Written by Sven Verdoolaege, INRIA Saclay - Ile-de-France,
 * Parc Club Orsay Universite, ZAC des vignes, 4 rue Jacques Monod,
 * 91893 Orsay, France
 * and Leiden Institute of Advanced Computer Science,
 * Universiteit Leiden, Niels Bohrweg 1, 2333 CA Leiden, The Netherlands
 */

#include <isl_map_private.h>
#include <isl_aff_private.h>
#include <isl_dim_private.h>
#include <isl_local_space_private.h>
#include <isl_mat_private.h>
#include <isl/constraint.h>
#include <isl/seq.h>
#include <isl/set.h>

__isl_give isl_aff *isl_aff_alloc_vec(__isl_take isl_local_space *ls,
	__isl_take isl_vec *v)
{
	isl_aff *aff;

	if (!ls || !v)
		goto error;

	aff = isl_calloc_type(v->ctx, struct isl_aff);
	if (!aff)
		goto error;

	aff->ref = 1;
	aff->ls = ls;
	aff->v = v;

	return aff;
error:
	isl_local_space_free(ls);
	isl_vec_free(v);
	return NULL;
}

__isl_give isl_aff *isl_aff_alloc(__isl_take isl_local_space *ls)
{
	isl_ctx *ctx;
	isl_vec *v;
	unsigned total;

	if (!ls)
		return NULL;

	ctx = isl_local_space_get_ctx(ls);
	if (!isl_local_space_divs_known(ls))
		isl_die(ctx, isl_error_invalid, "local space has unknown divs",
			goto error);

	total = isl_local_space_dim(ls, isl_dim_all);
	v = isl_vec_alloc(ctx, 1 + 1 + total);
	return isl_aff_alloc_vec(ls, v);
error:
	isl_local_space_free(ls);
	return NULL;
}

__isl_give isl_aff *isl_aff_zero(__isl_take isl_local_space *ls)
{
	isl_aff *aff;

	aff = isl_aff_alloc(ls);
	if (!aff)
		return NULL;

	isl_int_set_si(aff->v->el[0], 1);
	isl_seq_clr(aff->v->el + 1, aff->v->size - 1);

	return aff;
}

__isl_give isl_aff *isl_aff_copy(__isl_keep isl_aff *aff)
{
	if (!aff)
		return NULL;

	aff->ref++;
	return aff;
}

__isl_give isl_aff *isl_aff_dup(__isl_keep isl_aff *aff)
{
	if (!aff)
		return NULL;

	return isl_aff_alloc_vec(isl_local_space_copy(aff->ls),
				 isl_vec_copy(aff->v));
}

__isl_give isl_aff *isl_aff_cow(__isl_take isl_aff *aff)
{
	if (!aff)
		return NULL;

	if (aff->ref == 1)
		return aff;
	aff->ref--;
	return isl_aff_dup(aff);
}

void *isl_aff_free(__isl_take isl_aff *aff)
{
	if (!aff)
		return NULL;

	if (--aff->ref > 0)
		return NULL;

	isl_local_space_free(aff->ls);
	isl_vec_free(aff->v);

	free(aff);

	return NULL;
}

isl_ctx *isl_aff_get_ctx(__isl_keep isl_aff *aff)
{
	return aff ? isl_local_space_get_ctx(aff->ls) : NULL;
}

int isl_aff_dim(__isl_keep isl_aff *aff, enum isl_dim_type type)
{
	return aff ? isl_local_space_dim(aff->ls, type) : 0;
}

__isl_give isl_dim *isl_aff_get_dim(__isl_keep isl_aff *aff)
{
	return aff ? isl_local_space_get_dim(aff->ls) : NULL;
}

__isl_give isl_local_space *isl_aff_get_local_space(__isl_keep isl_aff *aff)
{
	return aff ? isl_local_space_copy(aff->ls) : NULL;
}

const char *isl_aff_get_dim_name(__isl_keep isl_aff *aff,
	enum isl_dim_type type, unsigned pos)
{
	return aff ? isl_local_space_get_dim_name(aff->ls, type, pos) : 0;
}

__isl_give isl_aff *isl_aff_reset_dim(__isl_take isl_aff *aff,
	__isl_take isl_dim *dim)
{
	aff = isl_aff_cow(aff);
	if (!aff || !dim)
		goto error;

	aff->ls = isl_local_space_reset_dim(aff->ls, dim);
	if (!aff->ls)
		return isl_aff_free(aff);

	return aff;
error:
	isl_aff_free(aff);
	isl_dim_free(dim);
	return NULL;
}

/* Reorder the coefficients of the affine expression based
 * on the given reodering.
 * The reordering r is assumed to have been extended with the local
 * variables.
 */
static __isl_give isl_vec *vec_reorder(__isl_take isl_vec *vec,
	__isl_take isl_reordering *r, int n_div)
{
	isl_vec *res;
	int i;

	if (!vec || !r)
		goto error;

	res = isl_vec_alloc(vec->ctx, 2 + isl_dim_total(r->dim) + n_div);
	isl_seq_cpy(res->el, vec->el, 2);
	isl_seq_clr(res->el + 2, res->size - 2);
	for (i = 0; i < r->len; ++i)
		isl_int_set(res->el[2 + r->pos[i]], vec->el[2 + i]);

	isl_reordering_free(r);
	isl_vec_free(vec);
	return res;
error:
	isl_vec_free(vec);
	isl_reordering_free(r);
	return NULL;
}

/* Reorder the dimensions of "aff" according to the given reordering.
 */
__isl_give isl_aff *isl_aff_realign(__isl_take isl_aff *aff,
	__isl_take isl_reordering *r)
{
	aff = isl_aff_cow(aff);
	if (!aff)
		goto error;

	r = isl_reordering_extend(r, aff->ls->div->n_row);
	aff->v = vec_reorder(aff->v, isl_reordering_copy(r),
				aff->ls->div->n_row);
	aff->ls = isl_local_space_realign(aff->ls, r);

	if (!aff->v || !aff->ls)
		return isl_aff_free(aff);

	return aff;
error:
	isl_aff_free(aff);
	isl_reordering_free(r);
	return NULL;
}

int isl_aff_plain_is_zero(__isl_keep isl_aff *aff)
{
	if (!aff)
		return -1;

	return isl_seq_first_non_zero(aff->v->el + 1, aff->v->size - 1) < 0;
}

int isl_aff_plain_is_equal(__isl_keep isl_aff *aff1, __isl_keep isl_aff *aff2)
{
	int equal;

	if (!aff1 || !aff2)
		return -1;

	equal = isl_local_space_is_equal(aff1->ls, aff2->ls);
	if (equal < 0 || !equal)
		return equal;

	return isl_vec_is_equal(aff1->v, aff2->v);
}

int isl_aff_get_denominator(__isl_keep isl_aff *aff, isl_int *v)
{
	if (!aff)
		return -1;
	isl_int_set(*v, aff->v->el[0]);
	return 0;
}

int isl_aff_get_constant(__isl_keep isl_aff *aff, isl_int *v)
{
	if (!aff)
		return -1;
	isl_int_set(*v, aff->v->el[1]);
	return 0;
}

int isl_aff_get_coefficient(__isl_keep isl_aff *aff,
	enum isl_dim_type type, int pos, isl_int *v)
{
	if (!aff)
		return -1;

	if (pos >= isl_local_space_dim(aff->ls, type))
		isl_die(aff->v->ctx, isl_error_invalid,
			"position out of bounds", return -1);

	pos += isl_local_space_offset(aff->ls, type);
	isl_int_set(*v, aff->v->el[1 + pos]);

	return 0;
}

__isl_give isl_aff *isl_aff_set_denominator(__isl_take isl_aff *aff, isl_int v)
{
	aff = isl_aff_cow(aff);
	if (!aff)
		return NULL;

	aff->v = isl_vec_cow(aff->v);
	if (!aff->v)
		return isl_aff_free(aff);

	isl_int_set(aff->v->el[0], v);

	return aff;
}

__isl_give isl_aff *isl_aff_set_constant(__isl_take isl_aff *aff, isl_int v)
{
	aff = isl_aff_cow(aff);
	if (!aff)
		return NULL;

	aff->v = isl_vec_cow(aff->v);
	if (!aff->v)
		return isl_aff_free(aff);

	isl_int_set(aff->v->el[1], v);

	return aff;
}

__isl_give isl_aff *isl_aff_add_constant(__isl_take isl_aff *aff, isl_int v)
{
	if (isl_int_is_zero(v))
		return aff;

	aff = isl_aff_cow(aff);
	if (!aff)
		return NULL;

	aff->v = isl_vec_cow(aff->v);
	if (!aff->v)
		return isl_aff_free(aff);

	isl_int_addmul(aff->v->el[1], aff->v->el[0], v);

	return aff;
}

__isl_give isl_aff *isl_aff_add_constant_si(__isl_take isl_aff *aff, int v)
{
	isl_int t;

	isl_int_init(t);
	isl_int_set_si(t, v);
	aff = isl_aff_add_constant(aff, t);
	isl_int_clear(t);

	return aff;
}

__isl_give isl_aff *isl_aff_set_constant_si(__isl_take isl_aff *aff, int v)
{
	aff = isl_aff_cow(aff);
	if (!aff)
		return NULL;

	aff->v = isl_vec_cow(aff->v);
	if (!aff->v)
		return isl_aff_free(aff);

	isl_int_set_si(aff->v->el[1], v);

	return aff;
}

__isl_give isl_aff *isl_aff_set_coefficient(__isl_take isl_aff *aff,
	enum isl_dim_type type, int pos, isl_int v)
{
	if (!aff)
		return NULL;

	if (pos >= isl_local_space_dim(aff->ls, type))
		isl_die(aff->v->ctx, isl_error_invalid,
			"position out of bounds", return isl_aff_free(aff));

	aff = isl_aff_cow(aff);
	if (!aff)
		return NULL;

	aff->v = isl_vec_cow(aff->v);
	if (!aff->v)
		return isl_aff_free(aff);

	pos += isl_local_space_offset(aff->ls, type);
	isl_int_set(aff->v->el[1 + pos], v);

	return aff;
}

__isl_give isl_aff *isl_aff_set_coefficient_si(__isl_take isl_aff *aff,
	enum isl_dim_type type, int pos, int v)
{
	if (!aff)
		return NULL;

	if (pos >= isl_local_space_dim(aff->ls, type))
		isl_die(aff->v->ctx, isl_error_invalid,
			"position out of bounds", return isl_aff_free(aff));

	aff = isl_aff_cow(aff);
	if (!aff)
		return NULL;

	aff->v = isl_vec_cow(aff->v);
	if (!aff->v)
		return isl_aff_free(aff);

	pos += isl_local_space_offset(aff->ls, type);
	isl_int_set_si(aff->v->el[1 + pos], v);

	return aff;
}

__isl_give isl_aff *isl_aff_add_coefficient(__isl_take isl_aff *aff,
	enum isl_dim_type type, int pos, isl_int v)
{
	if (!aff)
		return NULL;

	if (pos >= isl_local_space_dim(aff->ls, type))
		isl_die(aff->v->ctx, isl_error_invalid,
			"position out of bounds", return isl_aff_free(aff));

	aff = isl_aff_cow(aff);
	if (!aff)
		return NULL;

	aff->v = isl_vec_cow(aff->v);
	if (!aff->v)
		return isl_aff_free(aff);

	pos += isl_local_space_offset(aff->ls, type);
	isl_int_addmul(aff->v->el[1 + pos], aff->v->el[0], v);

	return aff;
}

__isl_give isl_aff *isl_aff_add_coefficient_si(__isl_take isl_aff *aff,
	enum isl_dim_type type, int pos, int v)
{
	isl_int t;

	isl_int_init(t);
	isl_int_set_si(t, v);
	aff = isl_aff_add_coefficient(aff, type, pos, t);
	isl_int_clear(t);

	return aff;
}

__isl_give isl_div *isl_aff_get_div(__isl_keep isl_aff *aff, int pos)
{
	if (!aff)
		return NULL;

	return isl_local_space_get_div(aff->ls, pos);
}

__isl_give isl_aff *isl_aff_neg(__isl_take isl_aff *aff)
{
	aff = isl_aff_cow(aff);
	if (!aff)
		return NULL;
	aff->v = isl_vec_cow(aff->v);
	if (!aff->v)
		return isl_aff_free(aff);

	isl_seq_neg(aff->v->el + 1, aff->v->el + 1, aff->v->size - 1);

	return aff;
}

/* Given f, return floor(f).
 * If f is an integer expression, then just return f.
 * Otherwise, create a new div d = [f] and return the expression d.
 */
__isl_give isl_aff *isl_aff_floor(__isl_take isl_aff *aff)
{
	int size;
	isl_ctx *ctx;

	if (!aff)
		return NULL;

	if (isl_int_is_one(aff->v->el[0]))
		return aff;

	aff = isl_aff_cow(aff);
	if (!aff)
		return NULL;

	aff->ls = isl_local_space_add_div(aff->ls, isl_vec_copy(aff->v));
	if (!aff->ls)
		return isl_aff_free(aff);

	ctx = isl_aff_get_ctx(aff);
	size = aff->v->size;
	isl_vec_free(aff->v);
	aff->v = isl_vec_alloc(ctx, size + 1);
	aff->v = isl_vec_clr(aff->v);
	if (!aff->v)
		return isl_aff_free(aff);
	isl_int_set_si(aff->v->el[0], 1);
	isl_int_set_si(aff->v->el[size], 1);

	return aff;
}

/* Given f, return ceil(f).
 * If f is an integer expression, then just return f.
 * Otherwise, create a new div d = [-f] and return the expression -d.
 */
__isl_give isl_aff *isl_aff_ceil(__isl_take isl_aff *aff)
{
	if (!aff)
		return NULL;

	if (isl_int_is_one(aff->v->el[0]))
		return aff;

	aff = isl_aff_neg(aff);
	aff = isl_aff_floor(aff);
	aff = isl_aff_neg(aff);

	return aff;
}

/* Apply the expansion computed by isl_merge_divs.
 * The expansion itself is given by "exp" while the resulting
 * list of divs is given by "div".
 */
__isl_give isl_aff *isl_aff_expand_divs( __isl_take isl_aff *aff,
	__isl_take isl_mat *div, int *exp)
{
	int i, j;
	int old_n_div;
	int new_n_div;
	int offset;

	aff = isl_aff_cow(aff);
	if (!aff || !div)
		goto error;

	old_n_div = isl_local_space_dim(aff->ls, isl_dim_div);
	new_n_div = isl_mat_rows(div);
	if (new_n_div < old_n_div)
		isl_die(isl_mat_get_ctx(div), isl_error_invalid,
			"not an expansion", goto error);

	aff->v = isl_vec_extend(aff->v, aff->v->size + new_n_div - old_n_div);
	if (!aff->v)
		goto error;

	offset = 1 + isl_local_space_offset(aff->ls, isl_dim_div);
	j = old_n_div - 1;
	for (i = new_n_div - 1; i >= 0; --i) {
		if (j >= 0 && exp[j] == i) {
			if (i != j)
				isl_int_swap(aff->v->el[offset + i],
					     aff->v->el[offset + j]);
			j--;
		} else
			isl_int_set_si(aff->v->el[offset + i], 0);
	}

	aff->ls = isl_local_space_replace_divs(aff->ls, isl_mat_copy(div));
	if (!aff->ls)
		goto error;
	isl_mat_free(div);
	return aff;
error:
	isl_aff_free(aff);
	isl_mat_free(div);
	return NULL;
}

/* Add two affine expressions that live in the same local space.
 */
static __isl_give isl_aff *add_expanded(__isl_take isl_aff *aff1,
	__isl_take isl_aff *aff2)
{
	isl_int gcd, f;

	aff1 = isl_aff_cow(aff1);
	if (!aff1 || !aff2)
		goto error;

	aff1->v = isl_vec_cow(aff1->v);
	if (!aff1->v)
		goto error;

	isl_int_init(gcd);
	isl_int_init(f);
	isl_int_gcd(gcd, aff1->v->el[0], aff2->v->el[0]);
	isl_int_divexact(f, aff2->v->el[0], gcd);
	isl_seq_scale(aff1->v->el + 1, aff1->v->el + 1, f, aff1->v->size - 1);
	isl_int_divexact(f, aff1->v->el[0], gcd);
	isl_seq_addmul(aff1->v->el + 1, f, aff2->v->el + 1, aff1->v->size - 1);
	isl_int_divexact(f, aff2->v->el[0], gcd);
	isl_int_mul(aff1->v->el[0], aff1->v->el[0], f);
	isl_int_clear(f);
	isl_int_clear(gcd);

	isl_aff_free(aff2);
	return aff1;
error:
	isl_aff_free(aff1);
	isl_aff_free(aff2);
	return NULL;
}

__isl_give isl_aff *isl_aff_add(__isl_take isl_aff *aff1,
	__isl_take isl_aff *aff2)
{
	isl_ctx *ctx;
	int *exp1 = NULL;
	int *exp2 = NULL;
	isl_mat *div;

	if (!aff1 || !aff2)
		goto error;

	ctx = isl_aff_get_ctx(aff1);
	if (!isl_dim_equal(aff1->ls->dim, aff2->ls->dim))
		isl_die(ctx, isl_error_invalid,
			"spaces don't match", goto error);

	if (aff1->ls->div->n_row == 0 && aff2->ls->div->n_row == 0)
		return add_expanded(aff1, aff2);

	exp1 = isl_alloc_array(ctx, int, aff1->ls->div->n_row);
	exp2 = isl_alloc_array(ctx, int, aff2->ls->div->n_row);
	if (!exp1 || !exp2)
		goto error;

	div = isl_merge_divs(aff1->ls->div, aff2->ls->div, exp1, exp2);
	aff1 = isl_aff_expand_divs(aff1, isl_mat_copy(div), exp1);
	aff2 = isl_aff_expand_divs(aff2, div, exp2);
	free(exp1);
	free(exp2);

	return add_expanded(aff1, aff2);
error:
	free(exp1);
	free(exp2);
	isl_aff_free(aff1);
	isl_aff_free(aff2);
	return NULL;
}

__isl_give isl_aff *isl_aff_sub(__isl_take isl_aff *aff1,
	__isl_take isl_aff *aff2)
{
	return isl_aff_add(aff1, isl_aff_neg(aff2));
}

__isl_give isl_aff *isl_aff_scale(__isl_take isl_aff *aff, isl_int f)
{
	isl_int gcd;

	if (isl_int_is_one(f))
		return aff;

	aff = isl_aff_cow(aff);
	if (!aff)
		return NULL;
	aff->v = isl_vec_cow(aff->v);
	if (!aff->v)
		return isl_aff_free(aff);

	isl_int_init(gcd);
	isl_int_gcd(gcd, aff->v->el[0], f);
	isl_int_divexact(aff->v->el[0], aff->v->el[0], gcd);
	isl_int_divexact(gcd, f, gcd);
	isl_seq_scale(aff->v->el + 1, aff->v->el + 1, gcd, aff->v->size - 1);
	isl_int_clear(gcd);

	return aff;
}

__isl_give isl_aff *isl_aff_scale_down(__isl_take isl_aff *aff, isl_int f)
{
	isl_int gcd;

	if (isl_int_is_one(f))
		return aff;

	aff = isl_aff_cow(aff);
	if (!aff)
		return NULL;
	aff->v = isl_vec_cow(aff->v);
	if (!aff->v)
		return isl_aff_free(aff);

	isl_int_init(gcd);
	isl_seq_gcd(aff->v->el + 1, aff->v->size - 1, &gcd);
	isl_int_gcd(gcd, gcd, f);
	isl_seq_scale_down(aff->v->el + 1, aff->v->el + 1, gcd, aff->v->size - 1);
	isl_int_divexact(gcd, f, gcd);
	isl_int_mul(aff->v->el[0], aff->v->el[0], gcd);
	isl_int_clear(gcd);

	return aff;
}

__isl_give isl_aff *isl_aff_scale_down_ui(__isl_take isl_aff *aff, unsigned f)
{
	isl_int v;

	if (f == 1)
		return aff;

	isl_int_init(v);
	isl_int_set_ui(v, f);
	aff = isl_aff_scale_down(aff, v);
	isl_int_clear(v);

	return aff;
}

__isl_give isl_aff *isl_aff_set_dim_name(__isl_take isl_aff *aff,
	enum isl_dim_type type, unsigned pos, const char *s)
{
	aff = isl_aff_cow(aff);
	if (!aff)
		return NULL;
	aff->ls = isl_local_space_set_dim_name(aff->ls, type, pos, s);
	if (!aff->ls)
		return isl_aff_free(aff);

	return aff;
}

/* Exploit the equalities in "eq" to simplify the affine expression
 * and the expressions of the integer divisions in the local space.
 * The integer divisions in this local space are assumed to appear
 * as regular dimensions in "eq".
 */
static __isl_give isl_aff *isl_aff_substitute_equalities(
	__isl_take isl_aff *aff, __isl_take isl_basic_set *eq)
{
	int i, j;
	unsigned total;
	unsigned n_div;

	if (!eq)
		goto error;
	if (eq->n_eq == 0) {
		isl_basic_set_free(eq);
		return aff;
	}

	aff = isl_aff_cow(aff);
	if (!aff)
		goto error;

	aff->ls = isl_local_space_substitute_equalities(aff->ls,
							isl_basic_set_copy(eq));
	if (!aff->ls)
		goto error;

	total = 1 + isl_dim_total(eq->dim);
	n_div = eq->n_div;
	for (i = 0; i < eq->n_eq; ++i) {
		j = isl_seq_last_non_zero(eq->eq[i], total + n_div);
		if (j < 0 || j == 0 || j >= total)
			continue;

		isl_seq_elim(aff->v->el + 1, eq->eq[i], j, total,
				&aff->v->el[0]);
	}

	isl_basic_set_free(eq);
	return aff;
error:
	isl_basic_set_free(eq);
	isl_aff_free(aff);
	return NULL;
}

/* Look for equalities among the variables shared by context and aff
 * and the integer divisions of aff, if any.
 * The equalities are then used to eliminate coefficients and/or integer
 * divisions from aff.
 */
__isl_give isl_aff *isl_aff_gist(__isl_take isl_aff *aff,
	__isl_take isl_set *context)
{
	isl_basic_set *hull;
	int n_div;

	if (!aff)
		goto error;
	n_div = isl_local_space_dim(aff->ls, isl_dim_div);
	if (n_div > 0) {
		isl_basic_set *bset;
		context = isl_set_add_dims(context, isl_dim_set, n_div);
		bset = isl_basic_set_from_local_space(
					    isl_aff_get_local_space(aff));
		bset = isl_basic_set_lift(bset);
		bset = isl_basic_set_flatten(bset);
		context = isl_set_intersect(context,
					    isl_set_from_basic_set(bset));
	}

	hull = isl_set_affine_hull(context);
	return isl_aff_substitute_equalities(aff, hull);
error:
	isl_aff_free(aff);
	isl_set_free(context);
	return NULL;
}

/* Return a basic set containing those elements in the space
 * of aff where it is non-negative.
 */
__isl_give isl_basic_set *isl_aff_nonneg_basic_set(__isl_take isl_aff *aff)
{
	isl_constraint *ineq;

	ineq = isl_inequality_from_aff(aff);

	return isl_basic_set_from_constraint(ineq);
}

/* Return a basic set containing those elements in the space
 * of aff where it is zero.
 */
__isl_give isl_basic_set *isl_aff_zero_basic_set(__isl_take isl_aff *aff)
{
	isl_constraint *ineq;

	ineq = isl_equality_from_aff(aff);

	return isl_basic_set_from_constraint(ineq);
}

/* Return a basic set containing those elements in the shared space
 * of aff1 and aff2 where aff1 is greater than or equal to aff2.
 */
__isl_give isl_basic_set *isl_aff_ge_basic_set(__isl_take isl_aff *aff1,
	__isl_take isl_aff *aff2)
{
	aff1 = isl_aff_sub(aff1, aff2);

	return isl_aff_nonneg_basic_set(aff1);
}

__isl_give isl_aff *isl_aff_add_on_domain(__isl_keep isl_set *dom,
	__isl_take isl_aff *aff1, __isl_take isl_aff *aff2)
{
	aff1 = isl_aff_add(aff1, aff2);
	aff1 = isl_aff_gist(aff1, isl_set_copy(dom));
	return aff1;
}

int isl_aff_is_empty(__isl_keep isl_aff *aff)
{
	if (!aff)
		return -1;

	return 0;
}

/* Set active[i] to 1 if the dimension at position i is involved
 * in the affine expression.
 */
static int set_active(__isl_keep isl_aff *aff, int *active)
{
	int i, j;
	unsigned total;
	unsigned offset;

	if (!aff || !active)
		return -1;

	total = aff->v->size - 2;
	for (i = 0; i < total; ++i)
		active[i] = !isl_int_is_zero(aff->v->el[2 + i]);

	offset = isl_local_space_offset(aff->ls, isl_dim_div) - 1;
	for (i = aff->ls->div->n_row - 1; i >= 0; --i) {
		if (!active[offset + i])
			continue;
		for (j = 0; j < total; ++j)
			active[j] |=
				!isl_int_is_zero(aff->ls->div->row[i][2 + j]);
	}

	return 0;
}

/* Check whether the given affine expression has non-zero coefficient
 * for any dimension in the given range or if any of these dimensions
 * appear with non-zero coefficients in any of the integer divisions
 * involved in the affine expression.
 */
int isl_aff_involves_dims(__isl_keep isl_aff *aff,
	enum isl_dim_type type, unsigned first, unsigned n)
{
	int i;
	isl_ctx *ctx;
	int *active = NULL;
	int involves = 0;

	if (!aff)
		return -1;
	if (n == 0)
		return 0;

	ctx = isl_aff_get_ctx(aff);
	if (first + n > isl_aff_dim(aff, type))
		isl_die(ctx, isl_error_invalid,
			"range out of bounds", return -1);

	active = isl_calloc_array(ctx, int,
				  isl_local_space_dim(aff->ls, isl_dim_all));
	if (set_active(aff, active) < 0)
		goto error;

	first += isl_local_space_offset(aff->ls, type) - 1;
	for (i = 0; i < n; ++i)
		if (active[first + i]) {
			involves = 1;
			break;
		}

	free(active);

	return involves;
error:
	free(active);
	return -1;
}

__isl_give isl_aff *isl_aff_drop_dims(__isl_take isl_aff *aff,
	enum isl_dim_type type, unsigned first, unsigned n)
{
	isl_ctx *ctx;

	if (!aff)
		return NULL;
	if (n == 0 && !isl_local_space_is_named_or_nested(aff->ls, type))
		return aff;

	ctx = isl_aff_get_ctx(aff);
	if (first + n > isl_aff_dim(aff, type))
		isl_die(ctx, isl_error_invalid, "range out of bounds",
			return isl_aff_free(aff));

	aff = isl_aff_cow(aff);
	if (!aff)
		return NULL;

	aff->ls = isl_local_space_drop_dims(aff->ls, type, first, n);
	if (!aff->ls)
		return isl_aff_free(aff);

	first += 1 + isl_local_space_offset(aff->ls, type);
	aff->v = isl_vec_drop_els(aff->v, first, n);
	if (!aff->v)
		return isl_aff_free(aff);

	return aff;
}

__isl_give isl_aff *isl_aff_insert_dims(__isl_take isl_aff *aff,
	enum isl_dim_type type, unsigned first, unsigned n)
{
	isl_ctx *ctx;

	if (!aff)
		return NULL;
	if (n == 0 && !isl_local_space_is_named_or_nested(aff->ls, type))
		return aff;

	ctx = isl_aff_get_ctx(aff);
	if (first > isl_aff_dim(aff, type))
		isl_die(ctx, isl_error_invalid, "position out of bounds",
			return isl_aff_free(aff));

	aff = isl_aff_cow(aff);
	if (!aff)
		return NULL;

	aff->ls = isl_local_space_insert_dims(aff->ls, type, first, n);
	if (!aff->ls)
		return isl_aff_free(aff);

	first += 1 + isl_local_space_offset(aff->ls, type);
	aff->v = isl_vec_insert_zero_els(aff->v, first, n);
	if (!aff->v)
		return isl_aff_free(aff);

	return aff;
}

__isl_give isl_aff *isl_aff_add_dims(__isl_take isl_aff *aff,
	enum isl_dim_type type, unsigned n)
{
	unsigned pos;

	pos = isl_aff_dim(aff, type);

	return isl_aff_insert_dims(aff, type, pos, n);
}

__isl_give isl_pw_aff *isl_pw_aff_add_dims(__isl_take isl_pw_aff *pwaff,
	enum isl_dim_type type, unsigned n)
{
	unsigned pos;

	pos = isl_pw_aff_dim(pwaff, type);

	return isl_pw_aff_insert_dims(pwaff, type, pos, n);
}

#undef PW
#define PW isl_pw_aff
#undef EL
#define EL isl_aff
#undef EL_IS_ZERO
#define EL_IS_ZERO is_empty
#undef ZERO
#define ZERO empty
#undef IS_ZERO
#define IS_ZERO is_empty
#undef FIELD
#define FIELD aff

#define NO_EVAL
#define NO_OPT
#define NO_MOVE_DIMS
#define NO_LIFT
#define NO_MORPH

#include <isl_pw_templ.c>

/* Compute a piecewise quasi-affine expression with a domain that
 * is the union of those of pwaff1 and pwaff2 and such that on each
 * cell, the quasi-affine expression is the maximum of those of pwaff1
 * and pwaff2.  If only one of pwaff1 or pwaff2 is defined on a given
 * cell, then the associated expression is the defined one.
 */
__isl_give isl_pw_aff *isl_pw_aff_max(__isl_take isl_pw_aff *pwaff1,
	__isl_take isl_pw_aff *pwaff2)
{
	int i, j, n;
	isl_pw_aff *res;
	isl_ctx *ctx;
	isl_set *set;

	if (!pwaff1 || !pwaff2)
		goto error;

	ctx = isl_dim_get_ctx(pwaff1->dim);
	if (!isl_dim_equal(pwaff1->dim, pwaff2->dim))
		isl_die(ctx, isl_error_invalid,
			"arguments should live in same space", goto error);

	if (isl_pw_aff_is_empty(pwaff1)) {
		isl_pw_aff_free(pwaff1);
		return pwaff2;
	}

	if (isl_pw_aff_is_empty(pwaff2)) {
		isl_pw_aff_free(pwaff2);
		return pwaff1;
	}

	n = 2 * (pwaff1->n + 1) * (pwaff2->n + 1);
	res = isl_pw_aff_alloc_(isl_dim_copy(pwaff1->dim), n);

	for (i = 0; i < pwaff1->n; ++i) {
		set = isl_set_copy(pwaff1->p[i].set);
		for (j = 0; j < pwaff2->n; ++j) {
			struct isl_set *common;
			isl_set *ge;

			common = isl_set_intersect(
					isl_set_copy(pwaff1->p[i].set),
					isl_set_copy(pwaff2->p[j].set));
			ge = isl_set_from_basic_set(isl_aff_ge_basic_set(
					isl_aff_copy(pwaff2->p[j].aff),
					isl_aff_copy(pwaff1->p[i].aff)));
			ge = isl_set_intersect(common, ge);
			if (isl_set_plain_is_empty(ge)) {
				isl_set_free(ge);
				continue;
			}
			set = isl_set_subtract(set, isl_set_copy(ge));

			res = isl_pw_aff_add_piece(res, ge,
						isl_aff_copy(pwaff2->p[j].aff));
		}
		res = isl_pw_aff_add_piece(res, set,
						isl_aff_copy(pwaff1->p[i].aff));
	}

	for (j = 0; j < pwaff2->n; ++j) {
		set = isl_set_copy(pwaff2->p[j].set);
		for (i = 0; i < pwaff1->n; ++i)
			set = isl_set_subtract(set,
					isl_set_copy(pwaff1->p[i].set));
		res = isl_pw_aff_add_piece(res, set,
						isl_aff_copy(pwaff2->p[j].aff));
	}

	isl_pw_aff_free(pwaff1);
	isl_pw_aff_free(pwaff2);

	return res;
error:
	isl_pw_aff_free(pwaff1);
	isl_pw_aff_free(pwaff2);
	return NULL;
}

/* Construct a map with as domain the domain of pwaff and
 * one-dimensional range corresponding to the affine expressions.
 */
__isl_give isl_map *isl_map_from_pw_aff(__isl_take isl_pw_aff *pwaff)
{
	int i;
	isl_dim *dim;
	isl_map *map;

	if (!pwaff)
		return NULL;

	dim = isl_pw_aff_get_dim(pwaff);
	dim = isl_dim_from_domain(dim);
	dim = isl_dim_add(dim, isl_dim_out, 1);
	map = isl_map_empty(dim);

	for (i = 0; i < pwaff->n; ++i) {
		isl_basic_map *bmap;
		isl_map *map_i;

		bmap = isl_basic_map_from_aff(isl_aff_copy(pwaff->p[i].aff));
		map_i = isl_map_from_basic_map(bmap);
		map_i = isl_map_intersect_domain(map_i,
						isl_set_copy(pwaff->p[i].set));
		map = isl_map_union_disjoint(map, map_i);
	}

	isl_pw_aff_free(pwaff);

	return map;
}

/* Return a set containing those elements in the domain
 * of pwaff where it is non-negative.
 */
__isl_give isl_set *isl_pw_aff_nonneg_set(__isl_take isl_pw_aff *pwaff)
{
	int i;
	isl_set *set;

	if (!pwaff)
		return NULL;

	set = isl_set_empty(isl_pw_aff_get_dim(pwaff));

	for (i = 0; i < pwaff->n; ++i) {
		isl_basic_set *bset;
		isl_set *set_i;

		bset = isl_aff_nonneg_basic_set(isl_aff_copy(pwaff->p[i].aff));
		set_i = isl_set_from_basic_set(bset);
		set_i = isl_set_intersect(set_i, isl_set_copy(pwaff->p[i].set));
		set = isl_set_union_disjoint(set, set_i);
	}

	isl_pw_aff_free(pwaff);

	return set;
}

/* Return a set containing those elements in the domain
 * of pwaff where it is zero.
 */
__isl_give isl_set *isl_pw_aff_zero_set(__isl_take isl_pw_aff *pwaff)
{
	int i;
	isl_set *set;

	if (!pwaff)
		return NULL;

	set = isl_set_empty(isl_pw_aff_get_dim(pwaff));

	for (i = 0; i < pwaff->n; ++i) {
		isl_basic_set *bset;
		isl_set *set_i;

		bset = isl_aff_zero_basic_set(isl_aff_copy(pwaff->p[i].aff));
		set_i = isl_set_from_basic_set(bset);
		set_i = isl_set_intersect(set_i, isl_set_copy(pwaff->p[i].set));
		set = isl_set_union_disjoint(set, set_i);
	}

	isl_pw_aff_free(pwaff);

	return set;
}

/* Return a set containing those elements in the shared domain
 * of pwaff1 and pwaff2 where pwaff1 is greater than (or equal) to pwaff2.
 *
 * We compute the difference on the shared domain and then construct
 * the set of values where this difference is non-negative.
 * If strict is set, we first subtract 1 from the difference.
 * If equal is set, we only return the elements where pwaff1 and pwaff2
 * are equal.
 */
static __isl_give isl_set *pw_aff_gte_set(__isl_take isl_pw_aff *pwaff1,
	__isl_take isl_pw_aff *pwaff2, int strict, int equal)
{
	isl_set *set1, *set2;

	set1 = isl_pw_aff_domain(isl_pw_aff_copy(pwaff1));
	set2 = isl_pw_aff_domain(isl_pw_aff_copy(pwaff2));
	set1 = isl_set_intersect(set1, set2);
	pwaff1 = isl_pw_aff_intersect_domain(pwaff1, isl_set_copy(set1));
	pwaff2 = isl_pw_aff_intersect_domain(pwaff2, isl_set_copy(set1));
	pwaff1 = isl_pw_aff_add(pwaff1, isl_pw_aff_neg(pwaff2));

	if (strict) {
		isl_dim *dim = isl_set_get_dim(set1);
		isl_aff *aff;
		aff = isl_aff_zero(isl_local_space_from_dim(dim));
		aff = isl_aff_add_constant_si(aff, -1);
		pwaff1 = isl_pw_aff_add(pwaff1, isl_pw_aff_alloc(set1, aff));
	} else
		isl_set_free(set1);

	if (equal)
		return isl_pw_aff_zero_set(pwaff1);
	return isl_pw_aff_nonneg_set(pwaff1);
}

/* Return a set containing those elements in the shared domain
 * of pwaff1 and pwaff2 where pwaff1 is equal to pwaff2.
 */
__isl_give isl_set *isl_pw_aff_eq_set(__isl_take isl_pw_aff *pwaff1,
	__isl_take isl_pw_aff *pwaff2)
{
	return pw_aff_gte_set(pwaff1, pwaff2, 0, 1);
}

/* Return a set containing those elements in the shared domain
 * of pwaff1 and pwaff2 where pwaff1 is greater than or equal to pwaff2.
 */
__isl_give isl_set *isl_pw_aff_ge_set(__isl_take isl_pw_aff *pwaff1,
	__isl_take isl_pw_aff *pwaff2)
{
	return pw_aff_gte_set(pwaff1, pwaff2, 0, 0);
}

/* Return a set containing those elements in the shared domain
 * of pwaff1 and pwaff2 where pwaff1 is strictly greater than pwaff2.
 */
__isl_give isl_set *isl_pw_aff_gt_set(__isl_take isl_pw_aff *pwaff1,
	__isl_take isl_pw_aff *pwaff2)
{
	return pw_aff_gte_set(pwaff1, pwaff2, 1, 0);
}

__isl_give isl_set *isl_pw_aff_le_set(__isl_take isl_pw_aff *pwaff1,
	__isl_take isl_pw_aff *pwaff2)
{
	return isl_pw_aff_ge_set(pwaff2, pwaff1);
}

__isl_give isl_set *isl_pw_aff_lt_set(__isl_take isl_pw_aff *pwaff1,
	__isl_take isl_pw_aff *pwaff2)
{
	return isl_pw_aff_gt_set(pwaff2, pwaff1);
}

__isl_give isl_pw_aff *isl_pw_aff_scale_down(__isl_take isl_pw_aff *pwaff,
	isl_int v)
{
	int i;

	if (isl_int_is_one(v))
		return pwaff;
	if (!isl_int_is_pos(v))
		isl_die(isl_pw_aff_get_ctx(pwaff), isl_error_invalid,
			"factor needs to be positive",
			return isl_pw_aff_free(pwaff));
	pwaff = isl_pw_aff_cow(pwaff);
	if (!pwaff)
		return NULL;
	if (pwaff->n == 0)
		return pwaff;

	for (i = 0; i < pwaff->n; ++i) {
		pwaff->p[i].aff = isl_aff_scale_down(pwaff->p[i].aff, v);
		if (!pwaff->p[i].aff)
			return isl_pw_aff_free(pwaff);
	}

	return pwaff;
}

__isl_give isl_pw_aff *isl_pw_aff_floor(__isl_take isl_pw_aff *pwaff)
{
	int i;

	pwaff = isl_pw_aff_cow(pwaff);
	if (!pwaff)
		return NULL;
	if (pwaff->n == 0)
		return pwaff;

	for (i = 0; i < pwaff->n; ++i) {
		pwaff->p[i].aff = isl_aff_floor(pwaff->p[i].aff);
		if (!pwaff->p[i].aff)
			return isl_pw_aff_free(pwaff);
	}

	return pwaff;
}

__isl_give isl_pw_aff *isl_pw_aff_ceil(__isl_take isl_pw_aff *pwaff)
{
	int i;

	pwaff = isl_pw_aff_cow(pwaff);
	if (!pwaff)
		return NULL;
	if (pwaff->n == 0)
		return pwaff;

	for (i = 0; i < pwaff->n; ++i) {
		pwaff->p[i].aff = isl_aff_ceil(pwaff->p[i].aff);
		if (!pwaff->p[i].aff)
			return isl_pw_aff_free(pwaff);
	}

	return pwaff;
}

/* Return an affine expression that is equal to pwaff_true for elements
 * in "cond" and to pwaff_false for elements not in "cond".
 * That is, return cond ? pwaff_true : pwaff_false;
 */
__isl_give isl_pw_aff *isl_pw_aff_cond(__isl_take isl_set *cond,
	__isl_take isl_pw_aff *pwaff_true, __isl_take isl_pw_aff *pwaff_false)
{
	isl_set *comp;

	comp = isl_set_complement(isl_set_copy(cond));
	pwaff_true = isl_pw_aff_intersect_domain(pwaff_true, cond);
	pwaff_false = isl_pw_aff_intersect_domain(pwaff_false, comp);

	return isl_pw_aff_add_disjoint(pwaff_true, pwaff_false);
}
