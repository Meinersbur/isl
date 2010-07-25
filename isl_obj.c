/*
 * Copyright 2010      INRIA Saclay
 *
 * Use of this software is governed by the GNU LGPLv2.1 license
 *
 * Written by Sven Verdoolaege, INRIA Saclay - Ile-de-France,
 * Parc Club Orsay Universite, ZAC des vignes, 4 rue Jacques Monod,
 * 91893 Orsay, France 
 */

#include <isl_obj.h>

static void *isl_obj_map_copy(void *v)
{
	return isl_map_copy((struct isl_map *)v);
}

static void isl_obj_map_free(void *v)
{
	isl_map_free((struct isl_map *)v);
}

static __isl_give isl_printer *isl_obj_map_print(__isl_take isl_printer *p,
	void *v)
{
	return isl_printer_print_map(p, (struct isl_map *)v);
}

static void *isl_obj_map_add(void *v1, void *v2)
{
	return isl_map_union((struct isl_map *)v1, (struct isl_map *)v2);
}

struct isl_obj_vtable isl_obj_map_vtable = {
	isl_obj_map_copy,
	isl_obj_map_add,
	isl_obj_map_print,
	isl_obj_map_free
};

static void *isl_obj_union_map_copy(void *v)
{
	return isl_union_map_copy((isl_union_map *)v);
}

static void isl_obj_union_map_free(void *v)
{
	isl_union_map_free((isl_union_map *)v);
}

static __isl_give isl_printer *isl_obj_union_map_print(__isl_take isl_printer *p,
	void *v)
{
	return isl_printer_print_union_map(p, (isl_union_map *)v);
}

static void *isl_obj_union_map_add(void *v1, void *v2)
{
	return isl_union_map_union((isl_union_map *)v1, (isl_union_map *)v2);
}

struct isl_obj_vtable isl_obj_union_map_vtable = {
	isl_obj_union_map_copy,
	isl_obj_union_map_add,
	isl_obj_union_map_print,
	isl_obj_union_map_free
};

static void *isl_obj_set_copy(void *v)
{
	return isl_set_copy((struct isl_set *)v);
}

static void isl_obj_set_free(void *v)
{
	isl_set_free((struct isl_set *)v);
}

static __isl_give isl_printer *isl_obj_set_print(__isl_take isl_printer *p,
	void *v)
{
	return isl_printer_print_set(p, (struct isl_set *)v);
}

static void *isl_obj_set_add(void *v1, void *v2)
{
	return isl_set_union((struct isl_set *)v1, (struct isl_set *)v2);
}

struct isl_obj_vtable isl_obj_set_vtable = {
	isl_obj_set_copy,
	isl_obj_set_add,
	isl_obj_set_print,
	isl_obj_set_free
};

static void *isl_obj_union_set_copy(void *v)
{
	return isl_union_set_copy((isl_union_set *)v);
}

static void isl_obj_union_set_free(void *v)
{
	isl_union_set_free((isl_union_set *)v);
}

static __isl_give isl_printer *isl_obj_union_set_print(__isl_take isl_printer *p,
	void *v)
{
	return isl_printer_print_union_set(p, (isl_union_set *)v);
}

static void *isl_obj_union_set_add(void *v1, void *v2)
{
	return isl_union_set_union((isl_union_set *)v1, (isl_union_set *)v2);
}

struct isl_obj_vtable isl_obj_union_set_vtable = {
	isl_obj_union_set_copy,
	isl_obj_union_set_add,
	isl_obj_union_set_print,
	isl_obj_union_set_free
};

static void *isl_obj_none_copy(void *v)
{
	return v;
}

static void isl_obj_none_free(void *v)
{
}

static __isl_give isl_printer *isl_obj_none_print(__isl_take isl_printer *p,
	void *v)
{
	return p;
}

static void *isl_obj_none_add(void *v1, void *v2)
{
	return NULL;
}

struct isl_obj_vtable isl_obj_none_vtable = {
	isl_obj_none_copy,
	isl_obj_none_add,
	isl_obj_none_print,
	isl_obj_none_free
};

static void *isl_obj_pw_qp_copy(void *v)
{
	return isl_pw_qpolynomial_copy((struct isl_pw_qpolynomial *)v);
}

static void isl_obj_pw_qp_free(void *v)
{
	isl_pw_qpolynomial_free((struct isl_pw_qpolynomial *)v);
}

static __isl_give isl_printer *isl_obj_pw_qp_print(__isl_take isl_printer *p,
	void *v)
{
	return isl_printer_print_pw_qpolynomial(p,
						(struct isl_pw_qpolynomial *)v);
}

static void *isl_obj_pw_qp_add(void *v1, void *v2)
{
	return isl_pw_qpolynomial_add((struct isl_pw_qpolynomial *)v1,
					(struct isl_pw_qpolynomial *)v2);
}

struct isl_obj_vtable isl_obj_pw_qpolynomial_vtable = {
	isl_obj_pw_qp_copy,
	isl_obj_pw_qp_add,
	isl_obj_pw_qp_print,
	isl_obj_pw_qp_free
};

static void *isl_obj_union_pw_qp_copy(void *v)
{
	return isl_union_pw_qpolynomial_copy((struct isl_union_pw_qpolynomial *)v);
}

static void isl_obj_union_pw_qp_free(void *v)
{
	isl_union_pw_qpolynomial_free((struct isl_union_pw_qpolynomial *)v);
}

static __isl_give isl_printer *isl_obj_union_pw_qp_print(
	__isl_take isl_printer *p, void *v)
{
	return isl_printer_print_union_pw_qpolynomial(p,
					(struct isl_union_pw_qpolynomial *)v);
}

static void *isl_obj_union_pw_qp_add(void *v1, void *v2)
{
	return isl_union_pw_qpolynomial_add(
					(struct isl_union_pw_qpolynomial *)v1,
					(struct isl_union_pw_qpolynomial *)v2);
}

struct isl_obj_vtable isl_obj_union_pw_qpolynomial_vtable = {
	isl_obj_union_pw_qp_copy,
	isl_obj_union_pw_qp_add,
	isl_obj_union_pw_qp_print,
	isl_obj_union_pw_qp_free
};

static void *isl_obj_pw_qpf_copy(void *v)
{
	return isl_pw_qpolynomial_fold_copy((struct isl_pw_qpolynomial_fold *)v);
}

static void isl_obj_pw_qpf_free(void *v)
{
	isl_pw_qpolynomial_fold_free((struct isl_pw_qpolynomial_fold *)v);
}

static __isl_give isl_printer *isl_obj_pw_qpf_print(__isl_take isl_printer *p,
	void *v)
{
	return isl_printer_print_pw_qpolynomial_fold(p,
					(struct isl_pw_qpolynomial_fold *)v);
}

static void *isl_obj_pw_qpf_add(void *v1, void *v2)
{
	return isl_pw_qpolynomial_fold_add((struct isl_pw_qpolynomial_fold *)v1,
					    (struct isl_pw_qpolynomial_fold *)v2);
}

struct isl_obj_vtable isl_obj_pw_qpolynomial_fold_vtable = {
	isl_obj_pw_qpf_copy,
	isl_obj_pw_qpf_add,
	isl_obj_pw_qpf_print,
	isl_obj_pw_qpf_free
};

static void *isl_obj_union_pw_qpf_copy(void *v)
{
	return isl_union_pw_qpolynomial_fold_copy((struct isl_union_pw_qpolynomial_fold *)v);
}

static void isl_obj_union_pw_qpf_free(void *v)
{
	isl_union_pw_qpolynomial_fold_free((struct isl_union_pw_qpolynomial_fold *)v);
}

static __isl_give isl_printer *isl_obj_union_pw_qpf_print(
	__isl_take isl_printer *p, void *v)
{
	return isl_printer_print_union_pw_qpolynomial_fold(p,
				    (struct isl_union_pw_qpolynomial_fold *)v);
}

static void *isl_obj_union_pw_qpf_add(void *v1, void *v2)
{
	return isl_union_pw_qpolynomial_fold_add(
				    (struct isl_union_pw_qpolynomial_fold *)v1,
				    (struct isl_union_pw_qpolynomial_fold *)v2);
}

struct isl_obj_vtable isl_obj_union_pw_qpolynomial_fold_vtable = {
	isl_obj_union_pw_qpf_copy,
	isl_obj_union_pw_qpf_add,
	isl_obj_union_pw_qpf_print,
	isl_obj_union_pw_qpf_free
};
