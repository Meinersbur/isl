/*
 * Copyright 2008-2009 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv2.1 license
 *
 * Written by Sven Verdoolaege, K.U.Leuven, Departement
 * Computerwetenschappen, Celestijnenlaan 200A, B-3001 Leuven, Belgium
 */

#ifndef ISL_NAME_H
#define ISL_NAME_H

#include <isl/ctx.h>

#if defined(__cplusplus)
extern "C" {
#endif

struct isl_name {
	int ref;

	const char *name;
	uint32_t hash;
};

struct isl_name *isl_name_alloc(struct isl_ctx *ctx, const char *name);
struct isl_name *isl_name_get(struct isl_ctx *ctx, const char *name);
struct isl_name *isl_name_copy(struct isl_ctx *ctx, struct isl_name *name);
uint32_t isl_hash_name(uint32_t hash, struct isl_name *name);
void isl_name_free(struct isl_ctx *ctx, struct isl_name *name);

#if defined(__cplusplus)
}
#endif

#endif
