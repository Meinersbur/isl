/*
 * Copyright 2008-2009 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv2.1 license
 *
 * Written by Sven Verdoolaege, K.U.Leuven, Departement
 * Computerwetenschappen, Celestijnenlaan 200A, B-3001 Leuven, Belgium
 */

#ifndef ISL_ARG_H
#define ISL_ARG_H

#include <stddef.h>

#if defined(__cplusplus)
extern "C" {
#endif

struct isl_arg_choice {
	const char	*name;
	unsigned	 value;
};

enum isl_arg_type {
	isl_arg_end,
	isl_arg_choice,
	isl_arg_bool,
	isl_arg_child
};

struct isl_arg {
	enum isl_arg_type	 type;
	char			 short_name;
	const char		*long_name;
	size_t			 offset;
	union {
	struct {
		struct isl_arg_choice	*choice;
		unsigned	 	 default_value;
	} choice;
	struct {
		unsigned		 default_value;
	} b;
	struct {
		struct isl_arg		*child;
		size_t			 size;
	} child;
	} u;
};

#define ISL_ARG_CHOICE(st,f,s,l,c,d)	{				\
	.type = isl_arg_choice,						\
	.short_name = s,						\
	.long_name = l,							\
	.offset = offsetof(st, f),					\
	.u = { .choice = { .choice = c, .default_value = d } }		\
},
#define ISL_ARG_BOOL(st,f,s,l,d)	{				\
	.type = isl_arg_bool,						\
	.short_name = s,						\
	.long_name = l,							\
	.offset = offsetof(st, f),					\
	.u = { .b = { .default_value = d } }				\
},
#define ISL_ARG_CHILD(st,f,l,c)		{				\
	.type = isl_arg_child,						\
	.long_name = l,							\
	.offset = offsetof(st, f),					\
	.u = { .child = { .child = c, .size = sizeof(*((st *)NULL)->f) } }\
},
#define ISL_ARG_END	{ isl_arg_end }

#define ISL_ARG_ALL	(1 << 0)

void isl_arg_set_defaults(struct isl_arg *arg, void *opt);
int isl_arg_parse(struct isl_arg *arg, int argc, char **argv, void *opt,
	unsigned flags);

#define ISL_ARG_DECL(prefix,st,arg)					\
st *prefix ## _new_with_defaults();					\
int prefix ## _parse(st *opt, int argc, char **argv, unsigned flags);

#define ISL_ARG_DEF(prefix,st,arg)					\
st *prefix ## _new_with_defaults()					\
{									\
	st *opt = (st *)calloc(1, sizeof(st));				\
	if (opt)							\
		isl_arg_set_defaults(arg, opt);				\
	return opt;							\
}									\
									\
int prefix ## _parse(st *opt, int argc, char **argv, unsigned flags)	\
{									\
	return isl_arg_parse(arg, argc, argv, opt, flags);		\
}

#if defined(__cplusplus)
}
#endif

#endif
