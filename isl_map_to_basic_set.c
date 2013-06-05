/*
 * Copyright 2011      INRIA Saclay
 *
 * Use of this software is governed by the MIT license
 *
 * Written by Sven Verdoolaege, INRIA Saclay - Ile-de-France,
 * Parc Club Orsay Universite, ZAC des vignes, 4 rue Jacques Monod,
 * 91893 Orsay, France
 */

#include <isl_map_to_basic_set.h>

struct isl_map_basic_set_pair {
	isl_map		*key;
	isl_basic_set	*val;
};

__isl_give isl_map_to_basic_set *isl_map_to_basic_set_alloc(isl_ctx *ctx,
	int min_size)
{
	isl_map_to_basic_set *hmap;

	hmap = isl_calloc_type(ctx, isl_map_to_basic_set);
	if (!hmap)
		return NULL;

	hmap->ctx = ctx;
	isl_ctx_ref(ctx);

	if (isl_hash_table_init(ctx, &hmap->table, min_size) < 0)
		return isl_map_to_basic_set_free(hmap);

	return hmap;
}

static int free_pair(void **entry, void *user)
{
	struct isl_map_basic_set_pair *pair = *entry;
	isl_map_free(pair->key);
	isl_basic_set_free(pair->val);
	free(pair);
	*entry = NULL;
	return 0;
}

void *isl_map_to_basic_set_free(__isl_take isl_map_to_basic_set *hmap)
{
	if (!hmap)
		return NULL;
	isl_hash_table_foreach(hmap->ctx, &hmap->table, &free_pair, NULL);
	isl_hash_table_clear(&hmap->table);
	isl_ctx_deref(hmap->ctx);
	free(hmap);
	return NULL;
}

isl_ctx *isl_map_to_basic_set_get_ctx(__isl_keep isl_map_to_basic_set *hmap)
{
	return hmap ? hmap->ctx : NULL;
}

static int has_key(const void *entry, const void *key)
{
	const struct isl_map_basic_set_pair *pair = entry;
	isl_map *map = (isl_map *)key;

	return isl_map_plain_is_equal(pair->key, map);
}

int isl_map_to_basic_set_has(__isl_keep isl_map_to_basic_set *hmap,
	__isl_keep isl_map *key)
{
	uint32_t hash;

	if (!hmap)
		return -1;

	hash = isl_map_get_hash(key);
	return !!isl_hash_table_find(hmap->ctx, &hmap->table, hash,
					&has_key, key, 0);
}

__isl_give isl_basic_set *isl_map_to_basic_set_get(
	__isl_keep isl_map_to_basic_set *hmap, __isl_take isl_map *key)
{
	struct isl_hash_table_entry *entry;
	struct isl_map_basic_set_pair *pair;
	uint32_t hash;

	if (!hmap || !key)
		goto error;

	hash = isl_map_get_hash(key);
	entry = isl_hash_table_find(hmap->ctx, &hmap->table, hash,
					&has_key, key, 0);
	isl_map_free(key);

	if (!entry)
		return NULL;

	pair = entry->data;

	return isl_basic_set_copy(pair->val);
error:
	isl_map_free(key);
	return NULL;
}

int isl_map_to_basic_set_set(__isl_keep isl_map_to_basic_set *hmap,
	__isl_take isl_map *key, __isl_take isl_basic_set *val)
{
	struct isl_hash_table_entry *entry;
	struct isl_map_basic_set_pair *pair;
	uint32_t hash;

	if (!hmap)
		goto error;

	hash = isl_map_get_hash(key);
	entry = isl_hash_table_find(hmap->ctx, &hmap->table, hash,
					&has_key, key, 1);

	if (!entry)
		goto error;

	if (entry->data) {
		pair = entry->data;
		isl_basic_set_free(pair->val);
		pair->val = val;
		isl_map_free(key);
		return 0;
	}

	pair = isl_alloc_type(hmap->ctx, struct isl_map_basic_set_pair);
	if (!pair)
		goto error;

	entry->data = pair;
	pair->key = key;
	pair->val = val;
	return 0;
error:
	isl_map_free(key);
	isl_basic_set_free(val);
	return -1;
}
