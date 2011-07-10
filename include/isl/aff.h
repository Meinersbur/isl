#ifndef ISL_AFF_H
#define ISL_AFF_H

#include <isl/div.h>
#include <isl/local_space.h>
#include <isl/printer.h>
#include <isl/set_type.h>
#include <isl/aff_type.h>

#if defined(__cplusplus)
extern "C" {
#endif

__isl_give isl_aff *isl_aff_zero(__isl_take isl_local_space *ls);

__isl_give isl_aff *isl_aff_copy(__isl_keep isl_aff *aff);
void *isl_aff_free(__isl_take isl_aff *aff);

isl_ctx *isl_aff_get_ctx(__isl_keep isl_aff *aff);

int isl_aff_dim(__isl_keep isl_aff *aff, enum isl_dim_type type);
int isl_aff_involves_dims(__isl_keep isl_aff *aff,
	enum isl_dim_type type, unsigned first, unsigned n);

__isl_give isl_dim *isl_aff_get_dim(__isl_keep isl_aff *aff);
__isl_give isl_local_space *isl_aff_get_local_space(__isl_keep isl_aff *aff);

const char *isl_aff_get_dim_name(__isl_keep isl_aff *aff,
	enum isl_dim_type type, unsigned pos);
int isl_aff_get_constant(__isl_keep isl_aff *aff, isl_int *v);
int isl_aff_get_coefficient(__isl_keep isl_aff *aff,
	enum isl_dim_type type, int pos, isl_int *v);
int isl_aff_get_denominator(__isl_keep isl_aff *aff, isl_int *v);
__isl_give isl_aff *isl_aff_set_constant(__isl_take isl_aff *aff, isl_int v);
__isl_give isl_aff *isl_aff_set_constant_si(__isl_take isl_aff *aff, int v);
__isl_give isl_aff *isl_aff_set_coefficient(__isl_take isl_aff *aff,
	enum isl_dim_type type, int pos, isl_int v);
__isl_give isl_aff *isl_aff_set_coefficient_si(__isl_take isl_aff *aff,
	enum isl_dim_type type, int pos, int v);
__isl_give isl_aff *isl_aff_set_denominator(__isl_take isl_aff *aff, isl_int v);
__isl_give isl_aff *isl_aff_add_constant(__isl_take isl_aff *aff, isl_int v);
__isl_give isl_aff *isl_aff_add_constant_si(__isl_take isl_aff *aff, int v);
__isl_give isl_aff *isl_aff_add_coefficient(__isl_take isl_aff *aff,
	enum isl_dim_type type, int pos, isl_int v);
__isl_give isl_aff *isl_aff_add_coefficient_si(__isl_take isl_aff *aff,
	enum isl_dim_type type, int pos, int v);

__isl_give isl_aff *isl_aff_set_dim_name(__isl_take isl_aff *aff,
	enum isl_dim_type type, unsigned pos, const char *s);

int isl_aff_plain_is_equal(__isl_keep isl_aff *aff1, __isl_keep isl_aff *aff2);
int isl_aff_plain_is_zero(__isl_keep isl_aff *aff);

__isl_give isl_div *isl_aff_get_div(__isl_keep isl_aff *aff, int pos);

__isl_give isl_aff *isl_aff_neg(__isl_take isl_aff *aff);
__isl_give isl_aff *isl_aff_ceil(__isl_take isl_aff *aff);
__isl_give isl_aff *isl_aff_floor(__isl_take isl_aff *aff);

__isl_give isl_aff *isl_aff_add(__isl_take isl_aff *aff1,
	__isl_take isl_aff *aff2);
__isl_give isl_aff *isl_aff_sub(__isl_take isl_aff *aff1,
	__isl_take isl_aff *aff2);

__isl_give isl_aff *isl_aff_scale(__isl_take isl_aff *aff, isl_int f);
__isl_give isl_aff *isl_aff_scale_down(__isl_take isl_aff *aff, isl_int f);
__isl_give isl_aff *isl_aff_scale_down_ui(__isl_take isl_aff *aff, unsigned f);

__isl_give isl_aff *isl_aff_insert_dims(__isl_take isl_aff *aff,
	enum isl_dim_type type, unsigned first, unsigned n);
__isl_give isl_aff *isl_aff_add_dims(__isl_take isl_aff *aff,
	enum isl_dim_type type, unsigned n);
__isl_give isl_aff *isl_aff_drop_dims(__isl_take isl_aff *aff,
	enum isl_dim_type type, unsigned first, unsigned n);

__isl_give isl_aff *isl_aff_gist(__isl_take isl_aff *aff,
	__isl_take isl_set *context);

__isl_give isl_basic_set *isl_aff_ge_basic_set(__isl_take isl_aff *aff1,
	__isl_take isl_aff *aff2);

__isl_give isl_printer *isl_printer_print_aff(__isl_take isl_printer *p,
	__isl_keep isl_aff *aff);
void isl_aff_dump(__isl_keep isl_aff *aff);

isl_ctx *isl_pw_aff_get_ctx(__isl_keep isl_pw_aff *pwaff);
__isl_give isl_dim *isl_pw_aff_get_dim(__isl_keep isl_pw_aff *pwaff);

__isl_give isl_pw_aff *isl_pw_aff_empty(__isl_take isl_dim *dim);
__isl_give isl_pw_aff *isl_pw_aff_alloc(__isl_take isl_set *set,
	__isl_take isl_aff *aff);

int isl_pw_aff_is_empty(__isl_keep isl_pw_aff *pwaff);

__isl_give isl_pw_aff *isl_pw_aff_max(__isl_take isl_pw_aff *pwaff1,
	__isl_take isl_pw_aff *pwaff2);

__isl_give isl_pw_aff *isl_pw_aff_copy(__isl_keep isl_pw_aff *pwaff);
void *isl_pw_aff_free(__isl_take isl_pw_aff *pwaff);

unsigned isl_pw_aff_dim(__isl_keep isl_pw_aff *pwaff, enum isl_dim_type type);
int isl_pw_aff_involves_dims(__isl_keep isl_pw_aff *pwaff,
	enum isl_dim_type type, unsigned first, unsigned n);

__isl_give isl_pw_aff *isl_pw_aff_align_params(__isl_take isl_pw_aff *pwaff,
	__isl_take isl_dim *model);

__isl_give isl_set *isl_pw_aff_domain(__isl_take isl_pw_aff *pwaff);

__isl_give isl_pw_aff *isl_pw_aff_add(__isl_take isl_pw_aff *pwaff1,
	__isl_take isl_pw_aff *pwaff2);
__isl_give isl_pw_aff *isl_pw_aff_sub(__isl_take isl_pw_aff *pwaff1,
	__isl_take isl_pw_aff *pwaff2);
__isl_give isl_pw_aff *isl_pw_aff_neg(__isl_take isl_pw_aff *pwaff);
__isl_give isl_pw_aff *isl_pw_aff_ceil(__isl_take isl_pw_aff *pwaff);
__isl_give isl_pw_aff *isl_pw_aff_floor(__isl_take isl_pw_aff *pwaff);

__isl_give isl_pw_aff *isl_pw_aff_cond(__isl_take isl_set *cond,
	__isl_take isl_pw_aff *pwaff_true, __isl_take isl_pw_aff *pwaff_false);

__isl_give isl_pw_aff *isl_pw_aff_scale(__isl_take isl_pw_aff *pwaff,
	isl_int f);
__isl_give isl_pw_aff *isl_pw_aff_scale_down(__isl_take isl_pw_aff *pwaff,
	isl_int f);

__isl_give isl_pw_aff *isl_pw_aff_insert_dims(__isl_take isl_pw_aff *pwaff,
	enum isl_dim_type type, unsigned first, unsigned n);
__isl_give isl_pw_aff *isl_pw_aff_add_dims(__isl_take isl_pw_aff *pwaff,
	enum isl_dim_type type, unsigned n);
__isl_give isl_pw_aff *isl_pw_aff_drop_dims(__isl_take isl_pw_aff *pwaff,
	enum isl_dim_type type, unsigned first, unsigned n);

__isl_give isl_pw_aff *isl_pw_aff_coalesce(__isl_take isl_pw_aff *pwqp);
__isl_give isl_pw_aff *isl_pw_aff_gist(__isl_take isl_pw_aff *pwaff,
	__isl_take isl_set *context);

__isl_give isl_map *isl_map_from_pw_aff(__isl_take isl_pw_aff *pwaff);

__isl_give isl_set *isl_pw_aff_nonneg_set(__isl_take isl_pw_aff *pwaff);

__isl_give isl_set *isl_pw_aff_eq_set(__isl_take isl_pw_aff *pwaff1,
	__isl_take isl_pw_aff *pwaff2);
__isl_give isl_set *isl_pw_aff_le_set(__isl_take isl_pw_aff *pwaff1,
	__isl_take isl_pw_aff *pwaff2);
__isl_give isl_set *isl_pw_aff_lt_set(__isl_take isl_pw_aff *pwaff1,
	__isl_take isl_pw_aff *pwaff2);
__isl_give isl_set *isl_pw_aff_ge_set(__isl_take isl_pw_aff *pwaff1,
	__isl_take isl_pw_aff *pwaff2);
__isl_give isl_set *isl_pw_aff_gt_set(__isl_take isl_pw_aff *pwaff1,
	__isl_take isl_pw_aff *pwaff2);

__isl_give isl_printer *isl_printer_print_pw_aff(__isl_take isl_printer *p,
	__isl_keep isl_pw_aff *pwaff);
void isl_pw_aff_dump(__isl_keep isl_pw_aff *pwaff);

#if defined(__cplusplus)
}
#endif

#endif
