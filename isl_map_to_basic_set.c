/*
 * Copyright 2011      INRIA Saclay
 * Copyright 2013      Ecole Normale Superieure
 *
 * Use of this software is governed by the MIT license
 *
 * Written by Sven Verdoolaege, INRIA Saclay - Ile-de-France,
 * Parc Club Orsay Universite, ZAC des vignes, 4 rue Jacques Monod,
 * 91893 Orsay, France
 * and Ecole Normale Superieure, 45 rue d’Ulm, 75230 Paris, France
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
	hmap->ref = 1;

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
	if (--hmap->ref > 0)
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

/* Add a mapping from "key" to "val" to the associative array
 * pointed to by user.
 */
static int add_key_val(__isl_take isl_map *key, __isl_take isl_basic_set *val,
	void *user)
{
	isl_map_to_basic_set **hmap = (isl_map_to_basic_set **) user;

	*hmap = isl_map_to_basic_set_set(*hmap, key, val);

	if (!*hmap)
		return -1;

	return 0;
}

__isl_give isl_map_to_basic_set *isl_map_to_basic_set_dup(
	__isl_keep isl_map_to_basic_set *hmap)
{
	isl_map_to_basic_set *dup;

	if (!hmap)
		return NULL;

	dup = isl_map_to_basic_set_alloc(hmap->ctx, hmap->table.n);
	if (isl_map_to_basic_set_foreach(hmap, &add_key_val, &dup) < 0)
		return isl_map_to_basic_set_free(dup);

	return dup;
}

__isl_give isl_map_to_basic_set *isl_map_to_basic_set_cow(
	__isl_take isl_map_to_basic_set *hmap)
{
	if (!hmap)
		return NULL;

	if (hmap->ref == 1)
		return hmap;
	hmap->ref--;
	return isl_map_to_basic_set_dup(hmap);
}

__isl_give isl_map_to_basic_set *isl_map_to_basic_set_copy(
	__isl_keep isl_map_to_basic_set *hmap)
{
	if (!hmap)
		return NULL;

	hmap->ref++;
	return hmap;
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

/* Add a mapping from "key" to "val" to "hmap".
 * If "key" was already mapped to something else, then that mapping
 * is replaced.
 * If key happened to be mapped to "val" already, then we leave
 * "hmap" untouched.
 */
__isl_give isl_map_to_basic_set *isl_map_to_basic_set_set(
	__isl_take isl_map_to_basic_set *hmap,
	__isl_take isl_map *key, __isl_take isl_basic_set *val)
{
	struct isl_hash_table_entry *entry;
	struct isl_map_basic_set_pair *pair;
	uint32_t hash;

	if (!hmap)
		goto error;

	hash = isl_map_get_hash(key);
	entry = isl_hash_table_find(hmap->ctx, &hmap->table, hash,
					&has_key, key, 0);
	if (entry) {
		int equal;
		pair = entry->data;
		equal = isl_basic_set_plain_is_equal(pair->val, val);
		if (equal < 0)
			goto error;
		if (equal) {
			isl_map_free(key);
			isl_basic_set_free(val);
			return hmap;
		}
	}

	hmap = isl_map_to_basic_set_cow(hmap);
	if (!hmap)
		goto error;

	entry = isl_hash_table_find(hmap->ctx, &hmap->table, hash,
					&has_key, key, 1);

	if (!entry)
		goto error;

	if (entry->data) {
		pair = entry->data;
		isl_basic_set_free(pair->val);
		pair->val = val;
		isl_map_free(key);
		return hmap;
	}

	pair = isl_alloc_type(hmap->ctx, struct isl_map_basic_set_pair);
	if (!pair)
		goto error;

	entry->data = pair;
	pair->key = key;
	pair->val = val;
	return hmap;
error:
	isl_map_free(key);
	isl_basic_set_free(val);
	return isl_map_to_basic_set_free(hmap);
}

/* Internal data structure for isl_map_to_basic_set_foreach.
 *
 * fn is the function that should be called on each entry.
 * user is the user-specified final argument to fn.
 */
struct isl_map_to_basic_set_foreach_data {
	int (*fn)(__isl_take isl_map *key, __isl_take isl_basic_set *val,
		    void *user);
	void *user;
};

/* Call data->fn on a copy of the key and value in *entry.
 */
static int call_on_copy(void **entry, void *user)
{
	struct isl_map_basic_set_pair *pair = *entry;
	struct isl_map_to_basic_set_foreach_data *data;
	data = (struct isl_map_to_basic_set_foreach_data *) user;

	return data->fn(isl_map_copy(pair->key), isl_basic_set_copy(pair->val),
			data->user);
}

/* Call "fn" on each pair of key and value in "hmap".
 */
int isl_map_to_basic_set_foreach(__isl_keep isl_map_to_basic_set *hmap,
	int (*fn)(__isl_take isl_map *key, __isl_take isl_basic_set *val,
		    void *user), void *user)
{
	struct isl_map_to_basic_set_foreach_data data = { fn, user };

	if (!hmap)
		return -1;

	return isl_hash_table_foreach(hmap->ctx, &hmap->table,
				      &call_on_copy, &data);
}

/* Internal data structure for print_pair.
 *
 * p is the printer on which the associative array is being printed.
 * first is set if the current key-value pair is the first to be printed.
 */
struct isl_map_to_basic_set_print_data {
	isl_printer *p;
	int first;
};

/* Print the given key-value pair to data->p.
 */
static int print_pair(__isl_take isl_map *key, __isl_take isl_basic_set *val,
	void *user)
{
	struct isl_map_to_basic_set_print_data *data = user;

	if (!data->first)
		data->p = isl_printer_print_str(data->p, ", ");
	data->p = isl_printer_print_map(data->p, key);
	data->p = isl_printer_print_str(data->p, ": ");
	data->p = isl_printer_print_basic_set(data->p, val);
	data->first = 0;

	isl_map_free(key);
	isl_basic_set_free(val);
	return 0;
}

/* Print the associative array to "p".
 */
__isl_give isl_printer *isl_printer_print_map_to_basic_set(
	__isl_take isl_printer *p, __isl_keep isl_map_to_basic_set *hmap)
{
	struct isl_map_to_basic_set_print_data data;

	if (!p || !hmap)
		return isl_printer_free(p);

	p = isl_printer_print_str(p, "{");
	data.p = p;
	data.first = 1;
	if (isl_map_to_basic_set_foreach(hmap, &print_pair, &data) < 0)
		data.p = isl_printer_free(data.p);
	p = data.p;
	p = isl_printer_print_str(p, "}");

	return p;
}

void isl_map_to_basic_set_dump(__isl_keep isl_map_to_basic_set *hmap)
{
	isl_printer *printer;

	if (!hmap)
		return;

	printer = isl_printer_to_file(isl_map_to_basic_set_get_ctx(hmap),
					stderr);
	printer = isl_printer_print_map_to_basic_set(printer, hmap);
	printer = isl_printer_end_line(printer);

	isl_printer_free(printer);
}
