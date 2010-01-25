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

static void print_constraint_polylib(struct isl_basic_set *bset,
	int ineq, int n,
	FILE *out, int indent, const char *prefix, const char *suffix)
{
	int i;
	unsigned dim = isl_basic_set_n_dim(bset);
	unsigned nparam = isl_basic_set_n_param(bset);
	isl_int *c = ineq ? bset->ineq[n] : bset->eq[n];

	fprintf(out, "%*s%s", indent, "", prefix ? prefix : "");
	fprintf(out, "%d", ineq);
	for (i = 0; i < dim; ++i) {
		fprintf(out, " ");
		isl_int_print(out, c[1+nparam+i], 5);
	}
	for (i = 0; i < bset->n_div; ++i) {
		fprintf(out, " ");
		isl_int_print(out, c[1+nparam+dim+i], 5);
	}
	for (i = 0; i < nparam; ++i) {
		fprintf(out, " ");
		isl_int_print(out, c[1+i], 5);
	}
	fprintf(out, " ");
	isl_int_print(out, c[0], 5);
	fprintf(out, "%s\n", suffix ? suffix : "");
}

static void print_constraints_polylib(struct isl_basic_set *bset,
	FILE *out, int indent, const char *prefix, const char *suffix)
{
	int i;

	for (i = 0; i < bset->n_eq; ++i)
		print_constraint_polylib(bset, 0, i, out,
					indent, prefix, suffix);
	for (i = 0; i < bset->n_ineq; ++i)
		print_constraint_polylib(bset, 1, i, out,
					indent, prefix, suffix);
}

static void isl_basic_set_print_polylib(struct isl_basic_set *bset, FILE *out,
	int indent, const char *prefix, const char *suffix)
{
	unsigned total = isl_basic_set_total_dim(bset);
	fprintf(out, "%*s%s", indent, "", prefix ? prefix : "");
	fprintf(out, "%d %d", bset->n_eq + bset->n_ineq, 1 + total + 1);
	fprintf(out, "%s\n", suffix ? suffix : "");
	print_constraints_polylib(bset, out, indent, prefix, suffix);
}

static void isl_set_print_polylib(struct isl_set *set, FILE *out, int indent)
{
	int i;

	fprintf(out, "%*s", indent, "");
	fprintf(out, "%d\n", set->n);
	for (i = 0; i < set->n; ++i) {
		fprintf(out, "\n");
		isl_basic_set_print_polylib(set->p[i], out, indent, NULL, NULL);
	}
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

	if (!isl_int_is_one(c))
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
		if (isl_int_is_zero(c[i]))
			continue;
		if (!first && isl_int_is_pos(c[i]))
			fprintf(out, " + ");
		first = 0;
		print_term(bmap->dim, c[i], i, out, set);
	}
	if (first)
		fprintf(out, "0");
}

static void print_constraint(struct isl_basic_map *bmap, FILE *out,
	isl_int *c, const char *suffix, int first_constraint, int set)
{
	if (!first_constraint)
		fprintf(out, " and ");

	print_affine(bmap, out, c, set);

	fprintf(out, " %s", suffix);
}

static void print_constraints(__isl_keep isl_basic_map *bmap, FILE *out,
	int set)
{
	int i;

	for (i = 0; i < bmap->n_eq; ++i)
		print_constraint(bmap, out, bmap->eq[i], "= 0", !i, set);
	for (i = 0; i < bmap->n_ineq; ++i)
		print_constraint(bmap, out, bmap->ineq[i], ">= 0",
					!bmap->n_eq && !i, set);
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
		print_disjunct(map->p[i], out, 0);
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
		print_constraints_polylib(bset, out, indent, prefix, suffix);
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
	else
		isl_assert(map->ctx, 0, return);
}
