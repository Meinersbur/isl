#ifndef ISL_MAP_TYPE_H
#define ISL_MAP_TYPE_H

#include <isl/ctx.h>

#if defined(__cplusplus)
extern "C" {
#endif

__isl_subclass(isl_map)
struct isl_basic_map;
typedef struct isl_basic_map isl_basic_map;
__isl_subclass(isl_union_map)
struct isl_map;
typedef struct isl_map isl_map;

#ifndef isl_basic_set
__isl_subclass(isl_set)
struct isl_basic_set;
typedef struct isl_basic_set isl_basic_set;
#endif
#ifndef isl_set
__isl_subclass(isl_union_set)
struct isl_set;
typedef struct isl_set isl_set;
#endif

#if defined(__cplusplus)
}
#endif

#endif
