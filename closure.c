#include <isl_map.h>

int main(int argc, char **argv)
{
	struct isl_ctx *ctx;
	struct isl_map *map;
	int exact;

	ctx = isl_ctx_alloc();

	map = isl_map_read_from_file(ctx, stdin, -1);
	map = isl_map_transitive_closure(map, &exact);
	if (!exact)
		printf("# NOT exact\n");
	isl_map_print(map, stdout, 0, ISL_FORMAT_ISL);
	map = isl_map_compute_divs(map);
	map = isl_map_coalesce(map);
	printf("# coalesced\n");
	isl_map_print(map, stdout, 0, ISL_FORMAT_ISL);
	isl_map_free(map);

	isl_ctx_free(ctx);

	return 0;
}
