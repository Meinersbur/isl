#include <isl_map.h>

int main(int argc, char **argv)
{
	struct isl_ctx *ctx;
	struct isl_map *map;

	ctx = isl_ctx_alloc();

	map = isl_map_read_from_file(ctx, stdin, -1);
	isl_map_print(map, stdout, 0, ISL_FORMAT_ISL);
	isl_map_free(map);

	isl_ctx_free(ctx);

	return 0;
}
