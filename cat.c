#include <isl_map.h>

struct isl_arg_choice cat_format[] = {
	{"isl",		ISL_FORMAT_ISL},
	{"omega",	ISL_FORMAT_OMEGA},
	{"polylib",	ISL_FORMAT_POLYLIB},
	{"latex",	ISL_FORMAT_LATEX},
	{0}
};

struct cat_options {
	struct isl_options	*isl;
	unsigned		 format;
};

struct isl_arg cat_options_arg[] = {
ISL_ARG_CHILD(struct cat_options, isl, "isl", isl_options_arg, "isl options")
ISL_ARG_CHOICE(struct cat_options, format, 0, "format", \
	cat_format,	ISL_FORMAT_ISL, "output format")
ISL_ARG_END
};

ISL_ARG_DEF(cat_options, struct cat_options, cat_options_arg)

int main(int argc, char **argv)
{
	struct isl_ctx *ctx;
	struct isl_map *map;
	struct cat_options *options;

	options = cat_options_new_with_defaults();
	assert(options);
	argc = cat_options_parse(options, argc, argv, ISL_ARG_ALL);

	ctx = isl_ctx_alloc_with_options(options->isl);
	options->isl = NULL;

	map = isl_map_read_from_file(ctx, stdin, -1);
	isl_map_print(map, stdout, 0, options->format);
	if (options->format == ISL_FORMAT_ISL)
		printf("\n");
	isl_map_free(map);

	isl_ctx_free(ctx);
	free(options);

	return 0;
}
