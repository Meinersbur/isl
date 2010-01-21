/*
 * Copyright 2008-2009 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv2.1 license
 *
 * Written by Sven Verdoolaege, K.U.Leuven, Departement
 * Computerwetenschappen, Celestijnenlaan 200A, B-3001 Leuven, Belgium
 */

#ifndef ISL_LIST_H
#define ISL_LIST_H

#include <isl/ctx.h>

struct isl_basic_set;

struct isl_basic_set_list {
	int ref;
	struct isl_ctx *ctx;

	int n;

	size_t size;
	struct isl_basic_set *p[1];
};

struct isl_basic_set_list *isl_basic_set_list_alloc(struct isl_ctx *ctx, int n);
void isl_basic_set_list_free(struct isl_basic_set_list *list);
struct isl_basic_set_list *isl_basic_set_list_add(
	struct isl_basic_set_list *list,
	struct isl_basic_set *bset);

#endif
