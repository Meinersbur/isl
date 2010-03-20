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

#include <string.h>
#include <isl_set.h>
#include <isl_seq.h>
#include <isl_polynomial_private.h>
#include <isl_printer_private.h>

static __isl_give isl_printer *print_constraint_polylib(
	struct isl_basic_map *bmap, int ineq, int n, __isl_take isl_printer *p)
{
	int i;
	unsigned n_in = isl_basic_map_dim(bmap, isl_dim_in);
	unsigned n_out = isl_basic_map_dim(bmap, isl_dim_out);
	unsigned nparam = isl_basic_map_dim(bmap, isl_dim_param);
	isl_int *c = ineq ? bmap->ineq[n] : bmap->eq[n];

	p = isl_printer_start_line(p);
	p = isl_printer_print_int(p, ineq);
	for (i = 0; i < n_out; ++i) {
		p = isl_printer_print_str(p, " ");
		p = isl_printer_print_isl_int(p, c[1+nparam+n_in+i]);
	}
	for (i = 0; i < n_in; ++i) {
		p = isl_printer_print_str(p, " ");
		p = isl_printer_print_isl_int(p, c[1+nparam+i]);
	}
	for (i = 0; i < bmap->n_div; ++i) {
		p = isl_printer_print_str(p, " ");
		p = isl_printer_print_isl_int(p, c[1+nparam+n_in+n_out+i]);
	}
	for (i = 0; i < nparam; ++i) {
		p = isl_printer_print_str(p, " ");
		p = isl_printer_print_isl_int(p, c[1+i]);
	}
	p = isl_printer_print_str(p, " ");
	p = isl_printer_print_isl_int(p, c[0]);
	p = isl_printer_end_line(p);
	return p;
}

static __isl_give isl_printer *print_constraints_polylib(
	struct isl_basic_map *bmap, __isl_take isl_printer *p)
{
	int i;

	p = isl_printer_set_isl_int_width(p, 5);

	for (i = 0; i < bmap->n_eq; ++i)
		p = print_constraint_polylib(bmap, 0, i, p);
	for (i = 0; i < bmap->n_ineq; ++i)
		p = print_constraint_polylib(bmap, 1, i, p);

	return p;
}

static __isl_give isl_printer *bset_print_constraints_polylib(
	struct isl_basic_set *bset, __isl_take isl_printer *p)
{
	return print_constraints_polylib((struct isl_basic_map *)bset, p);
}

static __isl_give isl_printer *isl_basic_map_print_polylib(
	__isl_keep isl_basic_map *bmap, __isl_take isl_printer *p)
{
	unsigned total = isl_basic_map_total_dim(bmap);
	p = isl_printer_start_line(p);
	p = isl_printer_print_int(p, bmap->n_eq + bmap->n_ineq);
	p = isl_printer_print_str(p, " ");
	p = isl_printer_print_int(p, 1 + total + 1);
	p = isl_printer_end_line(p);
	return print_constraints_polylib(bmap, p);
}

static __isl_give isl_printer *isl_basic_set_print_polylib(
	__isl_keep isl_basic_set *bset, __isl_take isl_printer *p)
{
	return isl_basic_map_print_polylib((struct isl_basic_map *)bset, p);
}

static __isl_give isl_printer *isl_map_print_polylib(__isl_keep isl_map *map,
	__isl_take isl_printer *p)
{
	int i;

	p = isl_printer_start_line(p);
	p = isl_printer_print_int(p, map->n);
	p = isl_printer_end_line(p);
	for (i = 0; i < map->n; ++i) {
		p = isl_printer_start_line(p);
		p = isl_printer_end_line(p);
		p = isl_basic_map_print_polylib(map->p[i], p);
	}
	return p;
}

static __isl_give isl_printer *isl_set_print_polylib(__isl_keep isl_set *set,
	__isl_take isl_printer *p)
{
	return isl_map_print_polylib((struct isl_map *)set, p);
}

static int count_same_name(__isl_keep isl_dim *dim,
	enum isl_dim_type type, unsigned pos, const char *name)
{
	enum isl_dim_type t;
	unsigned p, s;
	int count = 0;

	for (t = isl_dim_param; t <= type && t <= isl_dim_out; ++t) {
		s = t == type ? pos : isl_dim_size(dim, t);
		for (p = 0; p < s; ++p) {
			const char *n = isl_dim_get_name(dim, t, p);
			if (n && !strcmp(n, name))
				count++;
		}
	}
	return count;
}

static __isl_give isl_printer *print_name(__isl_keep isl_dim *dim,
	__isl_take isl_printer *p, enum isl_dim_type type, unsigned pos, int set)
{
	const char *name;
	char buffer[20];
	int primes;

	name = type == isl_dim_div ? NULL : isl_dim_get_name(dim, type, pos);

	if (!name) {
		const char *prefix;
		if (type == isl_dim_param)
			prefix = "p";
		else if (type == isl_dim_div)
			prefix = "e";
		else if (set || type == isl_dim_in)
			prefix = "i";
		else
			prefix = "o";
		snprintf(buffer, sizeof(buffer), "%s%d", prefix, pos);
		name = buffer;
	}
	primes = count_same_name(dim, name == buffer ? isl_dim_div : type,
				 pos, name);
	p = isl_printer_print_str(p, name);
	while (primes-- > 0)
		p = isl_printer_print_str(p, "'");
	return p;
}

static __isl_give isl_printer *print_var_list(__isl_keep isl_dim *dim,
	__isl_take isl_printer *p, enum isl_dim_type type, int set)
{
	int i;

	for (i = 0; i < isl_dim_size(dim, type); ++i) {
		if (i)
			p = isl_printer_print_str(p, ", ");
		p = print_name(dim, p, type, i, set);
	}
	return p;
}

static __isl_give isl_printer *print_tuple(__isl_keep isl_dim *dim,
	__isl_take isl_printer *p, enum isl_dim_type type, int set)
{
	p = isl_printer_print_str(p, "[");
	p = print_var_list(dim, p, type, set);
	p = isl_printer_print_str(p, "]");
	return p;
}

static __isl_give isl_printer *print_omega_parameters(__isl_keep isl_dim *dim,
	__isl_take isl_printer *p)
{
	if (isl_dim_size(dim, isl_dim_param) == 0)
		return p;

	p = isl_printer_start_line(p);
	p = isl_printer_print_str(p, "symbolic ");
	p = print_var_list(dim, p, isl_dim_param, 0);
	p = isl_printer_print_str(p, ";");
	p = isl_printer_end_line(p);
	return p;
}

static enum isl_dim_type pos2type(__isl_keep isl_dim *dim, unsigned *pos)
{
	enum isl_dim_type type;
	unsigned n_in = isl_dim_size(dim, isl_dim_in);
	unsigned n_out = isl_dim_size(dim, isl_dim_out);
	unsigned nparam = isl_dim_size(dim, isl_dim_param);

	if (*pos < 1 + nparam) {
		type = isl_dim_param;
		*pos -= 1;
	} else if (*pos < 1 + nparam + n_in) {
		type = isl_dim_in;
		*pos -= 1 + nparam;
	} else if (*pos < 1 + nparam + n_in + n_out) {
		type = isl_dim_out;
		*pos -= 1 + nparam + n_in;
	} else {
		type = isl_dim_div;
		*pos -= 1 + nparam + n_in + n_out;
	}

	return type;
}

static __isl_give isl_printer *print_term(__isl_keep isl_dim *dim,
			isl_int c, unsigned pos, __isl_take isl_printer *p, int set)
{
	enum isl_dim_type type;

	if (pos == 0)
		return isl_printer_print_isl_int(p, c);

	if (isl_int_is_one(c))
		;
	else if (isl_int_is_negone(c))
		p = isl_printer_print_str(p, "-");
	else
		p = isl_printer_print_isl_int(p, c);
	type = pos2type(dim, &pos);
	p = print_name(dim, p, type, pos, set);
	return p;
}

static __isl_give isl_printer *print_affine_of_len(__isl_keep isl_dim *dim,
	__isl_take isl_printer *p, isl_int *c, int len, int set)
{
	int i;
	int first;

	for (i = 0, first = 1; i < len; ++i) {
		int flip = 0;
		if (isl_int_is_zero(c[i]))
			continue;
		if (!first) {
			if (isl_int_is_neg(c[i])) {
				flip = 1;
				isl_int_neg(c[i], c[i]);
				p = isl_printer_print_str(p, " - ");
			} else 
				p = isl_printer_print_str(p, " + ");
		}
		first = 0;
		p = print_term(dim, c[i], i, p, set);
		if (flip)
			isl_int_neg(c[i], c[i]);
	}
	if (first)
		p = isl_printer_print_str(p, "0");
	return p;
}

static __isl_give isl_printer *print_affine(__isl_keep isl_basic_map *bmap,
	__isl_keep isl_dim *dim, __isl_take isl_printer *p, isl_int *c, int set)
{
	unsigned len = 1 + isl_basic_map_total_dim(bmap);
	return print_affine_of_len(dim, p, c, len, set);
}

static __isl_give isl_printer *print_constraint(struct isl_basic_map *bmap,
	__isl_keep isl_dim *dim, __isl_take isl_printer *p,
	isl_int *c, int last, const char *op, int first_constraint, int set)
{
	if (!first_constraint)
		p = isl_printer_print_str(p, " and ");

	isl_int_abs(c[last], c[last]);

	p = print_term(dim, c[last], last, p, set);

	p = isl_printer_print_str(p, " ");
	p = isl_printer_print_str(p, op);
	p = isl_printer_print_str(p, " ");

	isl_int_set_si(c[last], 0);
	p = print_affine(bmap, dim, p, c, set);

	return p;
}

static __isl_give isl_printer *print_constraints(__isl_keep isl_basic_map *bmap,
	__isl_keep isl_dim *dim, __isl_take isl_printer *p, int set)
{
	int i;
	struct isl_vec *c;
	unsigned total = isl_basic_map_total_dim(bmap);

	c = isl_vec_alloc(bmap->ctx, 1 + total);
	if (!c)
		goto error;

	for (i = bmap->n_eq - 1; i >= 0; --i) {
		int l = isl_seq_last_non_zero(bmap->eq[i], 1 + total);
		isl_assert(bmap->ctx, l >= 0, goto error);
		if (isl_int_is_neg(bmap->eq[i][l]))
			isl_seq_cpy(c->el, bmap->eq[i], 1 + total);
		else
			isl_seq_neg(c->el, bmap->eq[i], 1 + total);
		p = print_constraint(bmap, dim, p, c->el, l,
				    "=", i == bmap->n_eq - 1, set);
	}
	for (i = 0; i < bmap->n_ineq; ++i) {
		int l = isl_seq_last_non_zero(bmap->ineq[i], 1 + total);
		int s;
		isl_assert(bmap->ctx, l >= 0, goto error);
		s = isl_int_sgn(bmap->ineq[i][l]);
		if (s < 0)
			isl_seq_cpy(c->el, bmap->ineq[i], 1 + total);
		else
			isl_seq_neg(c->el, bmap->ineq[i], 1 + total);
		p = print_constraint(bmap, dim, p, c->el, l,
				    s < 0 ? "<=" : ">=", !bmap->n_eq && !i, set);
	}

	isl_vec_free(c);

	return p;
error:
	isl_printer_free(p);
	return NULL;
}

static __isl_give isl_printer *print_omega_constraints(
	__isl_keep isl_basic_map *bmap, __isl_take isl_printer *p, int set)
{
	if (bmap->n_eq + bmap->n_ineq == 0)
		return p;

	p = isl_printer_print_str(p, ": ");
	if (bmap->n_div > 0) {
		int i;
		p = isl_printer_print_str(p, "exists (");
		for (i = 0; i < bmap->n_div; ++i) {
			if (i)
				p = isl_printer_print_str(p, ", ");
			p = print_name(bmap->dim, p, isl_dim_div, i, 0);
		}
		p = isl_printer_print_str(p, ": ");
	}
	p = print_constraints(bmap, bmap->dim, p, set);
	if (bmap->n_div > 0)
		p = isl_printer_print_str(p, ")");
	return p;
}

static __isl_give isl_printer *basic_map_print_omega(
	__isl_keep isl_basic_map *bmap, __isl_take isl_printer *p)
{
	p = isl_printer_print_str(p, "{ [");
	p = print_var_list(bmap->dim, p, isl_dim_in, 0);
	p = isl_printer_print_str(p, "] -> [");
	p = print_var_list(bmap->dim, p, isl_dim_out, 0);
	p = isl_printer_print_str(p, "] ");
	p = print_omega_constraints(bmap, p, 0);
	p = isl_printer_print_str(p, " }");
	return p;
}

static __isl_give isl_printer *isl_basic_map_print_omega(
	__isl_keep isl_basic_map *bmap, __isl_take isl_printer *p)
{
	p = print_omega_parameters(bmap->dim, p);

	p = isl_printer_start_line(p);
	p = basic_map_print_omega(bmap, p);
	p = isl_printer_end_line(p);
	return p;
}

static __isl_give isl_printer *basic_set_print_omega(
	__isl_keep isl_basic_set *bset, __isl_take isl_printer *p)
{
	p = isl_printer_print_str(p, "{ [");
	p = print_var_list(bset->dim, p, isl_dim_set, 1);
	p = isl_printer_print_str(p, "] ");
	p = print_omega_constraints((isl_basic_map *)bset, p, 1);
	p = isl_printer_print_str(p, " }");
	return p;
}

static __isl_give isl_printer *isl_basic_set_print_omega(
	__isl_keep isl_basic_set *bset, __isl_take isl_printer *p)
{
	p = print_omega_parameters(bset->dim, p);

	p = isl_printer_start_line(p);
	p = basic_set_print_omega(bset, p);
	p = isl_printer_end_line(p);
	return p;
}

static __isl_give isl_printer *isl_map_print_omega(__isl_keep isl_map *map,
	__isl_take isl_printer *p)
{
	int i;

	p = print_omega_parameters(map->dim, p);

	p = isl_printer_start_line(p);
	for (i = 0; i < map->n; ++i) {
		if (i)
			p = isl_printer_print_str(p, " union ");
		p = basic_map_print_omega(map->p[i], p);
	}
	p = isl_printer_end_line(p);
	return p;
}

static __isl_give isl_printer *isl_set_print_omega(__isl_keep isl_set *set,
	__isl_take isl_printer *p)
{
	int i;

	p = print_omega_parameters(set->dim, p);

	p = isl_printer_start_line(p);
	for (i = 0; i < set->n; ++i) {
		if (i)
			p = isl_printer_print_str(p, " union ");
		p = basic_set_print_omega(set->p[i], p);
	}
	p = isl_printer_end_line(p);
	return p;
}

static __isl_give isl_printer *print_disjunct(__isl_keep isl_basic_map *bmap,
	__isl_keep isl_dim *dim, __isl_take isl_printer *p, int set)
{
	if (bmap->n_div > 0) {
		int i;
		p = isl_printer_print_str(p, "exists (");
		for (i = 0; i < bmap->n_div; ++i) {
			if (i)
				p = isl_printer_print_str(p, ", ");
			p = print_name(dim, p, isl_dim_div, i, 0);
			if (isl_int_is_zero(bmap->div[i][0]))
				continue;
			p = isl_printer_print_str(p, " = [(");
			p = print_affine(bmap, dim, p, bmap->div[i] + 1, set);
			p = isl_printer_print_str(p, ")/");
			p = isl_printer_print_isl_int(p, bmap->div[i][0]);
			p = isl_printer_print_str(p, "]");
		}
		p = isl_printer_print_str(p, ": ");
	}

	p = print_constraints(bmap, dim, p, set);

	if (bmap->n_div > 0)
		p = isl_printer_print_str(p, ")");
	return p;
}

static __isl_give isl_printer *isl_basic_map_print_isl(
	__isl_keep isl_basic_map *bmap, __isl_take isl_printer *p)
{
	int i;

	p = isl_printer_start_line(p);
	if (isl_basic_map_dim(bmap, isl_dim_param) > 0) {
		p = print_tuple(bmap->dim, p, isl_dim_param, 0);
		p = isl_printer_print_str(p, " -> ");
	}
	p = isl_printer_print_str(p, "{ ");
	p = print_tuple(bmap->dim, p, isl_dim_in, 0);
	p = isl_printer_print_str(p, " -> ");
	p = print_tuple(bmap->dim, p, isl_dim_out, 0);
	p = isl_printer_print_str(p, " : ");
	p = print_disjunct(bmap, bmap->dim, p, 0);
	p = isl_printer_end_line(p);
	return p;
}

static __isl_give isl_printer *isl_basic_set_print_isl(
	__isl_keep isl_basic_set *bset, __isl_take isl_printer *p)
{
	int i;

	p = isl_printer_start_line(p);
	if (isl_basic_set_dim(bset, isl_dim_param) > 0) {
		p = print_tuple(bset->dim, p, isl_dim_param, 0);
		p = isl_printer_print_str(p, " -> ");
	}
	p = isl_printer_print_str(p, "{ ");
	p = print_tuple(bset->dim, p, isl_dim_set, 1);
	p = isl_printer_print_str(p, " : ");
	p = print_disjunct((isl_basic_map *)bset, bset->dim, p, 1);
	p = isl_printer_end_line(p);
	return p;
}

static __isl_give isl_printer *print_disjuncts(__isl_keep isl_map *map,
	__isl_take isl_printer *p, int set)
{
	int i;

	if (isl_map_fast_is_universe(map))
		return p;

	p = isl_printer_print_str(p, " : ");
	if (map->n == 0)
		p = isl_printer_print_str(p, "1 = 0");
	for (i = 0; i < map->n; ++i) {
		if (i)
			p = isl_printer_print_str(p, " or ");
		if (map->n > 1 && map->p[i]->n_eq + map->p[i]->n_ineq > 1)
			p = isl_printer_print_str(p, "(");
		p = print_disjunct(map->p[i], map->dim, p, set);
		if (map->n > 1 && map->p[i]->n_eq + map->p[i]->n_ineq > 1)
			p = isl_printer_print_str(p, ")");
	}
	return p;
}

static __isl_give isl_printer *isl_map_print_isl(__isl_keep isl_map *map,
	__isl_take isl_printer *p)
{
	if (isl_map_dim(map, isl_dim_param) > 0) {
		p = print_tuple(map->dim, p, isl_dim_param, 0);
		p = isl_printer_print_str(p, " -> ");
	}
	p = isl_printer_print_str(p, "{ ");
	p = print_tuple(map->dim, p, isl_dim_in, 0);
	p = isl_printer_print_str(p, " -> ");
	p = print_tuple(map->dim, p, isl_dim_out, 0);
	p = print_disjuncts(map, p, 0);
	p = isl_printer_print_str(p, " }");
	return p;
}

static __isl_give isl_printer *isl_set_print_isl(__isl_keep isl_set *set,
	__isl_take isl_printer *p)
{
	int i;

	if (isl_set_dim(set, isl_dim_param) > 0) {
		p = print_tuple(set->dim, p, isl_dim_param, 0);
		p = isl_printer_print_str(p, " -> ");
	}
	p = isl_printer_print_str(p, "{ ");
	p = print_tuple(set->dim, p, isl_dim_set, 1);
	p = print_disjuncts((isl_map *)set, p, 1);
	p = isl_printer_print_str(p, " }");
	return p;
}

__isl_give isl_printer *isl_printer_print_basic_map(__isl_take isl_printer *p,
	__isl_keep isl_basic_map *bmap)
{
	if (!p || !bmap)
		goto error;
	if (p->output_format == ISL_FORMAT_ISL)
		return isl_basic_map_print_isl(bmap, p);
	else if (p->output_format == ISL_FORMAT_OMEGA)
		return isl_basic_map_print_omega(bmap, p);
	isl_assert(bmap->ctx, 0, goto error);
error:
	isl_printer_free(p);
	return NULL;
}

void isl_basic_map_print(__isl_keep isl_basic_map *bmap, FILE *out, int indent,
	const char *prefix, const char *suffix, unsigned output_format)
{
	isl_printer *printer;

	if (!bmap)
		return;

	printer = isl_printer_to_file(bmap->ctx, out);
	printer = isl_printer_set_indent(printer, indent);
	printer = isl_printer_set_prefix(printer, prefix);
	printer = isl_printer_set_suffix(printer, suffix);
	printer = isl_printer_set_output_format(printer, output_format);
	isl_printer_print_basic_map(printer, bmap);

	isl_printer_free(printer);
}

__isl_give isl_printer *isl_printer_print_basic_set(__isl_take isl_printer *p,
	__isl_keep isl_basic_set *bset)
{
	if (!p || !bset)
		goto error;

	if (p->output_format == ISL_FORMAT_ISL)
		return isl_basic_set_print_isl(bset, p);
	else if (p->output_format == ISL_FORMAT_POLYLIB)
		return isl_basic_set_print_polylib(bset, p);
	else if (p->output_format == ISL_FORMAT_POLYLIB_CONSTRAINTS)
		return bset_print_constraints_polylib(bset, p);
	else if (p->output_format == ISL_FORMAT_OMEGA)
		return isl_basic_set_print_omega(bset, p);
	isl_assert(p->ctx, 0, goto error);
error:
	isl_printer_free(p);
	return NULL;
}

void isl_basic_set_print(struct isl_basic_set *bset, FILE *out, int indent,
	const char *prefix, const char *suffix, unsigned output_format)
{
	isl_printer *printer;

	if (!bset)
		return;

	printer = isl_printer_to_file(bset->ctx, out);
	printer = isl_printer_set_indent(printer, indent);
	printer = isl_printer_set_prefix(printer, prefix);
	printer = isl_printer_set_suffix(printer, suffix);
	printer = isl_printer_set_output_format(printer, output_format);
	isl_printer_print_basic_set(printer, bset);

	isl_printer_free(printer);
}

__isl_give isl_printer *isl_printer_print_set(__isl_take isl_printer *p,
	__isl_keep isl_set *set)
{
	if (!p || !set)
		goto error;
	if (p->output_format == ISL_FORMAT_ISL)
		return isl_set_print_isl(set, p);
	else if (p->output_format == ISL_FORMAT_POLYLIB)
		return isl_set_print_polylib(set, p);
	else if (p->output_format == ISL_FORMAT_OMEGA)
		return isl_set_print_omega(set, p);
	isl_assert(set->ctx, 0, goto error);
error:
	isl_printer_free(p);
	return NULL;
}

void isl_set_print(struct isl_set *set, FILE *out, int indent,
	unsigned output_format)
{
	isl_printer *printer;

	if (!set)
		return;

	printer = isl_printer_to_file(set->ctx, out);
	printer = isl_printer_set_indent(printer, indent);
	printer = isl_printer_set_output_format(printer, output_format);
	printer = isl_printer_print_set(printer, set);

	isl_printer_free(printer);
}

__isl_give isl_printer *isl_printer_print_map(__isl_take isl_printer *p,
	__isl_keep isl_map *map)
{
	if (!p || !map)
		goto error;

	if (p->output_format == ISL_FORMAT_ISL)
		return isl_map_print_isl(map, p);
	else if (p->output_format == ISL_FORMAT_POLYLIB)
		return isl_map_print_polylib(map, p);
	else if (p->output_format == ISL_FORMAT_OMEGA)
		return isl_map_print_omega(map, p);
	isl_assert(map->ctx, 0, goto error);
error:
	isl_printer_free(p);
	return NULL;
}

void isl_map_print(__isl_keep isl_map *map, FILE *out, int indent,
	unsigned output_format)
{
	isl_printer *printer;

	if (!map)
		return;

	printer = isl_printer_to_file(map->ctx, out);
	printer = isl_printer_set_indent(printer, indent);
	printer = isl_printer_set_output_format(printer, output_format);
	printer = isl_printer_print_map(printer, map);

	isl_printer_free(printer);
}

static int upoly_rec_n_non_zero(__isl_keep struct isl_upoly_rec *rec)
{
	int i;
	int n;

	for (i = 0, n = 0; i < rec->n; ++i)
		if (!isl_upoly_is_zero(rec->p[i]))
			++n;

	return n;
}

static __isl_give isl_printer *print_div(__isl_keep isl_dim *dim,
	__isl_keep isl_mat *div, int pos, __isl_take isl_printer *p)
{
	int c = p->output_format == ISL_FORMAT_C;
	p = isl_printer_print_str(p, c ? "floord(" : "[(");
	p = print_affine_of_len(dim, p, div->row[pos] + 1, div->n_col - 1, 1);
	p = isl_printer_print_str(p, c ? ", " : ")/");
	p = isl_printer_print_isl_int(p, div->row[pos][0]);
	p = isl_printer_print_str(p, c ? ")" : "]");
	return p;
}

static __isl_give isl_printer *upoly_print_cst(__isl_keep struct isl_upoly *up,
	__isl_take isl_printer *p, int first)
{
	struct isl_upoly_cst *cst;
	int neg;

	cst = isl_upoly_as_cst(up);
	if (!cst)
		goto error;
	neg = !first && isl_int_is_neg(cst->n);
	if (!first)
		p = isl_printer_print_str(p, neg ? " - " :  " + ");
	if (neg)
		isl_int_neg(cst->n, cst->n);
	if (isl_int_is_zero(cst->d)) {
		int sgn = isl_int_sgn(cst->n);
		p = isl_printer_print_str(p, sgn < 0 ? "-infty" :
					    sgn == 0 ? "NaN" : "infty");
	} else
		p = isl_printer_print_isl_int(p, cst->n);
	if (neg)
		isl_int_neg(cst->n, cst->n);
	if (!isl_int_is_zero(cst->d) && !isl_int_is_one(cst->d)) {
		p = isl_printer_print_str(p, "/");
		p = isl_printer_print_isl_int(p, cst->d);
	}
	return p;
error:
	isl_printer_free(p);
	return NULL;
}

static __isl_give isl_printer *print_base(__isl_take isl_printer *p,
	__isl_keep isl_dim *dim, __isl_keep isl_mat *div, int var)
{
	unsigned total;

	total = isl_dim_total(dim);
	if (var < total)
		p = print_term(dim, dim->ctx->one, 1 + var, p, 1);
	else
		p = print_div(dim, div, var - total, p);
	return p;
}

static __isl_give isl_printer *print_pow(__isl_take isl_printer *p,
	__isl_keep isl_dim *dim, __isl_keep isl_mat *div, int var, int exp)
{
	p = print_base(p, dim, div, var);
	if (exp == 1)
		return p;
	if (p->output_format == ISL_FORMAT_C) {
		int i;
		for (i = 1; i < exp; ++i) {
			p = isl_printer_print_str(p, "*");
			p = print_base(p, dim, div, var);
		}
	} else {
		p = isl_printer_print_str(p, "^");
		p = isl_printer_print_int(p, exp);
	}
	return p;
}

static __isl_give isl_printer *upoly_print(__isl_keep struct isl_upoly *up,
	__isl_keep isl_dim *dim, __isl_keep isl_mat *div,
	__isl_take isl_printer *p)
{
	int i, n, first;
	struct isl_upoly_rec *rec;

	if (!p || !up || !dim || !div)
		goto error;

	if (isl_upoly_is_cst(up))
		return upoly_print_cst(up, p, 1);

	rec = isl_upoly_as_rec(up);
	if (!rec)
		goto error;
	n = upoly_rec_n_non_zero(rec);
	if (n > 1)
		p = isl_printer_print_str(p, "(");
	for (i = 0, first = 1; i < rec->n; ++i) {
		if (isl_upoly_is_zero(rec->p[i]))
			continue;
		if (isl_upoly_is_negone(rec->p[i])) {
			if (!i)
				p = isl_printer_print_str(p, "-1");
			else if (first)
				p = isl_printer_print_str(p, "-");
			else
				p = isl_printer_print_str(p, " - ");
		} else if (isl_upoly_is_cst(rec->p[i]) &&
				!isl_upoly_is_one(rec->p[i]))
			p = upoly_print_cst(rec->p[i], p, first);
		else {
			if (!first)
				p = isl_printer_print_str(p, " + ");
			if (i == 0 || !isl_upoly_is_one(rec->p[i]))
				p = upoly_print(rec->p[i], dim, div, p);
		}
		first = 0;
		if (i == 0)
			continue;
		if (!isl_upoly_is_one(rec->p[i]) &&
		    !isl_upoly_is_negone(rec->p[i]))
			p = isl_printer_print_str(p, " * ");
		p = print_pow(p, dim, div, rec->up.var, i);
	}
	if (n > 1)
		p = isl_printer_print_str(p, ")");
	return p;
error:
	isl_printer_free(p);
	return NULL;
}

__isl_give isl_printer *isl_printer_print_qpolynomial(__isl_take isl_printer *p,
	__isl_keep isl_qpolynomial *qp)
{
	if (!p || !qp)
		goto error;
	p = upoly_print(qp->upoly, qp->dim, qp->div, p);
	return p;
error:
	isl_printer_free(p);
	return NULL;
}

void isl_qpolynomial_print(__isl_keep isl_qpolynomial *qp, FILE *out,
	unsigned output_format)
{
	isl_printer *p;

	if  (!qp)
		return;

	isl_assert(qp->dim->ctx, output_format == ISL_FORMAT_ISL, return);
	p = isl_printer_to_file(qp->dim->ctx, out);
	p = isl_printer_print_qpolynomial(p, qp);
	isl_printer_free(p);
}

static __isl_give isl_printer *qpolynomial_fold_print(
	__isl_keep isl_qpolynomial_fold *fold, __isl_take isl_printer *p)
{
	int i;

	if (fold->type == isl_fold_min)
		p = isl_printer_print_str(p, "min");
	else if (fold->type == isl_fold_max)
		p = isl_printer_print_str(p, "max");
	p = isl_printer_print_str(p, "(");
	for (i = 0; i < fold->n; ++i) {
		if (i)
			p = isl_printer_print_str(p, ", ");
		p = isl_printer_print_qpolynomial(p, fold->qp[i]);
	}
	p = isl_printer_print_str(p, ")");
	return p;
}

__isl_give isl_printer *isl_printer_print_qpolynomial_fold(
	__isl_take isl_printer *p, __isl_keep isl_qpolynomial_fold *fold)
{
	if  (!p || !fold)
		goto error;
	p = qpolynomial_fold_print(fold, p);
	return p;
error:
	isl_printer_free(p);
	return NULL;
}

void isl_qpolynomial_fold_print(__isl_keep isl_qpolynomial_fold *fold,
	FILE *out, unsigned output_format)
{
	isl_printer *p;

	if (!fold)
		return;

	isl_assert(fold->dim->ctx, output_format == ISL_FORMAT_ISL, return);

	p = isl_printer_to_file(fold->dim->ctx, out);
	p = isl_printer_print_qpolynomial_fold(p, fold);

	isl_printer_free(p);
}

static __isl_give isl_printer *print_pw_qpolynomial_isl(
	__isl_take isl_printer *p, __isl_keep isl_pw_qpolynomial *pwqp)
{
	int i = 0;

	if (!p || !pwqp)
		goto error;

	if (isl_dim_size(pwqp->dim, isl_dim_param) > 0) {
		p = print_tuple(pwqp->dim, p, isl_dim_param, 0);
		p = isl_printer_print_str(p, " -> ");
	}
	p = isl_printer_print_str(p, "{ ");
	if (pwqp->n == 0) {
		if (isl_dim_size(pwqp->dim, isl_dim_set) > 0) {
			p = print_tuple(pwqp->dim, p, isl_dim_set, 1);
			p = isl_printer_print_str(p, " -> ");
		}
		p = isl_printer_print_str(p, "0");
	}
	for (i = 0; i < pwqp->n; ++i) {
		if (i)
			p = isl_printer_print_str(p, "; ");
		if (isl_dim_size(pwqp->p[i].set->dim, isl_dim_set) > 0) {
			p = print_tuple(pwqp->p[i].set->dim, p, isl_dim_set, 1);
			p = isl_printer_print_str(p, " -> ");
		}
		p = isl_printer_print_qpolynomial(p, pwqp->p[i].qp);
		p = print_disjuncts((isl_map *)pwqp->p[i].set, p, 1);
	}
	p = isl_printer_print_str(p, " }");
	return p;
error:
	isl_printer_free(p);
	return NULL;
}

void isl_pw_qpolynomial_print(__isl_keep isl_pw_qpolynomial *pwqp, FILE *out,
	unsigned output_format)
{
	isl_printer *p;

	if (!pwqp)
		return;

	p = isl_printer_to_file(pwqp->dim->ctx, out);
	p = isl_printer_set_output_format(p, output_format);
	p = isl_printer_print_pw_qpolynomial(p, pwqp);

	isl_printer_free(p);
}

static __isl_give isl_printer *print_pw_qpolynomial_fold_isl(
	__isl_take isl_printer *p, __isl_keep isl_pw_qpolynomial_fold *pwf)
{
	int i = 0;

	if (isl_dim_size(pwf->dim, isl_dim_param) > 0) {
		p = print_tuple(pwf->dim, p, isl_dim_param, 0);
		p = isl_printer_print_str(p, " -> ");
	}
	p = isl_printer_print_str(p, "{ ");
	if (pwf->n == 0) {
		if (isl_dim_size(pwf->dim, isl_dim_set) > 0) {
			p = print_tuple(pwf->dim, p, isl_dim_set, 0);
			p = isl_printer_print_str(p, " -> ");
		}
		p = isl_printer_print_str(p, "0");
	}
	for (i = 0; i < pwf->n; ++i) {
		if (i)
			p = isl_printer_print_str(p, "; ");
		if (isl_dim_size(pwf->p[i].set->dim, isl_dim_set) > 0) {
			p = print_tuple(pwf->p[i].set->dim, p, isl_dim_set, 0);
			p = isl_printer_print_str(p, " -> ");
		}
		p = qpolynomial_fold_print(pwf->p[i].fold, p);
		p = print_disjuncts((isl_map *)pwf->p[i].set, p, 1);
	}
	p = isl_printer_print_str(p, " }");
	return p;
}

static __isl_give isl_printer *print_affine_c(__isl_take isl_printer *p,
	__isl_keep isl_basic_set *bset, isl_int *c);

static __isl_give isl_printer *print_name_c(__isl_take isl_printer *p,
	__isl_keep isl_basic_set *bset, enum isl_dim_type type, unsigned pos)
{
	if (type == isl_dim_div) {
		p = isl_printer_print_str(p, "floord(");
		p = print_affine_c(p, bset, bset->div[pos] + 1);
		p = isl_printer_print_str(p, ", ");
		p = isl_printer_print_isl_int(p, bset->div[pos][0]);
		p = isl_printer_print_str(p, ")");
	} else {
		const char *name;

		name = isl_dim_get_name(bset->dim, type, pos);
		if (!name)
			name = "UNNAMED";
		p = isl_printer_print_str(p, name);
	}
	return p;
}

static __isl_give isl_printer *print_term_c(__isl_take isl_printer *p,
	__isl_keep isl_basic_set *bset, isl_int c, unsigned pos)
{
	enum isl_dim_type type;
	unsigned nparam = isl_basic_set_dim(bset, isl_dim_param);

	if (pos == 0)
		return isl_printer_print_isl_int(p, c);

	if (isl_int_is_one(c))
		;
	else if (isl_int_is_negone(c))
		p = isl_printer_print_str(p, "-");
	else {
		p = isl_printer_print_isl_int(p, c);
		p = isl_printer_print_str(p, "*");
	}
	type = pos2type(bset->dim, &pos);
	p = print_name_c(p, bset, type, pos);
	return p;
}

static __isl_give isl_printer *print_affine_c(__isl_take isl_printer *p,
	__isl_keep isl_basic_set *bset, isl_int *c)
{
	int i;
	int first;
	unsigned len = 1 + isl_basic_set_total_dim(bset);

	for (i = 0, first = 1; i < len; ++i) {
		int flip = 0;
		if (isl_int_is_zero(c[i]))
			continue;
		if (!first) {
			if (isl_int_is_neg(c[i])) {
				flip = 1;
				isl_int_neg(c[i], c[i]);
				p = isl_printer_print_str(p, " - ");
			} else 
				p = isl_printer_print_str(p, " + ");
		}
		first = 0;
		p = print_term(bset->dim, c[i], i, p, 1);
		if (flip)
			isl_int_neg(c[i], c[i]);
	}
	if (first)
		p = isl_printer_print_str(p, "0");
	return p;
}

static __isl_give isl_printer *print_constraint_c(__isl_take isl_printer *p,
	__isl_keep isl_basic_set *bset, isl_int *c, const char *op, int first)
{
	if (!first)
		p = isl_printer_print_str(p, " && ");

	p = print_affine_c(p, bset, c);
	p = isl_printer_print_str(p, " ");
	p = isl_printer_print_str(p, op);
	p = isl_printer_print_str(p, " 0");
	return p;
}

static __isl_give isl_printer *print_basic_set_c(__isl_take isl_printer *p,
	__isl_keep isl_basic_set *bset)
{
	int i;

	for (i = 0; i < bset->n_eq; ++i)
		p = print_constraint_c(p, bset, bset->eq[i], "==", !i);
	for (i = 0; i < bset->n_ineq; ++i)
		p = print_constraint_c(p, bset, bset->ineq[i], ">=",
					!bset->n_eq && !i);
	return p;
}

static __isl_give isl_printer *print_set_c(__isl_take isl_printer *p,
	__isl_keep isl_set *set)
{
	int i;

	if (set->n == 0)
		p = isl_printer_print_str(p, "0");

	for (i = 0; i < set->n; ++i) {
		if (i)
			p = isl_printer_print_str(p, " || ");
		if (set->n > 1)
			p = isl_printer_print_str(p, "(");
		p = print_basic_set_c(p, set->p[i]);
		if (set->n > 1)
			p = isl_printer_print_str(p, ")");
	}
	return p;
}

static __isl_give isl_printer *print_qpolynomial_c(__isl_take isl_printer *p,
	__isl_keep isl_qpolynomial *qp)
{
	isl_int den;

	isl_int_init(den);
	isl_qpolynomial_get_den(qp, &den);
	if (!isl_int_is_one(den)) {
		isl_qpolynomial *f;
		p = isl_printer_print_str(p, "(");
		qp = isl_qpolynomial_copy(qp);
		f = isl_qpolynomial_rat_cst(isl_dim_copy(qp->dim),
						den, qp->dim->ctx->one);
		qp = isl_qpolynomial_mul(qp, f);
	}
	if (qp)
		p = upoly_print(qp->upoly, qp->dim, qp->div, p);
	if (!isl_int_is_one(den)) {
		p = isl_printer_print_str(p, ")/");
		p = isl_printer_print_isl_int(p, den);
		isl_qpolynomial_free(qp);
	}
	isl_int_clear(den);
	return p;
}

static __isl_give isl_printer *print_pw_qpolynomial_c(
	__isl_take isl_printer *p, __isl_keep isl_pw_qpolynomial *pwpq)
{
	int i;

	if (pwpq->n == 1 && isl_set_fast_is_universe(pwpq->p[0].set))
		return print_qpolynomial_c(p, pwpq->p[0].qp);

	for (i = 0; i < pwpq->n; ++i) {
		p = isl_printer_print_str(p, "(");
		p = print_set_c(p, pwpq->p[i].set);
		p = isl_printer_print_str(p, ") ? (");
		p = print_qpolynomial_c(p, pwpq->p[i].qp);
		p = isl_printer_print_str(p, ") : ");
	}

	p = isl_printer_print_str(p, "0");
	return p;
}

__isl_give isl_printer *isl_printer_print_pw_qpolynomial(
	__isl_take isl_printer *p, __isl_keep isl_pw_qpolynomial *pwqp)
{
	if (!p || !pwqp)
		goto error;

	if (p->output_format == ISL_FORMAT_ISL)
		return print_pw_qpolynomial_isl(p, pwqp);
	else if (p->output_format == ISL_FORMAT_C)
		return print_pw_qpolynomial_c(p, pwqp);
	isl_assert(p->ctx, 0, goto error);
error:
	isl_printer_free(p);
	return NULL;
}

static __isl_give isl_printer *print_qpolynomial_fold_c(
	__isl_take isl_printer *p, __isl_keep isl_qpolynomial_fold *fold)
{
	int i;

	for (i = 0; i < fold->n - 1; ++i)
		if (fold->type == isl_fold_min)
			p = isl_printer_print_str(p, "min(");
		else if (fold->type == isl_fold_max)
			p = isl_printer_print_str(p, "max(");

	for (i = 0; i < fold->n; ++i) {
		if (i)
			p = isl_printer_print_str(p, ", ");
		p = print_qpolynomial_c(p, fold->qp[i]);
		if (i)
			p = isl_printer_print_str(p, ")");
	}
	return p;
}

static __isl_give isl_printer *print_pw_qpolynomial_fold_c(
	__isl_take isl_printer *p, __isl_keep isl_pw_qpolynomial_fold *pwf)
{
	int i;

	if (pwf->n == 1 && isl_set_fast_is_universe(pwf->p[0].set))
		return print_qpolynomial_fold_c(p, pwf->p[0].fold);

	for (i = 0; i < pwf->n; ++i) {
		p = isl_printer_print_str(p, "(");
		p = print_set_c(p, pwf->p[i].set);
		p = isl_printer_print_str(p, ") ? (");
		p = print_qpolynomial_fold_c(p, pwf->p[i].fold);
		p = isl_printer_print_str(p, ") : ");
	}

	p = isl_printer_print_str(p, "0");
	return p;
}

__isl_give isl_printer *isl_printer_print_pw_qpolynomial_fold(
	__isl_take isl_printer *p, __isl_keep isl_pw_qpolynomial_fold *pwf)
{
	if (!p || !pwf)
		goto error;

	if (p->output_format == ISL_FORMAT_ISL)
		return print_pw_qpolynomial_fold_isl(p, pwf);
	else if (p->output_format == ISL_FORMAT_C)
		return print_pw_qpolynomial_fold_c(p, pwf);
	isl_assert(p->ctx, 0, goto error);
error:
	isl_printer_free(p);
	return NULL;
}

void isl_pw_qpolynomial_fold_print(__isl_keep isl_pw_qpolynomial_fold *pwf,
	FILE *out, unsigned output_format)
{
	isl_printer *p;

	if (!pwf)
		return;

	p = isl_printer_to_file(pwf->dim->ctx, out);
	p = isl_printer_set_output_format(p, output_format);
	p = isl_printer_print_pw_qpolynomial_fold(p, pwf);

	isl_printer_free(p);
}
