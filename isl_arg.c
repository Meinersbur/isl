/*
 * Copyright 2008-2009 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv2.1 license
 *
 * Written by Sven Verdoolaege, K.U.Leuven, Departement
 * Computerwetenschappen, Celestijnenlaan 200A, B-3001 Leuven, Belgium
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <isl/arg.h>
#include <isl/ctx.h>

static void set_default_choice(struct isl_arg *arg, void *opt)
{
	*(unsigned *)(((char *)opt) + arg->offset) = arg->u.choice.default_value;
}

static void set_default_flags(struct isl_arg *arg, void *opt)
{
	*(unsigned *)(((char *)opt) + arg->offset) = arg->u.flags.default_value;
}

static void set_default_bool(struct isl_arg *arg, void *opt)
{
	*(unsigned *)(((char *)opt) + arg->offset) = arg->u.b.default_value;
}

static void set_default_child(struct isl_arg *arg, void *opt)
{
	void *child = calloc(1, arg->u.child.size);

	if (child)
		isl_arg_set_defaults(arg->u.child.child, child);

	*(void **)(((char *)opt) + arg->offset) = child;
}

static void set_default_user(struct isl_arg *arg, void *opt)
{
	arg->u.user.init(((char *)opt) + arg->offset);
}

static void set_default_long(struct isl_arg *arg, void *opt)
{
	*(long *)(((char *)opt) + arg->offset) = arg->u.l.default_value;
}

static void set_default_ulong(struct isl_arg *arg, void *opt)
{
	*(unsigned long *)(((char *)opt) + arg->offset) = arg->u.ul.default_value;
}

static void set_default_str(struct isl_arg *arg, void *opt)
{
	const char *str = NULL;
	if (arg->u.str.default_value)
		str = strdup(arg->u.str.default_value);
	*(const char **)(((char *)opt) + arg->offset) = str;
}

void isl_arg_set_defaults(struct isl_arg *arg, void *opt)
{
	int i;

	for (i = 0; arg[i].type != isl_arg_end; ++i) {
		switch (arg[i].type) {
		case isl_arg_choice:
			set_default_choice(&arg[i], opt);
			break;
		case isl_arg_flags:
			set_default_flags(&arg[i], opt);
			break;
		case isl_arg_bool:
			set_default_bool(&arg[i], opt);
			break;
		case isl_arg_child:
			set_default_child(&arg[i], opt);
			break;
		case isl_arg_user:
			set_default_user(&arg[i], opt);
			break;
		case isl_arg_long:
			set_default_long(&arg[i], opt);
			break;
		case isl_arg_ulong:
			set_default_ulong(&arg[i], opt);
			break;
		case isl_arg_arg:
		case isl_arg_str:
			set_default_str(&arg[i], opt);
			break;
		case isl_arg_version:
		case isl_arg_end:
			break;
		}
	}
}

void isl_arg_free(struct isl_arg *arg, void *opt)
{
	int i;

	if (!opt)
		return;

	for (i = 0; arg[i].type != isl_arg_end; ++i) {
		switch (arg[i].type) {
		case isl_arg_child:
			isl_arg_free(arg[i].u.child.child,
				*(void **)(((char *)opt) + arg[i].offset));
			break;
		case isl_arg_arg:
		case isl_arg_str:
			free(*(char **)(((char *)opt) + arg[i].offset));
			break;
		case isl_arg_user:
			if (arg[i].u.user.clear)
				arg[i].u.user.clear(((char *)opt) + arg[i].offset);
			break;
		case isl_arg_bool:
		case isl_arg_choice:
		case isl_arg_flags:
		case isl_arg_long:
		case isl_arg_ulong:
		case isl_arg_version:
		case isl_arg_end:
			break;
		}
	}

	free(opt);
}

static int print_arg_help(struct isl_arg *decl, const char *prefix, int no)
{
	int len = 0;

	if (!decl->long_name) {
		printf("  -%c", decl->short_name);
		return 4;
	}

	if (decl->short_name)
		printf("  -%c, --", decl->short_name);
	else
		printf("      --");
	len += 8;

	if (prefix) {
		printf("%s-", prefix);
		len += strlen(prefix) + 1;
	}
	if (no) {
		printf("no-");
		len += 3;
	}
	printf("%s", decl->long_name);
	len += strlen(decl->long_name);

	return len;
}

const void *isl_memrchr(const void *s, int c, size_t n)
{
	const char *p = s;
	while (n-- > 0)
		if (p[n] == c)
			return p + n;
	return NULL;
}

static int print_help_msg(struct isl_arg *decl, int pos)
{
	int len;
	const char *s;

	if (!decl->help_msg)
		return pos;

	if (pos >= 29)
		printf("\n%30s", "");
	else
		printf("%*s", 30 - pos, "");

	s = decl->help_msg;
	len = strlen(s);
	while (len > 45) {
		const char *space = isl_memrchr(s, ' ', 45);
		int l;

		if (!space)
			space = strchr(s + 45, ' ');
		if (!space)
			break;
		l = space - s;
		printf("%.*s", l, s);
		s = space + 1;
		len -= l + 1;
		printf("\n%30s", "");
	}

	printf("%s", s);
	return len;
}

static void print_default(struct isl_arg *decl, const char *def, int pos)
{
	int i;
	const char *default_prefix = "[default: ";
	const char *default_suffix = "]";
	int len;

	len = strlen(default_prefix) + strlen(def) + strlen(default_suffix);

	if (!decl->help_msg) {
		if (pos >= 29)
			printf("\n%30s", "");
		else
			printf("%*s", 30 - pos, "");
		pos = 0;
	} else {
		if (pos + len >= 48)
			printf("\n%30s", "");
		else
			printf(" ");
	}
	printf("%s%s%s", default_prefix, def, default_suffix);
}

static void print_default_choice(struct isl_arg *decl, int pos)
{
	int i;
	const char *s = "none";

	for (i = 0; decl->u.choice.choice[i].name; ++i)
		if (decl->u.choice.choice[i].value == decl->u.choice.default_value) {
			s = decl->u.choice.choice[i].name;
			break;
		}

	print_default(decl, s, pos);
}

static void print_choice_help(struct isl_arg *decl, const char *prefix)
{
	int i;
	int pos;

	pos = print_arg_help(decl, prefix, 0);
	printf("=");
	pos++;

	for (i = 0; decl->u.choice.choice[i].name; ++i) {
		if (i) {
			printf("|");
			pos++;
		}
		printf("%s", decl->u.choice.choice[i].name);
		pos += strlen(decl->u.choice.choice[i].name);
	}

	pos = print_help_msg(decl, pos);
	print_default_choice(decl, pos);

	printf("\n");
}

static void print_default_flags(struct isl_arg *decl, int pos)
{
	int i, first;
	const char *default_prefix = "[default: ";
	const char *default_suffix = "]";
	int len = strlen(default_prefix) + strlen(default_suffix);

	for (i = 0; decl->u.flags.flags[i].name; ++i)
		if ((decl->u.flags.default_value & decl->u.flags.flags[i].mask) ==
		     decl->u.flags.flags[i].value)
			len += strlen(decl->u.flags.flags[i].name);

	if (!decl->help_msg) {
		if (pos >= 29)
			printf("\n%30s", "");
		else
			printf("%*s", 30 - pos, "");
		pos = 0;
	} else {
		if (pos + len >= 48)
			printf("\n%30s", "");
		else
			printf(" ");
	}
	printf("%s", default_prefix);

	for (first = 1, i = 0; decl->u.flags.flags[i].name; ++i)
		if ((decl->u.flags.default_value & decl->u.flags.flags[i].mask) ==
		     decl->u.flags.flags[i].value) {
			if (!first)
				printf(",");
			printf("%s", decl->u.flags.flags[i].name);
			first = 0;
		}

	printf("%s", default_suffix);
}

static void print_flags_help(struct isl_arg *decl, const char *prefix)
{
	int i, j;
	int pos;

	pos = print_arg_help(decl, prefix, 0);
	printf("=");
	pos++;

	for (i = 0; decl->u.flags.flags[i].name; ++i) {
		if (i) {
			printf(",");
			pos++;
		}
		for (j = i;
		     decl->u.flags.flags[j].mask == decl->u.flags.flags[i].mask;
		     ++j) {
			if (j != i) {
				printf("|");
				pos++;
			}
			printf("%s", decl->u.flags.flags[j].name);
			pos += strlen(decl->u.flags.flags[j].name);
		}
		i = j - 1;
	}

	pos = print_help_msg(decl, pos);
	print_default_flags(decl, pos);

	printf("\n");
}

static void print_bool_help(struct isl_arg *decl, const char *prefix)
{
	int pos;
	int no = decl->u.b.default_value == 1;
	pos = print_arg_help(decl, prefix, no);
	pos = print_help_msg(decl, pos);
	print_default(decl, no ? "yes" : "no", pos);
	printf("\n");
}

static void print_long_help(struct isl_arg *decl, const char *prefix)
{
	int pos;
	pos = print_arg_help(decl, prefix, 0);
	if (decl->u.l.default_value != decl->u.l.default_selected) {
		printf("[");
		pos++;
	}
	printf("=long");
	pos += 5;
	if (decl->u.l.default_value != decl->u.l.default_selected) {
		printf("]");
		pos++;
	}
	print_help_msg(decl, pos);
	printf("\n");
}

static void print_ulong_help(struct isl_arg *decl, const char *prefix)
{
	int pos;
	pos = print_arg_help(decl, prefix, 0);
	printf("=ulong");
	pos += 6;
	print_help_msg(decl, pos);
	printf("\n");
}

static void print_str_help(struct isl_arg *decl, const char *prefix)
{
	int pos;
	const char *a = decl->argument_name ? decl->argument_name : "string";
	pos = print_arg_help(decl, prefix, 0);
	printf("=%s", a);
	pos += 1 + strlen(a);
	print_help_msg(decl, pos);
	printf("\n");
}

static void print_help(struct isl_arg *arg, const char *prefix)
{
	int i;

	for (i = 0; arg[i].type != isl_arg_end; ++i) {
		switch (arg[i].type) {
		case isl_arg_flags:
			print_flags_help(&arg[i], prefix);
			break;
		case isl_arg_choice:
			print_choice_help(&arg[i], prefix);
			break;
		case isl_arg_bool:
			print_bool_help(&arg[i], prefix);
			break;
		case isl_arg_long:
			print_long_help(&arg[i], prefix);
			break;
		case isl_arg_ulong:
			print_ulong_help(&arg[i], prefix);
			break;
		case isl_arg_str:
			print_str_help(&arg[i], prefix);
			break;
		case isl_arg_version:
			printf("  -V, --version\n");
			break;
		case isl_arg_arg:
		case isl_arg_child:
		case isl_arg_user:
		case isl_arg_end:
			break;
		}
	}

	for (i = 0; arg[i].type != isl_arg_end; ++i) {
		if (arg[i].type != isl_arg_child)
			continue;

		printf("\n");
		if (arg[i].help_msg)
			printf(" %s\n", arg[i].help_msg);
		print_help(arg[i].u.child.child, arg[i].long_name);
	}
}

static const char *prog_name(const char *prog)
{
	const char *slash;

	slash = strrchr(prog, '/');
	if (slash)
		prog = slash + 1;
	if (strncmp(prog, "lt-", 3) == 0)
		prog += 3;

	return prog;
}

static void print_help_and_exit(struct isl_arg *arg, const char *prog)
{
	int i;

	printf("Usage: %s [OPTION...]", prog_name(prog));

	for (i = 0; arg[i].type != isl_arg_end; ++i)
		if (arg[i].type == isl_arg_arg)
			printf(" %s", arg[i].argument_name);

	printf("\n\n");

	print_help(arg, NULL);

	exit(0);
}

static const char *skip_name(struct isl_arg *decl, const char *arg,
	const char *prefix, int need_argument, int *has_argument)
{
	const char *equal;
	const char *name;
	const char *end;

	if (arg[0] == '-' && arg[1] && arg[1] == decl->short_name) {
		if (need_argument && !arg[2])
			return NULL;
		if (has_argument)
			*has_argument = arg[2] != '\0';
		return arg + 2;
	}

	if (strncmp(arg, "--", 2))
		return NULL;

	name = arg + 2;
	equal = strchr(name, '=');
	if (need_argument && !equal)
		return NULL;

	if (has_argument)
		*has_argument = !!equal;
	end = equal ? equal : name + strlen(name);

	if (prefix) {
		size_t prefix_len = strlen(prefix);
		if (strncmp(name, prefix, prefix_len) == 0 &&
		    name[prefix_len] == '-')
			name += prefix_len + 1;
	}

	if (end - name != strlen(decl->long_name) ||
	    strncmp(name, decl->long_name, end - name))
		return NULL;

	return equal ? equal + 1 : end;
}

static int parse_choice_option(struct isl_arg *decl, char **arg,
	const char *prefix, void *opt)
{
	int i;
	int has_argument;
	const char *choice;

	choice = skip_name(decl, arg[0], prefix, 0, &has_argument);
	if (!choice)
		return 0;

	if (!has_argument && (!arg[1] || arg[1][0] == '-')) {
		unsigned u = decl->u.choice.default_selected;
		if (decl->u.choice.set)
			decl->u.choice.set(opt, u);
		else
			*(unsigned *)(((char *)opt) + decl->offset) = u;

		return 1;
	}

	if (!has_argument)
		choice = arg[1];

	for (i = 0; decl->u.choice.choice[i].name; ++i) {
		unsigned u;

		if (strcmp(choice, decl->u.choice.choice[i].name))
			continue;

		u = decl->u.choice.choice[i].value;
		if (decl->u.choice.set)
			decl->u.choice.set(opt, u);
		else
			*(unsigned *)(((char *)opt) + decl->offset) = u;

		return has_argument ? 1 : 2;
	}

	return 0;
}

static int set_flag(struct isl_arg *decl, unsigned *val, const char *flag,
	size_t len)
{
	int i;

	for (i = 0; decl->u.flags.flags[i].name; ++i) {
		if (strncmp(flag, decl->u.flags.flags[i].name, len))
			continue;

		*val &= ~decl->u.flags.flags[i].mask;
		*val |= decl->u.flags.flags[i].value;

		return 1;
	}

	return 0;
}

static int parse_flags_option(struct isl_arg *decl, char **arg,
	const char *prefix, void *opt)
{
	int has_argument;
	const char *flags;
	const char *comma;
	unsigned val;

	flags = skip_name(decl, arg[0], prefix, 0, &has_argument);
	if (!flags)
		return 0;

	if (!has_argument && !arg[1])
		return 0;

	if (!has_argument)
		flags = arg[1];

	val = *(unsigned *)(((char *)opt) + decl->offset);

	while ((comma = strchr(flags, ',')) != NULL) {
		if (!set_flag(decl, &val, flags, comma - flags))
			return 0;
		flags = comma + 1;
	}
	if (!set_flag(decl, &val, flags, strlen(flags)))
		return 0;

	*(unsigned *)(((char *)opt) + decl->offset) = val;

	return has_argument ? 1 : 2;
}

static int parse_bool_option(struct isl_arg *decl, const char *arg,
	const char *prefix, void *opt)
{
	if (skip_name(decl, arg, prefix, 0, NULL)) {
		*(unsigned *)(((char *)opt) + decl->offset) = 1;

		return 1;
	}

	if (strncmp(arg, "--", 2))
		return 0;
	arg += 2;

	if (prefix) {
		size_t prefix_len = strlen(prefix);
		if (strncmp(arg, prefix, prefix_len) == 0 &&
		    arg[prefix_len] == '-') {
			arg += prefix_len + 1;
			prefix = NULL;
		}
	}

	if (strncmp(arg, "no-", 3))
		return 0;
	arg += 3;

	if (prefix) {
		size_t prefix_len = strlen(prefix);
		if (strncmp(arg, prefix, prefix_len) == 0 &&
		    arg[prefix_len] == '-')
			arg += prefix_len + 1;
	}

	if (!strcmp(arg, decl->long_name)) {
		*(unsigned *)(((char *)opt) + decl->offset) = 0;

		return 1;
	}

	return 0;
}

static int parse_str_option(struct isl_arg *decl, char **arg,
	const char *prefix, void *opt)
{
	int has_argument;
	const char *s;
	char **p = (char **)(((char *)opt) + decl->offset);

	s = skip_name(decl, arg[0], prefix, 0, &has_argument);
	if (!s)
		return 0;

	if (has_argument) {
		*p = strdup(s);
		return 1;
	}

	if (arg[1]) {
		*p = strdup(arg[1]);
		return 2;
	}

	return 0;
}

static int parse_long_option(struct isl_arg *decl, char **arg,
	const char *prefix, void *opt)
{
	int has_argument;
	const char *val;
	char *endptr;
	long *p = (long *)(((char *)opt) + decl->offset);

	val = skip_name(decl, arg[0], prefix, 0, &has_argument);
	if (!val)
		return 0;

	if (has_argument) {
		long l = strtol(val, NULL, 0);
		if (decl->u.l.set)
			decl->u.l.set(opt, l);
		else
			*p = l;
		return 1;
	}

	if (arg[1]) {
		long l = strtol(arg[1], &endptr, 0);
		if (*endptr == '\0') {
			if (decl->u.l.set)
				decl->u.l.set(opt, l);
			else
				*p = l;
			return 2;
		}
	}

	if (decl->u.l.default_value != decl->u.l.default_selected) {
		if (decl->u.l.set)
			decl->u.l.set(opt, decl->u.l.default_selected);
		else
			*p = decl->u.l.default_selected;
		return 1;
	}

	return 0;
}

static int parse_ulong_option(struct isl_arg *decl, char **arg,
	const char *prefix, void *opt)
{
	int has_argument;
	const char *val;
	char *endptr;
	unsigned long *p = (unsigned long *)(((char *)opt) + decl->offset);

	val = skip_name(decl, arg[0], prefix, 0, &has_argument);
	if (!val)
		return 0;

	if (has_argument) {
		*p = strtoul(val, NULL, 0);
		return 1;
	}

	if (arg[1]) {
		unsigned long ul = strtoul(arg[1], &endptr, 0);
		if (*endptr == '\0') {
			*p = ul;
			return 2;
		}
	}

	return 0;
}

static int parse_option(struct isl_arg *decl, char **arg,
	const char *prefix, void *opt);

static int parse_child_option(struct isl_arg *decl, char **arg, void *opt)
{
	return parse_option(decl->u.child.child, arg, decl->long_name,
				*(void **)(((char *)opt) + decl->offset));
}

static int parse_option(struct isl_arg *decl, char **arg,
	const char *prefix, void *opt)
{
	int i;

	for (i = 0; decl[i].type != isl_arg_end; ++i) {
		int parsed = 0;
		switch (decl[i].type) {
		case isl_arg_choice:
			parsed = parse_choice_option(&decl[i], arg, prefix, opt);
			break;
		case isl_arg_flags:
			parsed = parse_flags_option(&decl[i], arg, prefix, opt);
			break;
		case isl_arg_long:
			parsed = parse_long_option(&decl[i], arg, prefix, opt);
			break;
		case isl_arg_ulong:
			parsed = parse_ulong_option(&decl[i], arg, prefix, opt);
			break;
		case isl_arg_bool:
			parsed = parse_bool_option(&decl[i], *arg, prefix, opt);
			break;
		case isl_arg_str:
			parsed = parse_str_option(&decl[i], arg, prefix, opt);
			break;
		case isl_arg_child:
			parsed = parse_child_option(&decl[i], arg, opt);
			break;
		case isl_arg_arg:
		case isl_arg_user:
		case isl_arg_version:
		case isl_arg_end:
			break;
		}
		if (parsed)
			return parsed;
	}

	return 0;
}

static int any_version(struct isl_arg *decl)
{
	int i;

	for (i = 0; decl[i].type != isl_arg_end; ++i) {
		switch (decl[i].type) {
		case isl_arg_version:
			return 1;
		case isl_arg_child:
			if (any_version(decl[i].u.child.child))
				return 1;
			break;
		default:
			break;
		}
	}

	return 0;
}

static void print_version(struct isl_arg *decl)
{
	int i;

	for (i = 0; decl[i].type != isl_arg_end; ++i) {
		switch (decl[i].type) {
		case isl_arg_version:
			decl[i].u.version.print_version();
			break;
		case isl_arg_child:
			print_version(decl[i].u.child.child);
			break;
		default:
			break;
		}
	}
}

static void print_version_and_exit(struct isl_arg *decl)
{
	print_version(decl);

	exit(0);
}

static int drop_argument(int argc, char **argv, int drop, int n)
{
	for (; drop < argc; ++drop)
		argv[drop] = argv[drop + n];

	return argc - n;
}

static int n_arg(struct isl_arg *arg)
{
	int i;
	int n_arg = 0;

	for (i = 0; arg[i].type != isl_arg_end; ++i)
		if (arg[i].type == isl_arg_arg)
			n_arg++;

	return n_arg;
}

int isl_arg_parse(struct isl_arg *arg, int argc, char **argv, void *opt,
	unsigned flags)
{
	int skip = 0;
	int i;
	int n;

	n = n_arg(arg);

	for (i = 1; i < argc; ++i) {
		if (strcmp(argv[i], "--help") == 0)
			print_help_and_exit(arg, argv[0]);
	}

	for (i = 1; i < argc; ++i) {
		if ((strcmp(argv[i], "--version") == 0 ||
		     strcmp(argv[i], "-V") == 0) && any_version(arg))
			print_version_and_exit(arg);
	}

	while (argc > 1 + skip) {
		int parsed;
		if (argv[1 + skip][0] != '-')
			break;
		parsed = parse_option(arg, &argv[1 + skip], NULL, opt);
		if (parsed)
			argc = drop_argument(argc, argv, 1 + skip, parsed);
		else if (ISL_FL_ISSET(flags, ISL_ARG_ALL)) {
			fprintf(stderr, "%s: unrecognized option: %s\n",
					prog_name(argv[0]), argv[1 + skip]);
			exit(-1);
		} else
			++skip;
	}

	if (ISL_FL_ISSET(flags, ISL_ARG_ALL) ? argc != 1 + n
					     : argc < 1 + skip + n) {
		fprintf(stderr, "%s: expecting %d arguments\n",
				prog_name(argv[0]), n);
		exit(-1);
	}

	for (i = 0; arg[i].type != isl_arg_end; ++i) {
		const char *str;
		if (arg[i].type != isl_arg_arg)
			continue;
		str = strdup(argv[1 + skip]);
		*(const char **)(((char *)opt) + arg[i].offset) = str;
		argc = drop_argument(argc, argv, 1 + skip, 1);
	}

	return argc;
}
