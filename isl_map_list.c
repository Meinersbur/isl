#include <isl/map.h>

#undef EL
#define EL isl_basic_map

#include <isl_list_templ.h>

#undef BASE
#define BASE basic_map

#include <isl_list_templ.c>
