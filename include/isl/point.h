#ifndef ISL_POINT_H
#define ISL_POINT_H

#include <stdio.h>
#include <isl/dim.h>

#if defined(__cplusplus)
extern "C" {
#endif

struct isl_point;
typedef struct isl_point isl_point;

isl_ctx *isl_point_get_ctx(__isl_keep isl_point *pnt);
__isl_give isl_dim *isl_point_get_dim(__isl_keep isl_point *pnt);

__isl_give isl_point *isl_point_zero(__isl_take isl_dim *dim);
__isl_give isl_point *isl_point_copy(__isl_keep isl_point *pnt);
void isl_point_free(__isl_take isl_point *pnt);

void isl_point_get_coordinate(__isl_keep isl_point *pnt,
	enum isl_dim_type type, int pos, isl_int *v);
__isl_give isl_point *isl_point_set_coordinate(__isl_take isl_point *pnt,
	enum isl_dim_type type, int pos, isl_int v);

__isl_give isl_point *isl_point_add_ui(__isl_take isl_point *pnt,
	enum isl_dim_type type, int pos, unsigned val);
__isl_give isl_point *isl_point_sub_ui(__isl_take isl_point *pnt,
	enum isl_dim_type type, int pos, unsigned val);

__isl_give isl_point *isl_point_void(__isl_take isl_dim *dim);
int isl_point_is_void(__isl_keep isl_point *pnt);

void isl_point_print(__isl_keep isl_point *pnt, FILE *out);

#if defined(__cplusplus)
}
#endif

#endif
