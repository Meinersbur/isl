#ifndef ISL_MAP_POLYLIB_H
#define ISL_MAP_POLYLIB_H

#include <isl_map.h>
#include <isl_polylib.h>

#if defined(__cplusplus)
extern "C" {
#endif

struct isl_basic_map *isl_basic_map_new_from_polylib(
			struct isl_ctx *ctx, Polyhedron *P,
			unsigned nparam, unsigned in, unsigned out);
struct isl_map *isl_map_new_from_polylib(struct isl_ctx *ctx,
			Polyhedron *D,
			unsigned nparam, unsigned in, unsigned out);
Polyhedron *isl_basic_map_to_polylib(struct isl_basic_map *bmap);
Polyhedron *isl_map_to_polylib(struct isl_map *map);

#if defined(__cplusplus)
}
#endif

#endif
