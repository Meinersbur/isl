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

#include <isl/dim.h>
#include <isl/aff_type.h>

#if defined(__cplusplus)
extern "C" {
#endif

struct isl_div;
typedef struct isl_div isl_div;

isl_ctx *isl_div_get_ctx(__isl_keep isl_div *div);

struct isl_div *isl_div_alloc(struct isl_dim *dim);
__isl_give isl_div *isl_div_copy(__isl_keep isl_div *div);
void isl_div_free(struct isl_div *c);

void isl_div_get_constant(__isl_keep isl_div *div, isl_int *v);
void isl_div_get_denominator(__isl_keep isl_div *div, isl_int *v);
void isl_div_get_coefficient(__isl_keep isl_div *div,
	enum isl_dim_type type, int pos, isl_int *v);
void isl_div_set_constant(struct isl_div *div, isl_int v);
void isl_div_set_denominator(struct isl_div *div, isl_int v);
void isl_div_set_coefficient(struct isl_div *div,
	enum isl_dim_type type, int pos, isl_int v);

unsigned isl_div_dim(__isl_keep isl_div *div, enum isl_dim_type type);
__isl_give isl_div *isl_div_div(__isl_take isl_div *div, int pos);

__isl_give isl_aff *isl_aff_from_div(__isl_take isl_div *div);

#if defined(__cplusplus)
}
#endif

#endif
