/*
 * Copyright 2008-2009 Katholieke Universiteit Leuven
 * Copyright 2010      INRIA Saclay
 *
 * Use of this software is governed by the GNU LGPLv2.1 license
 *
 * Written by Sven Verdoolaege, K.U.Leuven, Departement
 * Computerwetenschappen, Celestijnenlaan 200A, B-3001 Leuven, Belgium
 * and INRIA Saclay - Ile-de-France, Parc Club Orsay Universite,
 * ZAC des vignes, 4 rue Jacques Monod, 91893 Orsay, France 
 */

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <isl_set.h>
#include <isl_seq.h>
#include <isl_div.h>
#include "isl_stream.h"
#include "isl_map_private.h"
#include "isl_obj.h"
#include "isl_polynomial_private.h"

struct variable {
	char    	    	*name;
	int	     		 pos;
	struct variable		*next;
};

struct vars {
	struct isl_ctx	*ctx;
	int		 n;
	struct variable	*v;
};

static struct vars *vars_new(struct isl_ctx *ctx)
{
	struct vars *v;
	v = isl_alloc_type(ctx, struct vars);
	if (!v)
		return NULL;
	v->ctx = ctx;
	v->n = 0;
	v->v = NULL;
	return v;
}

static void variable_free(struct variable *var)
{
	while (var) {
		struct variable *next = var->next;
		free(var->name);
		free(var);
		var = next;
	}
}

static void vars_free(struct vars *v)
{
	if (!v)
		return;
	variable_free(v->v);
	free(v);
}

static void vars_drop(struct vars *v, int n)
{
	struct variable *var;

	if (!v)
		return;

	v->n -= n;

	var = v->v;
	while (--n >= 0) {
		struct variable *next = var->next;
		free(var->name);
		free(var);
		var = next;
	}
	v->v = var;
}

static struct variable *variable_new(struct vars *v, const char *name, int len,
				int pos)
{
	struct variable *var;
	var = isl_alloc_type(v->ctx, struct variable);
	if (!var)
		goto error;
	var->name = strdup(name);
	var->name[len] = '\0';
	var->pos = pos;
	var->next = v->v;
	return var;
error:
	variable_free(v->v);
	return NULL;
}

static int vars_pos(struct vars *v, const char *s, int len)
{
	int pos;
	struct variable *q;

	if (len == -1)
		len = strlen(s);
	for (q = v->v; q; q = q->next) {
		if (strncmp(q->name, s, len) == 0 && q->name[len] == '\0')
			break;
	}
	if (q)
		pos = q->pos;
	else {
		pos = v->n;
		v->v = variable_new(v, s, len, v->n);
		if (!v->v)
			return -1;
		v->n++;
	}
	return pos;
}

static __isl_give isl_basic_map *set_name(__isl_take isl_basic_map *bmap,
	enum isl_dim_type type, unsigned pos, char *name)
{
	char *prime;

	if (!bmap)
		return NULL;
	if (!name)
		return bmap;

	prime = strchr(name, '\'');
	if (prime)
		*prime = '\0';
	bmap->dim = isl_dim_set_name(bmap->dim, type, pos, name);
	if (prime)
		*prime = '\'';

	if (!bmap->dim)
		goto error;

	return bmap;
error:
	isl_basic_map_free(bmap);
	return NULL;
}

static struct isl_vec *accept_affine(struct isl_stream *s, struct vars *v)
{
	struct isl_token *tok = NULL;
	struct isl_vec *aff;
	int sign = 1;

	aff = isl_vec_alloc(v->ctx, 1 + v->n);
	isl_seq_clr(aff->el, aff->size);
	if (!aff)
		return NULL;

	for (;;) {
		tok = isl_stream_next_token(s);
		if (!tok) {
			isl_stream_error(s, NULL, "unexpected EOF");
			goto error;
		}
		if (tok->type == ISL_TOKEN_IDENT) {
			int n = v->n;
			int pos = vars_pos(v, tok->u.s, -1);
			if (pos < 0)
				goto error;
			if (pos >= n) {
				isl_stream_error(s, tok, "unknown identifier");
				isl_token_free(tok);
				goto error;
			}
			if (sign > 0)
				isl_int_add_ui(aff->el[1 + pos],
					       aff->el[1 + pos], 1);
			else
				isl_int_sub_ui(aff->el[1 + pos],
					       aff->el[1 + pos], 1);
			sign = 1;
		} else if (tok->type == ISL_TOKEN_VALUE) {
			struct isl_token *tok2;
			int n = v->n;
			int pos = -1;
			tok2 = isl_stream_next_token(s);
			if (tok2 && tok2->type == ISL_TOKEN_IDENT) {
				pos = vars_pos(v, tok2->u.s, -1);
				if (pos < 0)
					goto error;
				if (pos >= n) {
					isl_stream_error(s, tok2,
						"unknown identifier");
					isl_token_free(tok);
					isl_token_free(tok2);
					goto error;
				}
				isl_token_free(tok2);
			} else if (tok2)
				isl_stream_push_token(s, tok2);
			if (sign < 0)
				isl_int_neg(tok->u.v, tok->u.v);
			isl_int_add(aff->el[1 + pos],
					aff->el[1 + pos], tok->u.v);
			sign = 1;
		} else if (tok->type == '-') {
			sign = -sign;
		} else if (tok->type == '+') {
			/* nothing */
		} else {
			isl_stream_push_token(s, tok);
			break;
		}
		isl_token_free(tok);
	}

	return aff;
error:
	isl_vec_free(aff);
	return NULL;
}

static __isl_give isl_basic_map *read_var_list(struct isl_stream *s,
	__isl_take isl_basic_map *bmap, enum isl_dim_type type, struct vars *v)
{
	int i = 0;
	struct isl_token *tok;

	while ((tok = isl_stream_next_token(s)) != NULL) {
		int new_name = 0;

		if (tok->type == ISL_TOKEN_IDENT) {
			int n = v->n;
			int p = vars_pos(v, tok->u.s, -1);
			if (p < 0)
				goto error;
			new_name = p >= n;
		}

		if (new_name) {
			bmap = isl_basic_map_add(bmap, type, 1);
			bmap = set_name(bmap, type, i, v->v->name);
			isl_token_free(tok);
		} else if (tok->type == ISL_TOKEN_IDENT ||
			   tok->type == ISL_TOKEN_VALUE) {
			struct isl_vec *vec;
			int k;
			if (type == isl_dim_param) {
				isl_stream_error(s, tok,
						"expecting unique identifier");
				goto error;
			}
			isl_stream_push_token(s, tok);
			tok = NULL;
			vec = accept_affine(s, v);
			if (!vec)
				goto error;
			v->v = variable_new(v, "", 0, v->n);
			if (!v->v)
				goto error;
			v->n++;
			bmap = isl_basic_map_add(bmap, type, 1);
			bmap = isl_basic_map_extend_constraints(bmap, 1, 0);
			k = isl_basic_map_alloc_equality(bmap);
			if (k >= 0) {
				isl_seq_cpy(bmap->eq[k], vec->el, vec->size);
				isl_int_set_si(bmap->eq[k][vec->size], -1);
			}
			isl_vec_free(vec);
			if (k < 0)
				goto error;
		} else
			break;

		tok = isl_stream_next_token(s);
		if (!tok || tok->type != ',')
			break;

		isl_token_free(tok);
		i++;
	}
	if (tok)
		isl_stream_push_token(s, tok);

	return bmap;
error:
	isl_token_free(tok);
	isl_basic_map_free(bmap);
	return NULL;
}

static __isl_give isl_mat *accept_affine_list(struct isl_stream *s,
	struct vars *v)
{
	struct isl_vec *vec;
	struct isl_mat *mat;
	struct isl_token *tok = NULL;

	vec = accept_affine(s, v);
	mat = isl_mat_from_row_vec(vec);
	if (!mat)
		return NULL;

	for (;;) {
		tok = isl_stream_next_token(s);
		if (!tok) {
			isl_stream_error(s, NULL, "unexpected EOF");
			goto error;
		}
		if (tok->type != ',') {
			isl_stream_push_token(s, tok);
			break;
		}
		isl_token_free(tok);

		vec = accept_affine(s, v);
		mat = isl_mat_vec_concat(mat, vec);
		if (!mat)
			return NULL;
	}

	return mat;
error:
	isl_mat_free(mat);
	return NULL;
}

static struct isl_basic_map *add_div_definition(struct isl_stream *s,
	struct vars *v, struct isl_basic_map *bmap, int k)
{
	struct isl_token *tok;
	int seen_paren = 0;
	struct isl_vec *aff;

	if (isl_stream_eat(s, '['))
		goto error;

	tok = isl_stream_next_token(s);
	if (!tok)
		goto error;
	if (tok->type == '(') {
		seen_paren = 1;
		isl_token_free(tok);
	} else
		isl_stream_push_token(s, tok);

	aff = accept_affine(s, v);
	if (!aff)
		goto error;

	isl_seq_cpy(bmap->div[k] + 1, aff->el, aff->size);

	isl_vec_free(aff);

	if (seen_paren && isl_stream_eat(s, ')'))
		goto error;
	if (isl_stream_eat(s, '/'))
		goto error;

	tok = isl_stream_next_token(s);
	if (!tok)
		goto error;
	if (tok->type != ISL_TOKEN_VALUE) {
		isl_stream_error(s, tok, "expected denominator");
		isl_stream_push_token(s, tok);
		goto error;
	}
	isl_int_set(bmap->div[k][0], tok->u.v);
	isl_token_free(tok);

	if (isl_stream_eat(s, ']'))
		goto error;

	if (isl_basic_map_add_div_constraints(bmap, k) < 0)
		goto error;

	return bmap;
error:
	isl_basic_map_free(bmap);
	return NULL;
}

static struct isl_basic_map *read_defined_var_list(struct isl_stream *s,
	struct vars *v, struct isl_basic_map *bmap)
{
	struct isl_token *tok;

	while ((tok = isl_stream_next_token(s)) != NULL) {
		int k;
		int p;
		int n = v->n;
		unsigned total = isl_basic_map_total_dim(bmap);

		if (tok->type != ISL_TOKEN_IDENT)
			break;

		p = vars_pos(v, tok->u.s, -1);
		if (p < 0)
			goto error;
		if (p < n) {
			isl_stream_error(s, tok, "expecting unique identifier");
			goto error;
		}
		isl_token_free(tok);

		bmap = isl_basic_map_cow(bmap);
		bmap = isl_basic_map_extend_dim(bmap, isl_dim_copy(bmap->dim),
						1, 0, 2);

		if ((k = isl_basic_map_alloc_div(bmap)) < 0)
			goto error;
		isl_seq_clr(bmap->div[k], 1 + 1 + total);

		tok = isl_stream_next_token(s);
		if (tok && tok->type == '=') {
			isl_token_free(tok);
			bmap = add_div_definition(s, v, bmap, k);
			tok = isl_stream_next_token(s);
		}

		if (!tok || tok->type != ',')
			break;

		isl_token_free(tok);
	}
	if (tok)
		isl_stream_push_token(s, tok);

	return bmap;
error:
	isl_token_free(tok);
	isl_basic_map_free(bmap);
	return NULL;
}

static __isl_give isl_basic_map *read_tuple(struct isl_stream *s,
	__isl_take isl_basic_map *bmap, enum isl_dim_type type, struct vars *v)
{
	struct isl_token *tok;

	tok = isl_stream_next_token(s);
	if (!tok || tok->type != '[') {
		isl_stream_error(s, tok, "expecting '['");
		goto error;
	}
	isl_token_free(tok);
	bmap = read_var_list(s, bmap, type, v);
	tok = isl_stream_next_token(s);
	if (!tok || tok->type != ']') {
		isl_stream_error(s, tok, "expecting ']'");
		goto error;
	}
	isl_token_free(tok);

	return bmap;
error:
	if (tok)
		isl_token_free(tok);
	isl_basic_map_free(bmap);
	return NULL;
}

static struct isl_basic_map *add_constraints(struct isl_stream *s,
	struct vars *v, struct isl_basic_map *bmap);

static struct isl_basic_map *add_exists(struct isl_stream *s,
	struct vars *v, struct isl_basic_map *bmap)
{
	struct isl_token *tok;
	int n = v->n;
	int extra;
	int seen_paren = 0;
	int i;
	unsigned total;

	tok = isl_stream_next_token(s);
	if (!tok)
		goto error;
	if (tok->type == '(') {
		seen_paren = 1;
		isl_token_free(tok);
	} else
		isl_stream_push_token(s, tok);

	bmap = read_defined_var_list(s, v, bmap);
	if (!bmap)
		goto error;

	if (isl_stream_eat(s, ':'))
		goto error;
	bmap = add_constraints(s, v, bmap);
	if (seen_paren && isl_stream_eat(s, ')'))
		goto error;
	return bmap;
error:
	isl_basic_map_free(bmap);
	return NULL;
}

static __isl_give isl_basic_map *construct_constraint(
	__isl_take isl_basic_map *bmap, enum isl_token_type type,
	isl_int *left, isl_int *right)
{
	int k;
	unsigned len;
	struct isl_ctx *ctx;

	if (!bmap)
		return NULL;
	len = 1 + isl_basic_map_total_dim(bmap);
	ctx = bmap->ctx;

	k = isl_basic_map_alloc_inequality(bmap);
	if (k < 0)
		goto error;
	if (type == ISL_TOKEN_LE)
		isl_seq_combine(bmap->ineq[k], ctx->negone, left,
					       ctx->one, right,
					       len);
	else if (type == ISL_TOKEN_GE)
		isl_seq_combine(bmap->ineq[k], ctx->one, left,
					       ctx->negone, right,
					       len);
	else if (type == ISL_TOKEN_LT) {
		isl_seq_combine(bmap->ineq[k], ctx->negone, left,
					       ctx->one, right,
					       len);
		isl_int_sub_ui(bmap->ineq[k][0], bmap->ineq[k][0], 1);
	} else if (type == ISL_TOKEN_GT) {
		isl_seq_combine(bmap->ineq[k], ctx->one, left,
					       ctx->negone, right,
					       len);
		isl_int_sub_ui(bmap->ineq[k][0], bmap->ineq[k][0], 1);
	} else {
		isl_seq_combine(bmap->ineq[k], ctx->one, left,
					       ctx->negone, right,
					       len);
		isl_basic_map_inequality_to_equality(bmap, k);
	}

	return bmap;
error:
	isl_basic_map_free(bmap);
	return NULL;
}

static int is_comparator(struct isl_token *tok)
{
	if (!tok)
		return 0;

	switch (tok->type) {
	case ISL_TOKEN_LT:
	case ISL_TOKEN_GT:
	case ISL_TOKEN_LE:
	case ISL_TOKEN_GE:
	case '=':
		return 1;
	default:
		return 0;
	}
}

static struct isl_basic_map *add_constraint(struct isl_stream *s,
	struct vars *v, struct isl_basic_map *bmap)
{
	int i, j;
	unsigned total = isl_basic_map_total_dim(bmap);
	struct isl_token *tok = NULL;
	struct isl_mat *aff1 = NULL, *aff2 = NULL;

	tok = isl_stream_next_token(s);
	if (!tok)
		goto error;
	if (tok->type == ISL_TOKEN_EXISTS) {
		isl_token_free(tok);
		return add_exists(s, v, bmap);
	}
	isl_stream_push_token(s, tok);
	tok = NULL;

	bmap = isl_basic_map_cow(bmap);

	aff1 = accept_affine_list(s, v);
	if (!aff1)
		goto error;
	tok = isl_stream_next_token(s);
	if (!is_comparator(tok)) {
		isl_stream_error(s, tok, "missing operator");
		if (tok)
			isl_stream_push_token(s, tok);
		tok = NULL;
		goto error;
	}
	isl_assert(aff1->ctx, aff1->n_col == 1 + total, goto error);
	for (;;) {
		aff2 = accept_affine_list(s, v);
		if (!aff2)
			goto error;
		isl_assert(aff2->ctx, aff2->n_col == 1 + total, goto error);

		bmap = isl_basic_map_extend_constraints(bmap, 0,
						aff1->n_row * aff2->n_row);
		for (i = 0; i < aff1->n_row; ++i)
			for (j = 0; j < aff2->n_row; ++j)
				bmap = construct_constraint(bmap, tok->type,
						    aff1->row[i], aff2->row[j]);
		isl_token_free(tok);
		isl_mat_free(aff1);
		aff1 = aff2;

		tok = isl_stream_next_token(s);
		if (!is_comparator(tok)) {
			if (tok)
				isl_stream_push_token(s, tok);
			break;
		}
	}
	isl_mat_free(aff1);

	return bmap;
error:
	if (tok)
		isl_token_free(tok);
	isl_mat_free(aff1);
	isl_mat_free(aff2);
	isl_basic_map_free(bmap);
	return NULL;
}

static struct isl_basic_map *add_constraints(struct isl_stream *s,
	struct vars *v, struct isl_basic_map *bmap)
{
	struct isl_token *tok;

	for (;;) {
		bmap = add_constraint(s, v, bmap);
		if (!bmap)
			return NULL;
		tok = isl_stream_next_token(s);
		if (!tok) {
			isl_stream_error(s, NULL, "unexpected EOF");
			goto error;
		}
		if (tok->type != ISL_TOKEN_AND)
			break;
		isl_token_free(tok);
	}
	isl_stream_push_token(s, tok);

	return bmap;
error:
	if (tok)
		isl_token_free(tok);
	isl_basic_map_free(bmap);
	return NULL;
}

static struct isl_basic_map *read_disjunct(struct isl_stream *s,
	struct vars *v, __isl_take isl_dim *dim)
{
	int seen_paren = 0;
	struct isl_token *tok;
	struct isl_basic_map *bmap;

	bmap = isl_basic_map_alloc_dim(dim, 0, 0, 0);
	if (!bmap)
		return NULL;

	tok = isl_stream_next_token(s);
	if (!tok)
		goto error;
	if (tok->type == '(') {
		seen_paren = 1;
		isl_token_free(tok);
	} else
		isl_stream_push_token(s, tok);

	bmap = add_constraints(s, v, bmap);
	bmap = isl_basic_map_simplify(bmap);
	bmap = isl_basic_map_finalize(bmap);

	if (seen_paren && isl_stream_eat(s, ')'))
		goto error;

	return bmap;
error:
	isl_basic_map_free(bmap);
	return NULL;
}

static struct isl_map *read_disjuncts(struct isl_stream *s,
	struct vars *v, __isl_take isl_dim *dim)
{
	struct isl_token *tok;
	struct isl_map *map;

	tok = isl_stream_next_token(s);
	if (!tok) {
		isl_stream_error(s, NULL, "unexpected EOF");
		goto error;
	}
	if (tok->type == '}') {
		isl_stream_push_token(s, tok);
		return isl_map_universe(dim);
	}
	isl_stream_push_token(s, tok);

	map = isl_map_empty(isl_dim_copy(dim));
	for (;;) {
		struct isl_basic_map *bmap;
		int n = v->n;

		bmap = read_disjunct(s, v, isl_dim_copy(dim));
		map = isl_map_union(map, isl_map_from_basic_map(bmap));

		vars_drop(v, v->n - n);

		tok = isl_stream_next_token(s);
		if (!tok || tok->type != ISL_TOKEN_OR)
			break;
		isl_token_free(tok);
	}
	if (tok)
		isl_stream_push_token(s, tok);

	isl_dim_free(dim);
	return map;
error:
	isl_dim_free(dim);
	return NULL;
}

static int polylib_pos_to_isl_pos(__isl_keep isl_basic_map *bmap, int pos)
{
	if (pos < isl_basic_map_dim(bmap, isl_dim_out))
		return 1 + isl_basic_map_dim(bmap, isl_dim_param) +
			   isl_basic_map_dim(bmap, isl_dim_in) + pos;
	pos -= isl_basic_map_dim(bmap, isl_dim_out);

	if (pos < isl_basic_map_dim(bmap, isl_dim_in))
		return 1 + isl_basic_map_dim(bmap, isl_dim_param) + pos;
	pos -= isl_basic_map_dim(bmap, isl_dim_in);

	if (pos < isl_basic_map_dim(bmap, isl_dim_div))
		return 1 + isl_basic_map_dim(bmap, isl_dim_param) +
			   isl_basic_map_dim(bmap, isl_dim_in) +
			   isl_basic_map_dim(bmap, isl_dim_out) + pos;
	pos -= isl_basic_map_dim(bmap, isl_dim_div);

	if (pos < isl_basic_map_dim(bmap, isl_dim_param))
		return 1 + pos;

	return 0;
}

static __isl_give isl_basic_map *basic_map_read_polylib_constraint(
	struct isl_stream *s, __isl_take isl_basic_map *bmap)
{
	int j;
	struct isl_token *tok;
	int type;
	int k;
	isl_int *c;
	unsigned nparam;
	unsigned dim;

	if (!bmap)
		return NULL;

	nparam = isl_basic_map_dim(bmap, isl_dim_param);
	dim = isl_basic_map_dim(bmap, isl_dim_out);

	tok = isl_stream_next_token(s);
	if (!tok || tok->type != ISL_TOKEN_VALUE) {
		isl_stream_error(s, tok, "expecting coefficient");
		if (tok)
			isl_stream_push_token(s, tok);
		goto error;
	}
	if (!tok->on_new_line) {
		isl_stream_error(s, tok, "coefficient should appear on new line");
		isl_stream_push_token(s, tok);
		goto error;
	}

	type = isl_int_get_si(tok->u.v);
	isl_token_free(tok);

	isl_assert(s->ctx, type == 0 || type == 1, goto error);
	if (type == 0) {
		k = isl_basic_map_alloc_equality(bmap);
		c = bmap->eq[k];
	} else {
		k = isl_basic_map_alloc_inequality(bmap);
		c = bmap->ineq[k];
	}
	if (k < 0)
		goto error;

	for (j = 0; j < 1 + isl_basic_map_total_dim(bmap); ++j) {
		int pos;
		tok = isl_stream_next_token(s);
		if (!tok || tok->type != ISL_TOKEN_VALUE) {
			isl_stream_error(s, tok, "expecting coefficient");
			if (tok)
				isl_stream_push_token(s, tok);
			goto error;
		}
		if (tok->on_new_line) {
			isl_stream_error(s, tok,
				"coefficient should not appear on new line");
			isl_stream_push_token(s, tok);
			goto error;
		}
		pos = polylib_pos_to_isl_pos(bmap, j);
		isl_int_set(c[pos], tok->u.v);
		isl_token_free(tok);
	}

	return bmap;
error:
	isl_basic_map_free(bmap);
	return NULL;
}

static __isl_give isl_basic_map *basic_map_read_polylib(struct isl_stream *s,
	int nparam)
{
	int i;
	struct isl_token *tok;
	struct isl_token *tok2;
	int n_row, n_col;
	int on_new_line;
	unsigned in = 0, out, local = 0;
	struct isl_basic_map *bmap = NULL;

	if (nparam < 0)
		nparam = 0;

	tok = isl_stream_next_token(s);
	if (!tok) {
		isl_stream_error(s, NULL, "unexpected EOF");
		return NULL;
	}
	tok2 = isl_stream_next_token(s);
	if (!tok2) {
		isl_token_free(tok);
		isl_stream_error(s, NULL, "unexpected EOF");
		return NULL;
	}
	n_row = isl_int_get_si(tok->u.v);
	n_col = isl_int_get_si(tok2->u.v);
	on_new_line = tok2->on_new_line;
	isl_token_free(tok2);
	isl_token_free(tok);
	isl_assert(s->ctx, !on_new_line, return NULL);
	isl_assert(s->ctx, n_row >= 0, return NULL);
	isl_assert(s->ctx, n_col >= 2 + nparam, return NULL);
	tok = isl_stream_next_token_on_same_line(s);
	if (tok) {
		if (tok->type != ISL_TOKEN_VALUE) {
			isl_stream_error(s, tok,
				    "expecting number of output dimensions");
			isl_stream_push_token(s, tok);
			goto error;
		}
		out = isl_int_get_si(tok->u.v);
		isl_token_free(tok);

		tok = isl_stream_next_token_on_same_line(s);
		if (!tok || tok->type != ISL_TOKEN_VALUE) {
			isl_stream_error(s, tok,
				    "expecting number of input dimensions");
			if (tok)
				isl_stream_push_token(s, tok);
			goto error;
		}
		in = isl_int_get_si(tok->u.v);
		isl_token_free(tok);

		tok = isl_stream_next_token_on_same_line(s);
		if (!tok || tok->type != ISL_TOKEN_VALUE) {
			isl_stream_error(s, tok,
				    "expecting number of existentials");
			if (tok)
				isl_stream_push_token(s, tok);
			goto error;
		}
		local = isl_int_get_si(tok->u.v);
		isl_token_free(tok);

		tok = isl_stream_next_token_on_same_line(s);
		if (!tok || tok->type != ISL_TOKEN_VALUE) {
			isl_stream_error(s, tok,
				    "expecting number of parameters");
			if (tok)
				isl_stream_push_token(s, tok);
			goto error;
		}
		nparam = isl_int_get_si(tok->u.v);
		isl_token_free(tok);
		if (n_col != 1 + out + in + local + nparam + 1) {
			isl_stream_error(s, NULL,
				    "dimensions don't match");
			goto error;
		}
	} else
		out = n_col - 2 - nparam;
	bmap = isl_basic_map_alloc(s->ctx, nparam, in, out, local, n_row, n_row);
	if (!bmap)
		return NULL;

	for (i = 0; i < local; ++i) {
		int k = isl_basic_map_alloc_div(bmap);
		if (k < 0)
			goto error;
	}

	for (i = 0; i < n_row; ++i)
		bmap = basic_map_read_polylib_constraint(s, bmap);

	tok = isl_stream_next_token_on_same_line(s);
	if (tok) {
		isl_stream_error(s, tok, "unexpected extra token on line");
		isl_stream_push_token(s, tok);
		goto error;
	}

	bmap = isl_basic_map_simplify(bmap);
	bmap = isl_basic_map_finalize(bmap);
	return bmap;
error:
	isl_basic_map_free(bmap);
	return NULL;
}

static struct isl_map *map_read_polylib(struct isl_stream *s, int nparam)
{
	struct isl_token *tok;
	struct isl_token *tok2;
	int i, n;
	struct isl_map *map;

	tok = isl_stream_next_token(s);
	if (!tok) {
		isl_stream_error(s, NULL, "unexpected EOF");
		return NULL;
	}
	tok2 = isl_stream_next_token_on_same_line(s);
	if (tok2) {
		isl_stream_push_token(s, tok2);
		isl_stream_push_token(s, tok);
		return isl_map_from_basic_map(basic_map_read_polylib(s, nparam));
	}
	n = isl_int_get_si(tok->u.v);
	isl_token_free(tok);

	isl_assert(s->ctx, n >= 1, return NULL);

	map = isl_map_from_basic_map(basic_map_read_polylib(s, nparam));

	for (i = 1; i < n; ++i)
		map = isl_map_union(map,
			isl_map_from_basic_map(basic_map_read_polylib(s, nparam)));

	return map;
}

static int optional_power(struct isl_stream *s)
{
	int pow;
	struct isl_token *tok;

	tok = isl_stream_next_token(s);
	if (!tok)
		return 1;
	if (tok->type != '^') {
		isl_stream_push_token(s, tok);
		return 1;
	}
	isl_token_free(tok);
	tok = isl_stream_next_token(s);
	if (!tok || tok->type != ISL_TOKEN_VALUE) {
		isl_stream_error(s, tok, "expecting exponent");
		if (tok)
			isl_stream_push_token(s, tok);
		return 1;
	}
	pow = isl_int_get_si(tok->u.v);
	isl_token_free(tok);
	return pow;
}

static __isl_give isl_div *read_div(struct isl_stream *s,
	__isl_keep isl_basic_map *bmap, struct vars *v)
{
	unsigned total = isl_basic_map_total_dim(bmap);
	int k;

	bmap = isl_basic_map_copy(bmap);
	bmap = isl_basic_map_cow(bmap);
	bmap = isl_basic_map_extend_dim(bmap, isl_dim_copy(bmap->dim),
					1, 0, 2);

	if ((k = isl_basic_map_alloc_div(bmap)) < 0)
		goto error;
	isl_seq_clr(bmap->div[k], 1 + 1 + total);
	bmap = add_div_definition(s, v, bmap, k);

	return isl_basic_map_div(bmap, k);
error:
	isl_basic_map_free(bmap);
	return NULL;
}

static __isl_give isl_qpolynomial *read_term(struct isl_stream *s,
	__isl_keep isl_basic_map *bmap, struct vars *v);

static __isl_give isl_qpolynomial *read_factor(struct isl_stream *s,
	__isl_keep isl_basic_map *bmap, struct vars *v)
{
	struct isl_qpolynomial *qp;
	struct isl_token *tok;

	tok = isl_stream_next_token(s);
	if (!tok) {
		isl_stream_error(s, NULL, "unexpected EOF");
		return NULL;
	}
	if (tok->type == '(') {
		isl_token_free(tok);
		qp = read_term(s, bmap, v);
		if (!qp)
			return NULL;
		if (isl_stream_eat(s, ')'))
			goto error;
	} else if (tok->type == ISL_TOKEN_VALUE) {
		struct isl_token *tok2;
		tok2 = isl_stream_next_token(s);
		if (tok2 && tok2->type == '/') {
			isl_token_free(tok2);
			tok2 = isl_stream_next_token(s);
			if (!tok2 || tok2->type != ISL_TOKEN_VALUE) {
				isl_stream_error(s, tok2, "expected denominator");
				isl_token_free(tok);
				isl_token_free(tok2);
				return NULL;
			}
			qp = isl_qpolynomial_rat_cst(isl_basic_map_get_dim(bmap),
						    tok->u.v, tok2->u.v);
			isl_token_free(tok2);
		} else {
			isl_stream_push_token(s, tok2);
			qp = isl_qpolynomial_cst(isl_basic_map_get_dim(bmap),
						tok->u.v);
		}
		isl_token_free(tok);
	} else if (tok->type == ISL_TOKEN_INFTY) {
		isl_token_free(tok);
		qp = isl_qpolynomial_infty(isl_basic_map_get_dim(bmap));
	} else if (tok->type == ISL_TOKEN_NAN) {
		isl_token_free(tok);
		qp = isl_qpolynomial_nan(isl_basic_map_get_dim(bmap));
	} else if (tok->type == ISL_TOKEN_IDENT) {
		int n = v->n;
		int pos = vars_pos(v, tok->u.s, -1);
		int pow;
		isl_token_free(tok);
		if (pos < 0)
			goto error;
		if (pos >= n) {
			isl_stream_error(s, tok, "unknown identifier");
			return NULL;
		}
		pow = optional_power(s);
		qp = isl_qpolynomial_pow(isl_basic_map_get_dim(bmap), pos, pow);
	} else if (tok->type == '[') {
		isl_div *div;
		int pow;

		isl_stream_push_token(s, tok);
		div = read_div(s, bmap, v);
		pow = optional_power(s);
		qp = isl_qpolynomial_div_pow(div, pow);
	} else if (tok->type == '-') {
		struct isl_qpolynomial *qp2;

		isl_token_free(tok);
		qp = isl_qpolynomial_cst(isl_basic_map_get_dim(bmap),
					    s->ctx->negone);
		qp2 = read_factor(s, bmap, v);
		qp = isl_qpolynomial_mul(qp, qp2);
	} else {
		isl_stream_error(s, tok, "unexpected isl_token");
		isl_stream_push_token(s, tok);
		return NULL;
	}

	if (isl_stream_eat_if_available(s, '*') ||
	    isl_stream_next_token_is(s, ISL_TOKEN_IDENT)) {
		struct isl_qpolynomial *qp2;

		qp2 = read_factor(s, bmap, v);
		qp = isl_qpolynomial_mul(qp, qp2);
	}

	return qp;
error:
	isl_qpolynomial_free(qp);
	return NULL;
}

static __isl_give isl_qpolynomial *read_term(struct isl_stream *s,
	__isl_keep isl_basic_map *bmap, struct vars *v)
{
	struct isl_token *tok;
	struct isl_qpolynomial *qp;

	qp = read_factor(s, bmap, v);

	for (;;) {
		tok = isl_stream_next_token(s);
		if (!tok)
			return qp;

		if (tok->type == '+') {
			struct isl_qpolynomial *qp2;

			isl_token_free(tok);
			qp2 = read_factor(s, bmap, v);
			qp = isl_qpolynomial_add(qp, qp2);
		} else if (tok->type == '-') {
			struct isl_qpolynomial *qp2;

			isl_token_free(tok);
			qp2 = read_factor(s, bmap, v);
			qp = isl_qpolynomial_sub(qp, qp2);
		} else if (tok->type == ISL_TOKEN_VALUE &&
			    isl_int_is_neg(tok->u.v)) {
			struct isl_qpolynomial *qp2;

			qp2 = isl_qpolynomial_cst(isl_basic_map_get_dim(bmap),
							tok->u.v);
			isl_token_free(tok);
			qp = isl_qpolynomial_add(qp, qp2);
		} else {
			isl_stream_push_token(s, tok);
			break;
		}
	}

	return qp;
}

static __isl_give isl_map *read_optional_disjuncts(struct isl_stream *s,
	__isl_take isl_basic_map *bmap, struct vars *v)
{
	struct isl_token *tok;
	struct isl_map *map;

	tok = isl_stream_next_token(s);
	if (!tok) {
		isl_stream_error(s, NULL, "unexpected EOF");
		goto error;
	}
	map = isl_map_from_basic_map(isl_basic_map_copy(bmap));
	if (tok->type == ':') {
		isl_token_free(tok);
		map = isl_map_intersect(map,
			    read_disjuncts(s, v, isl_basic_map_get_dim(bmap)));
	} else
		isl_stream_push_token(s, tok);

	isl_basic_map_free(bmap);

	return map;
error:
	isl_basic_map_free(bmap);
	return NULL;
}

static struct isl_obj obj_read_poly(struct isl_stream *s,
	__isl_take isl_basic_map *bmap, struct vars *v, int n)
{
	struct isl_obj obj = { isl_obj_pw_qpolynomial, NULL };
	struct isl_pw_qpolynomial *pwqp;
	struct isl_qpolynomial *qp;
	struct isl_map *map;
	struct isl_set *set;

	bmap = isl_basic_map_reverse(bmap);

	qp = read_term(s, bmap, v);
	map = read_optional_disjuncts(s, bmap, v);
	set = isl_set_from_map(map);

	pwqp = isl_pw_qpolynomial_alloc(set, qp);

	vars_drop(v, v->n - n);

	obj.v = pwqp;
	return obj;
}

static struct isl_obj obj_read_body(struct isl_stream *s,
	__isl_take isl_basic_map *bmap, struct vars *v, isl_obj_type type)
{
	struct isl_map *map = NULL;
	struct isl_token *tok;
	struct isl_obj obj = { type, NULL };
	int n = v->n;

	if (!isl_stream_next_token_is(s, '['))
		return obj_read_poly(s, bmap, v, n);

	bmap = read_tuple(s, bmap, isl_dim_in, v);
	if (!bmap)
		goto error;
	tok = isl_stream_next_token(s);
	if (tok && tok->type == ISL_TOKEN_TO) {
		obj.type = isl_obj_map;
		isl_token_free(tok);
		tok = isl_stream_next_token(s);
		if (tok && tok->type != '[') {
			isl_stream_push_token(s, tok);
			return obj_read_poly(s, bmap, v, n);
		}
		isl_stream_push_token(s, tok);
		bmap = read_tuple(s, bmap, isl_dim_out, v);
		if (!bmap)
			goto error;
	} else {
		bmap = isl_basic_map_reverse(bmap);
		if (tok)
			isl_stream_push_token(s, tok);
	}

	map = read_optional_disjuncts(s, bmap, v);

	vars_drop(v, v->n - n);

	obj.v = map;
	return obj;
error:
	isl_basic_map_free(bmap);
	obj.type = isl_obj_none;
	return obj;
}

static struct isl_obj obj_add(struct isl_ctx *ctx,
	struct isl_obj obj1, struct isl_obj obj2)
{
	isl_assert(ctx, obj1.type == obj2.type, goto error);
	obj1.v = obj1.type->add(obj1.v, obj2.v);
	return obj1;
error:
	obj1.type->free(obj1.v);
	obj2.type->free(obj2.v);
	obj1.type = isl_obj_none;
	obj1.v = NULL;
	return obj1;
}

static struct isl_obj obj_read(struct isl_stream *s, int nparam)
{
	struct isl_basic_map *bmap = NULL;
	struct isl_token *tok;
	struct vars *v = NULL;
	struct isl_obj obj = { isl_obj_set, NULL };

	tok = isl_stream_next_token(s);
	if (!tok) {
		isl_stream_error(s, NULL, "unexpected EOF");
		goto error;
	}
	if (tok->type == ISL_TOKEN_VALUE) {
		struct isl_map *map;
		isl_stream_push_token(s, tok);
		map = map_read_polylib(s, nparam);
		if (!map)
			goto error;
		if (isl_map_dim(map, isl_dim_in) > 0)
			obj.type = isl_obj_map;
		obj.v = map;
		return obj;
	}
	v = vars_new(s->ctx);
	bmap = isl_basic_map_alloc(s->ctx, 0, 0, 0, 0, 0, 0);
	if (tok->type == '[') {
		isl_stream_push_token(s, tok);
		bmap = read_tuple(s, bmap, isl_dim_param, v);
		if (!bmap)
			goto error;
		if (nparam >= 0)
			isl_assert(s->ctx, nparam == v->n, goto error);
		tok = isl_stream_next_token(s);
		if (!tok || tok->type != ISL_TOKEN_TO) {
			isl_stream_error(s, tok, "expecting '->'");
			if (tok)
				isl_stream_push_token(s, tok);
			goto error;
		}
		isl_token_free(tok);
		tok = isl_stream_next_token(s);
	} if (nparam > 0)
		bmap = isl_basic_map_add(bmap, isl_dim_param, nparam);
	if (!tok || tok->type != '{') {
		isl_stream_error(s, tok, "expecting '{'");
		if (tok)
			isl_stream_push_token(s, tok);
		goto error;
	}
	isl_token_free(tok);

	for (;;) {
		struct isl_obj o;
		tok = NULL;
		o = obj_read_body(s, isl_basic_map_copy(bmap), v, obj.type);
		if (o.type == isl_obj_none)
			break;
		if (!obj.v)
			obj = o;
		else {
			obj = obj_add(s->ctx, obj, o);
			if (obj.type == isl_obj_none)
				break;
		}
		tok = isl_stream_next_token(s);
		if (!tok || tok->type != ';')
			break;
		isl_token_free(tok);
	}

	if (tok && tok->type == '}') {
		isl_token_free(tok);
	} else {
		isl_stream_error(s, tok, "unexpected isl_token");
		if (tok)
			isl_token_free(tok);
		goto error;
	}
	vars_free(v);
	isl_basic_map_free(bmap);

	return obj;
error:
	isl_basic_map_free(bmap);
	obj.type->free(obj.v);
	if (v)
		vars_free(v);
	obj.v = NULL;
	return obj;
}

struct isl_obj isl_stream_read_obj(struct isl_stream *s)
{
	return obj_read(s, -1);
}

static struct isl_basic_map *basic_map_read(struct isl_stream *s, int nparam)
{
	struct isl_obj obj;
	struct isl_map *map;
	struct isl_basic_map *bmap;

	obj = obj_read(s, nparam);
	map = obj.v;
	if (!map)
		return NULL;

	isl_assert(map->ctx, map->n <= 1, goto error);

	if (map->n == 0)
		bmap = isl_basic_map_empty_like_map(map);
	else
		bmap = isl_basic_map_copy(map->p[0]);

	isl_map_free(map);

	return bmap;
error:
	isl_map_free(map);
	return NULL;
}

__isl_give isl_basic_map *isl_basic_map_read_from_file(isl_ctx *ctx,
		FILE *input, int nparam)
{
	struct isl_basic_map *bmap;
	struct isl_stream *s = isl_stream_new_file(ctx, input);
	if (!s)
		return NULL;
	bmap = basic_map_read(s, nparam);
	isl_stream_free(s);
	return bmap;
}

__isl_give isl_basic_set *isl_basic_set_read_from_file(isl_ctx *ctx,
		FILE *input, int nparam)
{
	struct isl_basic_map *bmap;
	bmap = isl_basic_map_read_from_file(ctx, input, nparam);
	if (!bmap)
		return NULL;
	isl_assert(ctx, isl_basic_map_n_in(bmap) == 0, goto error);
	return (struct isl_basic_set *)bmap;
error:
	isl_basic_map_free(bmap);
	return NULL;
}

struct isl_basic_map *isl_basic_map_read_from_str(struct isl_ctx *ctx,
		const char *str, int nparam)
{
	struct isl_basic_map *bmap;
	struct isl_stream *s = isl_stream_new_str(ctx, str);
	if (!s)
		return NULL;
	bmap = basic_map_read(s, nparam);
	isl_stream_free(s);
	return bmap;
}

struct isl_basic_set *isl_basic_set_read_from_str(struct isl_ctx *ctx,
		const char *str, int nparam)
{
	struct isl_basic_map *bmap;
	bmap = isl_basic_map_read_from_str(ctx, str, nparam);
	if (!bmap)
		return NULL;
	isl_assert(ctx, isl_basic_map_n_in(bmap) == 0, goto error);
	return (struct isl_basic_set *)bmap;
error:
	isl_basic_map_free(bmap);
	return NULL;
}

__isl_give isl_map *isl_map_read_from_file(struct isl_ctx *ctx,
		FILE *input, int nparam)
{
	struct isl_obj obj;
	struct isl_map *map;
	struct isl_stream *s = isl_stream_new_file(ctx, input);
	if (!s)
		return NULL;
	obj = obj_read(s, nparam);
	isl_stream_free(s);
	return obj.v;
}

__isl_give isl_map *isl_map_read_from_str(struct isl_ctx *ctx,
		const char *str, int nparam)
{
	struct isl_obj obj;
	struct isl_map *map;
	struct isl_stream *s = isl_stream_new_str(ctx, str);
	if (!s)
		return NULL;
	obj = obj_read(s, nparam);
	isl_stream_free(s);
	return obj.v;
}

__isl_give isl_set *isl_set_read_from_file(struct isl_ctx *ctx,
		FILE *input, int nparam)
{
	struct isl_map *map;
	map = isl_map_read_from_file(ctx, input, nparam);
	if (!map)
		return NULL;
	isl_assert(ctx, isl_map_n_in(map) == 0, goto error);
	return (struct isl_set *)map;
error:
	isl_map_free(map);
	return NULL;
}

struct isl_set *isl_set_read_from_str(struct isl_ctx *ctx,
		const char *str, int nparam)
{
	struct isl_map *map;
	map = isl_map_read_from_str(ctx, str, nparam);
	if (!map)
		return NULL;
	isl_assert(ctx, isl_map_n_in(map) == 0, goto error);
	return (struct isl_set *)map;
error:
	isl_map_free(map);
	return NULL;
}

static char *next_line(FILE *input, char *line, unsigned len)
{
	char *p;

	do {
		if (!(p = fgets(line, len, input)))
			return NULL;
		while (isspace(*p) && *p != '\n')
			++p;
	} while (*p == '#' || *p == '\n');

	return p;
}

static struct isl_vec *isl_vec_read_from_file_polylib(struct isl_ctx *ctx,
		FILE *input)
{
	struct isl_vec *vec = NULL;
	char line[1024];
	char val[1024];
	char *p;
	unsigned size;
	int j;
	int n;
	int offset;

	isl_assert(ctx, next_line(input, line, sizeof(line)), return NULL);
	isl_assert(ctx, sscanf(line, "%u", &size) == 1, return NULL);

	vec = isl_vec_alloc(ctx, size);

	p = next_line(input, line, sizeof(line));
	isl_assert(ctx, p, goto error);

	for (j = 0; j < size; ++j) {
		n = sscanf(p, "%s%n", val, &offset);
		isl_assert(ctx, n != 0, goto error);
		isl_int_read(vec->el[j], val);
		p += offset;
	}

	return vec;
error:
	isl_vec_free(vec);
	return NULL;
}

struct isl_vec *isl_vec_read_from_file(struct isl_ctx *ctx,
		FILE *input, unsigned input_format)
{
	if (input_format == ISL_FORMAT_POLYLIB)
		return isl_vec_read_from_file_polylib(ctx, input);
	else
		isl_assert(ctx, 0, return NULL);
}
