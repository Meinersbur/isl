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

#if defined(__cplusplus)
extern "C" {
#endif

#define ISL_DECLARE_LIST(EL)						\
struct isl_##EL;							\
struct isl_##EL##_list;							\
typedef struct isl_##EL##_list isl_##EL##_list;				\
__isl_give isl_##EL##_list *isl_##EL##_list_alloc(isl_ctx *ctx, int n);	\
__isl_give isl_##EL##_list *isl_##EL##_list_copy(			\
	__isl_keep isl_##EL##_list *list);				\
void isl_##EL##_list_free(__isl_take isl_##EL##_list *list);		\
__isl_give isl_##EL##_list *isl_##EL##_list_add(			\
	__isl_take isl_##EL##_list *list,				\
	__isl_take struct isl_##EL *el);

ISL_DECLARE_LIST(basic_set)
ISL_DECLARE_LIST(set)

#if defined(__cplusplus)
}
#endif

#endif
