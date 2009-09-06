#ifndef ISL_MAP_POLYLIB_H
#define ISL_MAP_POLYLIB_H

#include <isl_dim.h>
#include <isl_map.h>
#include <isl_polylib.h>

#if defined(__cplusplus)
extern "C" {
#endif

__isl_give isl_basic_map *isl_basic_map_new_from_polylib(Polyhedron *P,
			__isl_take isl_dim *dim);
__isl_give isl_map *isl_map_new_from_polylib(Polyhedron *D,
			__isl_take isl_dim *dim);
Polyhedron *isl_basic_map_to_polylib(__isl_keep isl_basic_map *bmap);
Polyhedron *isl_map_to_polylib(__isl_keep isl_map *map);

#if defined(__cplusplus)
}
#endif

#endif
