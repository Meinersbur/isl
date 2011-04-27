/*
 * Copyright 2008-2009 Katholieke Universiteit Leuven
 * Copyright 2011      INRIA Saclay
 *
 * Use of this software is governed by the GNU LGPLv2.1 license
 *
 * Written by Sven Verdoolaege, K.U.Leuven, Departement
 * Computerwetenschappen, Celestijnenlaan 200A, B-3001 Leuven, Belgium
 * and INRIA Saclay - Ile-de-France, Parc Club Orsay Universite,
 * ZAC des vignes, 4 rue Jacques Monod, 91893 Orsay, France
 */

#define xCAT(A,B) A ## B
#define CAT(A,B) xCAT(A,B)
#undef EL
#define EL CAT(isl_,BASE)
#define xFN(TYPE,NAME) TYPE ## _ ## NAME
#define FN(TYPE,NAME) xFN(TYPE,NAME)
#define xLIST(EL) EL ## _list
#define LIST(EL) xLIST(EL)

isl_ctx *FN(LIST(EL),get_ctx)(__isl_keep LIST(EL) *list)
{
	return list ? list->ctx : NULL;
}

__isl_give LIST(EL) *FN(LIST(EL),alloc)(isl_ctx *ctx, int n)
{
	LIST(EL) *list;

	if (n < 0)
		isl_die(ctx, isl_error_invalid,
			"cannot create list of negative length",
			return NULL);
	list = isl_alloc(ctx, LIST(EL),
			 sizeof(LIST(EL)) + (n - 1) * sizeof(struct EL *));
	if (!list)
		return NULL;

	list->ctx = ctx;
	isl_ctx_ref(ctx);
	list->ref = 1;
	list->size = n;
	list->n = 0;
	return list;
}

__isl_give LIST(EL) *FN(LIST(EL),copy)(__isl_keep LIST(EL) *list)
{
	if (!list)
		return NULL;

	list->ref++;
	return list;
}

__isl_give LIST(EL) *FN(LIST(EL),add)(__isl_take LIST(EL) *list,
	__isl_take struct EL *el)
{
	if (!list || !el)
		goto error;
	isl_assert(list->ctx, list->n < list->size, goto error);
	list->p[list->n] = el;
	list->n++;
	return list;
error:
	FN(EL,free)(el);
	FN(LIST(EL),free)(list);
	return NULL;
}

void FN(LIST(EL),free)(__isl_take LIST(EL) *list)
{
	int i;

	if (!list)
		return;

	if (--list->ref > 0)
		return;

	isl_ctx_deref(list->ctx);
	for (i = 0; i < list->n; ++i)
		FN(EL,free)(list->p[i]);
	free(list);
}

int FN(FN(LIST(EL),n),BASE)(__isl_keep LIST(EL) *list)
{
	return list ? list->n : 0;
}

int FN(LIST(EL),foreach)(__isl_keep LIST(EL) *list,
	int (*fn)(__isl_take EL *el, void *user), void *user)
{
	int i;

	if (!list)
		return -1;

	for (i = 0; i < list->n; ++i) {
		EL *el = FN(EL,copy(list->p[i]));
		if (!el)
			return -1;
		if (fn(el, user) < 0)
			return -1;
	}

	return 0;
}
