/*
 * Copyright 2010      INRIA Saclay
 *
 * Use of this software is governed by the GNU LGPLv2.1 license
 *
 * Written by Sven Verdoolaege, INRIA Saclay - Ile-de-France,
 * Parc Club Orsay Universite, ZAC des vignes, 4 rue Jacques Monod,
 * 91893 Orsay, France 
 */

#include <isl_map_private.h>
#include <isl/ctx.h>
#include <isl/hash.h>
#include <isl/map.h>
#include <isl/set.h>
#include <isl_dim_private.h>
#include <isl_union_map_private.h>
#include <isl/union_set.h>

static __isl_give isl_union_map *isl_union_map_alloc(__isl_take isl_dim *dim,
	int size)
{
	isl_union_map *umap;

	if (!dim)
		return NULL;

	umap = isl_calloc_type(dim->ctx, isl_union_map);
	if (!umap)
		return NULL;

	umap->ref = 1;
	umap->dim = dim;
	if (isl_hash_table_init(dim->ctx, &umap->table, size) < 0)
		goto error;

	return umap;
error:
	isl_dim_free(dim);
	isl_union_map_free(umap);
	return NULL;
}

__isl_give isl_union_map *isl_union_map_empty(__isl_take isl_dim *dim)
{
	return isl_union_map_alloc(dim, 16);
}

__isl_give isl_union_set *isl_union_set_empty(__isl_take isl_dim *dim)
{
	return isl_union_map_empty(dim);
}

isl_ctx *isl_union_map_get_ctx(__isl_keep isl_union_map *umap)
{
	return umap ? umap->dim->ctx : NULL;
}

isl_ctx *isl_union_set_get_ctx(__isl_keep isl_union_set *uset)
{
	return uset ? uset->dim->ctx : NULL;
}

__isl_give isl_dim *isl_union_map_get_dim(__isl_keep isl_union_map *umap)
{
	if (!umap)
		return NULL;
	return isl_dim_copy(umap->dim);
}

__isl_give isl_dim *isl_union_set_get_dim(__isl_keep isl_union_set *uset)
{
	return isl_union_map_get_dim(uset);
}

static int free_umap_entry(void **entry, void *user)
{
	isl_map *map = *entry;
	isl_map_free(map);
	return 0;
}

static int add_map(__isl_take isl_map *map, void *user)
{
	isl_union_map **umap = (isl_union_map **)user;

	*umap = isl_union_map_add_map(*umap, map);

	return 0;
}

__isl_give isl_union_map *isl_union_map_dup(__isl_keep isl_union_map *umap)
{
	isl_union_map *dup;

	if (!umap)
		return NULL;

	dup = isl_union_map_empty(isl_dim_copy(umap->dim));
	if (isl_union_map_foreach_map(umap, &add_map, &dup) < 0)
		goto error;
	return dup;
error:
	isl_union_map_free(dup);
	return NULL;
}

__isl_give isl_union_map *isl_union_map_cow(__isl_take isl_union_map *umap)
{
	if (!umap)
		return NULL;

	if (umap->ref == 1)
		return umap;
	umap->ref--;
	return isl_union_map_dup(umap);
}

struct isl_union_align {
	isl_reordering *exp;
	isl_union_map *res;
};

static int align_entry(void **entry, void *user)
{
	isl_map *map = *entry;
	isl_reordering *exp;
	struct isl_union_align *data = user;

	exp = isl_reordering_extend_dim(isl_reordering_copy(data->exp),
				    isl_map_get_dim(map));

	data->res = isl_union_map_add_map(data->res,
					isl_map_realign(isl_map_copy(map), exp));

	return 0;
}

/* Align the parameters of umap along those of model.
 * The result has the parameters of model first, in the same order
 * as they appear in model, followed by any remaining parameters of
 * umap that do not appear in model.
 */
__isl_give isl_union_map *isl_union_map_align_params(
	__isl_take isl_union_map *umap, __isl_take isl_dim *model)
{
	struct isl_union_align data = { NULL, NULL };

	if (!umap || !model)
		goto error;

	if (isl_dim_match(umap->dim, isl_dim_param, model, isl_dim_param)) {
		isl_dim_free(model);
		return umap;
	}

	data.exp = isl_parameter_alignment_reordering(umap->dim, model);
	if (!data.exp)
		goto error;

	data.res = isl_union_map_alloc(isl_dim_copy(data.exp->dim),
					umap->table.n);
	if (isl_hash_table_foreach(umap->dim->ctx, &umap->table,
					&align_entry, &data) < 0)
		goto error;

	isl_reordering_free(data.exp);
	isl_union_map_free(umap);
	isl_dim_free(model);
	return data.res;
error:
	isl_reordering_free(data.exp);
	isl_union_map_free(umap);
	isl_union_map_free(data.res);
	isl_dim_free(model);
	return NULL;
}

__isl_give isl_union_set *isl_union_set_align_params(
	__isl_take isl_union_set *uset, __isl_take isl_dim *model)
{
	return isl_union_map_align_params(uset, model);
}

__isl_give isl_union_map *isl_union_map_union(__isl_take isl_union_map *umap1,
	__isl_take isl_union_map *umap2)
{
	umap1 = isl_union_map_align_params(umap1, isl_union_map_get_dim(umap2));
	umap2 = isl_union_map_align_params(umap2, isl_union_map_get_dim(umap1));

	umap1 = isl_union_map_cow(umap1);

	if (!umap1 || !umap2)
		goto error;

	if (isl_union_map_foreach_map(umap2, &add_map, &umap1) < 0)
		goto error;

	isl_union_map_free(umap2);

	return umap1;
error:
	isl_union_map_free(umap1);
	isl_union_map_free(umap2);
	return NULL;
}

__isl_give isl_union_set *isl_union_set_union(__isl_take isl_union_set *uset1,
	__isl_take isl_union_set *uset2)
{
	return isl_union_map_union(uset1, uset2);
}

__isl_give isl_union_map *isl_union_map_copy(__isl_keep isl_union_map *umap)
{
	if (!umap)
		return NULL;

	umap->ref++;
	return umap;
}

__isl_give isl_union_set *isl_union_set_copy(__isl_keep isl_union_set *uset)
{
	return isl_union_map_copy(uset);
}

void isl_union_map_free(__isl_take isl_union_map *umap)
{
	if (!umap)
		return;

	if (--umap->ref > 0)
		return;

	isl_hash_table_foreach(umap->dim->ctx, &umap->table,
			       &free_umap_entry, NULL);
	isl_hash_table_clear(&umap->table);
	isl_dim_free(umap->dim);
	free(umap);
}

void isl_union_set_free(__isl_take isl_union_set *uset)
{
	isl_union_map_free(uset);
}

static int has_dim(const void *entry, const void *val)
{
	isl_map *map = (isl_map *)entry;
	isl_dim *dim = (isl_dim *)val;

	return isl_dim_equal(map->dim, dim);
}

__isl_give isl_union_map *isl_union_map_add_map(__isl_take isl_union_map *umap,
	__isl_take isl_map *map)
{
	uint32_t hash;
	struct isl_hash_table_entry *entry;

	if (isl_map_fast_is_empty(map)) {
		isl_map_free(map);
		return umap;
	}

	umap = isl_union_map_cow(umap);

	if (!map || !umap)
		goto error;

	isl_assert(map->ctx, isl_dim_match(map->dim, isl_dim_param, umap->dim,
					   isl_dim_param), goto error);

	hash = isl_dim_get_hash(map->dim);
	entry = isl_hash_table_find(umap->dim->ctx, &umap->table, hash,
				    &has_dim, map->dim, 1);
	if (!entry)
		goto error;

	if (!entry->data)
		entry->data = map;
	else {
		entry->data = isl_map_union(entry->data, isl_map_copy(map));
		if (!entry->data)
			goto error;
		isl_map_free(map);
	}

	return umap;
error:
	isl_map_free(map);
	isl_union_map_free(umap);
	return NULL;
}

__isl_give isl_union_set *isl_union_set_add_set(__isl_take isl_union_set *uset,
	__isl_take isl_set *set)
{
	return isl_union_map_add_map(uset, (isl_map *)set);
}

__isl_give isl_union_map *isl_union_map_from_map(__isl_take isl_map *map)
{
	isl_dim *dim;
	isl_union_map *umap;

	if (!map)
		return NULL;

	dim = isl_map_get_dim(map);
	dim = isl_dim_drop(dim, isl_dim_in, 0, isl_dim_size(dim, isl_dim_in));
	dim = isl_dim_drop(dim, isl_dim_out, 0, isl_dim_size(dim, isl_dim_out));
	umap = isl_union_map_empty(dim);
	umap = isl_union_map_add_map(umap, map);

	return umap;
}

__isl_give isl_union_set *isl_union_set_from_set(__isl_take isl_set *set)
{
	return isl_union_map_from_map((isl_map *)set);
}

struct isl_union_map_foreach_data
{
	int (*fn)(__isl_take isl_map *map, void *user);
	void *user;
};

static int call_on_copy(void **entry, void *user)
{
	isl_map *map = *entry;
	struct isl_union_map_foreach_data *data;
	data = (struct isl_union_map_foreach_data *)user;

	return data->fn(isl_map_copy(map), data->user);
}

int isl_union_map_n_map(__isl_keep isl_union_map *umap)
{
	return umap ? umap->table.n : 0;
}

int isl_union_set_n_set(__isl_keep isl_union_set *uset)
{
	return uset ? uset->table.n : 0;
}

int isl_union_map_foreach_map(__isl_keep isl_union_map *umap,
	int (*fn)(__isl_take isl_map *map, void *user), void *user)
{
	struct isl_union_map_foreach_data data = { fn, user };

	if (!umap)
		return -1;

	return isl_hash_table_foreach(umap->dim->ctx, &umap->table,
				      &call_on_copy, &data);
}

__isl_give isl_map *isl_union_map_extract_map(__isl_keep isl_union_map *umap,
	__isl_take isl_dim *dim)
{
	uint32_t hash;
	struct isl_hash_table_entry *entry;

	if (!umap || !dim)
		goto error;

	hash = isl_dim_get_hash(dim);
	entry = isl_hash_table_find(umap->dim->ctx, &umap->table, hash,
				    &has_dim, dim, 0);
	if (!entry)
		return isl_map_empty(dim);
	isl_dim_free(dim);
	return isl_map_copy(entry->data);
error:
	isl_dim_free(dim);
	return NULL;
}

__isl_give isl_set *isl_union_set_extract_set(__isl_keep isl_union_set *uset,
	__isl_take isl_dim *dim)
{
	return (isl_set *)isl_union_map_extract_map(uset, dim);
}

int isl_union_set_foreach_set(__isl_keep isl_union_set *uset,
	int (*fn)(__isl_take isl_set *set, void *user), void *user)
{
	return isl_union_map_foreach_map(uset,
		(int(*)(__isl_take isl_map *, void*))fn, user);
}

struct isl_union_set_foreach_point_data {
	int (*fn)(__isl_take isl_point *pnt, void *user);
	void *user;
};

static int foreach_point(__isl_take isl_set *set, void *user)
{
	struct isl_union_set_foreach_point_data *data = user;
	int r;

	r = isl_set_foreach_point(set, data->fn, data->user);
	isl_set_free(set);

	return r;
}

int isl_union_set_foreach_point(__isl_keep isl_union_set *uset,
	int (*fn)(__isl_take isl_point *pnt, void *user), void *user)
{
	struct isl_union_set_foreach_point_data data = { fn, user };
	return isl_union_set_foreach_set(uset, &foreach_point, &data);
}

struct isl_union_map_gen_bin_data {
	isl_union_map *umap2;
	isl_union_map *res;
};

static int subtract_entry(void **entry, void *user)
{
	struct isl_union_map_gen_bin_data *data = user;
	uint32_t hash;
	struct isl_hash_table_entry *entry2;
	isl_map *map = *entry;

	hash = isl_dim_get_hash(map->dim);
	entry2 = isl_hash_table_find(data->umap2->dim->ctx, &data->umap2->table,
				     hash, &has_dim, map->dim, 0);
	map = isl_map_copy(map);
	if (entry2) {
		int empty;
		map = isl_map_subtract(map, isl_map_copy(entry2->data));

		empty = isl_map_is_empty(map);
		if (empty < 0) {
			isl_map_free(map);
			return -1;
		}
		if (empty) {
			isl_map_free(map);
			return 0;
		}
	}
	data->res = isl_union_map_add_map(data->res, map);

	return 0;
}

static __isl_give isl_union_map *gen_bin_op(__isl_take isl_union_map *umap1,
	__isl_take isl_union_map *umap2, int (*fn)(void **, void *))
{
	struct isl_union_map_gen_bin_data data = { NULL, NULL };

	umap1 = isl_union_map_align_params(umap1, isl_union_map_get_dim(umap2));
	umap2 = isl_union_map_align_params(umap2, isl_union_map_get_dim(umap1));

	if (!umap1 || !umap2)
		goto error;

	data.umap2 = umap2;
	data.res = isl_union_map_alloc(isl_dim_copy(umap1->dim),
				       umap1->table.n);
	if (isl_hash_table_foreach(umap1->dim->ctx, &umap1->table,
				   fn, &data) < 0)
		goto error;

	isl_union_map_free(umap1);
	isl_union_map_free(umap2);
	return data.res;
error:
	isl_union_map_free(umap1);
	isl_union_map_free(umap2);
	isl_union_map_free(data.res);
	return NULL;
}

__isl_give isl_union_map *isl_union_map_subtract(
	__isl_take isl_union_map *umap1, __isl_take isl_union_map *umap2)
{
	return gen_bin_op(umap1, umap2, &subtract_entry);
}

__isl_give isl_union_set *isl_union_set_subtract(
	__isl_take isl_union_set *uset1, __isl_take isl_union_set *uset2)
{
	return isl_union_map_subtract(uset1, uset2);
}

struct isl_union_map_match_bin_data {
	isl_union_map *umap2;
	isl_union_map *res;
	__isl_give isl_map *(*fn)(__isl_take isl_map*, __isl_take isl_map*);
};

static int match_bin_entry(void **entry, void *user)
{
	struct isl_union_map_match_bin_data *data = user;
	uint32_t hash;
	struct isl_hash_table_entry *entry2;
	isl_map *map = *entry;
	int empty;

	hash = isl_dim_get_hash(map->dim);
	entry2 = isl_hash_table_find(data->umap2->dim->ctx, &data->umap2->table,
				     hash, &has_dim, map->dim, 0);
	if (!entry2)
		return 0;

	map = isl_map_copy(map);
	map = data->fn(map, isl_map_copy(entry2->data));

	empty = isl_map_is_empty(map);
	if (empty < 0) {
		isl_map_free(map);
		return -1;
	}
	if (empty) {
		isl_map_free(map);
		return 0;
	}

	data->res = isl_union_map_add_map(data->res, map);

	return 0;
}

static __isl_give isl_union_map *match_bin_op(__isl_take isl_union_map *umap1,
	__isl_take isl_union_map *umap2,
	__isl_give isl_map *(*fn)(__isl_take isl_map*, __isl_take isl_map*))
{
	struct isl_union_map_match_bin_data data = { NULL, NULL, fn };

	umap1 = isl_union_map_align_params(umap1, isl_union_map_get_dim(umap2));
	umap2 = isl_union_map_align_params(umap2, isl_union_map_get_dim(umap1));

	if (!umap1 || !umap2)
		goto error;

	data.umap2 = umap2;
	data.res = isl_union_map_alloc(isl_dim_copy(umap1->dim),
				       umap1->table.n);
	if (isl_hash_table_foreach(umap1->dim->ctx, &umap1->table,
				   &match_bin_entry, &data) < 0)
		goto error;

	isl_union_map_free(umap1);
	isl_union_map_free(umap2);
	return data.res;
error:
	isl_union_map_free(umap1);
	isl_union_map_free(umap2);
	isl_union_map_free(data.res);
	return NULL;
}

__isl_give isl_union_map *isl_union_map_intersect(
	__isl_take isl_union_map *umap1, __isl_take isl_union_map *umap2)
{
	return match_bin_op(umap1, umap2, &isl_map_intersect);
}

__isl_give isl_union_set *isl_union_set_intersect(
	__isl_take isl_union_set *uset1, __isl_take isl_union_set *uset2)
{
	return isl_union_map_intersect(uset1, uset2);
}

__isl_give isl_union_map *isl_union_map_gist(__isl_take isl_union_map *umap,
	__isl_take isl_union_map *context)
{
	return match_bin_op(umap, context, &isl_map_gist);
}

__isl_give isl_union_set *isl_union_set_gist(__isl_take isl_union_set *uset,
	__isl_take isl_union_set *context)
{
	return isl_union_map_gist(uset, context);
}

static __isl_give isl_map *lex_le_set(__isl_take isl_map *set1,
	__isl_take isl_map *set2)
{
	return isl_set_lex_le_set((isl_set *)set1, (isl_set *)set2);
}

static __isl_give isl_map *lex_lt_set(__isl_take isl_map *set1,
	__isl_take isl_map *set2)
{
	return isl_set_lex_lt_set((isl_set *)set1, (isl_set *)set2);
}

__isl_give isl_union_map *isl_union_set_lex_lt_union_set(
	__isl_take isl_union_set *uset1, __isl_take isl_union_set *uset2)
{
	return match_bin_op(uset1, uset2, &lex_lt_set);
}

__isl_give isl_union_map *isl_union_set_lex_le_union_set(
	__isl_take isl_union_set *uset1, __isl_take isl_union_set *uset2)
{
	return match_bin_op(uset1, uset2, &lex_le_set);
}

__isl_give isl_union_map *isl_union_set_lex_gt_union_set(
	__isl_take isl_union_set *uset1, __isl_take isl_union_set *uset2)
{
	return isl_union_map_reverse(isl_union_set_lex_lt_union_set(uset2, uset1));
}

__isl_give isl_union_map *isl_union_set_lex_ge_union_set(
	__isl_take isl_union_set *uset1, __isl_take isl_union_set *uset2)
{
	return isl_union_map_reverse(isl_union_set_lex_le_union_set(uset2, uset1));
}

__isl_give isl_union_map *isl_union_map_lex_gt_union_map(
	__isl_take isl_union_map *umap1, __isl_take isl_union_map *umap2)
{
	return isl_union_map_reverse(isl_union_map_lex_lt_union_map(umap2, umap1));
}

__isl_give isl_union_map *isl_union_map_lex_ge_union_map(
	__isl_take isl_union_map *umap1, __isl_take isl_union_map *umap2)
{
	return isl_union_map_reverse(isl_union_map_lex_le_union_map(umap2, umap1));
}

static int intersect_domain_entry(void **entry, void *user)
{
	struct isl_union_map_gen_bin_data *data = user;
	uint32_t hash;
	struct isl_hash_table_entry *entry2;
	isl_dim *dim;
	isl_map *map = *entry;
	int empty;

	dim = isl_map_get_dim(map);
	dim = isl_dim_domain(dim);
	hash = isl_dim_get_hash(dim);
	entry2 = isl_hash_table_find(data->umap2->dim->ctx, &data->umap2->table,
				     hash, &has_dim, dim, 0);
	isl_dim_free(dim);
	if (!entry2)
		return 0;

	map = isl_map_copy(map);
	map = isl_map_intersect_domain(map, isl_set_copy(entry2->data));

	empty = isl_map_is_empty(map);
	if (empty < 0) {
		isl_map_free(map);
		return -1;
	}
	if (empty) {
		isl_map_free(map);
		return 0;
	}

	data->res = isl_union_map_add_map(data->res, map);

	return 0;
}

__isl_give isl_union_map *isl_union_map_intersect_domain(
	__isl_take isl_union_map *umap, __isl_take isl_union_set *uset)
{
	return gen_bin_op(umap, uset, &intersect_domain_entry);
}

static int intersect_range_entry(void **entry, void *user)
{
	struct isl_union_map_gen_bin_data *data = user;
	uint32_t hash;
	struct isl_hash_table_entry *entry2;
	isl_dim *dim;
	isl_map *map = *entry;
	int empty;

	dim = isl_map_get_dim(map);
	dim = isl_dim_range(dim);
	hash = isl_dim_get_hash(dim);
	entry2 = isl_hash_table_find(data->umap2->dim->ctx, &data->umap2->table,
				     hash, &has_dim, dim, 0);
	isl_dim_free(dim);
	if (!entry2)
		return 0;

	map = isl_map_copy(map);
	map = isl_map_intersect_range(map, isl_set_copy(entry2->data));

	empty = isl_map_is_empty(map);
	if (empty < 0) {
		isl_map_free(map);
		return -1;
	}
	if (empty) {
		isl_map_free(map);
		return 0;
	}

	data->res = isl_union_map_add_map(data->res, map);

	return 0;
}

__isl_give isl_union_map *isl_union_map_intersect_range(
	__isl_take isl_union_map *umap, __isl_take isl_union_set *uset)
{
	return gen_bin_op(umap, uset, &intersect_range_entry);
}

struct isl_union_map_bin_data {
	isl_union_map *umap2;
	isl_union_map *res;
	isl_map *map;
	int (*fn)(void **entry, void *user);
};

static int apply_range_entry(void **entry, void *user)
{
	struct isl_union_map_bin_data *data = user;
	isl_map *map2 = *entry;
	int empty;

	if (!isl_dim_tuple_match(data->map->dim, isl_dim_out,
				 map2->dim, isl_dim_in))
		return 0;

	map2 = isl_map_apply_range(isl_map_copy(data->map), isl_map_copy(map2));

	empty = isl_map_is_empty(map2);
	if (empty < 0) {
		isl_map_free(map2);
		return -1;
	}
	if (empty) {
		isl_map_free(map2);
		return 0;
	}

	data->res = isl_union_map_add_map(data->res, map2);

	return 0;
}

static int bin_entry(void **entry, void *user)
{
	struct isl_union_map_bin_data *data = user;
	isl_map *map = *entry;

	data->map = map;
	if (isl_hash_table_foreach(data->umap2->dim->ctx, &data->umap2->table,
				   data->fn, data) < 0)
		return -1;

	return 0;
}

static __isl_give isl_union_map *bin_op(__isl_take isl_union_map *umap1,
	__isl_take isl_union_map *umap2, int (*fn)(void **entry, void *user))
{
	struct isl_union_map_bin_data data = { NULL, NULL, NULL, fn };

	umap1 = isl_union_map_align_params(umap1, isl_union_map_get_dim(umap2));
	umap2 = isl_union_map_align_params(umap2, isl_union_map_get_dim(umap1));

	if (!umap1 || !umap2)
		goto error;

	data.umap2 = umap2;
	data.res = isl_union_map_alloc(isl_dim_copy(umap1->dim),
				       umap1->table.n);
	if (isl_hash_table_foreach(umap1->dim->ctx, &umap1->table,
				   &bin_entry, &data) < 0)
		goto error;

	isl_union_map_free(umap1);
	isl_union_map_free(umap2);
	return data.res;
error:
	isl_union_map_free(umap1);
	isl_union_map_free(umap2);
	isl_union_map_free(data.res);
	return NULL;
}

__isl_give isl_union_map *isl_union_map_apply_range(
	__isl_take isl_union_map *umap1, __isl_take isl_union_map *umap2)
{
	return bin_op(umap1, umap2, &apply_range_entry);
}

__isl_give isl_union_map *isl_union_map_apply_domain(
	__isl_take isl_union_map *umap1, __isl_take isl_union_map *umap2)
{
	umap1 = isl_union_map_reverse(umap1);
	umap1 = isl_union_map_apply_range(umap1, umap2);
	return isl_union_map_reverse(umap1);
}

__isl_give isl_union_set *isl_union_set_apply(
	__isl_take isl_union_set *uset, __isl_take isl_union_map *umap)
{
	return isl_union_map_apply_range(uset, umap);
}

static int map_lex_lt_entry(void **entry, void *user)
{
	struct isl_union_map_bin_data *data = user;
	isl_map *map2 = *entry;

	if (!isl_dim_tuple_match(data->map->dim, isl_dim_out,
				 map2->dim, isl_dim_out))
		return 0;

	map2 = isl_map_lex_lt_map(isl_map_copy(data->map), isl_map_copy(map2));

	data->res = isl_union_map_add_map(data->res, map2);

	return 0;
}

__isl_give isl_union_map *isl_union_map_lex_lt_union_map(
	__isl_take isl_union_map *umap1, __isl_take isl_union_map *umap2)
{
	return bin_op(umap1, umap2, &map_lex_lt_entry);
}

static int map_lex_le_entry(void **entry, void *user)
{
	struct isl_union_map_bin_data *data = user;
	isl_map *map2 = *entry;

	if (!isl_dim_tuple_match(data->map->dim, isl_dim_out,
				 map2->dim, isl_dim_out))
		return 0;

	map2 = isl_map_lex_le_map(isl_map_copy(data->map), isl_map_copy(map2));

	data->res = isl_union_map_add_map(data->res, map2);

	return 0;
}

__isl_give isl_union_map *isl_union_map_lex_le_union_map(
	__isl_take isl_union_map *umap1, __isl_take isl_union_map *umap2)
{
	return bin_op(umap1, umap2, &map_lex_le_entry);
}

static int product_entry(void **entry, void *user)
{
	struct isl_union_map_bin_data *data = user;
	isl_map *map2 = *entry;

	map2 = isl_map_product(isl_map_copy(data->map), isl_map_copy(map2));

	data->res = isl_union_map_add_map(data->res, map2);

	return 0;
}

__isl_give isl_union_map *isl_union_map_product(__isl_take isl_union_map *umap1,
	__isl_take isl_union_map *umap2)
{
	return bin_op(umap1, umap2, &product_entry);
}

__isl_give isl_union_set *isl_union_set_product(__isl_take isl_union_set *uset1,
	__isl_take isl_union_set *uset2)
{
	return isl_union_map_product(uset1, uset2);
}

static int range_product_entry(void **entry, void *user)
{
	struct isl_union_map_bin_data *data = user;
	isl_map *map2 = *entry;

	map2 = isl_map_range_product(isl_map_copy(data->map),
				     isl_map_copy(map2));

	data->res = isl_union_map_add_map(data->res, map2);

	return 0;
}

__isl_give isl_union_map *isl_union_map_range_product(
	__isl_take isl_union_map *umap1, __isl_take isl_union_map *umap2)
{
	return bin_op(umap1, umap2, &range_product_entry);
}

__isl_give isl_union_map *isl_union_map_from_range(
	__isl_take isl_union_set *uset)
{
	return uset;
}

__isl_give isl_union_map *isl_union_map_from_domain(
	__isl_take isl_union_set *uset)
{
	return isl_union_map_reverse(isl_union_map_from_range(uset));
}

__isl_give isl_union_map *isl_union_map_from_domain_and_range(
	__isl_take isl_union_set *domain, __isl_take isl_union_set *range)
{
	return isl_union_map_apply_range(isl_union_map_from_domain(domain),
				         isl_union_map_from_range(range));
}

static __isl_give isl_union_map *un_op(__isl_take isl_union_map *umap,
	int (*fn)(void **, void *))
{
	umap = isl_union_map_cow(umap);
	if (!umap)
		return NULL;

	if (isl_hash_table_foreach(umap->dim->ctx, &umap->table, fn, NULL) < 0)
		goto error;

	return umap;
error:
	isl_union_map_free(umap);
	return NULL;
}

static int affine_entry(void **entry, void *user)
{
	isl_map **map = (isl_map **)entry;

	*map = isl_map_from_basic_map(isl_map_affine_hull(*map));

	return *map ? 0 : -1;
}

__isl_give isl_union_map *isl_union_map_affine_hull(
	__isl_take isl_union_map *umap)
{
	return un_op(umap, &affine_entry);
}

__isl_give isl_union_set *isl_union_set_affine_hull(
	__isl_take isl_union_set *uset)
{
	return isl_union_map_affine_hull(uset);
}

static int polyhedral_entry(void **entry, void *user)
{
	isl_map **map = (isl_map **)entry;

	*map = isl_map_from_basic_map(isl_map_polyhedral_hull(*map));

	return *map ? 0 : -1;
}

__isl_give isl_union_map *isl_union_map_polyhedral_hull(
	__isl_take isl_union_map *umap)
{
	return un_op(umap, &polyhedral_entry);
}

__isl_give isl_union_set *isl_union_set_polyhedral_hull(
	__isl_take isl_union_set *uset)
{
	return isl_union_map_polyhedral_hull(uset);
}

static int simple_entry(void **entry, void *user)
{
	isl_map **map = (isl_map **)entry;

	*map = isl_map_from_basic_map(isl_map_simple_hull(*map));

	return *map ? 0 : -1;
}

__isl_give isl_union_map *isl_union_map_simple_hull(
	__isl_take isl_union_map *umap)
{
	return un_op(umap, &simple_entry);
}

__isl_give isl_union_set *isl_union_set_simple_hull(
	__isl_take isl_union_set *uset)
{
	return isl_union_map_simple_hull(uset);
}

static int inplace_entry(void **entry, void *user)
{
	__isl_give isl_map *(*fn)(__isl_take isl_map *);
	isl_map **map = (isl_map **)entry;
	isl_map *copy;

	fn = *(__isl_give isl_map *(**)(__isl_take isl_map *)) user;
	copy = fn(isl_map_copy(*map));
	if (!copy)
		return -1;

	isl_map_free(*map);
	*map = copy;

	return 0;
}

static __isl_give isl_union_map *inplace(__isl_take isl_union_map *umap,
	__isl_give isl_map *(*fn)(__isl_take isl_map *))
{
	if (!umap)
		return NULL;

	if (isl_hash_table_foreach(umap->dim->ctx, &umap->table,
				    &inplace_entry, &fn) < 0)
		goto error;

	return umap;
error:
	isl_union_map_free(umap);
	return NULL;
}

__isl_give isl_union_map *isl_union_map_coalesce(
	__isl_take isl_union_map *umap)
{
	return inplace(umap, &isl_map_coalesce);
}

__isl_give isl_union_set *isl_union_set_coalesce(
	__isl_take isl_union_set *uset)
{
	return isl_union_map_coalesce(uset);
}

__isl_give isl_union_map *isl_union_map_detect_equalities(
	__isl_take isl_union_map *umap)
{
	return inplace(umap, &isl_map_detect_equalities);
}

__isl_give isl_union_set *isl_union_set_detect_equalities(
	__isl_take isl_union_set *uset)
{
	return isl_union_map_detect_equalities(uset);
}

__isl_give isl_union_map *isl_union_map_compute_divs(
	__isl_take isl_union_map *umap)
{
	return inplace(umap, &isl_map_compute_divs);
}

__isl_give isl_union_set *isl_union_set_compute_divs(
	__isl_take isl_union_set *uset)
{
	return isl_union_map_compute_divs(uset);
}

static int lexmin_entry(void **entry, void *user)
{
	isl_map **map = (isl_map **)entry;

	*map = isl_map_lexmin(*map);

	return *map ? 0 : -1;
}

__isl_give isl_union_map *isl_union_map_lexmin(
	__isl_take isl_union_map *umap)
{
	return un_op(umap, &lexmin_entry);
}

__isl_give isl_union_set *isl_union_set_lexmin(
	__isl_take isl_union_set *uset)
{
	return isl_union_map_lexmin(uset);
}

static int lexmax_entry(void **entry, void *user)
{
	isl_map **map = (isl_map **)entry;

	*map = isl_map_lexmax(*map);

	return *map ? 0 : -1;
}

__isl_give isl_union_map *isl_union_map_lexmax(
	__isl_take isl_union_map *umap)
{
	return un_op(umap, &lexmax_entry);
}

__isl_give isl_union_set *isl_union_set_lexmax(
	__isl_take isl_union_set *uset)
{
	return isl_union_map_lexmax(uset);
}

static __isl_give isl_union_set *cond_un_op(__isl_take isl_union_map *umap,
	int (*fn)(void **, void *))
{
	isl_union_set *res;

	if (!umap)
		return NULL;

	res = isl_union_map_alloc(isl_dim_copy(umap->dim), umap->table.n);
	if (isl_hash_table_foreach(umap->dim->ctx, &umap->table, fn, &res) < 0)
		goto error;

	isl_union_map_free(umap);
	return res;
error:
	isl_union_map_free(umap);
	isl_union_set_free(res);
	return NULL;
}

static int reverse_entry(void **entry, void *user)
{
	isl_map *map = *entry;
	isl_union_map **res = user;

	*res = isl_union_map_add_map(*res, isl_map_reverse(isl_map_copy(map)));

	return 0;
}

__isl_give isl_union_map *isl_union_map_reverse(__isl_take isl_union_map *umap)
{
	return cond_un_op(umap, &reverse_entry);
}

static int domain_entry(void **entry, void *user)
{
	isl_map *map = *entry;
	isl_union_set **res = user;

	*res = isl_union_set_add_set(*res, isl_map_domain(isl_map_copy(map)));

	return 0;
}

__isl_give isl_union_set *isl_union_map_domain(__isl_take isl_union_map *umap)
{
	return cond_un_op(umap, &domain_entry);
}

static int range_entry(void **entry, void *user)
{
	isl_map *map = *entry;
	isl_union_set **res = user;

	*res = isl_union_set_add_set(*res, isl_map_range(isl_map_copy(map)));

	return 0;
}

__isl_give isl_union_set *isl_union_map_range(__isl_take isl_union_map *umap)
{
	return cond_un_op(umap, &range_entry);
}

static int domain_map_entry(void **entry, void *user)
{
	isl_map *map = *entry;
	isl_union_set **res = user;

	*res = isl_union_map_add_map(*res,
					isl_map_domain_map(isl_map_copy(map)));

	return 0;
}

__isl_give isl_union_map *isl_union_map_domain_map(
	__isl_take isl_union_map *umap)
{
	return cond_un_op(umap, &domain_map_entry);
}

static int range_map_entry(void **entry, void *user)
{
	isl_map *map = *entry;
	isl_union_set **res = user;

	*res = isl_union_map_add_map(*res,
					isl_map_range_map(isl_map_copy(map)));

	return 0;
}

__isl_give isl_union_map *isl_union_map_range_map(
	__isl_take isl_union_map *umap)
{
	return cond_un_op(umap, &range_map_entry);
}

static int deltas_entry(void **entry, void *user)
{
	isl_map *map = *entry;
	isl_union_set **res = user;

	if (!isl_dim_tuple_match(map->dim, isl_dim_in, map->dim, isl_dim_out))
		return 0;

	*res = isl_union_set_add_set(*res, isl_map_deltas(isl_map_copy(map)));

	return 0;
}

__isl_give isl_union_set *isl_union_map_deltas(__isl_take isl_union_map *umap)
{
	return cond_un_op(umap, &deltas_entry);
}

static int deltas_map_entry(void **entry, void *user)
{
	isl_map *map = *entry;
	isl_union_map **res = user;

	if (!isl_dim_tuple_match(map->dim, isl_dim_in, map->dim, isl_dim_out))
		return 0;

	*res = isl_union_map_add_map(*res,
				     isl_map_deltas_map(isl_map_copy(map)));

	return 0;
}

__isl_give isl_union_map *isl_union_map_deltas_map(
	__isl_take isl_union_map *umap)
{
	return cond_un_op(umap, &deltas_map_entry);
}

static int identity_entry(void **entry, void *user)
{
	isl_set *set = *entry;
	isl_union_map **res = user;

	*res = isl_union_map_add_map(*res, isl_set_identity(isl_set_copy(set)));

	return 0;
}

__isl_give isl_union_map *isl_union_set_identity(__isl_take isl_union_set *uset)
{
	return cond_un_op(uset, &identity_entry);
}

static int unwrap_entry(void **entry, void *user)
{
	isl_set *set = *entry;
	isl_union_set **res = user;

	if (!isl_set_is_wrapping(set))
		return 0;

	*res = isl_union_map_add_map(*res, isl_set_unwrap(isl_set_copy(set)));

	return 0;
}

__isl_give isl_union_map *isl_union_set_unwrap(__isl_take isl_union_set *uset)
{
	return cond_un_op(uset, &unwrap_entry);
}

static int wrap_entry(void **entry, void *user)
{
	isl_map *map = *entry;
	isl_union_set **res = user;

	*res = isl_union_set_add_set(*res, isl_map_wrap(isl_map_copy(map)));

	return 0;
}

__isl_give isl_union_set *isl_union_map_wrap(__isl_take isl_union_map *umap)
{
	return cond_un_op(umap, &wrap_entry);
}

struct isl_union_map_is_subset_data {
	isl_union_map *umap2;
	int is_subset;
};

static int is_subset_entry(void **entry, void *user)
{
	struct isl_union_map_is_subset_data *data = user;
	uint32_t hash;
	struct isl_hash_table_entry *entry2;
	isl_map *map = *entry;

	hash = isl_dim_get_hash(map->dim);
	entry2 = isl_hash_table_find(data->umap2->dim->ctx, &data->umap2->table,
				     hash, &has_dim, map->dim, 0);
	if (!entry2) {
		data->is_subset = 0;
		return -1;
	}

	data->is_subset = isl_map_is_subset(map, entry2->data);
	if (data->is_subset < 0 || !data->is_subset)
		return -1;

	return 0;
}

int isl_union_map_is_subset(__isl_keep isl_union_map *umap1,
	__isl_keep isl_union_map *umap2)
{
	struct isl_union_map_is_subset_data data = { NULL, 1 };

	umap1 = isl_union_map_copy(umap1);
	umap2 = isl_union_map_copy(umap2);
	umap1 = isl_union_map_align_params(umap1, isl_union_map_get_dim(umap2));
	umap2 = isl_union_map_align_params(umap2, isl_union_map_get_dim(umap1));

	if (!umap1 || !umap2)
		goto error;

	data.umap2 = umap2;
	if (isl_hash_table_foreach(umap1->dim->ctx, &umap1->table,
				   &is_subset_entry, &data) < 0 &&
	    data.is_subset)
		goto error;

	isl_union_map_free(umap1);
	isl_union_map_free(umap2);

	return data.is_subset;
error:
	isl_union_map_free(umap1);
	isl_union_map_free(umap2);
	return -1;
}

int isl_union_set_is_subset(__isl_keep isl_union_set *uset1,
	__isl_keep isl_union_set *uset2)
{
	return isl_union_map_is_subset(uset1, uset2);
}

int isl_union_map_is_equal(__isl_keep isl_union_map *umap1,
	__isl_keep isl_union_map *umap2)
{
	int is_subset;

	if (!umap1 || !umap2)
		return -1;
	is_subset = isl_union_map_is_subset(umap1, umap2);
	if (is_subset != 1)
		return is_subset;
	is_subset = isl_union_map_is_subset(umap2, umap1);
	return is_subset;
}

int isl_union_set_is_equal(__isl_keep isl_union_set *uset1,
	__isl_keep isl_union_set *uset2)
{
	return isl_union_map_is_equal(uset1, uset2);
}

int isl_union_map_is_strict_subset(__isl_keep isl_union_map *umap1,
	__isl_keep isl_union_map *umap2)
{
	int is_subset;

	if (!umap1 || !umap2)
		return -1;
	is_subset = isl_union_map_is_subset(umap1, umap2);
	if (is_subset != 1)
		return is_subset;
	is_subset = isl_union_map_is_subset(umap2, umap1);
	if (is_subset == -1)
		return is_subset;
	return !is_subset;
}

int isl_union_set_is_strict_subset(__isl_keep isl_union_set *uset1,
	__isl_keep isl_union_set *uset2)
{
	return isl_union_map_is_strict_subset(uset1, uset2);
}

static int sample_entry(void **entry, void *user)
{
	isl_basic_map **sample = (isl_basic_map **)user;
	isl_map *map = *entry;

	*sample = isl_map_sample(isl_map_copy(map));
	if (!*sample)
		return -1;
	if (!isl_basic_map_fast_is_empty(*sample))
		return -1;
	return 0;
}

__isl_give isl_basic_map *isl_union_map_sample(__isl_take isl_union_map *umap)
{
	isl_basic_map *sample = NULL;

	if (!umap)
		return NULL;

	if (isl_hash_table_foreach(umap->dim->ctx, &umap->table,
				   &sample_entry, &sample) < 0 &&
	    !sample)
		goto error;

	if (!sample)
		sample = isl_basic_map_empty(isl_union_map_get_dim(umap));

	isl_union_map_free(umap);

	return sample;
error:
	isl_union_map_free(umap);
	return NULL;
}

__isl_give isl_basic_set *isl_union_set_sample(__isl_take isl_union_set *uset)
{
	return (isl_basic_set *)isl_union_map_sample(uset);
}

static int empty_entry(void **entry, void *user)
{
	int *empty = user;
	isl_map *map = *entry;

	if (isl_map_is_empty(map))
		return 0;

	*empty = 0;

	return -1;
}

__isl_give int isl_union_map_is_empty(__isl_keep isl_union_map *umap)
{
	int empty = 1;

	if (!umap)
		return -1;

	if (isl_hash_table_foreach(umap->dim->ctx, &umap->table,
				   &empty_entry, &empty) < 0 && empty)
		return -1;

	return empty;
}

int isl_union_set_is_empty(__isl_keep isl_union_set *uset)
{
	return isl_union_map_is_empty(uset);
}

static int zip_entry(void **entry, void *user)
{
	isl_map *map = *entry;
	isl_union_map **res = user;

	if (!isl_map_can_zip(map))
		return 0;

	*res = isl_union_map_add_map(*res, isl_map_zip(isl_map_copy(map)));

	return 0;
}

__isl_give isl_union_map *isl_union_map_zip(__isl_take isl_union_map *umap)
{
	return cond_un_op(umap, &zip_entry);
}
