#ifndef ISL_LOCAL_SPACE_H
#define ISL_LOCAL_SPACE_H

#include <isl/div.h>
#include <isl/printer.h>

#if defined(__cplusplus)
extern "C" {
#endif

struct isl_local_space;
typedef struct isl_local_space isl_local_space;

isl_ctx *isl_local_space_get_ctx(__isl_keep isl_local_space *ls);

__isl_give isl_local_space *isl_local_space_from_dim(__isl_take isl_dim *dim);

__isl_give isl_local_space *isl_local_space_copy(
	__isl_keep isl_local_space *ls);
void *isl_local_space_free(__isl_take isl_local_space *ls);

int isl_local_space_dim(__isl_keep isl_local_space *ls,
	enum isl_dim_type type);
const char *isl_local_space_get_dim_name(__isl_keep isl_local_space *ls,
	enum isl_dim_type type, unsigned pos);
__isl_give isl_local_space *isl_local_space_set_dim_name(
	__isl_take isl_local_space *ls,
	enum isl_dim_type type, unsigned pos, const char *s);
__isl_give isl_dim *isl_local_space_get_dim(__isl_keep isl_local_space *ls);
__isl_give isl_div *isl_local_space_get_div(__isl_keep isl_local_space *ls,
	int pos);

__isl_give isl_local_space *isl_local_space_from_domain(
	__isl_take isl_local_space *ls);
__isl_give isl_local_space *isl_local_space_add_dims(
	__isl_take isl_local_space *ls, enum isl_dim_type type, unsigned n);
__isl_give isl_local_space *isl_local_space_drop_dims(
	__isl_take isl_local_space *ls,
	enum isl_dim_type type, unsigned first, unsigned n);
__isl_give isl_local_space *isl_local_space_insert_dims(
	__isl_take isl_local_space *ls,
	enum isl_dim_type type, unsigned first, unsigned n);

int isl_local_space_is_equal(__isl_keep isl_local_space *ls1,
	__isl_keep isl_local_space *ls2);

__isl_give isl_printer *isl_printer_print_local_space(__isl_take isl_printer *p,
	__isl_keep isl_local_space *ls);
void isl_local_space_dump(__isl_keep isl_local_space *ls);

#if defined(__cplusplus)
}
#endif

#endif
