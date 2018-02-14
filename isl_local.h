#ifndef ISL_LOCAL_H
#define ISL_LOCAL_H

#include <isl/mat.h>

typedef isl_mat isl_local;

isl_bool isl_local_div_is_marked_unknown(__isl_keep isl_local *local, int pos);
isl_bool isl_local_div_is_known(__isl_keep isl_local *local, int pos);
int isl_local_cmp(__isl_keep isl_local *local1, __isl_keep isl_local *local2);

#endif
