#ifndef ISL_SAMPLE_H
#define ISL_SAMPLE

#include <isl_set.h>

#if defined(__cplusplus)
extern "C" {
#endif

__isl_give isl_vec *isl_basic_set_sample_vec(__isl_take isl_basic_set *bset);
struct isl_vec *isl_basic_set_sample_bounded(struct isl_basic_set *bset);

__isl_give isl_basic_set *isl_basic_set_from_vec(__isl_take isl_vec *vec);

#if defined(__cplusplus)
}
#endif

#endif
