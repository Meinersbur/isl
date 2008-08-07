#ifndef ISL_PIPLIB_H
#define ISL_PIPLIB_H

#include <isl_ctx.h>
#include <isl_int.h>
#ifndef ISL_PIPLIB
#error "no piplib"
#endif

#include <piplib/piplibMP.h>

void isl_seq_cpy_to_pip(Entier *dst, isl_int *src, unsigned len);

#endif
