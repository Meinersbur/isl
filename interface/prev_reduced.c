#include <isl/set.h>
#include <isl/version.h>
int main() {
  const char *dynIslVersion = isl_version();
  const char *genIslVersion = "isl-0.27-78-gfc484e00-IMath-32";
  printf("### ISL version  : %s### at generation: %s\n", dynIslVersion, genIslVersion);
  isl_ctx *ctx1 = isl_ctx_alloc();
  isl_set *set492 = isl_set_read_from_str(ctx1, "[g, arg, arg'] -> { [i0, i1] : (exists (e0 = floor((1 + g)/2): i0 = 0 and 2e0 = 1 + g and i1 > 0 and i1 <= -arg)) or (exists (e0 = floor((1 + g)/2): i0 = 0 and i1 = 0 and 2e0 = 1 + g)) }");
  isl_set *set493 = isl_set_coalesce(set492);
  isl_set *set525 = isl_set_copy(set493);
  isl_set *set526 = isl_set_read_from_str(ctx1, "[g, arg, arg'] -> { [i0, i1] : (exists (e0 = floor((g)/2): i0 = 0 and 2e0 = g and i1 > 0 and i1 <= -arg)) or (exists (e0 = floor((g)/2): i0 = 0 and i1 = 0 and 2e0 = g)) }");
  isl_set *set527 = isl_set_union(set526, set525);
  isl_set *set531 = set493;
  isl_set *set533 = isl_set_union(set527, set531);
  fprintf(stderr, "Crash expected here:\n");
  isl_set_coalesce(set533);
}
