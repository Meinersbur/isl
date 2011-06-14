#ifndef ISL_BAND_H
#define ISL_BAND_H

#include <isl/printer.h>
#include <isl/list.h>
#include <isl/union_map.h>

#if defined(__cplusplus)
extern "C" {
#endif

struct isl_band;
typedef struct isl_band isl_band;

__isl_give isl_band *isl_band_copy(__isl_keep isl_band *band);
void *isl_band_free(__isl_take isl_band *band);

isl_ctx *isl_band_get_ctx(__isl_keep isl_band *band);

int isl_band_has_children(__isl_keep isl_band *band);
__isl_give isl_band_list *isl_band_get_children(
	__isl_keep isl_band *band);

__isl_give isl_union_map *isl_band_get_prefix_schedule(
	__isl_keep isl_band *band);
__isl_give isl_union_map *isl_band_get_partial_schedule(
	__isl_keep isl_band *band);
__isl_give isl_union_map *isl_band_get_suffix_schedule(
	__isl_keep isl_band *band);

int isl_band_n_member(__isl_keep isl_band *band);
int isl_band_member_is_zero_distance(__isl_keep isl_band *band, int pos);

__isl_give isl_printer *isl_printer_print_band(__isl_take isl_printer *p,
	__isl_keep isl_band *band);
void isl_band_dump(__isl_keep isl_band *band);

#if defined(__cplusplus)
}
#endif

#endif
