#ifndef ISL_AFF_H
#define ISL_AFF_H

#include <isl/div.h>
#include <isl/local_space.h>
#include <isl/printer.h>

#if defined(__cplusplus)
extern "C" {
#endif

struct isl_aff;
typedef struct isl_aff isl_aff;

__isl_give isl_aff *isl_aff_zero(__isl_take isl_local_space *ls);

__isl_give isl_aff *isl_aff_copy(__isl_keep isl_aff *aff);
void *isl_aff_free(__isl_take isl_aff *aff);

isl_ctx *isl_aff_get_ctx(__isl_keep isl_aff *aff);

int isl_aff_dim(__isl_keep isl_aff *aff, enum isl_dim_type type);

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
__isl_give isl_aff *isl_aff_add_coefficient_si(__isl_take isl_aff *aff,
	enum isl_dim_type type, int pos, int v);

__isl_give isl_div *isl_aff_get_div(__isl_keep isl_aff *aff, int pos);

__isl_give isl_aff *isl_aff_neg(__isl_take isl_aff *aff);
__isl_give isl_aff *isl_aff_ceil(__isl_take isl_aff *aff);

__isl_give isl_printer *isl_printer_print_aff(__isl_take isl_printer *p,
	__isl_keep isl_aff *aff);
void isl_aff_dump(__isl_keep isl_aff *aff);

#if defined(__cplusplus)
}
#endif

#endif
