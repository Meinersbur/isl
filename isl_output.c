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

#include <isl_set.h>
#include <isl_seq.h>

static void print_constraint_polylib(struct isl_basic_map *bmap,
	int ineq, int n,
	FILE *out, int indent, const char *prefix, const char *suffix)
{
	int i;
	unsigned n_in = isl_basic_map_dim(bmap, isl_dim_in);
	unsigned n_out = isl_basic_map_dim(bmap, isl_dim_out);
	unsigned nparam = isl_basic_map_dim(bmap, isl_dim_param);
	isl_int *c = ineq ? bmap->ineq[n] : bmap->eq[n];

	fprintf(out, "%*s%s", indent, "", prefix ? prefix : "");
	fprintf(out, "%d", ineq);
	for (i = 0; i < n_out; ++i) {
		fprintf(out, " ");
		isl_int_print(out, c[1+nparam+n_in+i], 5);
	}
	for (i = 0; i < n_in; ++i) {
		fprintf(out, " ");
		isl_int_print(out, c[1+nparam+i], 5);
	}
	for (i = 0; i < bmap->n_div; ++i) {
		fprintf(out, " ");
		isl_int_print(out, c[1+nparam+n_in+n_out+i], 5);
	}
	for (i = 0; i < nparam; ++i) {
		fprintf(out, " ");
		isl_int_print(out, c[1+i], 5);
	}
	fprintf(out, " ");
	isl_int_print(out, c[0], 5);
	fprintf(out, "%s\n", suffix ? suffix : "");
}

static void print_constraints_polylib(struct isl_basic_map *bmap,
	FILE *out, int indent, const char *prefix, const char *suffix)
{
	int i;

	for (i = 0; i < bmap->n_eq; ++i)
		print_constraint_polylib(bmap, 0, i, out,
					indent, prefix, suffix);
	for (i = 0; i < bmap->n_ineq; ++i)
		print_constraint_polylib(bmap, 1, i, out,
					indent, prefix, suffix);
}

static void bset_print_constraints_polylib(struct isl_basic_set *bset,
	FILE *out, int indent, const char *prefix, const char *suffix)
{
	print_constraints_polylib((struct isl_basic_map *)bset,
				 out, indent, prefix, suffix);
}

static void isl_basic_map_print_polylib(struct isl_basic_map *bmap, FILE *out,
	int indent, const char *prefix, const char *suffix)
{
	unsigned total = isl_basic_map_total_dim(bmap);
	fprintf(out, "%*s%s", indent, "", prefix ? prefix : "");
	fprintf(out, "%d %d", bmap->n_eq + bmap->n_ineq, 1 + total + 1);
	fprintf(out, "%s\n", suffix ? suffix : "");
	print_constraints_polylib(bmap, out, indent, prefix, suffix);
}

static void isl_basic_set_print_polylib(struct isl_basic_set *bset, FILE *out,
	int indent, const char *prefix, const char *suffix)
{
	isl_basic_map_print_polylib((struct isl_basic_map *)bset, out,
					indent, prefix, suffix);
}

static void isl_map_print_polylib(struct isl_map *map, FILE *out, int indent)
{
	int i;

	fprintf(out, "%*s", indent, "");
	fprintf(out, "%d\n", map->n);
	for (i = 0; i < map->n; ++i) {
		fprintf(out, "\n");
		isl_basic_map_print_polylib(map->p[i], out, indent, NULL, NULL);
	}
}

static void isl_set_print_polylib(struct isl_set *set, FILE *out, int indent)
{
	isl_map_print_polylib((struct isl_map *)set, out, indent);
}

static print_name(struct isl_dim *dim, FILE *out,
	enum isl_dim_type type, unsigned pos, int set)
{
	const char *name;

	name = type == isl_dim_div ? NULL : isl_dim_get_name(dim, type, pos);

	if (name)
		fprintf(out, "%s", name);
	else {
		const char *prefix;
		if (type == isl_dim_param)
			prefix = "p";
		else if (type == isl_dim_div)
			prefix = "e";
		else if (set || type == isl_dim_in)
			prefix = "i";
		else
			prefix = "o";
		fprintf(out, "%s%d", prefix, pos);
	}
}

static void print_var_list(struct isl_dim *dim, FILE *out,
	enum isl_dim_type type, int set)
{
	int i;

	for (i = 0; i < isl_dim_size(dim, type); ++i) {
		if (i)
			fprintf(out, ", ");
		print_name(dim, out, type, i, set);
	}
}

static void print_tuple(__isl_keep isl_dim *dim, FILE *out,
	enum isl_dim_type type, int set)
{
	fprintf(out, "[");
	print_var_list(dim, out, type, set);
	fprintf(out, "]");
}

static void print_omega_parameters(struct isl_dim *dim, FILE *out,
	int indent, const char *prefix, const char *suffix)
{
	if (isl_dim_size(dim, isl_dim_param) == 0)
		return;

	fprintf(out, "%*s%ssymbolic ", indent, "", prefix ? prefix : "");
	print_var_list(dim, out, isl_dim_param, 0);
	fprintf(out, ";%s\n", suffix ? suffix : "");
}

static void print_term(__isl_keep isl_dim *dim,
			isl_int c, int pos, FILE *out, int set)
{
	enum isl_dim_type type;
	unsigned n_in = isl_dim_size(dim, isl_dim_in);
	unsigned n_out = isl_dim_size(dim, isl_dim_out);
	unsigned nparam = isl_dim_size(dim, isl_dim_param);

	if (pos == 0) {
		isl_int_print(out, c, 0);
		return;
	}

	if (isl_int_is_one(c))
		;
	else if (isl_int_is_negone(c))
		fprintf(out, "-");
	else
		isl_int_print(out, c, 0);
	if (pos < 1 + nparam) {
		type = isl_dim_param;
		pos -= 1;
	} else if (pos < 1 + nparam + n_in) {
		type = isl_dim_in;
		pos -= 1 + nparam;
	} else if (pos < 1 + nparam + n_in + n_out) {
		type = isl_dim_out;
		pos -= 1 + nparam + n_in;
	} else {
		type = isl_dim_div;
		pos -= 1 + nparam + n_in + n_out;
	}
	print_name(dim, out, type, pos, set);
}

static void print_affine(__isl_keep isl_basic_map *bmap, FILE *out,
	isl_int *c, int set)
{
	int i;
	int first;
	unsigned len = 1 + isl_basic_map_total_dim(bmap);

	for (i = 0, first = 1; i < len; ++i) {
		int flip = 0;
		if (isl_int_is_zero(c[i]))
			continue;
		if (!first) {
			if (isl_int_is_neg(c[i])) {
				flip = 1;
				isl_int_neg(c[i], c[i]);
				fprintf(out, " - ");
			} else 
				fprintf(out, " + ");
		}
		first = 0;
		print_term(bmap->dim, c[i], i, out, set);
		if (flip)
			isl_int_neg(c[i], c[i]);
	}
	if (first)
		fprintf(out, "0");
}

static void print_constraint(struct isl_basic_map *bmap, FILE *out,
	isl_int *c, int last, const char *op, int first_constraint, int set)
{
	if (!first_constraint)
		fprintf(out, " and ");

	isl_int_abs(c[last], c[last]);

	print_term(bmap->dim, c[last], last, out, set);

	fprintf(out, " %s ", op);

	isl_int_set_si(c[last], 0);
	print_affine(bmap, out, c, set);
}

static void print_constraints(__isl_keep isl_basic_map *bmap, FILE *out,
	int set)
{
	int i;
	struct isl_vec *c;
	unsigned total = isl_basic_map_total_dim(bmap);

	c = isl_vec_alloc(bmap->ctx, 1 + total);
	if (!c)
		return;

	for (i = bmap->n_eq - 1; i >= 0; --i) {
		int l = isl_seq_last_non_zero(bmap->eq[i], 1 + total);
		isl_assert(bmap->ctx, l >= 0, return);
		if (isl_int_is_neg(bmap->eq[i][l]))
			isl_seq_cpy(c->el, bmap->eq[i], 1 + total);
		else
			isl_seq_neg(c->el, bmap->eq[i], 1 + total);
		print_constraint(bmap, out, c->el, l,
				    "=", i == bmap->n_eq - 1, set);
	}
	for (i = 0; i < bmap->n_ineq; ++i) {
		int l = isl_seq_last_non_zero(bmap->ineq[i], 1 + total);
		int s;
		isl_assert(bmap->ctx, l >= 0, return);
		s = isl_int_sgn(bmap->ineq[i][l]);
		if (s < 0)
			isl_seq_cpy(c->el, bmap->ineq[i], 1 + total);
		else
			isl_seq_neg(c->el, bmap->ineq[i], 1 + total);
		print_constraint(bmap, out, c->el, l,
				    s < 0 ? "<=" : ">=", !bmap->n_eq && !i, set);
	}

	isl_vec_free(c);
}

static void print_omega_constraints(__isl_keep isl_basic_map *bmap, FILE *out,
	int set)
{
	if (bmap->n_eq + bmap->n_ineq == 0)
		return;

	fprintf(out, ": ");
	if (bmap->n_div > 0) {
		int i;
		fprintf(out, "exists (");
		for (i = 0; i < bmap->n_div; ++i) {
			if (i)
				fprintf(out, ", ");
			print_name(bmap->dim, out, isl_dim_div, i, 0);
		}
		fprintf(out, ": ");
	}
	print_constraints(bmap, out, set);
	if (bmap->n_div > 0)
		fprintf(out, ")");
}

static void basic_map_print_omega(struct isl_basic_map *bmap, FILE *out)
{
	fprintf(out, "{ [");
	print_var_list(bmap->dim, out, isl_dim_in, 0);
	fprintf(out, "] -> [");
	print_var_list(bmap->dim, out, isl_dim_out, 0);
	fprintf(out, "] ");
	print_omega_constraints(bmap, out, 0);
	fprintf(out, " }");
}

static void isl_basic_map_print_omega(struct isl_basic_map *bmap, FILE *out,
	int indent, const char *prefix, const char *suffix)
{
	print_omega_parameters(bmap->dim, out, indent, prefix, suffix);

	fprintf(out, "%*s%s", indent, "", prefix ? prefix : "");
	basic_map_print_omega(bmap, out);
	fprintf(out, "%s\n", suffix ? suffix : "");
}

static void basic_set_print_omega(struct isl_basic_set *bset, FILE *out)
{
	fprintf(out, "{ [");
	print_var_list(bset->dim, out, isl_dim_set, 1);
	fprintf(out, "] ");
	print_omega_constraints((isl_basic_map *)bset, out, 1);
	fprintf(out, " }");
}

static void isl_basic_set_print_omega(struct isl_basic_set *bset, FILE *out,
	int indent, const char *prefix, const char *suffix)
{
	print_omega_parameters(bset->dim, out, indent, prefix, suffix);

	fprintf(out, "%*s%s", indent, "", prefix ? prefix : "");
	basic_set_print_omega(bset, out);
	fprintf(out, "%s\n", suffix ? suffix : "");
}

static void isl_map_print_omega(struct isl_map *map, FILE *out, int indent)
{
	int i;

	print_omega_parameters(map->dim, out, indent, "", "");

	fprintf(out, "%*s", indent, "");
	for (i = 0; i < map->n; ++i) {
		if (i)
			fprintf(out, " union ");
		basic_map_print_omega(map->p[i], out);
	}
	fprintf(out, "\n");
}

static void isl_set_print_omega(struct isl_set *set, FILE *out, int indent)
{
	int i;

	print_omega_parameters(set->dim, out, indent, "", "");

	fprintf(out, "%*s", indent, "");
	for (i = 0; i < set->n; ++i) {
		if (i)
			fprintf(out, " union ");
		basic_set_print_omega(set->p[i], out);
	}
	fprintf(out, "\n");
}

static void print_disjunct(__isl_keep isl_basic_map *bmap, FILE *out, int set)
{
	if (bmap->n_div > 0) {
		int i;
		fprintf(out, "exists (");
		for (i = 0; i < bmap->n_div; ++i) {
			if (i)
				fprintf(out, ", ");
			print_name(bmap->dim, out, isl_dim_div, i, 0);
			if (isl_int_is_zero(bmap->div[i][0]))
				continue;
			fprintf(out, " = [(");
			print_affine(bmap, out, bmap->div[i] + 1, set);
			fprintf(out, ")/");
			isl_int_print(out, bmap->div[i][0], 0);
			fprintf(out, "]");
		}
		fprintf(out, ": ");
	}

	print_constraints(bmap, out, set);

	if (bmap->n_div > 0)
		fprintf(out, ")");
}

static void isl_basic_map_print_isl(__isl_keep isl_basic_map *bmap, FILE *out,
	int indent, const char *prefix, const char *suffix)
{
	int i;

	fprintf(out, "%*s%s", indent, "", prefix ? prefix : "");
	if (isl_basic_map_dim(bmap, isl_dim_param) > 0) {
		print_tuple(bmap->dim, out, isl_dim_param, 0);
		fprintf(out, " -> ");
	}
	fprintf(out, "{ ");
	print_tuple(bmap->dim, out, isl_dim_in, 0);
	fprintf(out, " -> ");
	print_tuple(bmap->dim, out, isl_dim_out, 0);
	fprintf(out, " : ");
	print_disjunct(bmap, out, 0);
	fprintf(out, " }%s\n", suffix ? suffix : "");
}

static void isl_basic_set_print_isl(__isl_keep isl_basic_set *bset, FILE *out,
	int indent, const char *prefix, const char *suffix)
{
	int i;

	fprintf(out, "%*s%s", indent, "", prefix ? prefix : "");
	if (isl_basic_set_dim(bset, isl_dim_param) > 0) {
		print_tuple(bset->dim, out, isl_dim_param, 0);
		fprintf(out, " -> ");
	}
	fprintf(out, "{ ");
	print_tuple(bset->dim, out, isl_dim_set, 1);
	fprintf(out, " : ");
	print_disjunct((isl_basic_map *)bset, out, 1);
	fprintf(out, " }%s\n", suffix ? suffix : "");
}

static void isl_map_print_isl(__isl_keep isl_map *map, FILE *out, int indent)
{
	int i;

	fprintf(out, "%*s", indent, "");
	if (isl_map_dim(map, isl_dim_param) > 0) {
		print_tuple(map->dim, out, isl_dim_param, 0);
		fprintf(out, " -> ");
	}
	fprintf(out, "{ ");
	print_tuple(map->dim, out, isl_dim_in, 0);
	fprintf(out, " -> ");
	print_tuple(map->dim, out, isl_dim_out, 0);
	fprintf(out, " : ");
	if (map->n == 0)
		fprintf(out, "1 = 0");
	for (i = 0; i < map->n; ++i) {
		if (i)
			fprintf(out, " or ");
		if (map->n > 1 && map->p[i]->n_eq + map->p[i]->n_ineq > 1)
			fprintf(out, "(");
		print_disjunct(map->p[i], out, 0);
		if (map->n > 1 && map->p[i]->n_eq + map->p[i]->n_ineq > 1)
			fprintf(out, ")");
	}
	fprintf(out, " }\n");
}

static void isl_set_print_isl(__isl_keep isl_set *set, FILE *out, int indent)
{
	int i;

	fprintf(out, "%*s", indent, "");
	if (isl_set_dim(set, isl_dim_param) > 0) {
		print_tuple(set->dim, out, isl_dim_param, 0);
		fprintf(out, " -> ");
	}
	fprintf(out, "{ ");
	print_tuple(set->dim, out, isl_dim_set, 1);
	fprintf(out, " : ");
	if (set->n == 0)
		fprintf(out, "1 = 0");
	for (i = 0; i < set->n; ++i) {
		if (i)
			fprintf(out, " or ");
		print_disjunct((isl_basic_map *)set->p[i], out, 1);
	}
	fprintf(out, " }\n");
}

void isl_basic_map_print(__isl_keep isl_basic_map *bmap, FILE *out, int indent,
	const char *prefix, const char *suffix, unsigned output_format)
{
	if (!bmap)
		return;
	if (output_format == ISL_FORMAT_ISL)
		isl_basic_map_print_isl(bmap, out, indent, prefix, suffix);
	else if (output_format == ISL_FORMAT_OMEGA)
		isl_basic_map_print_omega(bmap, out, indent, prefix, suffix);
	else
		isl_assert(bmap->ctx, 0, return);
}

void isl_basic_set_print(struct isl_basic_set *bset, FILE *out, int indent,
	const char *prefix, const char *suffix, unsigned output_format)
{
	if (!bset)
		return;
	if (output_format == ISL_FORMAT_ISL)
		isl_basic_set_print_isl(bset, out, indent, prefix, suffix);
	else if (output_format == ISL_FORMAT_POLYLIB)
		isl_basic_set_print_polylib(bset, out, indent, prefix, suffix);
	else if (output_format == ISL_FORMAT_POLYLIB_CONSTRAINTS)
		bset_print_constraints_polylib(bset, out, indent, prefix, suffix);
	else if (output_format == ISL_FORMAT_OMEGA)
		isl_basic_set_print_omega(bset, out, indent, prefix, suffix);
	else
		isl_assert(bset->ctx, 0, return);
}

void isl_set_print(struct isl_set *set, FILE *out, int indent,
	unsigned output_format)
{
	if (!set)
		return;
	if (output_format == ISL_FORMAT_ISL)
		isl_set_print_isl(set, out, indent);
	else if (output_format == ISL_FORMAT_POLYLIB)
		isl_set_print_polylib(set, out, indent);
	else if (output_format == ISL_FORMAT_OMEGA)
		isl_set_print_omega(set, out, indent);
	else
		isl_assert(set->ctx, 0, return);
}

void isl_map_print(__isl_keep isl_map *map, FILE *out, int indent,
	unsigned output_format)
{
	if (!map)
		return;
	if (output_format == ISL_FORMAT_ISL)
		isl_map_print_isl(map, out, indent);
	else if (output_format == ISL_FORMAT_POLYLIB)
		isl_map_print_polylib(map, out, indent);
	else if (output_format == ISL_FORMAT_OMEGA)
		isl_map_print_omega(map, out, indent);
	else
		isl_assert(map->ctx, 0, return);
}
