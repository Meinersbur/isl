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

#include <isl_map_private.h>
#include <isl_dim_private.h>
#include <isl_dim_map.h>
#include <isl_reordering.h>

/* Maps dst positions to src positions */
struct isl_dim_map {
	unsigned len;
	int pos[1];
};

__isl_give isl_dim_map *isl_dim_map_alloc(isl_ctx *ctx, unsigned len)
{
	int i;
	struct isl_dim_map *dim_map;
	dim_map = isl_alloc(ctx, struct isl_dim_map,
				sizeof(struct isl_dim_map) + len * sizeof(int));
	if (!dim_map)
		return NULL;
	dim_map->len = 1 + len;
	dim_map->pos[0] = 0;
	for (i = 0; i < len; ++i)
		dim_map->pos[1 + i] = -1;
	return dim_map;
}

void isl_dim_map_dim_range(__isl_keep isl_dim_map *dim_map,
	struct isl_dim *dim, enum isl_dim_type type,
	unsigned first, unsigned n, unsigned dst_pos)
{
	int i;
	unsigned src_pos;

	if (!dim_map || !dim)
		return;
	
	src_pos = 1 + isl_dim_offset(dim, type);
	for (i = 0; i < n; ++i)
		dim_map->pos[1 + dst_pos + i] = src_pos + first + i;
}

void isl_dim_map_dim(__isl_keep isl_dim_map *dim_map, __isl_keep isl_dim *dim,
	enum isl_dim_type type, unsigned dst_pos)
{
	isl_dim_map_dim_range(dim_map, dim, type,
			      0, isl_dim_size(dim, type), dst_pos);
}

void isl_dim_map_div(__isl_keep isl_dim_map *dim_map,
	__isl_keep isl_basic_map *bmap, unsigned dst_pos)
{
	int i;
	unsigned src_pos;

	if (!dim_map || !bmap)
		return;
	
	src_pos = 1 + isl_dim_total(bmap->dim);
	for (i = 0; i < bmap->n_div; ++i)
		dim_map->pos[1 + dst_pos + i] = src_pos + i;
}

void isl_dim_map_dump(struct isl_dim_map *dim_map)
{
	int i;

	for (i = 0; i < dim_map->len; ++i)
		fprintf(stderr, "%d -> %d; ", i, dim_map->pos[i]);
	fprintf(stderr, "\n");
}

static void copy_constraint_dim_map(isl_int *dst, isl_int *src,
					struct isl_dim_map *dim_map)
{
	int i;

	for (i = 0; i < dim_map->len; ++i) {
		if (dim_map->pos[i] < 0)
			isl_int_set_si(dst[i], 0);
		else
			isl_int_set(dst[i], src[dim_map->pos[i]]);
	}
}

static void copy_div_dim_map(isl_int *dst, isl_int *src,
					struct isl_dim_map *dim_map)
{
	isl_int_set(dst[0], src[0]);
	copy_constraint_dim_map(dst+1, src+1, dim_map);
}

__isl_give isl_basic_map *isl_basic_map_add_constraints_dim_map(
	__isl_take isl_basic_map *dst, __isl_take isl_basic_map *src,
	__isl_take isl_dim_map *dim_map)
{
	int i;

	if (!src || !dst || !dim_map)
		goto error;

	for (i = 0; i < src->n_eq; ++i) {
		int i1 = isl_basic_map_alloc_equality(dst);
		if (i1 < 0)
			goto error;
		copy_constraint_dim_map(dst->eq[i1], src->eq[i], dim_map);
	}

	for (i = 0; i < src->n_ineq; ++i) {
		int i1 = isl_basic_map_alloc_inequality(dst);
		if (i1 < 0)
			goto error;
		copy_constraint_dim_map(dst->ineq[i1], src->ineq[i], dim_map);
	}

	for (i = 0; i < src->n_div; ++i) {
		int i1 = isl_basic_map_alloc_div(dst);
		if (i1 < 0)
			goto error;
		copy_div_dim_map(dst->div[i1], src->div[i], dim_map);
	}

	free(dim_map);
	isl_basic_map_free(src);

	return dst;
error:
	free(dim_map);
	isl_basic_map_free(src);
	isl_basic_map_free(dst);
	return NULL;
}

/* Extend the given dim_map with mappings for the divs in bmap.
 */
__isl_give isl_dim_map *isl_dim_map_extend(__isl_keep isl_dim_map *dim_map,
	__isl_keep isl_basic_map *bmap)
{
	int i;
	struct isl_dim_map *res;
	int offset;

	offset = isl_basic_map_offset(bmap, isl_dim_div);

	res = isl_dim_map_alloc(bmap->ctx, dim_map->len - 1 + bmap->n_div);
	if (!res)
		return NULL;

	for (i = 0; i < dim_map->len; ++i)
		res->pos[i] = dim_map->pos[i];
	for (i = 0; i < bmap->n_div; ++i)
		res->pos[dim_map->len + i] = offset + i;

	return res;
}

/* Extract a dim_map from a reordering.
 * We essentially need to reverse the mapping, and add an offset
 * of 1 for the constant term.
 */
__isl_give isl_dim_map *isl_dim_map_from_reordering(
	__isl_keep isl_reordering *exp)
{
	int i;
	isl_ctx *ctx;
	struct isl_dim_map *dim_map;

	if (!exp)
		return NULL;

	ctx = isl_dim_get_ctx(exp->dim);
	dim_map = isl_dim_map_alloc(ctx, isl_dim_total(exp->dim));
	if (!dim_map)
		return NULL;

	for (i = 0; i < exp->len; ++i)
		dim_map->pos[1 + exp->pos[i]] = 1 + i;

	return dim_map;
}
