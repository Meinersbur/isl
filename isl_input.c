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
#include <isl_ctx_private.h>
#include <isl_map_private.h>
#include <isl/set.h>
#include <isl/seq.h>
#include <isl/div.h>
#include <isl/stream.h>
#include <isl/obj.h>
#include "isl_polynomial_private.h"
#include <isl/union_map.h>
#include <isl_mat_private.h>

struct variable {
	char    	    	*name;
	int	     		 pos;
	isl_vec			*def;
	/* non-zero if variable represents a min (-1) or a max (1) */
	int			 sign;
	isl_mat			*list;
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
		isl_mat_free(var->list);
		isl_vec_free(var->def);
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

	if (!v || !v->v)
		return;

	v->n -= n;

	var = v->v;
	while (--n >= 0) {
		struct variable *next = var->next;
		isl_mat_free(var->list);
		isl_vec_free(var->def);
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
	var = isl_calloc_type(v->ctx, struct variable);
	if (!var)
		goto error;
	var->name = strdup(name);
	var->name[len] = '\0';
	var->pos = pos;
	var->def = NULL;
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

static int vars_add_anon(struct vars *v)
{
	v->v = variable_new(v, "", 0, v->n);

	if (!v->v)
		return -1;
	v->n++;

	return 0;
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
	bmap = isl_basic_map_set_dim_name(bmap, type, pos, name);
	if (prime)
		*prime = '\'';

	return bmap;
}

/* Obtain next token, with some preprocessing.
 * In particular, evaluate expressions of the form x^y,
 * with x and y values.
 */
static struct isl_token *next_token(struct isl_stream *s)
{
	struct isl_token *tok, *tok2;

	tok = isl_stream_next_token(s);
	if (!tok || tok->type != ISL_TOKEN_VALUE)
		return tok;
	if (!isl_stream_eat_if_available(s, '^'))
		return tok;
	tok2 = isl_stream_next_token(s);
	if (!tok2 || tok2->type != ISL_TOKEN_VALUE) {
		isl_stream_error(s, tok2, "expecting constant value");
		goto error;
	}

	isl_int_pow_ui(tok->u.v, tok->u.v, isl_int_get_ui(tok2->u.v));

	isl_token_free(tok2);
	return tok;
error:
	isl_token_free(tok);
	isl_token_free(tok2);
	return NULL;
}

static int accept_cst_factor(struct isl_stream *s, isl_int *f)
{
	struct isl_token *tok;

	tok = next_token(s);
	if (!tok || tok->type != ISL_TOKEN_VALUE) {
		isl_stream_error(s, tok, "expecting constant value");
		goto error;
	}

	isl_int_mul(*f, *f, tok->u.v);

	isl_token_free(tok);

	if (isl_stream_eat_if_available(s, '*'))
		return accept_cst_factor(s, f);

	return 0;
error:
	isl_token_free(tok);
	return -1;
}

/* Given an affine expression aff, return an affine expression
 * for aff % d, with d the next token on the stream, which is
 * assumed to be a constant.
 *
 * We introduce an integer division q = [aff/d] and the result
 * is set to aff - d q.
 */
static __isl_give isl_vec *affine_mod(struct isl_stream *s,
	struct vars *v, __isl_take isl_vec *aff)
{
	struct isl_token *tok;
	struct variable *var;
	isl_vec *mod;

	tok = next_token(s);
	if (!tok || tok->type != ISL_TOKEN_VALUE) {
		isl_stream_error(s, tok, "expecting constant value");
		goto error;
	}

	if (vars_add_anon(v) < 0)
		goto error;

	var = v->v;

	var->def = isl_vec_alloc(s->ctx, 2 + v->n);
	if (!var->def)
		goto error;
	isl_seq_cpy(var->def->el + 1, aff->el, aff->size);
	isl_int_set_si(var->def->el[1 + aff->size], 0);
	isl_int_set(var->def->el[0], tok->u.v);

	mod = isl_vec_alloc(v->ctx, 1 + v->n);
	if (!mod)
		goto error;

	isl_seq_cpy(mod->el, aff->el, aff->size);
	isl_int_neg(mod->el[aff->size], tok->u.v);

	isl_vec_free(aff);
	isl_token_free(tok);
	return mod;
error:
	isl_vec_free(aff);
	isl_token_free(tok);
	return NULL;
}

static struct isl_vec *accept_affine(struct isl_stream *s, struct vars *v);
static int read_div_definition(struct isl_stream *s, struct vars *v);
static int read_minmax_definition(struct isl_stream *s, struct vars *v);

static __isl_give isl_vec *accept_affine_factor(struct isl_stream *s,
	struct vars *v)
{
	struct isl_token *tok = NULL;
	isl_vec *aff = NULL;

	tok = next_token(s);
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
			goto error;
		}

		aff = isl_vec_alloc(v->ctx, 1 + v->n);
		if (!aff)
			goto error;
		isl_seq_clr(aff->el, aff->size);
		isl_int_set_si(aff->el[1 + pos], 1);
		isl_token_free(tok);
	} else if (tok->type == ISL_TOKEN_VALUE) {
		if (isl_stream_eat_if_available(s, '*')) {
			aff = accept_affine_factor(s, v);
			aff = isl_vec_scale(aff, tok->u.v);
		} else {
			aff = isl_vec_alloc(v->ctx, 1 + v->n);
			if (!aff)
				goto error;
			isl_seq_clr(aff->el, aff->size);
			isl_int_set(aff->el[0], tok->u.v);
		}
		isl_token_free(tok);
	} else if (tok->type == '(') {
		isl_token_free(tok);
		tok = NULL;
		aff = accept_affine(s, v);
		if (!aff)
			goto error;
		if (isl_stream_eat(s, ')'))
			goto error;
	} else if (tok->type == '[' ||
		    tok->type == ISL_TOKEN_FLOORD ||
		    tok->type == ISL_TOKEN_CEILD) {
		int ceil = tok->type == ISL_TOKEN_CEILD;
		struct variable *var;
		if (vars_add_anon(v) < 0)
			goto error;
		var = v->v;
		aff = isl_vec_alloc(v->ctx, 1 + v->n);
		if (!aff)
			goto error;
		isl_seq_clr(aff->el, aff->size);
		isl_int_set_si(aff->el[1 + v->n - 1], ceil ? -1 : 1);
		isl_stream_push_token(s, tok);
		tok = NULL;
		if (read_div_definition(s, v) < 0)
			goto error;
		if (ceil)
			isl_seq_neg(var->def->el + 1, var->def->el + 1,
				    var->def->size - 1);
		aff = isl_vec_zero_extend(aff, 1 + v->n);
	} else if (tok->type == ISL_TOKEN_MIN || tok->type == ISL_TOKEN_MAX) {
		if (vars_add_anon(v) < 0)
			goto error;
		aff = isl_vec_alloc(v->ctx, 1 + v->n);
		if (!aff)
			goto error;
		isl_seq_clr(aff->el, aff->size);
		isl_int_set_si(aff->el[1 + v->n - 1], 1);
		isl_stream_push_token(s, tok);
		tok = NULL;
		if (read_minmax_definition(s, v) < 0)
			goto error;
		aff = isl_vec_zero_extend(aff, 1 + v->n);
	} else {
		isl_stream_error(s, tok, "expecting factor");
		goto error;
	}
	if (isl_stream_eat_if_available(s, '%'))
		return affine_mod(s, v, aff);
	if (isl_stream_eat_if_available(s, '*')) {
		isl_int f;
		isl_int_init(f);
		isl_int_set_si(f, 1);
		if (accept_cst_factor(s, &f) < 0) {
			isl_int_clear(f);
			goto error2;
		}
		aff = isl_vec_scale(aff, f);
		isl_int_clear(f);
	}

	return aff;
error:
	isl_token_free(tok);
error2:
	isl_vec_free(aff);
	return NULL;
}

static struct isl_vec *accept_affine(struct isl_stream *s, struct vars *v)
{
	struct isl_token *tok = NULL;
	struct isl_vec *aff;
	int sign = 1;

	aff = isl_vec_alloc(v->ctx, 1 + v->n);
	if (!aff)
		return NULL;
	isl_seq_clr(aff->el, aff->size);

	for (;;) {
		tok = next_token(s);
		if (!tok) {
			isl_stream_error(s, NULL, "unexpected EOF");
			goto error;
		}
		if (tok->type == '-') {
			sign = -sign;
			isl_token_free(tok);
			continue;
		}
		if (tok->type == '(' || tok->type == '[' ||
		    tok->type == ISL_TOKEN_MIN || tok->type == ISL_TOKEN_MAX ||
		    tok->type == ISL_TOKEN_FLOORD ||
		    tok->type == ISL_TOKEN_CEILD ||
		    tok->type == ISL_TOKEN_IDENT) {
			isl_vec *aff2;
			isl_stream_push_token(s, tok);
			tok = NULL;
			aff2 = accept_affine_factor(s, v);
			if (sign < 0)
				aff2 = isl_vec_scale(aff2, s->ctx->negone);
			aff = isl_vec_zero_extend(aff, 1 + v->n);
			aff = isl_vec_add(aff, aff2);
			if (!aff)
				goto error;
			sign = 1;
		} else if (tok->type == ISL_TOKEN_VALUE) {
			if (sign < 0)
				isl_int_neg(tok->u.v, tok->u.v);
			if (isl_stream_eat_if_available(s, '*') ||
			    isl_stream_next_token_is(s, ISL_TOKEN_IDENT)) {
				isl_vec *aff2;
				aff2 = accept_affine_factor(s, v);
				aff2 = isl_vec_scale(aff2, tok->u.v);
				aff = isl_vec_zero_extend(aff, 1 + v->n);
				aff = isl_vec_add(aff, aff2);
				if (!aff)
					goto error;
			} else {
				isl_int_add(aff->el[0], aff->el[0], tok->u.v);
			}
			sign = 1;
		} else {
			isl_stream_error(s, tok, "unexpected isl_token");
			isl_stream_push_token(s, tok);
			isl_vec_free(aff);
			return NULL;
		}
		isl_token_free(tok);

		tok = next_token(s);
		if (tok && tok->type == '-') {
			sign = -sign;
			isl_token_free(tok);
		} else if (tok && tok->type == '+') {
			/* nothing */
			isl_token_free(tok);
		} else if (tok && tok->type == ISL_TOKEN_VALUE &&
			   isl_int_is_neg(tok->u.v)) {
			isl_stream_push_token(s, tok);
		} else {
			if (tok)
				isl_stream_push_token(s, tok);
			break;
		}
	}

	return aff;
error:
	isl_token_free(tok);
	isl_vec_free(aff);
	return NULL;
}

/* Add any variables in the variable list "v" that are not already in "bmap"
 * as existentially quantified variables in "bmap".
 */
static __isl_give isl_basic_map *add_divs(__isl_take isl_basic_map *bmap,
	struct vars *v)
{
	int i;
	int extra;
	struct variable *var;

	extra = v->n - isl_basic_map_total_dim(bmap);

	if (extra == 0)
		return bmap;

	bmap = isl_basic_map_extend_dim(bmap, isl_basic_map_get_dim(bmap),
					extra, 0, 2 * extra);

	for (i = 0; i < extra; ++i)
		if (isl_basic_map_alloc_div(bmap) < 0)
			goto error;

	for (i = 0, var = v->v; i < extra; ++i, var = var->next) {
		int k = bmap->n_div - 1 - i;

		isl_seq_cpy(bmap->div[k], var->def->el, var->def->size);
		isl_seq_clr(bmap->div[k] + var->def->size,
			    2 + v->n - var->def->size);

		if (isl_basic_map_add_div_constraints(bmap, k) < 0)
			goto error;
	}

	return bmap;
error:
	isl_basic_map_free(bmap);
	return NULL;
}

static __isl_give isl_basic_map *read_var_def(struct isl_stream *s,
	__isl_take isl_basic_map *bmap, enum isl_dim_type type, struct vars *v)
{
	isl_dim *dim;
	isl_basic_map *def = NULL;
	struct isl_vec *vec;
	int k;
	int n;

	if (vars_add_anon(v) < 0)
		goto error;
	n = v->n;

	vec = accept_affine(s, v);
	if (!vec)
		goto error;

	dim = isl_basic_map_get_dim(bmap);
	def = isl_basic_map_universe(dim);
	def = add_divs(def, v);
	def = isl_basic_map_extend_constraints(def, 1, 0);
	k = isl_basic_map_alloc_equality(def);
	if (k >= 0) {
		isl_seq_cpy(def->eq[k], vec->el, vec->size);
		isl_int_set_si(def->eq[k][1 + n - 1], -1);
	}
	isl_vec_free(vec);
	if (k < 0)
		goto error;

	vars_drop(v, v->n - n);

	def = isl_basic_map_simplify(def);
	def = isl_basic_map_finalize(def);
	bmap = isl_basic_map_intersect(bmap, def);
	return bmap;
error:
	isl_basic_map_free(bmap);
	isl_basic_map_free(def);
	return NULL;
}

static __isl_give isl_basic_map *read_var_list(struct isl_stream *s,
	__isl_take isl_basic_map *bmap, enum isl_dim_type type, struct vars *v)
{
	int i = 0;
	struct isl_token *tok;

	while ((tok = next_token(s)) != NULL) {
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
			   tok->type == ISL_TOKEN_VALUE ||
			   tok->type == '-' ||
			   tok->type == '(') {
			if (type == isl_dim_param) {
				isl_stream_error(s, tok,
						"expecting unique identifier");
				goto error;
			}
			isl_stream_push_token(s, tok);
			tok = NULL;
			bmap = isl_basic_map_add(bmap, type, 1);
			bmap = read_var_def(s, bmap, type, v);
		} else
			break;

		tok = isl_stream_next_token(s);
		if (tok && tok->type == ']' &&
		    isl_stream_next_token_is(s, '[')) {
			isl_token_free(tok);
			tok = isl_stream_next_token(s);
		} else if (!tok || tok->type != ',')
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
		mat = isl_mat_add_zero_cols(mat, 1 + v->n - isl_mat_cols(mat));
		mat = isl_mat_vec_concat(mat, vec);
		if (!mat)
			return NULL;
	}

	return mat;
error:
	isl_mat_free(mat);
	return NULL;
}

static int read_minmax_definition(struct isl_stream *s, struct vars *v)
{
	struct isl_token *tok;
	struct variable *var;

	var = v->v;

	tok = isl_stream_next_token(s);
	if (!tok)
		return -1;
	var->sign = tok->type == ISL_TOKEN_MIN ? -1 : 1;
	isl_token_free(tok);

	if (isl_stream_eat(s, '('))
		return -1;

	var->list = accept_affine_list(s, v);
	if (!var->list)
		return -1;

	if (isl_stream_eat(s, ')'))
		return -1;

	return 0;
}

static int read_div_definition(struct isl_stream *s, struct vars *v)
{
	struct isl_token *tok;
	int seen_paren = 0;
	struct isl_vec *aff;
	struct variable *var;
	int fc = 0;

	if (isl_stream_eat_if_available(s, ISL_TOKEN_FLOORD) ||
	    isl_stream_eat_if_available(s, ISL_TOKEN_CEILD)) {
		fc = 1;
		if (isl_stream_eat(s, '('))
			return -1;
	} else {
		if (isl_stream_eat(s, '['))
			return -1;
		if (isl_stream_eat_if_available(s, '('))
			seen_paren = 1;
	}

	var = v->v;

	aff = accept_affine(s, v);
	if (!aff)
		return -1;

	var->def = isl_vec_alloc(s->ctx, 2 + v->n);
	if (!var->def) {
		isl_vec_free(aff);
		return -1;
	}

	isl_seq_cpy(var->def->el + 1, aff->el, aff->size);

	isl_vec_free(aff);

	if (fc) {
		if (isl_stream_eat(s, ','))
			return -1;
	} else {
		if (seen_paren && isl_stream_eat(s, ')'))
			return -1;
		if (isl_stream_eat(s, '/'))
			return -1;
	}

	tok = next_token(s);
	if (!tok)
		return -1;
	if (tok->type != ISL_TOKEN_VALUE) {
		isl_stream_error(s, tok, "expected denominator");
		isl_stream_push_token(s, tok);
		return -1;
	}
	isl_int_set(var->def->el[0], tok->u.v);
	isl_token_free(tok);

	if (fc) {
		if (isl_stream_eat(s, ')'))
			return -1;
	} else {
		if (isl_stream_eat(s, ']'))
			return -1;
	}

	return 0;
}

static struct isl_basic_map *add_div_definition(struct isl_stream *s,
	struct vars *v, struct isl_basic_map *bmap, int pos)
{
	struct variable *var = v->v;
	unsigned o_out = isl_basic_map_offset(bmap, isl_dim_out) - 1;

	if (read_div_definition(s, v) < 0)
		goto error;

	if (isl_basic_map_add_div_constraints_var(bmap, o_out + pos,
						  var->def->el) < 0)
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
		int p;
		int n = v->n;
		unsigned n_out = isl_basic_map_dim(bmap, isl_dim_out);

		if (tok->type != ISL_TOKEN_IDENT)
			break;

		p = vars_pos(v, tok->u.s, -1);
		if (p < 0)
			goto error;
		if (p < n) {
			isl_stream_error(s, tok, "expecting unique identifier");
			goto error;
		}

		bmap = isl_basic_map_cow(bmap);
		bmap = isl_basic_map_add(bmap, isl_dim_out, 1);
		bmap = isl_basic_map_extend_dim(bmap, isl_dim_copy(bmap->dim),
						0, 0, 2);

		isl_token_free(tok);
		tok = isl_stream_next_token(s);
		if (tok && tok->type == '=') {
			isl_token_free(tok);
			bmap = add_div_definition(s, v, bmap, n_out);
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

static int next_is_tuple(struct isl_stream *s)
{
	struct isl_token *tok;
	int is_tuple;

	tok = isl_stream_next_token(s);
	if (!tok)
		return 0;
	if (tok->type == '[') {
		isl_stream_push_token(s, tok);
		return 1;
	}
	if (tok->type != ISL_TOKEN_IDENT && !tok->is_keyword) {
		isl_stream_push_token(s, tok);
		return 0;
	}

	is_tuple = isl_stream_next_token_is(s, '[');

	isl_stream_push_token(s, tok);

	return is_tuple;
}

static __isl_give isl_basic_map *read_tuple(struct isl_stream *s,
	__isl_take isl_basic_map *bmap, enum isl_dim_type type, struct vars *v);

static __isl_give isl_basic_map *read_nested_tuple(struct isl_stream *s,
	__isl_take isl_basic_map *bmap, struct vars *v)
{
	bmap = read_tuple(s, bmap, isl_dim_in, v);
	if (isl_stream_eat(s, ISL_TOKEN_TO))
		goto error;
	bmap = read_tuple(s, bmap, isl_dim_out, v);
	bmap = isl_basic_map_from_range(isl_basic_map_wrap(bmap));
	return bmap;
error:
	isl_basic_map_free(bmap);
	return NULL;
}

static __isl_give isl_basic_map *read_tuple(struct isl_stream *s,
	__isl_take isl_basic_map *bmap, enum isl_dim_type type, struct vars *v)
{
	struct isl_token *tok;
	char *name = NULL;

	tok = isl_stream_next_token(s);
	if (tok && (tok->type == ISL_TOKEN_IDENT || tok->is_keyword)) {
		name = strdup(tok->u.s);
		if (!name)
			goto error;
		isl_token_free(tok);
		tok = isl_stream_next_token(s);
	}
	if (!tok || tok->type != '[') {
		isl_stream_error(s, tok, "expecting '['");
		goto error;
	}
	isl_token_free(tok);
	if (type != isl_dim_param && next_is_tuple(s)) {
		isl_dim *dim = isl_basic_map_get_dim(bmap);
		int nparam = isl_dim_size(dim, isl_dim_param);
		int n_in = isl_dim_size(dim, isl_dim_in);
		isl_basic_map *nested;
		if (type == isl_dim_out)
			dim = isl_dim_move(dim, isl_dim_param, nparam,
						isl_dim_in, 0, n_in);
		nested = isl_basic_map_alloc_dim(dim, 0, 0, 0);
		nested = read_nested_tuple(s, nested, v);
		if (type == isl_dim_in) {
			nested = isl_basic_map_reverse(nested);
			bmap = isl_basic_map_intersect(nested, bmap);
		} else {
			isl_basic_set *bset;
			dim = isl_dim_range(isl_basic_map_get_dim(nested));
			dim = isl_dim_drop(dim, isl_dim_param, nparam, n_in);
			dim = isl_dim_join(isl_basic_map_get_dim(bmap), dim);
			bset = isl_basic_map_domain(bmap);
			nested = isl_basic_map_reset_dim(nested, dim);
			bmap = isl_basic_map_intersect_domain(nested, bset);
		}
	} else
		bmap = read_var_list(s, bmap, type, v);
	tok = isl_stream_next_token(s);
	if (!tok || tok->type != ']') {
		isl_stream_error(s, tok, "expecting ']'");
		goto error;
	}
	isl_token_free(tok);

	if (name) {
		bmap = isl_basic_map_set_tuple_name(bmap, type, name);
		free(name);
	}

	return bmap;
error:
	if (tok)
		isl_token_free(tok);
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

/* Add any variables in the variable list "v" that are not already in "bmap"
 * as output variables in "bmap".
 */
static __isl_give isl_basic_map *add_lifted_divs(__isl_take isl_basic_map *bmap,
	struct vars *v)
{
	int i;
	int extra;
	struct variable *var;

	extra = v->n - isl_basic_map_total_dim(bmap);

	if (extra == 0)
		return bmap;

	bmap = isl_basic_map_add(bmap, isl_dim_out, extra);
	bmap = isl_basic_map_extend_dim(bmap, isl_basic_map_get_dim(bmap),
					0, 0, 2 * extra);

	for (i = 0, var = v->v; i < extra; ++i, var = var->next) {
		if (!var->def)
			continue;
		var->def = isl_vec_zero_extend(var->def, 2 + v->n);
		if (!var->def)
			goto error;
		if (isl_basic_map_add_div_constraints_var(bmap, var->pos,
							  var->def->el) < 0)
			goto error;
	}

	return bmap;
error:
	isl_basic_map_free(bmap);
	return NULL;
}

static struct isl_basic_map *add_constraint(struct isl_stream *s,
	struct vars *v, struct isl_basic_map *bmap)
{
	int i, j;
	struct isl_token *tok = NULL;
	struct isl_mat *aff1 = NULL, *aff2 = NULL;

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
	for (;;) {
		aff2 = accept_affine_list(s, v);
		if (!aff2)
			goto error;

		aff1 = isl_mat_add_zero_cols(aff1, aff2->n_col - aff1->n_col);
		if (!aff1)
			goto error;
		bmap = add_lifted_divs(bmap, v);
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

/* Return first variable, starting at n, representing a min or max,
 * or NULL if there is no such variable.
 */
static struct variable *first_minmax(struct vars *v, int n)
{
	struct variable *first = NULL;
	struct variable *var;

	for (var = v->v; var && var->pos >= n; var = var->next)
		if (var->list)
			first = var;

	return first;
}

/* Check whether the variable at the given position only occurs in
 * inequalities and only with the given sign.
 */
static int all_coefficients_of_sign(__isl_keep isl_map *map, int pos, int sign)
{
	int i, j;

	if (!map)
		return -1;

	for (i = 0; i < map->n; ++i) {
		isl_basic_map *bmap = map->p[i];

		for (j = 0; j < bmap->n_eq; ++j)
			if (!isl_int_is_zero(bmap->eq[j][1 + pos]))
				return 0;
		for (j = 0; j < bmap->n_ineq; ++j) {
			int s = isl_int_sgn(bmap->ineq[j][1 + pos]);
			if (s == 0)
				continue;
			if (s != sign)
				return 0;
		}
	}

	return 1;
}

/* Given a variable m which represents a min or a max of n expressions
 * b_i, add the constraints
 *
 *	m <= b_i
 *
 * in case of a min (var->sign < 0) and m >= b_i in case of a max.
 */
static __isl_give isl_map *bound_minmax(__isl_take isl_map *map,
	struct variable *var)
{
	int i, k;
	isl_basic_map *bound;
	int total;

	total = isl_map_dim(map, isl_dim_all);
	bound = isl_basic_map_alloc_dim(isl_map_get_dim(map),
					0, 0, var->list->n_row);

	for (i = 0; i < var->list->n_row; ++i) {
		k = isl_basic_map_alloc_inequality(bound);
		if (k < 0)
			goto error;
		if (var->sign < 0)
			isl_seq_cpy(bound->ineq[k], var->list->row[i],
				    var->list->n_col);
		else
			isl_seq_neg(bound->ineq[k], var->list->row[i],
				    var->list->n_col);
		isl_int_set_si(bound->ineq[k][1 + var->pos], var->sign);
		isl_seq_clr(bound->ineq[k] + var->list->n_col,
			    1 + total - var->list->n_col);
	}

	map = isl_map_intersect(map, isl_map_from_basic_map(bound));

	return map;
error:
	isl_basic_map_free(bound);
	isl_map_free(map);
	return NULL;
}

/* Given a variable m which represents a min (or max) of n expressions
 * b_i, add constraints that assigns the minimal upper bound to m, i.e.,
 * divide the space into cells where one
 * of the upper bounds is smaller than all the others and assign
 * this upper bound to m.
 *
 * In particular, if there are n bounds b_i, then the input map
 * is split into n pieces, each with the extra constraints
 *
 *	m = b_i
 *	b_i <= b_j	for j > i
 *	b_i <  b_j	for j < i
 *
 * in case of a min (var->sign < 0) and similarly in case of a max.
 *
 * Note: this function is very similar to set_minimum in isl_tab_pip.c
 * Perhaps we should try to merge the two.
 */
static __isl_give isl_map *set_minmax(__isl_take isl_map *map,
	struct variable *var)
{
	int i, j, k;
	isl_basic_map *bmap = NULL;
	isl_ctx *ctx;
	isl_map *split = NULL;
	int total;

	ctx = isl_map_get_ctx(map);
	total = isl_map_dim(map, isl_dim_all);
	split = isl_map_alloc_dim(isl_map_get_dim(map),
				var->list->n_row, ISL_SET_DISJOINT);

	for (i = 0; i < var->list->n_row; ++i) {
		bmap = isl_basic_map_alloc_dim(isl_map_get_dim(map), 0,
					       1, var->list->n_row - 1);
		k = isl_basic_map_alloc_equality(bmap);
		if (k < 0)
			goto error;
		isl_seq_cpy(bmap->eq[k], var->list->row[i], var->list->n_col);
		isl_int_set_si(bmap->eq[k][1 + var->pos], -1);
		for (j = 0; j < var->list->n_row; ++j) {
			if (j == i)
				continue;
			k = isl_basic_map_alloc_inequality(bmap);
			if (k < 0)
				goto error;
			if (var->sign < 0)
				isl_seq_combine(bmap->ineq[k],
						ctx->one, var->list->row[j],
						ctx->negone, var->list->row[i],
						var->list->n_col);
			else
				isl_seq_combine(bmap->ineq[k],
						ctx->negone, var->list->row[j],
						ctx->one, var->list->row[i],
						var->list->n_col);
			isl_seq_clr(bmap->ineq[k] + var->list->n_col,
				    1 + total - var->list->n_col);
			if (j < i)
				isl_int_sub_ui(bmap->ineq[k][0],
					       bmap->ineq[k][0], 1);
		}
		bmap = isl_basic_map_finalize(bmap);
		split = isl_map_add_basic_map(split, bmap);
	}

	map = isl_map_intersect(map, split);

	return map;
error:
	isl_basic_map_free(bmap);
	isl_map_free(split);
	isl_map_free(map);
	return NULL;
}

/* Plug in the definitions of all min and max expressions.
 * If a min expression only appears in inequalities and only
 * with a positive coefficient, then we can simply bound
 * the variable representing the min by its defining terms
 * and similarly for a max expression.
 * Otherwise, we have to assign the different terms to the
 * variable under the condition that the assigned term is smaller
 * than the other terms.
 */
static __isl_give isl_map *add_minmax(__isl_take isl_map *map,
	struct vars *v, int n)
{
	struct variable *var;

	while (n < v->n) {
		var = first_minmax(v, n);
		if (!var)
			break;
		if (all_coefficients_of_sign(map, var->pos, -var->sign))
			map = bound_minmax(map, var);
		else
			map = set_minmax(map, var);
		n = var->pos + 1;
	}

	return map;
}

static isl_map *read_constraint(struct isl_stream *s,
	struct vars *v, __isl_take isl_basic_map *bmap)
{
	int n = v->n;
	isl_map *map;
	unsigned total;

	if (!bmap)
		return NULL;

	bmap = isl_basic_set_unwrap(isl_basic_set_lift(isl_basic_map_wrap(bmap)));
	total = isl_basic_map_total_dim(bmap);
	while (v->n < total)
		if (vars_add_anon(v) < 0)
			goto error;

	bmap = add_constraint(s, v, bmap);
	bmap = isl_basic_map_simplify(bmap);
	bmap = isl_basic_map_finalize(bmap);

	map = isl_map_from_basic_map(bmap);

	map = add_minmax(map, v, n);

	map = isl_set_unwrap(isl_map_domain(map));

	vars_drop(v, v->n - n);

	return map;
error:
	isl_basic_map_free(bmap);
	return NULL;
}

static struct isl_map *read_disjuncts(struct isl_stream *s,
	struct vars *v, __isl_take isl_basic_map *bmap);

static __isl_give isl_map *read_exists(struct isl_stream *s,
	struct vars *v, __isl_take isl_basic_map *bmap)
{
	int n = v->n;
	int seen_paren = isl_stream_eat_if_available(s, '(');
	isl_map *map = NULL;

	bmap = isl_basic_map_from_domain(isl_basic_map_wrap(bmap));
	bmap = read_defined_var_list(s, v, bmap);

	if (isl_stream_eat(s, ':'))
		goto error;

	map = read_disjuncts(s, v, bmap);
	map = isl_set_unwrap(isl_map_domain(map));
	bmap = NULL;

	vars_drop(v, v->n - n);
	if (seen_paren && isl_stream_eat(s, ')'))
		goto error;

	return map;
error:
	isl_basic_map_free(bmap);
	isl_map_free(map);
	return NULL;
}

static __isl_give isl_map *read_conjunct(struct isl_stream *s,
	struct vars *v, __isl_take isl_basic_map *bmap)
{
	isl_map *map;

	if (isl_stream_eat_if_available(s, '(')) {
		map = read_disjuncts(s, v, bmap);
		if (isl_stream_eat(s, ')'))
			goto error;
		return map;
	}

	if (isl_stream_eat_if_available(s, ISL_TOKEN_EXISTS))
		return read_exists(s, v, bmap);

	if (isl_stream_eat_if_available(s, ISL_TOKEN_TRUE))
		return isl_map_from_basic_map(bmap);

	if (isl_stream_eat_if_available(s, ISL_TOKEN_FALSE)) {
		isl_dim *dim = isl_basic_map_get_dim(bmap);
		isl_basic_map_free(bmap);
		return isl_map_empty(dim);
	}
		
	return read_constraint(s, v, bmap);
error:
	isl_map_free(map);
	return NULL;
}

static __isl_give isl_map *read_conjuncts(struct isl_stream *s,
	struct vars *v, __isl_take isl_basic_map *bmap)
{
	isl_map *map;
	int negate;

	negate = isl_stream_eat_if_available(s, ISL_TOKEN_NOT);
	map = read_conjunct(s, v, isl_basic_map_copy(bmap));
	if (negate) {
		isl_map *t;
		t = isl_map_from_basic_map(isl_basic_map_copy(bmap));
		map = isl_map_subtract(t, map);
	}

	while (isl_stream_eat_if_available(s, ISL_TOKEN_AND)) {
		isl_map *map_i;

		negate = isl_stream_eat_if_available(s, ISL_TOKEN_NOT);
		map_i = read_conjunct(s, v, isl_basic_map_copy(bmap));
		if (negate)
			map = isl_map_subtract(map, map_i);
		else
			map = isl_map_intersect(map, map_i);
	}

	isl_basic_map_free(bmap);
	return map;
}

static struct isl_map *read_disjuncts(struct isl_stream *s,
	struct vars *v, __isl_take isl_basic_map *bmap)
{
	struct isl_map *map;

	if (isl_stream_next_token_is(s, '}')) {
		isl_dim *dim = isl_basic_map_get_dim(bmap);
		isl_basic_map_free(bmap);
		return isl_map_universe(dim);
	}

	map = read_conjuncts(s, v, isl_basic_map_copy(bmap));
	while (isl_stream_eat_if_available(s, ISL_TOKEN_OR)) {
		isl_map *map_i;

		map_i = read_conjuncts(s, v, isl_basic_map_copy(bmap));
		map = isl_map_union(map, map_i);
	}

	isl_basic_map_free(bmap);
	return map;
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
	if (tok->type != ISL_TOKEN_VALUE || tok2->type != ISL_TOKEN_VALUE) {
		isl_stream_push_token(s, tok2);
		isl_stream_push_token(s, tok);
		isl_stream_error(s, NULL,
				 "expecting constraint matrix dimensions");
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
		isl_seq_clr(bmap->div[k], 1 + 1 + nparam + in + out + local);
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
	if (tok2 && tok2->type == ISL_TOKEN_VALUE) {
		isl_stream_push_token(s, tok2);
		isl_stream_push_token(s, tok);
		return isl_map_from_basic_map(basic_map_read_polylib(s, nparam));
	}
	if (tok2) {
		isl_stream_error(s, tok2, "unexpected token");
		isl_stream_push_token(s, tok2);
		isl_stream_push_token(s, tok);
		return NULL;
	}
	n = isl_int_get_si(tok->u.v);
	isl_token_free(tok);

	isl_assert(s->ctx, n >= 1, return NULL);

	map = isl_map_from_basic_map(basic_map_read_polylib(s, nparam));

	for (i = 1; map && i < n; ++i)
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
	__isl_take isl_dim *dim, struct vars *v)
{
	int n;
	isl_basic_map *bmap;

	n = v->n;
	bmap = isl_basic_map_universe(dim);

	if (vars_add_anon(v) < 0)
		goto error;
	if (read_div_definition(s, v) < 0)
		goto error;
	bmap = add_divs(bmap, v);
	bmap = isl_basic_map_order_divs(bmap);
	if (!bmap)
		goto error;
	vars_drop(v, v->n - n);

	return isl_basic_map_div(bmap, bmap->n_div - 1);
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

	tok = next_token(s);
	if (!tok) {
		isl_stream_error(s, NULL, "unexpected EOF");
		return NULL;
	}
	if (tok->type == '(') {
		int pow;

		isl_token_free(tok);
		qp = read_term(s, bmap, v);
		if (!qp)
			return NULL;
		if (isl_stream_eat(s, ')'))
			goto error;
		pow = optional_power(s);
		qp = isl_qpolynomial_pow(qp, pow);
	} else if (tok->type == ISL_TOKEN_VALUE) {
		struct isl_token *tok2;
		tok2 = isl_stream_next_token(s);
		if (tok2 && tok2->type == '/') {
			isl_token_free(tok2);
			tok2 = next_token(s);
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
		if (pos < 0) {
			isl_token_free(tok);
			return NULL;
		}
		if (pos >= n) {
			vars_drop(v, v->n - n);
			isl_stream_error(s, tok, "unknown identifier");
			isl_token_free(tok);
			return NULL;
		}
		isl_token_free(tok);
		pow = optional_power(s);
		qp = isl_qpolynomial_var_pow(isl_basic_map_get_dim(bmap), pos, pow);
	} else if (tok->type == '[') {
		isl_div *div;
		int pow;

		isl_stream_push_token(s, tok);
		div = read_div(s, isl_basic_map_get_dim(bmap), v);
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
		tok = next_token(s);
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

			isl_stream_push_token(s, tok);
			qp2 = read_factor(s, bmap, v);
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
	if (tok->type == ':' ||
	    (tok->type == ISL_TOKEN_OR && !strcmp(tok->u.s, "|"))) {
		isl_token_free(tok);
		map = isl_map_intersect(map,
			    read_disjuncts(s, v, isl_basic_map_copy(bmap)));
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

	qp = read_term(s, bmap, v);
	map = read_optional_disjuncts(s, bmap, v);
	set = isl_map_range(map);

	pwqp = isl_pw_qpolynomial_alloc(set, qp);

	vars_drop(v, v->n - n);

	obj.v = pwqp;
	return obj;
}

static struct isl_obj obj_read_poly_or_fold(struct isl_stream *s,
	__isl_take isl_basic_map *bmap, struct vars *v, int n)
{
	struct isl_obj obj = { isl_obj_pw_qpolynomial_fold, NULL };
	isl_qpolynomial *qp;
	isl_qpolynomial_fold *fold = NULL;
	isl_pw_qpolynomial_fold *pwf;
	isl_map *map;
	isl_set *set;

	if (!isl_stream_eat_if_available(s, ISL_TOKEN_MAX))
		return obj_read_poly(s, bmap, v, n);

	if (isl_stream_eat(s, '('))
		goto error;

	qp = read_term(s, bmap, v);
	fold = isl_qpolynomial_fold_alloc(isl_fold_max, qp);

	while (isl_stream_eat_if_available(s, ',')) {
		isl_qpolynomial_fold *fold_i;
		qp = read_term(s, bmap, v);
		fold_i = isl_qpolynomial_fold_alloc(isl_fold_max, qp);
		fold = isl_qpolynomial_fold_fold(fold, fold_i);
	}

	if (isl_stream_eat(s, ')'))
		goto error;

	map = read_optional_disjuncts(s, bmap, v);
	set = isl_map_range(map);
	pwf = isl_pw_qpolynomial_fold_alloc(isl_fold_max, set, fold);

	vars_drop(v, v->n - n);

	obj.v = pwf;
	return obj;
error:
	isl_basic_map_free(bmap);
	isl_qpolynomial_fold_free(fold);
	obj.type = isl_obj_none;
	return obj;
}

static int is_rational(struct isl_stream *s)
{
	struct isl_token *tok;

	tok = isl_stream_next_token(s);
	if (!tok)
		return 0;
	if (tok->type == ISL_TOKEN_RAT && isl_stream_next_token_is(s, ':')) {
		isl_token_free(tok);
		isl_stream_eat(s, ':');
		return 1;
	}

	isl_stream_push_token(s, tok);

	return 0;
}

static struct isl_obj obj_read_body(struct isl_stream *s,
	__isl_take isl_basic_map *bmap, struct vars *v)
{
	struct isl_map *map = NULL;
	struct isl_token *tok;
	struct isl_obj obj = { isl_obj_set, NULL };
	int n = v->n;

	if (is_rational(s))
		bmap = isl_basic_map_set_rational(bmap);

	if (!next_is_tuple(s))
		return obj_read_poly_or_fold(s, bmap, v, n);

	bmap = read_tuple(s, bmap, isl_dim_in, v);
	if (!bmap)
		goto error;
	tok = isl_stream_next_token(s);
	if (tok && tok->type == ISL_TOKEN_TO) {
		obj.type = isl_obj_map;
		isl_token_free(tok);
		if (!next_is_tuple(s)) {
			bmap = isl_basic_map_reverse(bmap);
			return obj_read_poly_or_fold(s, bmap, v, n);
		}
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

static struct isl_obj to_union(isl_ctx *ctx, struct isl_obj obj)
{
	if (obj.type == isl_obj_map) {
		obj.v = isl_union_map_from_map(obj.v);
		obj.type = isl_obj_union_map;
	} else if (obj.type == isl_obj_set) {
		obj.v = isl_union_set_from_set(obj.v);
		obj.type = isl_obj_union_set;
	} else if (obj.type == isl_obj_pw_qpolynomial) {
		obj.v = isl_union_pw_qpolynomial_from_pw_qpolynomial(obj.v);
		obj.type = isl_obj_union_pw_qpolynomial;
	} else if (obj.type == isl_obj_pw_qpolynomial_fold) {
		obj.v = isl_union_pw_qpolynomial_fold_from_pw_qpolynomial_fold(obj.v);
		obj.type = isl_obj_union_pw_qpolynomial_fold;
	} else
		isl_assert(ctx, 0, goto error);
	return obj;
error:
	obj.type->free(obj.v);
	obj.type = isl_obj_none;
	return obj;
}

static struct isl_obj obj_add(struct isl_ctx *ctx,
	struct isl_obj obj1, struct isl_obj obj2)
{
	if (obj1.type == isl_obj_set && obj2.type == isl_obj_union_set)
		obj1 = to_union(ctx, obj1);
	if (obj1.type == isl_obj_union_set && obj2.type == isl_obj_set)
		obj2 = to_union(ctx, obj2);
	if (obj1.type == isl_obj_map && obj2.type == isl_obj_union_map)
		obj1 = to_union(ctx, obj1);
	if (obj1.type == isl_obj_union_map && obj2.type == isl_obj_map)
		obj2 = to_union(ctx, obj2);
	if (obj1.type == isl_obj_pw_qpolynomial &&
	    obj2.type == isl_obj_union_pw_qpolynomial)
		obj1 = to_union(ctx, obj1);
	if (obj1.type == isl_obj_union_pw_qpolynomial &&
	    obj2.type == isl_obj_pw_qpolynomial)
		obj2 = to_union(ctx, obj2);
	if (obj1.type == isl_obj_pw_qpolynomial_fold &&
	    obj2.type == isl_obj_union_pw_qpolynomial_fold)
		obj1 = to_union(ctx, obj1);
	if (obj1.type == isl_obj_union_pw_qpolynomial_fold &&
	    obj2.type == isl_obj_pw_qpolynomial_fold)
		obj2 = to_union(ctx, obj2);
	isl_assert(ctx, obj1.type == obj2.type, goto error);
	if (obj1.type == isl_obj_map && !isl_map_has_equal_dim(obj1.v, obj2.v)) {
		obj1 = to_union(ctx, obj1);
		obj2 = to_union(ctx, obj2);
	}
	if (obj1.type == isl_obj_set && !isl_set_has_equal_dim(obj1.v, obj2.v)) {
		obj1 = to_union(ctx, obj1);
		obj2 = to_union(ctx, obj2);
	}
	if (obj1.type == isl_obj_pw_qpolynomial &&
	    !isl_pw_qpolynomial_has_equal_dim(obj1.v, obj2.v)) {
		obj1 = to_union(ctx, obj1);
		obj2 = to_union(ctx, obj2);
	}
	if (obj1.type == isl_obj_pw_qpolynomial_fold &&
	    !isl_pw_qpolynomial_fold_has_equal_dim(obj1.v, obj2.v)) {
		obj1 = to_union(ctx, obj1);
		obj2 = to_union(ctx, obj2);
	}
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
	isl_basic_map *bmap = NULL;
	struct isl_token *tok;
	struct vars *v = NULL;
	struct isl_obj obj = { isl_obj_set, NULL };

	tok = next_token(s);
	if (!tok) {
		isl_stream_error(s, NULL, "unexpected EOF");
		goto error;
	}
	if (tok->type == ISL_TOKEN_VALUE) {
		struct isl_token *tok2;
		struct isl_map *map;

		tok2 = isl_stream_next_token(s);
		if (!tok2 || tok2->type != ISL_TOKEN_VALUE ||
		    isl_int_is_neg(tok2->u.v)) {
			if (tok2)
				isl_stream_push_token(s, tok2);
			obj.type = isl_obj_int;
			obj.v = isl_int_obj_alloc(s->ctx, tok->u.v);
			isl_token_free(tok);
			return obj;
		}
		isl_stream_push_token(s, tok2);
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
	if (!v) {
		isl_stream_push_token(s, tok);
		goto error;
	}
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
	} else if (nparam > 0)
		bmap = isl_basic_map_add(bmap, isl_dim_param, nparam);
	if (!tok || tok->type != '{') {
		isl_stream_error(s, tok, "expecting '{'");
		if (tok)
			isl_stream_push_token(s, tok);
		goto error;
	}
	isl_token_free(tok);

	tok = isl_stream_next_token(s);
	if (!tok)
		;
	else if (tok->type == ISL_TOKEN_IDENT && !strcmp(tok->u.s, "Sym")) {
		isl_token_free(tok);
		if (isl_stream_eat(s, '='))
			goto error;
		bmap = read_tuple(s, bmap, isl_dim_param, v);
		if (!bmap)
			goto error;
		if (nparam >= 0)
			isl_assert(s->ctx, nparam == v->n, goto error);
	} else if (tok->type == '}') {
		obj.type = isl_obj_union_set;
		obj.v = isl_union_set_empty(isl_basic_map_get_dim(bmap));
		isl_token_free(tok);
		goto done;
	} else
		isl_stream_push_token(s, tok);

	for (;;) {
		struct isl_obj o;
		tok = NULL;
		o = obj_read_body(s, isl_basic_map_copy(bmap), v);
		if (o.type == isl_obj_none || !o.v)
			goto error;
		if (!obj.v)
			obj = o;
		else {
			obj = obj_add(s->ctx, obj, o);
			if (obj.type == isl_obj_none || !obj.v)
				goto error;
		}
		tok = isl_stream_next_token(s);
		if (!tok || tok->type != ';')
			break;
		isl_token_free(tok);
		if (isl_stream_next_token_is(s, '}')) {
			tok = isl_stream_next_token(s);
			break;
		}
	}

	if (tok && tok->type == '}') {
		isl_token_free(tok);
	} else {
		isl_stream_error(s, tok, "unexpected isl_token");
		if (tok)
			isl_token_free(tok);
		goto error;
	}
done:
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

__isl_give isl_map *isl_stream_read_map(struct isl_stream *s, int nparam)
{
	struct isl_obj obj;

	obj = obj_read(s, nparam);
	if (obj.v)
		isl_assert(s->ctx, obj.type == isl_obj_map ||
				   obj.type == isl_obj_set, goto error);

	return obj.v;
error:
	obj.type->free(obj.v);
	return NULL;
}

__isl_give isl_set *isl_stream_read_set(struct isl_stream *s, int nparam)
{
	struct isl_obj obj;

	obj = obj_read(s, nparam);
	if (obj.v)
		isl_assert(s->ctx, obj.type == isl_obj_set, goto error);

	return obj.v;
error:
	obj.type->free(obj.v);
	return NULL;
}

__isl_give isl_union_map *isl_stream_read_union_map(struct isl_stream *s)
{
	struct isl_obj obj;

	obj = obj_read(s, -1);
	if (obj.type == isl_obj_map) {
		obj.type = isl_obj_union_map;
		obj.v = isl_union_map_from_map(obj.v);
	}
	if (obj.type == isl_obj_set) {
		obj.type = isl_obj_union_set;
		obj.v = isl_union_set_from_set(obj.v);
	}
	if (obj.v)
		isl_assert(s->ctx, obj.type == isl_obj_union_map ||
				   obj.type == isl_obj_union_set, goto error);

	return obj.v;
error:
	obj.type->free(obj.v);
	return NULL;
}

__isl_give isl_union_set *isl_stream_read_union_set(struct isl_stream *s)
{
	struct isl_obj obj;

	obj = obj_read(s, -1);
	if (obj.type == isl_obj_set) {
		obj.type = isl_obj_union_set;
		obj.v = isl_union_set_from_set(obj.v);
	}
	if (obj.v)
		isl_assert(s->ctx, obj.type == isl_obj_union_set, goto error);

	return obj.v;
error:
	obj.type->free(obj.v);
	return NULL;
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
	struct isl_map *map;
	struct isl_stream *s = isl_stream_new_file(ctx, input);
	if (!s)
		return NULL;
	map = isl_stream_read_map(s, nparam);
	isl_stream_free(s);
	return map;
}

__isl_give isl_map *isl_map_read_from_str(struct isl_ctx *ctx,
		const char *str, int nparam)
{
	struct isl_map *map;
	struct isl_stream *s = isl_stream_new_str(ctx, str);
	if (!s)
		return NULL;
	map = isl_stream_read_map(s, nparam);
	isl_stream_free(s);
	return map;
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

__isl_give isl_union_map *isl_union_map_read_from_file(isl_ctx *ctx,
	FILE *input)
{
	isl_union_map *umap;
	struct isl_stream *s = isl_stream_new_file(ctx, input);
	if (!s)
		return NULL;
	umap = isl_stream_read_union_map(s);
	isl_stream_free(s);
	return umap;
}

__isl_give isl_union_map *isl_union_map_read_from_str(struct isl_ctx *ctx,
		const char *str)
{
	isl_union_map *umap;
	struct isl_stream *s = isl_stream_new_str(ctx, str);
	if (!s)
		return NULL;
	umap = isl_stream_read_union_map(s);
	isl_stream_free(s);
	return umap;
}

__isl_give isl_union_set *isl_union_set_read_from_file(isl_ctx *ctx,
	FILE *input)
{
	isl_union_set *uset;
	struct isl_stream *s = isl_stream_new_file(ctx, input);
	if (!s)
		return NULL;
	uset = isl_stream_read_union_set(s);
	isl_stream_free(s);
	return uset;
}

__isl_give isl_union_set *isl_union_set_read_from_str(struct isl_ctx *ctx,
		const char *str)
{
	isl_union_set *uset;
	struct isl_stream *s = isl_stream_new_str(ctx, str);
	if (!s)
		return NULL;
	uset = isl_stream_read_union_set(s);
	isl_stream_free(s);
	return uset;
}

static __isl_give isl_vec *isl_vec_read_polylib(struct isl_stream *s)
{
	struct isl_vec *vec = NULL;
	struct isl_token *tok;
	unsigned size;
	int j;

	tok = isl_stream_next_token(s);
	if (!tok || tok->type != ISL_TOKEN_VALUE) {
		isl_stream_error(s, tok, "expecting vector length");
		goto error;
	}

	size = isl_int_get_si(tok->u.v);
	isl_token_free(tok);

	vec = isl_vec_alloc(s->ctx, size);

	for (j = 0; j < size; ++j) {
		tok = isl_stream_next_token(s);
		if (!tok || tok->type != ISL_TOKEN_VALUE) {
			isl_stream_error(s, tok, "expecting constant value");
			goto error;
		}
		isl_int_set(vec->el[j], tok->u.v);
		isl_token_free(tok);
	}

	return vec;
error:
	isl_token_free(tok);
	isl_vec_free(vec);
	return NULL;
}

static __isl_give isl_vec *vec_read(struct isl_stream *s)
{
	return isl_vec_read_polylib(s);
}

__isl_give isl_vec *isl_vec_read_from_file(isl_ctx *ctx, FILE *input)
{
	isl_vec *v;
	struct isl_stream *s = isl_stream_new_file(ctx, input);
	if (!s)
		return NULL;
	v = vec_read(s);
	isl_stream_free(s);
	return v;
}

__isl_give isl_pw_qpolynomial *isl_stream_read_pw_qpolynomial(
	struct isl_stream *s)
{
	struct isl_obj obj;

	obj = obj_read(s, -1);
	if (obj.v)
		isl_assert(s->ctx, obj.type == isl_obj_pw_qpolynomial,
			   goto error);

	return obj.v;
error:
	obj.type->free(obj.v);
	return NULL;
}

__isl_give isl_pw_qpolynomial *isl_pw_qpolynomial_read_from_str(isl_ctx *ctx,
		const char *str)
{
	isl_pw_qpolynomial *pwqp;
	struct isl_stream *s = isl_stream_new_str(ctx, str);
	if (!s)
		return NULL;
	pwqp = isl_stream_read_pw_qpolynomial(s);
	isl_stream_free(s);
	return pwqp;
}

__isl_give isl_pw_qpolynomial *isl_pw_qpolynomial_read_from_file(isl_ctx *ctx,
		FILE *input)
{
	isl_pw_qpolynomial *pwqp;
	struct isl_stream *s = isl_stream_new_file(ctx, input);
	if (!s)
		return NULL;
	pwqp = isl_stream_read_pw_qpolynomial(s);
	isl_stream_free(s);
	return pwqp;
}
