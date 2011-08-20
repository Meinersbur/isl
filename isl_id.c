/*
 * Copyright 2008-2009 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv2.1 license
 *
 * Written by Sven Verdoolaege, K.U.Leuven, Departement
 * Computerwetenschappen, Celestijnenlaan 200A, B-3001 Leuven, Belgium
 */

#include <string.h>
#include <isl_ctx_private.h>
#include <isl_id_private.h>

isl_ctx *isl_id_get_ctx(__isl_keep isl_id *id)
{
	return id ? id->ctx : NULL;
}

void *isl_id_get_user(__isl_keep isl_id *id)
{
	return id ? id->user : NULL;
}

const char *isl_id_get_name(__isl_keep isl_id *id)
{
	return id ? id->name : NULL;
}

static __isl_give isl_id *id_alloc(isl_ctx *ctx, const char *name, void *user)
{
	const char *copy = name ? strdup(name) : NULL;
	isl_id *id;

	if (name && !copy)
		return NULL;
	id = isl_alloc_type(ctx, struct isl_id);
	if (!id)
		goto error;

	id->ctx = ctx;
	isl_ctx_ref(id->ctx);
	id->ref = 1;
	id->name = copy;
	id->user = user;

	id->hash = isl_hash_init();
	if (name)
		id->hash = isl_hash_string(id->hash, name);
	else
		id->hash = isl_hash_builtin(id->hash, user);

	return id;
error:
	free((char *)copy);
	return NULL;
}

static int isl_id_has_name(const void *entry, const void *val)
{
	isl_id *id = (isl_id *)entry;
	const char *s = (const char *)val;

	return !strcmp(id->name, s);
}

__isl_give isl_id *isl_id_alloc(isl_ctx *ctx, const char *name, void *user)
{
	struct isl_hash_table_entry *entry;
	uint32_t id_hash;

	id_hash = isl_hash_init();
	if (name)
		id_hash = isl_hash_string(id_hash, name);
	else
		id_hash = isl_hash_builtin(id_hash, user);
	entry = isl_hash_table_find(ctx, &ctx->id_table, id_hash,
					isl_id_has_name, name, 1);
	if (!entry)
		return NULL;
	if (entry->data)
		return isl_id_copy(entry->data);
	entry->data = id_alloc(ctx, name, user);
	if (!entry->data)
		ctx->id_table.n--;
	return entry->data;
}

__isl_give isl_id *isl_id_copy(isl_id *id)
{
	if (!id)
		return NULL;

	id->ref++;
	return id;
}

static int isl_id_eq(const void *entry, const void *name)
{
	return entry == name;
}

uint32_t isl_hash_id(uint32_t hash, __isl_keep isl_id *id)
{
	if (id)
		isl_hash_hash(hash, id->hash);

	return hash;
}

void *isl_id_free(__isl_take isl_id *id)
{
	struct isl_hash_table_entry *entry;

	if (!id)
		return NULL;

	if (--id->ref > 0)
		return NULL;

	entry = isl_hash_table_find(id->ctx, &id->ctx->id_table, id->hash,
					isl_id_eq, id, 0);
	if (!entry)
		isl_die(id->ctx, isl_error_unknown,
			"unable to find id", (void)0);
	else
		isl_hash_table_remove(id->ctx, &id->ctx->id_table, entry);

	free((char *)id->name);
	isl_ctx_deref(id->ctx);
	free(id);

	return NULL;
}

__isl_give isl_printer *isl_printer_print_id(__isl_take isl_printer *p,
	__isl_keep isl_id *id)
{
	if (!id)
		goto error;

	if (id->name)
		p = isl_printer_print_str(p, id->name);
	if (id->user) {
		char buffer[50];
		snprintf(buffer, sizeof(buffer), "@%p", id->user);
		p = isl_printer_print_str(p, buffer);
	}
	return p;
error:
	isl_printer_free(p);
	return NULL;
}
