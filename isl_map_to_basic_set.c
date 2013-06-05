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
	return (isl_map_to_basic_set *) isl_hash_table_alloc(ctx, min_size);
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

void isl_map_to_basic_set_free(isl_ctx *ctx,
	__isl_take isl_map_to_basic_set *hmap)
{
	if (!hmap)
		return;
	isl_hash_table_foreach(ctx, &hmap->table, &free_pair, NULL);
	isl_hash_table_free(ctx, &hmap->table);
}

static int has_key(const void *entry, const void *key)
{
	const struct isl_map_basic_set_pair *pair = entry;
	isl_map *map = (isl_map *)key;

	return isl_map_plain_is_equal(pair->key, map);
}

int isl_map_to_basic_set_has(isl_ctx *ctx,
	__isl_keep isl_map_to_basic_set *hmap, __isl_keep isl_map *key)
{
	uint32_t hash;

	hash = isl_map_get_hash(key);
	return !!isl_hash_table_find(ctx, &hmap->table, hash, &has_key, key, 0);
}

__isl_give isl_basic_set *isl_map_to_basic_set_get(isl_ctx *ctx,
	__isl_keep isl_map_to_basic_set *hmap, __isl_take isl_map *key)
{
	struct isl_hash_table_entry *entry;
	struct isl_map_basic_set_pair *pair;
	uint32_t hash;

	hash = isl_map_get_hash(key);
	entry = isl_hash_table_find(ctx, &hmap->table, hash, &has_key, key, 0);
	isl_map_free(key);

	if (!entry)
		return NULL;

	pair = entry->data;

	return isl_basic_set_copy(pair->val);
}

int isl_map_to_basic_set_set(isl_ctx *ctx,
	__isl_keep isl_map_to_basic_set *hmap, __isl_take isl_map *key,
	__isl_take isl_basic_set *val)
{
	struct isl_hash_table_entry *entry;
	struct isl_map_basic_set_pair *pair;
	uint32_t hash;

	hash = isl_map_get_hash(key);
	entry = isl_hash_table_find(ctx, &hmap->table, hash, &has_key, key, 1);

	if (!entry)
		goto error;

	if (entry->data) {
		pair = entry->data;
		isl_basic_set_free(pair->val);
		pair->val = val;
		isl_map_free(key);
		return 0;
	}

	pair = isl_alloc_type(ctx, struct isl_map_basic_set_pair);
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
