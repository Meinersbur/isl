/*
 * Copyright 2008-2009 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv2.1 license
 *
 * Written by Sven Verdoolaege, K.U.Leuven, Departement
 * Computerwetenschappen, Celestijnenlaan 200A, B-3001 Leuven, Belgium
 */

#ifndef ISL_DIV_H
#define ISL_DIV_H

#include "isl_dim.h"
#include "isl_set.h"

#if defined(__cplusplus)
extern "C" {
#endif

struct isl_div {
	int ref;
	struct isl_ctx *ctx;

	struct isl_basic_map	*bmap;
	isl_int			**line;
};
typedef struct isl_div isl_div;

struct isl_div *isl_div_alloc(struct isl_dim *dim);
struct isl_div *isl_basic_map_div(struct isl_basic_map *bmap, int pos);
struct isl_div *isl_basic_set_div(struct isl_basic_set *bset, int pos);
void isl_div_free(struct isl_div *c);

void isl_div_get_constant(__isl_keep isl_div *div, isl_int *v);
void isl_div_get_denominator(__isl_keep isl_div *div, isl_int *v);
void isl_div_get_coefficient(__isl_keep isl_div *div,
	enum isl_dim_type type, int pos, isl_int *v);
void isl_div_set_constant(struct isl_div *div, isl_int v);
void isl_div_set_denominator(struct isl_div *div, isl_int v);
void isl_div_set_coefficient(struct isl_div *div,
	enum isl_dim_type type, int pos, isl_int v);

#if defined(__cplusplus)
}
#endif

#endif
