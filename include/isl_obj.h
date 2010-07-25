#ifndef ISL_OBJ_H
#define ISL_OBJ_H

#include <isl_set.h>
#include <isl_map.h>
#include <isl_polynomial.h>
#include <isl_printer.h>

#if defined(__cplusplus)
extern "C" {
#endif

struct isl_obj_vtable {
	void *(*copy)(void *v1);
	void *(*add)(void *v1, void *v2);
	__isl_give isl_printer *(*print)(__isl_take isl_printer *p, void *v);
	void (*free)(void *v);
};
typedef struct isl_obj_vtable *isl_obj_type;
extern struct isl_obj_vtable isl_obj_none_vtable;
#define isl_obj_none		(&isl_obj_none_vtable)
extern struct isl_obj_vtable isl_obj_set_vtable;
#define isl_obj_set		(&isl_obj_set_vtable)
extern struct isl_obj_vtable isl_obj_map_vtable;
#define isl_obj_map		(&isl_obj_map_vtable)
extern struct isl_obj_vtable isl_obj_pw_qpolynomial_vtable;
#define isl_obj_pw_qpolynomial	(&isl_obj_pw_qpolynomial_vtable)
extern struct isl_obj_vtable isl_obj_pw_qpolynomial_fold_vtable;
#define isl_obj_pw_qpolynomial_fold	(&isl_obj_pw_qpolynomial_fold_vtable)
struct isl_obj {
	isl_obj_type	type;
	void		*v;
};

#if defined(__cplusplus)
}
#endif

#endif
