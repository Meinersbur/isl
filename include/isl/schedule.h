#ifndef ISL_SCHEDULE_H
#define ISL_SCHEDULE_H

#include <isl/union_set.h>
#include <isl/union_map.h>
#include <isl/list.h>

#if defined(__cplusplus)
extern "C" {
#endif

struct isl_schedule;
typedef struct isl_schedule isl_schedule;

__isl_give isl_schedule *isl_union_set_compute_schedule(
	__isl_take isl_union_set *domain,
	__isl_take isl_union_map *validity,
	__isl_take isl_union_map *proximity);
void *isl_schedule_free(__isl_take isl_schedule *sched);
__isl_give isl_union_map *isl_schedule_get_map(__isl_keep isl_schedule *sched);

isl_ctx *isl_schedule_get_ctx(__isl_keep isl_schedule *sched);

__isl_give isl_band_list *isl_schedule_get_band_forest(
	__isl_keep isl_schedule *schedule);

__isl_give isl_printer *isl_printer_print_schedule(__isl_take isl_printer *p,
	__isl_keep isl_schedule *schedule);
void isl_schedule_dump(__isl_keep isl_schedule *schedule);

#if defined(__cplusplus)
}
#endif

#endif
